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
#include "Athena_Global.h"
#include <time.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Global.cpp - Audio System Global State and Utilities
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Global variables and utility functions for Athena audio system
//		Contains DirectSound interfaces, resource lists, and helper functions
//
// Purpose:
//		- Central storage for audio system state
//		- DirectSound/DirectSound3D interface pointers
//		- Global resource lists (samples, mixers, ambiances, etc.)
//		- Path configuration for audio file loading
//		- Random number generation for audio variation
//		- Time/unit conversion utilities
//
// Global State:
//		- DirectSound device and buffers
//		- 3D listener and EAX environment
//		- Resource paths (samples, ambiances, environments)
//		- Audio format settings
//		- Session timing
//		- Debug logging
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	//=============================================================================
	// AUDIO DEVICE INTERFACE GLOBALS
	//=============================================================================
	// Description:
	//		DirectSound and DirectSound3D COM interface pointers
	//		These are the primary interfaces to Windows audio system
	//
	// device: Main DirectSound device (IDirectSound)
	//		Core audio device interface for sound playback
	//		Used to create buffers, set cooperative level, etc.
	//
	// primary: Primary DirectSound buffer (IDirectSoundBuffer)
	//		Special buffer representing audio output device
	//		Used to set output format and access 3D listener
	//
	// listener: DirectSound3D listener (IDirectSound3DListener)
	//		Represents player's ears in 3D audio space
	//		Controls position, orientation, velocity of listener
	//
	// environment: EAX property set (IKsPropertySet)
	//		Interface for EAX environmental effects
	//		Applies reverb, reflections, and other acoustic properties
	//
	// is_reverb_present: Flag indicating if EAX reverb available
	// environment_id: Currently active environment preset ID
	//
	//=============================================================================
	LPDIRECTSOUND device(NULL);					// DirectSound device
	LPDIRECTSOUNDBUFFER primary(NULL);			// Primary output buffer
	LPDIRECTSOUND3DLISTENER listener(NULL);		// 3D audio listener
	LPKSPROPERTYSET environment(NULL);			// EAX property set
	aalUBool is_reverb_present(AAL_UFALSE);		// EAX reverb availability
	aalSLong environment_id(AAL_SFALSE);		// Current environment ID

	//=============================================================================
	// GLOBAL SETTINGS
	//=============================================================================
	// Description:
	//		Configuration paths, limits, and session state
	//
	// root_path: Root installation directory (e.g., "C:\ArxFatalis\")
	// sample_path: Path to sound effects (e.g., "sfx\")
	// ambiance_path: Path to ambient sound loops (e.g., "amb\")
	// environment_path: Path to environment presets (e.g., "env\")
	//
	// debug_log: Debug log file handle (if logging enabled)
	//
	// stream_limit_ms: Streaming buffer size in milliseconds
	// stream_limit_bytes: Streaming buffer size in bytes (calculated)
	//
	// session_start: System time when audio session started (ms)
	// session_time: Current session time (ms since start)
	//
	// global_status: Status flags (packed resources, etc.)
	// global_format: Output audio format (frequency, channels, quality)
	//
	//=============================================================================
	char * root_path = NULL;						// Root installation path
	char * sample_path = NULL;						// Sound effects path
	char * ambiance_path = NULL;					// Ambiance audio path
	char * environment_path = NULL;					// Environment presets path
	FILE * debug_log = NULL;						// Debug log file
	aalULong stream_limit_ms(AAL_DEFAULT_STREAMLIMIT);	// Streaming limit (ms)
	aalULong stream_limit_bytes = 0;				// Streaming limit (bytes)
	aalULong session_start(0);						// Session start time
	aalULong session_time(0);						// Current session time
	aalULong global_status(0);						// Status flags
	aalFormat global_format = { 0, 0, 0 };			// Output format

	//=============================================================================
	// RESOURCE LISTS
	//=============================================================================
	// Description:
	//		Global lists of all audio resources
	//		Managed via ResourceList template (hash table + linked list)
	//
	// _mixer: Audio mixers (volume control groups)
	// _sample: Sound effect samples (short sounds)
	// _amb: Ambient sound loops (background audio)
	// _env: Environmental presets (reverb/echo settings)
	// _inst: Sound instances (playing sounds)
	//
	//=============================================================================
	ResourceList<Mixer> _mixer;			// Mixer list
	ResourceList<Sample> _sample;		// Sample list
	ResourceList<Ambiance> _amb;		// Ambiance list
	ResourceList<Environment> _env;		// Environment list
	ResourceList<Instance> _inst;		// Instance list

	//=============================================================================
	// UTILITY FUNCTIONS
	//=============================================================================

	//=============================================================================
	// Random Number Generator - Linear Congruential Generator (LCG)
	//=============================================================================
	// Description:
	//		Fast pseudo-random number generator for audio variation
	//		Used for randomizing pitch, volume, sample selection, etc.
	//
	// Algorithm:
	//		Linear Congruential Generator (LCG)
	//		Formula: X[n+1] = (X[n] * FACTOR + SHIFT) mod MODULO
	//
	// Parameters:
	//		SEED: Initial seed value (43)
	//		MODULO: 2^31 - 1 (Mersenne prime for good distribution)
	//		FACTOR: 16807 (Park-Miller "minimal standard" multiplier)
	//		SHIFT: 91 (additive constant)
	//
	// Notes:
	//		Not cryptographically secure, but fast and good enough for audio
	//		Period: ~2.1 billion values before repeating
	//
	//=============================================================================
	static const aalULong SEED = 43;				// Initial seed
	static const aalULong MODULO = 2147483647;		// 2^31 - 1 (max positive signed 32-bit)
	static const aalULong FACTOR = 16807;			// Park-Miller multiplier
	static const aalULong SHIFT = 91;				// Additive constant

	static aalULong __current(SEED);				// Current RNG state

	//=============================================================================
	// Random - Generate Random Integer
	//=============================================================================
	// Returns: Random integer in range [0, MODULO-1]
	//=============================================================================
	aalULong Random()
	{
		return __current = (__current * FACTOR + SHIFT) % MODULO;
	}

	//=============================================================================
	// FRandom - Generate Random Float
	//=============================================================================
	// Returns: Random float in range [0.0, 1.0]
	//=============================================================================
	aalFloat FRandom()
	{
		__current = (__current * FACTOR + SHIFT) % MODULO;
		return aalFloat(__current) / aalFloat(MODULO);	// Normalize to [0, 1]
	}

	//=============================================================================
	// InitSeed - Initialize RNG with System Time
	//=============================================================================
	// Description:
	//		Seeds random number generator with current system time
	//		Ensures different random sequences each run
	//
	// Returns: First random number after seeding
	//=============================================================================
	aalULong InitSeed()
	{
		__current = (aalULong)time(NULL);		// Seed with system time
		return Random();						// Return first random value
	}

	//=============================================================================
	// UnitsToBytes - Convert Time Units to Byte Count
	//=============================================================================
	// Description:
	//		Converts audio duration (milliseconds or samples) to byte count
	//		Used to calculate buffer sizes, seek positions, etc.
	//
	// Parameters:
	//		v: Value in specified units
	//		_format: Audio format (frequency, channels, bit depth)
	//		unit: Unit type (milliseconds, samples, or bytes)
	//
	// Returns:
	//		Byte count corresponding to input value
	//
	// Formulas:
	//		Milliseconds -> Bytes:
	//			bytes = (ms / 1000) * frequency * channels * bytes_per_sample
	//
	//		Samples -> Bytes:
	//			bytes = samples * channels * bytes_per_sample
	//
	//		Bytes -> Bytes:
	//			bytes = v (passthrough)
	//
	// Notes:
	//		_format.quality >> 3: Converts bits per sample to bytes per sample
	//			(16 bits >> 3 = 2 bytes, 8 bits >> 3 = 1 byte)
	//
	//=============================================================================
	aalULong UnitsToBytes(const aalULong & v, const aalFormat & _format, const aalUnit & unit)
	{
		switch (unit)
		{
			case AAL_UNIT_MS:		// Milliseconds -> Bytes
				return aalULong(aalFloat(v) * 0.001F * _format.frequency * _format.channels * (_format.quality >> 3));

			case AAL_UNIT_SAMPLES:	// Samples -> Bytes
				return v * _format.channels * (_format.quality >> 3);
		}

		return v;		// Assume already in bytes
	}

	//=============================================================================
	// BytesToUnits - Convert Byte Count to Time Units
	//=============================================================================
	// Description:
	//		Converts audio byte count to duration (milliseconds or samples)
	//		Inverse of UnitsToBytes
	//
	// Parameters:
	//		v: Byte count
	//		_format: Audio format (frequency, channels, bit depth)
	//		unit: Desired output unit (milliseconds, samples, or bytes)
	//
	// Returns:
	//		Value in specified units
	//
	// Formulas:
	//		Bytes -> Milliseconds:
	//			ms = (bytes / (frequency * channels * bytes_per_sample)) * 1000
	//
	//		Bytes -> Samples:
	//			samples = bytes / (channels * bytes_per_sample)
	//
	//		Bytes -> Bytes:
	//			bytes = v (passthrough)
	//
	//=============================================================================
	aalULong BytesToUnits(const aalULong & v, const aalFormat & _format, const aalUnit & unit)
	{
		switch (unit)
		{
			case AAL_UNIT_MS:		// Bytes -> Milliseconds
				return aalULong(aalFloat(v) * 1000.0F / (_format.frequency * _format.channels * (_format.quality >> 3)));

			case AAL_UNIT_SAMPLES:	// Bytes -> Samples
				return v / (_format.frequency * _format.channels * (_format.quality >> 3));
		}

		return v;		// Assume already in correct units
	}

	//=============================================================================
	// DebugLog - Write Debug Message to Log File
	//=============================================================================
	// Description:
	//		Writes debug text to log file and immediately flushes
	//		Used for debugging audio issues
	//
	// Parameters:
	//		text: Debug message string
	//
	// Notes:
	//		Only writes if debug_log file handle is valid
	//		Flushes immediately to ensure messages written even if crash occurs
	//
	//=============================================================================
	aalVoid DebugLog(const char * text)
	{
		fprintf(debug_log, text);		// Write to log file
		fflush(debug_log);				// Force flush (ensure written to disk)
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================