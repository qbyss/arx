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
#include "Athena_Sample.h"
#include "Athena_Global.h"
#include "Athena_Stream.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Sample.cpp - Sound Sample Management
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Sound sample resource management for short audio clips
//		Loads, stores, and manages one-shot sound effects
//		Provides callback system for timed events during playback
//
// Purpose:
//		- Load and store sound effects (footsteps, weapons, UI sounds, etc.)
//		- Manage sample metadata (format, length, name)
//		- Support playback callbacks at specific time points
//		- Resource handle for creating sound instances
//
// Sample vs Ambiance:
//		Sample: Short, one-shot sounds (typically < 5 seconds)
//			- Footsteps, gunshots, door open/close, UI clicks
//			- Loaded fully into memory
//			- Fast to trigger, low latency
//
//		Ambiance: Long, looping background audio (typically > 10 seconds)
//			- Music, wind, rain, crowd noise
//			- Streamed from disk
//			- Lower memory usage
//
// Sample Lifecycle:
//		1. Load: Read WAV file, extract format and length
//		2. Create Instance: Spawn playing instance from sample
//		3. Play: Instance plays from sample data
//		4. Callback: Optional callbacks triggered at specific times
//		5. Destroy: Delete sample (stops all instances using it)
//
// Callback System:
//		Allows game code to synchronize actions with audio
//		Examples:
//			- Trigger animation frame at specific sound moment
//			- Spawn particle effect when sound reaches impact
//			- Chain sounds together
//			- Lip-sync character mouth movements
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	//=============================================================================
	// Sample Constructor
	//=============================================================================
	// Description:
	//		Initialize empty sample
	//		Inherits from ResourceHandle for resource list management
	//
	// Default Values:
	//		name: NULL (no filename yet)
	//		data: NULL (no audio data loaded)
	//		callb_c: 0 (no callbacks)
	//		callb: NULL (callback array)
	//		length: 0 (no audio data)
	//
	//=============================================================================
	Sample::Sample() : ResourceHandle(),
		name(NULL),				// No filename yet
		data(NULL),				// No audio data
		callb_c(0), callb(NULL),	// No callbacks
		length(0)				// Zero length
	{
	}

	//=============================================================================
	// Sample Destructor
	//=============================================================================
	// Description:
	//		Cleanup sample and all associated instances
	//		Stops all sounds currently playing from this sample
	//
	// Algorithm:
	//		1. Delete all instances using this sample (stops playback)
	//		2. Free sample name string
	//		3. Free callback array
	//		4. Free audio data buffer
	//
	// Notes:
	//		Automatically stops all sounds using this sample
	//		Prevents crashes from dangling sample pointers
	//
	//=============================================================================
	Sample::~Sample()
	{
		// Delete all instances referencing this sample
		for (aalULong i(0); i < _inst.Size(); i++)
			if (_inst[i] && _inst[i]->sample == this)
				_inst.Delete(i);		// Stop and delete instance

		free(name);			// Free filename
		free(callb);		// Free callback array
		free(data);			// Free audio data
	}

	//=============================================================================
	// FILE I/O METHODS
	//=============================================================================

	//=============================================================================
	// Load - Load Sound Sample from File
	//=============================================================================
	// Description:
	//		Loads sample metadata from audio file
	//		Does NOT load audio data into memory (loaded on-demand by instances)
	//
	// Parameters:
	//		_name: Audio filename (e.g., "footstep.wav")
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_FILEIO if file not found or invalid format
	//		AAL_ERROR_MEMORY if name allocation fails
	//
	// Algorithm:
	//		1. Open audio stream (CreateStream finds and opens file)
	//		2. Extract format (sample rate, channels, bit depth)
	//		3. Extract length (duration in bytes)
	//		4. Close stream (don't keep file open)
	//		5. Store filename for later reference
	//
	// Notes:
	//		Lightweight operation - only reads file header, not audio data
	//		Audio data loaded later when instance created (lazy loading)
	//		Allows loading many samples without consuming memory
	//
	//=============================================================================
	aalError Sample::Load(const char * _name)
	{
		// Open audio stream (auto-detects format)
		Stream * stream = CreateStream(_name);
		if (!stream) return AAL_ERROR_FILEIO;

		// Extract format and length from file header
		stream->GetFormat(format);		// Sample rate, channels, bit depth
		stream->GetLength(length);		// Duration in bytes
		DeleteStream(stream);			// Close file

		// Store filename for later reference
		aalVoid * ptr = realloc(name, strlen(_name) + 1);
		if (!ptr) return AAL_ERROR_MEMORY;

		name = (char *)ptr;
		strcpy(name, _name);

		return AAL_OK;
	}

	//=============================================================================
	// SAMPLE SETUP METHODS
	//=============================================================================

	//=============================================================================
	// SetCallback - Register Playback Callback
	//=============================================================================
	// Description:
	//		Registers callback function to be called at specific time during playback
	//		Allows synchronizing game events with audio
	//
	// Parameters:
	//		func: Callback function pointer
	//		_data: User data passed to callback
	//		time: Time offset when callback should trigger
	//		unit: Time unit (milliseconds, samples, or bytes)
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_MEMORY if callback array allocation fails
	//
	// Algorithm:
	//		1. Grow callback array by 1
	//		2. Convert time to bytes for consistent storage
	//		3. Clamp time to sample length (prevent out-of-bounds)
	//		4. Store callback function, data, and time
	//
	// Example Usage:
	//		void OnImpact(void* data) { SpawnParticles(); }
	//		sample->SetCallback(OnImpact, NULL, 500, AAL_UNIT_MS);
	//		// Callback fires 500ms into sound playback
	//
	// Notes:
	//		Multiple callbacks can be registered per sample
	//		All instances of sample will trigger callbacks
	//		Callbacks triggered during Instance::Update()
	//
	//=============================================================================
	aalError Sample::SetCallback(aalSampleCallback func, aalVoid * _data, const aalULong & time, const aalUnit & unit)
	{
		aalVoid * ptr;

		// Grow callback array by 1
		ptr = realloc(callb, sizeof(Callback) * (callb_c + 1));
		if (!ptr) return AAL_ERROR_MEMORY;

		callb = (Callback *)ptr;

		// Store callback data
		callb[callb_c].func = func;					// Function pointer
		callb[callb_c].data = _data;				// User data
		callb[callb_c].time = UnitsToBytes(time, format, unit);	// Convert to bytes

		// Clamp callback time to sample length
		if (callb[callb_c].time > length)
			callb[callb_c].time = length;

		callb_c++;		// Increment callback count

		return AAL_OK;
	}

	//=============================================================================
	// SAMPLE STATUS QUERY METHODS
	//=============================================================================

	//=============================================================================
	// GetName - Retrieve Sample Filename
	//=============================================================================
	// Description:
	//		Copies sample filename to output buffer
	//
	// Parameters:
	//		_name: Output buffer
	//		max_char: Maximum characters to copy (buffer size)
	//
	// Returns:
	//		AAL_OK always
	//
	// Notes:
	//		Uses strncpy for safe copying (prevents buffer overflow)
	//
	//=============================================================================
	aalError Sample::GetName(char * _name, const aalULong & max_char)
	{
		strncpy(_name, name, max_char);		// Safe copy with size limit

		return AAL_OK;
	}

	//=============================================================================
	// GetLength - Retrieve Sample Duration
	//=============================================================================
	// Description:
	//		Returns sample duration in specified units
	//		Converts from internal byte representation
	//
	// Parameters:
	//		_length: [out] Sample duration
	//		unit: Desired unit (milliseconds, samples, or bytes)
	//
	// Returns:
	//		AAL_OK always
	//
	// Notes:
	//		Internal storage is in bytes
	//		Converted to requested unit on the fly
	//
	//=============================================================================
	aalError Sample::GetLength(aalULong & _length, const aalUnit & unit)
	{
		_length = BytesToUnits(length, format, unit);	// Convert from bytes

		return AAL_OK;
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================
