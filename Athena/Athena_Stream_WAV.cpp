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
#include "Athena_Stream_WAV.h"
#include <windows.h>
#include <mmreg.h>		// Windows multimedia format definitions
#include "Athena_Codec_RAW.h"
#include "Athena_Codec_ADPCM.h"
#include "Athena_FileIO.h"

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Stream_WAV.cpp - WAV Audio File Streaming
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		WAV (Waveform Audio File Format) streaming implementation
//		Primary audio format for Arx Fatalis music and ambient sounds
//		Supports both uncompressed PCM and ADPCM compression
//
// Purpose:
//		- Stream audio from WAV files for music and ambiances
//		- Parse RIFF/WAV file structure
//		- Support multiple audio codecs (PCM, ADPCM)
//		- Enable long audio playback without loading entire file
//
// WAV File Format (RIFF Structure):
//		WAV files use Microsoft RIFF (Resource Interchange File Format)
//		Chunk-based structure with nested chunks:
//
//		[RIFF Chunk] - Overall container
//			ChunkID: 'RIFF' (4 bytes)
//			ChunkSize: File size - 8 bytes (4 bytes)
//			Format: 'WAVE' (4 bytes)
//
//			[fmt  Chunk] - Format information
//				ChunkID: 'fmt ' (4 bytes, note space)
//				ChunkSize: 16 or more (4 bytes)
//				wFormatTag: Audio format (1=PCM, 2=ADPCM, etc.)
//				nChannels: Channel count (1=mono, 2=stereo)
//				nSamplesPerSec: Sample rate (44100, etc.)
//				nAvgBytesPerSec: Bytes per second
//				nBlockAlign: Block alignment
//				wBitsPerSample: Bits per sample (8, 16, etc.)
//				[Extra format bytes for compressed formats]
//
//			[fact Chunk] - Sample count (compressed formats only)
//				ChunkID: 'fact' (4 bytes)
//				ChunkSize: 4 (4 bytes)
//				SampleCount: Number of decompressed samples
//
//			[data Chunk] - Audio data
//				ChunkID: 'data' (4 bytes)
//				ChunkSize: Data size in bytes (4 bytes)
//				[Audio data bytes...]
//
// Supported Codecs:
//		WAVE_FORMAT_PCM (1): Uncompressed PCM - CodecRAW
//		WAVE_FORMAT_ADPCM (2): MS ADPCM compression - CodecADPCM
//
// Design Pattern:
//		Strategy Pattern - codec selected based on WAV format tag
//		Adapter Pattern - ChunkFile wraps FILE* for chunk parsing
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

