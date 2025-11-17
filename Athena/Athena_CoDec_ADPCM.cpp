/*
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
#include <windows.h>
#include <mmreg.h>
#include <Athena_Types.h>
#include "Athena_Codec_ADPCM.h"
#include "Athena_FileIO.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Codec_ADPCM.cpp - ADPCM Audio Codec Implementation
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		ADPCM (Adaptive Differential Pulse Code Modulation) audio codec
//		Lossy compression format that stores differences between samples
//		Achieves 4:1 compression ratio vs raw PCM (16-bit -> 4-bit)
//
// Purpose:
//		- Decompress ADPCM-encoded audio for playback
//		- Reduce memory and disk usage for audio assets
//		- Maintain acceptable audio quality for game sounds
//		- Support both mono and stereo audio
//
// ADPCM Algorithm Overview:
//		Instead of storing absolute sample values, ADPCM stores:
//		1. Difference between predicted and actual sample (4 bits)
//		2. Adaptive step size that adjusts to signal characteristics
//		3. Predictor coefficients for linear prediction
//
//		Compression: 16-bit PCM -> 4-bit ADPCM (75% size reduction)
//		Quality: Lossy but acceptable for game audio
//
// Technical Details:
//		- Block-based decoding (each block has header with state)
//		- Linear prediction using previous 2 samples
//		- Adaptive delta quantization (step size changes)
//		- 4-bit samples packed as nybbles (2 samples per byte)
//		- Supports mono (1 channel) and stereo (2 channels)
//
// Block Structure:
//		[Header]
//		- Predictor index (selects coefficient pair)
//		- Initial delta (step size)
//		- Sample 1 (previous sample for prediction)
//		- Sample 2 (second previous sample for prediction)
//		[Data]
//		- Packed nybbles (4-bit ADPCM samples)
//		- Optional padding bits for block alignment
//
// Decoding Process:
//		1. Read block header (predictor, delta, initial samples)
//		2. For each 4-bit ADPCM value:
//		   a. Predict next sample using: predict = samp1*coef1 + samp2*coef2
//		   b. Reconstruct PCM = ADPCM_value * delta + predict
//		   c. Update delta using adaptation table
//		   d. Shift sample history: samp2=samp1, samp1=PCM
//		3. Repeat for all samples in block
//
// Design Pattern:
//		Strategy Pattern (one of multiple codec implementations)
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	//=============================================================================
	// ADPCM Delta Adaptation Table
	//=============================================================================
	// Description:
	//		Fixed-point multipliers for adaptive delta (step size) adjustment
	//		Indexed by 4-bit ADPCM sample value (0-15)
	//
	// Purpose:
	//		Adapts step size based on signal characteristics:
	//		- Large changes (high values) -> increase step size (values > 256)
	//		- Small changes (low values) -> decrease step size (values < 256)
	//
	// Algorithm:
	//		new_delta = (old_delta * gai_p4[adpcm_sample]) >> 8
	//
	// Notes:
	//		Values are in 8.8 fixed-point format (256 = 1.0)
	//		Symmetrical around center for +/- adaptation
	//		Middle values (0-3, 12-15): Reduce delta (multiply by 230/256 ≈ 0.9)
	//		High values (4-7, 8-11): Increase delta (multiply up to 768/256 = 3.0)
	//
	//=============================================================================
	static const short gai_p4[] =
	{
		230, 230, 230, 230, 307, 409, 512, 614,		// Values 0-7
		768, 614, 512, 409, 307, 230, 230, 230		// Values 8-15
	};

	//=============================================================================
	// CodecADPCM Constructor
	//=============================================================================
	// Description:
	//		Initialize ADPCM codec with default values
	//		Allocates no memory yet (done later in SetHeader based on format)
	//
	// Member Variables:
	//		header: ADPCM format header (channels, sample rate, block size, etc.)
	//		stream: File stream for reading compressed data
	//		padding: Padding bits at end of each block for alignment
	//		shift: Channel shift (0 for mono, 1 for stereo) - used for bit ops
	//		sample_i: Current sample index within block
	//		predictor[]: Predictor index for each channel
	//		delta[]: Current delta (step size) for each channel
	//		samp1[], samp2[]: Previous samples for prediction (per channel)
	//		coef1[], coef2[]: Active predictor coefficients (per channel)
	//		nybble_c, nybble_i, nybble_l[]: Nybble buffer (4-bit samples)
	//		nybble: Current byte being unpacked
	//		odd: Toggle for high/low nybble in byte
	//		cache_c, cache_i, cache_l: Output sample cache
	//		cursor: Current position in decoded stream
	//
	//=============================================================================
	CodecADPCM::CodecADPCM() :
		header(NULL),				// ADPCM format header (set by SetHeader)
		stream(NULL),				// File stream (set by SetStream)
		padding(0),					// Block padding bits
		shift(0),					// Channel shift: 0=mono, 1=stereo
		sample_i(0xffffffff),		// Current sample index (invalid until first block)
		predictor(NULL),			// Predictor indices per channel
		delta(NULL),				// Delta (step size) per channel
		samp1(NULL), samp2(NULL),	// Previous samples for prediction
		coef1(NULL), coef2(NULL),	// Active predictor coefficients
		nybble_c(0), nybble_i(0), nybble_l(NULL),	// Nybble buffer
		nybble(0),					// Current byte being decoded
		odd(0),						// High/low nybble toggle
		cache_c(0), cache_i(0), cache_l(NULL),		// Output cache
		cursor(0)					// Stream position
	{
	}

	//=============================================================================
	// CodecADPCM Destructor
	//=============================================================================
	// Description:
	//		Free all dynamically allocated buffers
	//		Releases per-channel state arrays and decode buffers
	//
	// Notes:
	//		Stream and header are owned by caller, not freed here
	//
	//=============================================================================
	CodecADPCM::~CodecADPCM()
	{
		free(predictor);		// Free per-channel predictor indices
		free(delta);			// Free per-channel deltas
		free(samp1);			// Free per-channel sample history
		free(samp2);
		free(coef1);			// Free per-channel coefficients
		free(coef2);
		free(cache_l);			// Free output sample cache
		free(nybble_l);			// Free compressed nybble buffer
	}

	//=============================================================================
	// CODEC SETUP METHODS
	//=============================================================================

	//=============================================================================
	// SetHeader - Initialize Codec with ADPCM Format Header
	//=============================================================================
	// Description:
	//		Configures codec based on ADPCM format parameters
	//		Allocates per-channel buffers based on channel count
	//		Calculates block padding and buffer sizes
	//
	// Parameters:
	//		_header: Pointer to ADPCMWAVEFORMAT structure with:
	//			- nChannels: 1 (mono) or 2 (stereo)
	//			- wSamplesPerBlock: Samples per compressed block
	//			- nBlockAlign: Block size in bytes
	//			- wBitsPerSample: Output bits per sample (16)
	//			- wNumCoef: Number of predictor coefficient pairs
	//			- aCoef[]: Array of predictor coefficient pairs
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_FORMAT if unsupported channel count
	//		AAL_ERROR_MEMORY if allocation fails
	//
	// Algorithm:
	//		1. Validate channel count (1 or 2 only)
	//		2. Calculate shift factor (0=mono, 1=stereo) for array sizing
	//		3. Allocate per-channel arrays (predictor, delta, samples, coefs)
	//		4. Allocate decode buffers (nybble buffer, output cache)
	//		5. Calculate block padding bits
	//		6. Load first block header to initialize decoder state
	//
	// Notes:
	//		Uses realloc to allow codec reuse with different formats
	//		Shift trick: "<<shift" multiplies size by 1 (mono) or 2 (stereo)
	//
	//=============================================================================
	aalError CodecADPCM::SetHeader(aalVoid * _header)
	{
		header = (ADPCMWAVEFORMAT *)_header;	// Store ADPCM format header

		// Validate channel count (only mono and stereo supported)
		if (header->wfx.nChannels != 1 && header->wfx.nChannels != 2)
			return AAL_ERROR_FORMAT;

		shift = header->wfx.nChannels - 1;		// 0 for mono, 1 for stereo
		padding = 0;
		sample_i = 0xffffffff;					// Mark as uninitialized

		aalVoid * ptr;

		// Allocate per-channel predictor index array
		// Size: 1 byte * channels (mono=1, stereo=2)
		ptr = realloc(predictor, sizeof(char) << shift);
		if (!ptr) return AAL_ERROR_MEMORY;
		predictor = (char *)ptr;

		// Allocate per-channel delta (step size) array
		// Size: 2 bytes * channels
		ptr = realloc(delta, sizeof(aalSWord) << shift);
		if (!ptr) return AAL_ERROR_MEMORY;
		delta = (aalSWord *)ptr;

		// Allocate per-channel predictor coefficient 1 array
		ptr = realloc(coef1, sizeof(aalSWord) << shift);
		if (!ptr) return AAL_ERROR_MEMORY;
		coef1 = (aalSWord *)ptr;

		// Allocate per-channel predictor coefficient 2 array
		ptr = realloc(coef2, sizeof(aalSWord) << shift);
		if (!ptr) return AAL_ERROR_MEMORY;
		coef2 = (aalSWord *)ptr;

		// Allocate per-channel previous sample 1 array
		ptr = realloc(samp1, sizeof(aalSWord) << shift);
		if (!ptr) return AAL_ERROR_MEMORY;
		samp1 = (aalSWord *)ptr;

		// Allocate per-channel previous sample 2 array
		ptr = realloc(samp2, sizeof(aalSWord) << shift);
		if (!ptr) return AAL_ERROR_MEMORY;
		samp2 = (aalSWord *)ptr;

		// Allocate output sample cache (one 16-bit sample per channel)
		ptr = realloc(cache_l, cache_c = cache_i = (aalUByte)(sizeof(aalSWord) << shift));
		if (!ptr) return AAL_ERROR_MEMORY;
		cache_l = ptr;

		// Calculate nybble buffer size
		// wSamplesPerBlock includes 2 header samples, so subtract 2
		nybble_c = header->wSamplesPerBlock - 2;
		if (!shift) nybble_c >>= 1;		// Mono: 2 samples per byte, so halve

		// Allocate compressed nybble buffer
		ptr = realloc(nybble_l, nybble_c);
		if (!ptr) return AAL_ERROR_MEMORY;
		nybble_l = (aalSByte *)ptr;

		// Calculate padding bits at end of each block
		// Formula: total_block_bits - header_bits - data_bits = padding_bits
		padding = ((header->wfx.nBlockAlign - (7 << shift)) << 3) -
		          (header->wSamplesPerBlock - 2) * (header->wfx.wBitsPerSample << shift);

		// Load first block header to initialize decoder state
		aalError error(GetNextBlock());
		if (error) return error;

		sample_i++;		// Advance to first sample

		return AAL_OK;
	}

	//=============================================================================
	// SetStream - Set File Stream for Reading
	//=============================================================================
	// Description:
	//		Connects codec to file stream for reading compressed ADPCM data
	//		Stream must be positioned at start of audio data (after WAV header)
	//
	// Parameters:
	//		_stream: FILE pointer to opened ADPCM audio file
	//
	// Returns:
	//		AAL_OK always
	//
	// Notes:
	//		Must call SetHeader before SetStream to allocate buffers
	//		Stream position should be at first ADPCM block
	//
	//=============================================================================
	aalError CodecADPCM::SetStream(FILE * _stream)
	{
		stream = _stream;		// Store stream reference

		return AAL_OK;
	}

	//=============================================================================
	// SetPosition - Seek to Specific Sample Position
	//=============================================================================
	// Description:
	//		Seeks to arbitrary sample position in ADPCM stream
	//		Must decompress from start of target block (ADPCM cannot seek mid-block)
	//
	// Parameters:
	//		_position: Target sample position (in samples, not bytes)
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_FILEIO if seek fails
	//
	// Algorithm:
	//		1. Calculate which block contains target sample
	//		2. Seek file to start of that block
	//		3. Load block header
	//		4. Decode samples from block start up to target position
	//		   (required because ADPCM prediction depends on previous samples)
	//
	// Notes:
	//		Stream cursor must be at beginning of waveform data when called
	//		Cannot skip mid-block (must decode all samples to maintain state)
	//		Position is in samples, accounting for channels via shift
	//
	//=============================================================================
	aalError CodecADPCM::SetPosition(const aalULong & _position)
	{
		aalError error;

		// Calculate block index containing target position
		// Divide by samples per block, accounting for channels
		aalULong i = (_position >> shift) / header->wSamplesPerBlock;

		// Seek to start of target block
		if (FileSeek(stream, i * header->wfx.nBlockAlign, SEEK_CUR))
			return AAL_ERROR_FILEIO;

		// Calculate sample offset within target block
		i = _position - i * (header->wSamplesPerBlock << shift);

		// Load block header (initializes predictor state)
		error = GetNextBlock();
		if (error) return error;

		sample_i++;		// Advance past header samples
		cache_i = 0;	// Reset cache

		// Decode samples from block start to target position
		// (Cannot skip - ADPCM prediction requires all previous samples)
		char buffer[256];
		aalULong to_read, read;

		while (i)
		{
			to_read = i >= 256 ? 256 : i;		// Read in 256-sample chunks

			error = Read(buffer, to_read, read);
			if (error) return error;

			i -= read;		// Advance toward target
		}

		cursor = _position;		// Update position tracker

		return AAL_OK;
	}

	//=============================================================================
	// CODEC STATUS QUERY METHODS
	//=============================================================================

	//=============================================================================
	// GetHeader - Retrieve ADPCM Format Header
	//=============================================================================
	aalError CodecADPCM::GetHeader(aalVoid *&_header)
	{
		_header = header;		// Return stored header reference

		return AAL_OK;
	}

	//=============================================================================
	// GetStream - Retrieve File Stream
	//=============================================================================
	aalError CodecADPCM::GetStream(FILE *&_stream)
	{
		_stream = stream;		// Return stored stream reference

		return AAL_OK;
	}

	//=============================================================================
	// GetPosition - Retrieve Current Read Position
	//=============================================================================
	aalError CodecADPCM::GetPosition(aalULong & _position)
	{
		_position = cursor;		// Return current sample position

		return AAL_OK;
	}

	//=============================================================================
	// GetSample - Decode Single ADPCM Sample to PCM
	//=============================================================================
	// Description:
	//		Core ADPCM decoding algorithm - converts 4-bit ADPCM to 16-bit PCM
	//		Uses linear prediction and adaptive delta quantization
	//
	// Parameters:
	//		i: Channel index (0 for mono/left, 1 for right)
	//		adpcm_sample: 4-bit ADPCM value (0-15)
	//
	// Returns:
	//		Nothing (updates samp1[i] with decoded PCM sample)
	//
	// Algorithm:
	//		1. Adapt delta (step size) based on ADPCM value
	//			new_delta = (old_delta * gai_p4[adpcm_sample]) / 256
	//			Minimum delta = 16 (prevents underflow)
	//
	//		2. Sign-extend 4-bit ADPCM to signed value
	//			If bit 3 set, value is negative: subtract 16
	//
	//		3. Predict next sample using linear prediction
	//			predict = (samp1*coef1 + samp2*coef2) / 256
	//			Uses previous 2 samples and predictor coefficients
	//
	//		4. Reconstruct PCM sample
	//			pcm = adpcm_signed * old_delta + predict
	//			Adds scaled delta to prediction
	//
	//		5. Clip to 16-bit signed range [-32768, 32767]
	//			Prevents overflow/distortion
	//
	//		6. Update sample history
	//			samp2 = old samp1 (shift history back)
	//			samp1 = new pcm (store for next prediction)
	//
	// Performance:
	//		Inlined for speed (decodes thousands of samples per frame)
	//		Uses fixed-point math (no floating point)
	//
	//=============================================================================
	__forceinline aalVoid CodecADPCM::GetSample(const aalULong & i, aalSByte adpcm_sample)
	{
		aalSLong predict, pcm_sample, old_delta;

		// Step 1: Adapt delta (step size) using adaptation table
		old_delta = delta[i];
		delta[i] = aalSWord((gai_p4[adpcm_sample] * old_delta) >> 8);	// Fixed-point multiply
		if (delta[i] < 16) delta[i] = 16;		// Clamp minimum delta

		// Step 2: Sign-extend 4-bit ADPCM to signed value
		// Bit 3 is sign bit: 0-7 positive, 8-15 negative
		if (adpcm_sample & 0x08) adpcm_sample -= 16;	// Convert to range [-8, 7]

		// Step 3: Predict next sample using linear prediction
		// Uses previous 2 samples and predictor coefficients
		predict = ((aalSLong)samp1[i] * coef1[i] + (aalSLong)samp2[i] * coef2[i]) >> 8;

		// Step 4: Reconstruct PCM sample
		// Add scaled delta to prediction
		pcm_sample = adpcm_sample * old_delta + predict;

		// Step 5: Clip to 16-bit signed range
		if (pcm_sample > 32767) pcm_sample = 32767;
		else if (pcm_sample < -32768) pcm_sample = -32768;

		// Step 6: Update sample history for next prediction
		samp2[i] = samp1[i];		// Shift previous sample back
		samp1[i] = (aalSWord)pcm_sample;	// Store new sample
	}

	//=============================================================================
	// FILE I/O METHODS
	//=============================================================================

	//=============================================================================
	// Read - Read and Decode ADPCM Audio Data
	//=============================================================================
	// Description:
	//		Reads compressed ADPCM data and decodes to 16-bit PCM samples
	//		Handles block-based structure and nybble unpacking
	//
	// Parameters:
	//		buffer: Destination buffer for decoded PCM samples
	//		to_read: Number of bytes to read (PCM output bytes, not ADPCM)
	//		read: [out] Number of bytes actually decoded
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_FILEIO if file read fails
	//
	// Algorithm:
	//		While output buffer not full:
	//		1. If cached sample available, copy to output (fast path)
	//		2. If block exhausted, skip padding and load next block header
	//		3. If at sample 1, output samp1 from header (second header sample)
	//		4. Otherwise, decode next ADPCM sample:
	//			a. Unpack 4-bit nybble (high then low) from byte
	//			b. Call GetSample to decode ADPCM -> PCM
	//			c. Cache decoded sample
	//		5. Advance sample counter and reset cache index
	//
	// Notes:
	//		Block structure: [header: predictor, delta, samp1, samp2] [data: nybbles]
	//		Header samples (samp1, samp2) output first, then decoded samples
	//		Nybbles packed 2 per byte: high 4 bits first, low 4 bits second
	//		Output is 16-bit PCM samples (2 bytes per sample per channel)
	//
	//=============================================================================
	aalError CodecADPCM::Read(aalVoid * buffer, const aalULong & to_read, aalULong & read)
	{
		read = 0;

		while (read < to_read)
		{
			// Fast path: If cached sample bytes available, copy to output
			if (cache_i < cache_c)
			{
				((aalSByte *)buffer)[read++] = ((aalSByte *)cache_l)[cache_i++];
				continue;
			}

			// Load next block if current block exhausted
			if (sample_i >= header->wSamplesPerBlock)
			{
				aalError error;

				// Skip padding bits at end of block (if any)
				if (padding) FileSeek(stream, padding, SEEK_CUR);

				// Load next block header
				error = GetNextBlock();
				if (error) return error;
			}
			// Special case: Sample 1 from header (second header sample)
			else if (sample_i == 1)
			{
				// Output samp1 from block header for each channel
				for (aalULong i(0); i < header->wfx.nChannels; i++)
					((aalSWord *)cache_l)[i] = samp1[i];
			}
			// Normal case: Decode ADPCM samples
			else
			{
				// Decode one sample for each channel
				for (aalULong i(0); i < header->wfx.nChannels; i++)
				{
					// Unpack nybbles (4-bit samples) from byte
					// Each byte contains 2 samples: high 4 bits, then low 4 bits
					if (odd)
					{
						// Low nybble (bits 0-3)
						GetSample(i, (aalSByte)(nybble & 0x0f));
						odd = AAL_UFALSE;
					}
					else
					{
						// High nybble (bits 4-7) - fetch new byte
						nybble = nybble_l[nybble_i++];
						GetSample(i, aalSByte((nybble >> 4) & 0x0f));
						odd = AAL_UTRUE;
					}

					// Cache decoded sample for output
					((aalSWord *)cache_l)[i] = samp1[i];
				}
			}

			sample_i++;		// Advance to next sample in block
			cache_i = 0;	// Reset cache index (start outputting new sample)
		}

		return AAL_OK;
	}

	//=============================================================================
	// Write - Write Audio Data (NOT IMPLEMENTED)
	//=============================================================================
	// Description:
	//		ADPCM codec is read-only (no encoding support)
	//		Always returns error
	//
	//=============================================================================
	aalError CodecADPCM::Write(aalVoid *, const aalULong &, aalULong & write)
	{
		write = 0;		// No bytes written

		return AAL_ERROR;
	}

	//=============================================================================
	// GetNextBlock - Load ADPCM Block Header
	//=============================================================================
	// Description:
	//		Reads block header from stream and initializes decoder state
	//		Each block contains header with initial state for decoding
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_FILEIO if read fails
	//		AAL_ERROR_FORMAT if predictor index invalid
	//
	// Block Header Structure (per channel):
	//		1 byte:  Predictor index (selects coefficient pair)
	//		2 bytes: Delta (initial step size)
	//		2 bytes: Sample 1 (previous sample for prediction)
	//		2 bytes: Sample 2 (second previous sample for prediction)
	//		N bytes: Compressed nybbles (4-bit ADPCM samples)
	//
	// Algorithm:
	//		1. Read header fields for all channels
	//		2. Validate predictor indices
	//		3. Load predictor coefficients from header table
	//		4. Initialize cache with sample 2 (first output sample)
	//		5. Read compressed nybble data for block
	//
	// Notes:
	//		Header samples (samp1, samp2) are first 2 PCM samples of block
	//		They're output directly, then used for predicting remaining samples
	//		Predictor coefficients vary per block for better compression
	//
	//=============================================================================
	aalError CodecADPCM::GetNextBlock()
	{
		// Read block header fields for all channels
		if (!FileRead(predictor, sizeof(aalUByte) << shift, 1, stream)) return AAL_ERROR_FILEIO;
		if (!FileRead(delta, sizeof(aalSWord) << shift, 1, stream)) return AAL_ERROR_FILEIO;
		if (!FileRead(samp1, sizeof(aalSWord) << shift, 1, stream)) return AAL_ERROR_FILEIO;
		if (!FileRead(samp2, sizeof(aalSWord) << shift, 1, stream)) return AAL_ERROR_FILEIO;

		// Reset decode state
		odd = AAL_UFALSE;		// Start with high nybble
		sample_i = 0;			// First sample in block
		nybble_i = 0;			// Start of nybble buffer

		// Load predictor coefficients and initialize cache for each channel
		for (aalULong i(0); i < header->wfx.nChannels; i++)
		{
			// Validate predictor index
			if (predictor[i] >= header->wNumCoef) return AAL_ERROR_FORMAT;

			// Load predictor coefficients from table
			coef1[i] = header->aCoef[predictor[i]].iCoef1;
			coef2[i] = header->aCoef[predictor[i]].iCoef2;

			// Initialize output cache with samp2 (first sample to output)
			((aalSWord *)cache_l)[i] = samp2[i];
		}

		// Read compressed nybble data for this block
		if (!FileRead(nybble_l, nybble_c, 1, stream)) return AAL_ERROR_FILEIO;

		return AAL_OK;
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================