//=============================================================================
// Format Casting Macros
//=============================================================================
// Description:
//		Type-safe casts for WAV format structures
//		WAVEFORMATEX used for all formats, extended for ADPCM
//
// AS_FORMAT_PCM: Cast to WAVEFORMATEX (basic PCM format)
// AS_FORMAT_ADPCM: Cast to ADPCMWAVEFORMAT (extended for ADPCM)
//
//=============================================================================
#define AS_FORMAT_PCM(x) ((WAVEFORMATEX *)x)
#define AS_FORMAT_ADPCM(x) ((ADPCMWAVEFORMAT *)x)

	//=============================================================================
	// ChunkFile - RIFF Chunk Parser Helper Class
	//=============================================================================
	// Description:
	//		Helper class for parsing RIFF chunks in WAV files
	//		Provides sequential chunk navigation and reading
	//
	// Purpose:
	//		- Find specific chunks by ID ('fmt ', 'data', etc.)
	//		- Read chunk data with automatic offset tracking
	//		- Skip unwanted chunks
	//		- Validate chunk IDs
	//
	// RIFF Chunk Structure:
	//		Every chunk has: [4-byte ID] [4-byte size] [size bytes of data]
	//		Example: 'fmt ' [16] [16 bytes of format data]
	//
	// Usage Pattern:
	//		ChunkFile wave(file);
	//		wave.Check("RIFF");      // Verify RIFF signature
	//		wave.Find("fmt ");       // Find format chunk
	//		wave.Read(&format, 16);  // Read format data
	//
	//=============================================================================
	class ChunkFile
	{
		public:
			//Constructor and destructor
			ChunkFile(FILE * file);
			~ChunkFile();
			//I/O
			aalSBool Read(aalVoid *, const aalULong &);	// Read bytes from current chunk
			aalSBool Skip(const aalULong &);			// Skip N bytes
			aalSBool Find(const char *);				// Find chunk by ID
			aalSBool Check(const char *);				// Verify chunk ID at current position
			aalULong Size()								// Get current chunk size
			{
				return offset;
			};
			aalSBool Restart();							// Seek back to file start
		private:
			//Data
			FILE * file;			// File stream being parsed
			aalULong offset;		// Current chunk data size
	};

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Constructor and destructor                                                //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	//=============================================================================
	// StreamWAV - Constructor
	//=============================================================================
	// Description:
	//		Initialize WAV stream object with default null values
	//
	// Implementation:
	//		- All pointers initialized to NULL for safety
	//		- Size fields initialized to zero
	//		- Stream and codec will be set later via SetStream()
	//
	//=============================================================================
	StreamWAV::StreamWAV() :
		stream(NULL),		// File stream - set by SetStream()
		codec(NULL),		// Audio codec - created based on WAV format tag
		status(NULL),		// Status buffer (unused in current implementation)
		format(NULL),		// WAVEFORMATEX structure - allocated in SetStream()
		size(0),			// Compressed data size in bytes
		outsize(0),			// Decompressed output size in bytes
		offset(0),			// File offset to start of audio data
		cursor(0)			// Current playback position in decompressed data
	{
	}

	//=============================================================================
	// ~StreamWAV - Destructor
	//=============================================================================
	// Description:
	//		Clean up WAV stream resources
	//
	// Implementation:
	//		- Delete codec object (CodecRAW or CodecADPCM)
	//		- Free dynamically allocated format structure
	//		- Stream FILE* is owned by caller, not closed here
	//
	//=============================================================================
	StreamWAV::~StreamWAV()
	{
		if (codec) delete codec;	// Release codec (handles decompression cleanup)

		free(status);				// Free status buffer (if allocated)
		free(format);				// Free WAVEFORMATEX structure
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Setup                                                                     //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	//=============================================================================
	// SetStream - Initialize WAV Stream from File
	//=============================================================================
	// Description:
	//		Parse WAV file structure and initialize codec for streaming
	//
	// Parameters:
	//		_stream: Open FILE* pointing to WAV file (positioned at start)
	//
	// Returns:
	//		AAL_OK: Successfully parsed and initialized
	//		AAL_ERROR_FILEIO: Invalid file pointer
	//		AAL_ERROR_MEMORY: Failed to allocate format structure
	//		AAL_ERROR_FORMAT: Invalid WAV structure or unsupported format
	//
	// Algorithm:
	//		1. Parse RIFF header (verify 'RIFF' and 'WAVE' signatures)
	//		2. Find and parse 'fmt ' chunk (format information)
	//		3. For compressed formats: read extra format data and 'fact' chunk
	//		4. Create appropriate codec (CodecRAW for PCM, CodecADPCM for ADPCM)
	//		5. Find 'data' chunk and store offset for streaming
	//		6. Initialize codec with stream and format
	//
	// Implementation Notes:
	//		- WAVEFORMATEX dynamically sized for compressed formats
	//		- ChunkFile helper handles RIFF chunk navigation
	//		- outsize stores decompressed byte count (for seeking/length queries)
	//		- offset stores file position of audio data (for SetPosition())
	//
	//=============================================================================
	aalError StreamWAV::SetStream(FILE * _stream)
	{
		if (!_stream) return AAL_ERROR_FILEIO;

		stream = _stream;

		// Allocate format structure (basic WAVEFORMATEX size initially)
		format = malloc(sizeof(WAVEFORMATEX));

		if (!format) return AAL_ERROR_MEMORY;

		ChunkFile wave(stream);

		//-------------------------------------------------------------------------
		// Parse RIFF/WAVE Header and 'fmt ' Chunk
		//-------------------------------------------------------------------------
		// Expected structure:
		//   [RIFF][size]['WAVE'][fmt ][16+][format data...]
		//
		// Read format fields:
		//   wFormatTag: 1=PCM, 2=ADPCM, etc.
		//   nChannels: 1=mono, 2=stereo
		//   nSamplesPerSec: Sample rate (44100, 22050, etc.)
		//   nAvgBytesPerSec: Bitrate (for streaming buffer size calculation)
		//   nBlockAlign: Block size (important for ADPCM)
		//   wBitsPerSample: 8, 16, etc. (for PCM)
		//-------------------------------------------------------------------------
		if (wave.Check("RIFF") ||						// Verify RIFF signature
		        wave.Skip(4) ||							// Skip file size field
		        wave.Check("WAVE") ||					// Verify WAVE format
		        wave.Find("fmt ") ||					// Find format chunk
		        wave.Read(&AS_FORMAT_PCM(format)->wFormatTag, 2) ||
		        wave.Read(&AS_FORMAT_PCM(format)->nChannels, 2) ||
		        wave.Read(&AS_FORMAT_PCM(format)->nSamplesPerSec, 4) ||
		        wave.Read(&AS_FORMAT_PCM(format)->nAvgBytesPerSec, 4) ||
		        wave.Read(&AS_FORMAT_PCM(format)->nBlockAlign, 2) ||
		        wave.Read(&AS_FORMAT_PCM(format)->wBitsPerSample, 2))
			return AAL_ERROR_FORMAT;

		//-------------------------------------------------------------------------
		// Handle Compressed Format Extra Data
		//-------------------------------------------------------------------------
		// Compressed formats (ADPCM, etc.) have additional format bytes:
		//   cbSize: Size of extra format bytes
		//   [Extra format data specific to codec]
		//
		// Also need 'fact' chunk for compressed formats:
		//   Contains decompressed sample count (for length calculation)
		//-------------------------------------------------------------------------
		if (AS_FORMAT_PCM(format)->wFormatTag != WAVE_FORMAT_PCM)
		{
			aalVoid * ptr;

			// Read size of extra format bytes
			if (wave.Read(&AS_FORMAT_PCM(format)->cbSize, 2)) return AAL_ERROR_FORMAT;

			// Reallocate format structure to hold extra data
			ptr = realloc(format, sizeof(WAVEFORMATEX) + AS_FORMAT_PCM(format)->cbSize);

			if (!ptr) return AAL_ERROR_MEMORY;

			format = ptr;

			// Read extra format bytes (codec-specific parameters)
			wave.Read((char *)format + sizeof(WAVEFORMATEX), AS_FORMAT_PCM(format)->cbSize);

			// Get decompressed sample count from 'fact' chunk
			// (Required for compressed formats to know total output length)
			wave.Find("fact");
			wave.Read(&outsize, 4);		// outsize = decompressed sample count
		}

		//-------------------------------------------------------------------------
		// Create Codec Based on Format Tag
		//-------------------------------------------------------------------------
		// Supported codecs:
		//   WAVE_FORMAT_PCM (1): Uncompressed - use CodecRAW (pass-through)
		//   WAVE_FORMAT_ADPCM (2): MS ADPCM - use CodecADPCM (decompressor)
		//-------------------------------------------------------------------------
		switch (AS_FORMAT_PCM(format)->wFormatTag)
		{
			case WAVE_FORMAT_PCM   :
				codec = new CodecRAW;
				break;

			case WAVE_FORMAT_ADPCM :
				// ADPCM fact chunk has sample count, multiply by 2 for byte count
				// (ADPCM decompresses to 16-bit samples = 2 bytes per sample)
				outsize <<= 1;
				codec = new CodecADPCM;
				break;

			default                :
				return AAL_ERROR_FORMAT;	// Unsupported format
		}

		//-------------------------------------------------------------------------
		// Find Audio Data Chunk and Store Offset
		//-------------------------------------------------------------------------
		// Restart from beginning and skip RIFF header (12 bytes)
		// Find 'data' chunk which contains the actual audio data
		// Store file offset for later seeking (SetPosition)
		//-------------------------------------------------------------------------
		wave.Restart();
		wave.Skip(12);		// Skip 'RIFF' + size + 'WAVE'

		if (wave.Find("data")) return AAL_ERROR_FORMAT;

		size = wave.Size();		// Compressed data size in bytes

		// For PCM, output size equals input size (no compression)
		// For ADPCM, outsize already calculated from 'fact' chunk, multiply by channels
		if (AS_FORMAT_PCM(format)->wFormatTag == WAVE_FORMAT_PCM) outsize = size;
		else outsize *= AS_FORMAT_PCM(format)->nChannels;

		// Store file offset to start of audio data (after 'data' chunk header)
		offset = FileTell(stream);

		//-------------------------------------------------------------------------
		// Initialize Codec
		//-------------------------------------------------------------------------
		aalError error;

		// Give codec the file stream for reading compressed data
		error = codec->SetStream(stream);
		if (error) return error;

		// Give codec the format structure (for ADPCM decompression parameters)
		error = codec->SetHeader(format);
		if (error) return error;

		return AAL_OK;
	}

	//=============================================================================
	// SetFormat - Change Audio Format (Not Supported)
	//=============================================================================
	// Description:
	//		WAV files have fixed format determined by file header
	//		Cannot change format after file is opened
	//
	// Returns:
	//		AAL_ERROR: Operation not supported for WAV streams
	//
	//=============================================================================
	aalError StreamWAV::SetFormat(const aalFormat &)
	{
		return AAL_ERROR;		// Format is read-only from WAV file
	}

	//=============================================================================
	// SetLength - Change Stream Length (Not Supported)
	//=============================================================================
	// Description:
	//		WAV files have fixed length determined by 'data' chunk size
	//		Cannot change length after file is opened
	//
	// Returns:
	//		AAL_ERROR: Operation not supported for WAV streams
	//
	//=============================================================================
	aalError StreamWAV::SetLength(const aalULong &)
	{
		return AAL_ERROR;		// Length is read-only from WAV file
	}

	//=============================================================================
	// SetPosition - Seek to Position in Audio Stream
	//=============================================================================
	// Description:
	//		Seek to specified byte position in decompressed audio data
	//		Used for looping, random access, and resuming playback
	//
	// Parameters:
	//		position: Target position in decompressed bytes
	//
	// Returns:
	//		AAL_OK: Successfully seeked to position
	//		AAL_ERROR_FILEIO: Invalid position or seek failed
	//
	// Implementation:
	//		1. Validate position is within audio data bounds
	//		2. Update cursor (playback position tracker)
	//		3. Seek file stream back to start of audio data
	//		4. Let codec seek to position (handles decompression state)
	//
	// Note:
	//		For ADPCM, codec must decompress from block boundaries
	//		Cannot seek to arbitrary byte - codec handles block alignment
	//
	//=============================================================================
	aalError StreamWAV::SetPosition(const aalULong & position)
	{
		// Validate position is within audio data
		if (position >= outsize) return AAL_ERROR_FILEIO;

		// Update playback cursor
		cursor = position;

		// Reset file stream to start of audio data chunk
		// Codec will then seek forward from this position
		if (FileSeek(stream, offset, SEEK_SET)) return AAL_ERROR_FILEIO;

		// Let codec handle position seeking
		// (For ADPCM, must align to block boundaries and update decoder state)
		return codec->SetPosition(cursor);
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Status                                                                    //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	//=============================================================================
	// GetStream - Retrieve File Stream Pointer
	//=============================================================================
	// Description:
	//		Get the underlying FILE* for this WAV stream
	//
	// Parameters:
	//		_stream: [OUT] Receives FILE* pointer
	//
	// Returns:
	//		AAL_OK: Always succeeds
	//
	//=============================================================================
	aalError StreamWAV::GetStream(FILE *&_stream)
	{
		_stream = stream;
		return AAL_OK;
	}

	//=============================================================================
	// GetFormat - Retrieve Audio Format Information
	//=============================================================================
	// Description:
	//		Get audio format (sample rate, channels, bit depth)
	//		Translates Windows WAVEFORMATEX to Athena aalFormat
	//
	// Parameters:
	//		_format: [OUT] Receives format information
	//
	// Returns:
	//		AAL_OK: Always succeeds
	//
	// Implementation:
	//		- frequency: Copied from nSamplesPerSec (44100, 22050, etc.)
	//		- channels: Copied from nChannels (1=mono, 2=stereo)
	//		- quality: Bit depth
	//			* PCM: wBitsPerSample (8, 16, etc.)
	//			* ADPCM: Always 16 (decompresses to 16-bit)
	//
	//=============================================================================
	aalError StreamWAV::GetFormat(aalFormat & _format)
	{
		// Copy sample rate and channel count directly from WAVEFORMATEX
		_format.frequency = AS_FORMAT_PCM(format)->nSamplesPerSec;
		_format.channels = AS_FORMAT_PCM(format)->nChannels;

		// Determine bit depth based on format tag
		switch (AS_FORMAT_PCM(format)->wFormatTag)
		{
			case WAVE_FORMAT_PCM   :
				// PCM: Use bits per sample from file
				_format.quality = AS_FORMAT_PCM(format)->wBitsPerSample;
				break;

			case WAVE_FORMAT_ADPCM :
				// ADPCM: Always decompresses to 16-bit samples
				_format.quality = 16;
				break;
		}

		return AAL_OK;
	}

	//=============================================================================
	// GetLength - Get Total Audio Length in Bytes
	//=============================================================================
	// Description:
	//		Get total length of decompressed audio data in bytes
	//
	// Parameters:
	//		_length: [OUT] Receives length in bytes (decompressed)
	//
	// Returns:
	//		AAL_OK: Always succeeds
	//
	// Note:
	//		Returns decompressed size, not compressed file size
	//		For PCM: outsize == size (no compression)
	//		For ADPCM: outsize is from 'fact' chunk (decompressed)
	//
	//=============================================================================
	aalError StreamWAV::GetLength(aalULong & _length)
	{
		_length = outsize;		// Decompressed byte count
		return AAL_OK;
	}

	//=============================================================================
	// GetPosition - Get Current Playback Position
	//=============================================================================
	// Description:
	//		Get current playback position in decompressed bytes
	//
	// Parameters:
	//		_position: [OUT] Receives position in bytes (decompressed)
	//
	// Returns:
	//		AAL_OK: Success
	//		Error code from codec if failed
	//
	// Note:
	//		Delegates to codec which tracks actual decompression position
	//
	//=============================================================================
	aalError StreamWAV::GetPosition(aalULong & _position)
	{
		return codec->GetPosition(_position);
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// I/O                                                                       //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	//=============================================================================
	// Read - Read Audio Data from Stream
	//=============================================================================
	// Description:
	//		Read decompressed audio data into buffer
	//		Primary method for streaming audio playback
	//
	// Parameters:
	//		buffer: Output buffer for decompressed audio data
	//		to_read: Number of bytes to read (decompressed)
	//		_read: [OUT] Number of bytes actually read
	//
	// Returns:
	//		AAL_OK: Read successful (or reached end of stream)
	//		Error code from codec if decompression failed
	//
	// Algorithm:
	//		1. Check if at end of stream (cursor >= outsize)
	//		2. Clamp read size to remaining data
	//		3. Delegate to codec for decompression/read
	//		4. Update cursor position
	//
	// Implementation Notes:
	//		- cursor tracks decompressed byte position
	//		- For PCM: codec just reads raw bytes (pass-through)
	//		- For ADPCM: codec decompresses blocks to 16-bit PCM
	//		- Reaching end of stream is not an error (returns AAL_OK with _read=0)
	//
	//=============================================================================
	aalError StreamWAV::Read(aalVoid * buffer, const aalULong & to_read, aalULong & _read)
	{
		_read = 0;

		// Check if already at end of stream
		if (cursor >= outsize) return AAL_OK;

		// Clamp read size to remaining data
		// Don't read past end of audio data
		aalULong count(cursor + to_read > outsize ? outsize - cursor : to_read);

		// Let codec perform the read (decompression for ADPCM, direct read for PCM)
		aalError error;
		error = codec->Read(buffer, count, _read);

		if (error) return error;

		// Update playback cursor
		cursor += _read;

		return AAL_OK;
	}

	//=============================================================================
	// Write - Write Audio Data to Stream (Not Supported)
	//=============================================================================
	// Description:
	//		WAV streams are read-only - writing not supported
	//
	// Parameters:
	//		buffer: [UNUSED] Audio data to write
	//		to_write: [UNUSED] Number of bytes to write
	//		write: [OUT] Set to 0 (no bytes written)
	//
	// Returns:
	//		AAL_ERROR: Write operation not supported
	//
	//=============================================================================
	aalError StreamWAV::Write(aalVoid *, const aalULong &, aalULong & write)
	{
		write = 0;
		return AAL_ERROR;		// WAV streams are read-only
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// ChunkFile - RIFF Chunk Parser Implementation                              //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	//=============================================================================
	// ChunkFile - Constructor
	//=============================================================================
	// Description:
	//		Create chunk parser for RIFF file
	//
	// Parameters:
	//		ptr: FILE* to RIFF file (must be positioned at start)
	//
	// Implementation:
	//		- Store file pointer
	//		- Initialize offset to 0 (no chunk loaded yet)
	//
	//=============================================================================
	ChunkFile::ChunkFile(FILE * ptr) : file(ptr), offset(0)
	{
	}

	//=============================================================================
	// ~ChunkFile - Destructor
	//=============================================================================
	// Description:
	//		Cleanup chunk parser
	//
	// Note:
	//		Does not close file - file ownership remains with caller
	//
	//=============================================================================
	ChunkFile::~ChunkFile()
	{
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// ChunkFile I/O Methods                                                     //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	//=============================================================================
	// Read - Read Bytes from Current Chunk
	//=============================================================================
	// Description:
	//		Read data from current chunk (after Find() or Check())
	//		Automatically decrements offset counter
	//
	// Parameters:
	//		buffer: Output buffer for chunk data
	//		size: Number of bytes to read
	//
	// Returns:
	//		AAL_STRUE: Successfully read all bytes
	//		AAL_SFALSE: Read failed (EOF or I/O error)
	//
	// Implementation:
	//		- Read from file at current position
	//		- Decrement offset by bytes read (tracks remaining chunk data)
	//
	//=============================================================================
	aalSBool ChunkFile::Read(aalVoid * buffer, const aalULong & size)
	{
		// Read bytes from file
		if (FileRead(buffer, 1, size, file) != size) return AAL_SFALSE;

		// Decrement offset counter (tracks chunk data remaining)
		if (offset) offset -= size;

		return AAL_STRUE;
	}

	//=============================================================================
	// Skip - Skip Bytes in File
	//=============================================================================
	// Description:
	//		Skip forward N bytes in file
	//		Used to skip unwanted chunk data or headers
	//
	// Parameters:
	//		size: Number of bytes to skip
	//
	// Returns:
	//		AAL_STRUE: Successfully skipped
	//		AAL_SFALSE: Seek failed
	//
	// Implementation:
	//		- Seek forward from current position
	//		- Decrement offset if within chunk
	//
	//=============================================================================
	aalSBool ChunkFile::Skip(const aalULong & size)
	{
		// Seek forward
		if (FileSeek(file, size, SEEK_CUR)) return AAL_SFALSE;

		// Decrement offset counter if within chunk
		if (offset) offset -= size;

		return AAL_STRUE;
	}

	//=============================================================================
	// Find - Find Chunk by ID
	//=============================================================================
	// Description:
	//		Search for RIFF chunk with specified 4-character ID
	//		Skips other chunks until target found
	//
	// Parameters:
	//		id: 4-character chunk ID (e.g., "fmt ", "data", "fact")
	//
	// Returns:
	//		AAL_STRUE: Chunk found, file positioned after chunk header
	//		AAL_SFALSE: Chunk not found or I/O error
	//
	// Algorithm:
	//		1. Skip any remaining data from previous chunk
	//		2. Read chunk ID (4 bytes)
	//		3. Read chunk size (4 bytes)
	//		4. If ID matches: store size in offset, return success
	//		5. If ID doesn't match: skip chunk data, continue search
	//
	// Side Effects:
	//		- offset set to chunk data size when chunk found
	//		- File positioned at start of chunk data
	//
	//=============================================================================
	aalSBool ChunkFile::Find(const char * id)
	{
		aalUByte cc[4];

		// Skip any remaining data from previous chunk
		FileSeek(file, offset, SEEK_CUR);

		// Search through chunks
		while (FileRead(cc, 4, 1, file))		// Read chunk ID
		{
			// Read chunk size
			if (!FileRead(&offset, 4, 1, file)) return AAL_SFALSE;

			// Check if this is the chunk we're looking for
			if (!memcmp(cc, id, 4)) return AAL_STRUE;

			// Skip this chunk's data and continue searching
			if (FileSeek(file, offset, SEEK_CUR)) return AAL_SFALSE;
		}

		return AAL_SFALSE;		// Chunk not found
	}

	//=============================================================================
	// Check - Verify Chunk ID at Current Position
	//=============================================================================
	// Description:
	//		Verify that next 4 bytes match expected chunk ID
	//		Used for validating RIFF structure (e.g., "RIFF", "WAVE")
	//
	// Parameters:
	//		id: Expected 4-character chunk ID
	//
	// Returns:
	//		AAL_STRUE: ID matches
	//		AAL_SFALSE: ID doesn't match or read failed
	//
	// Implementation:
	//		- Read 4 bytes from current position
	//		- Compare to expected ID
	//		- Decrement offset if within chunk
	//
	//=============================================================================
	aalSBool ChunkFile::Check(const char * id)
	{
		aalUByte cc[4];

		// Read 4 bytes
		if (!FileRead(cc, 4, 1, file)) return AAL_SFALSE;

		// Compare to expected ID
		if (memcmp(cc, id, 4)) return AAL_SFALSE;

		// Decrement offset if within chunk
		if (offset) offset -= 4;

		return AAL_STRUE;
	}

	//=============================================================================
	// Restart - Seek to Beginning of File
	//=============================================================================
	// Description:
	//		Reset file position to start
	//		Used to re-parse file after initial pass
	//
	// Returns:
	//		AAL_STRUE: Successfully seeked to start
	//		AAL_SFALSE: Seek failed
	//
	// Implementation:
	//		- Reset offset to 0
	//		- Seek file to beginning
	//
	//=============================================================================
	aalSBool ChunkFile::Restart()
	{
		offset = 0;

		if (FileSeek(file, 0, SEEK_SET)) return AAL_SFALSE;

		return AAL_STRUE;
	}

}//ATHENA::