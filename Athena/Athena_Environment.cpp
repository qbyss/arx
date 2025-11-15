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
#include <stdio.h>
#include <eax.h>		// Creative EAX (Environmental Audio eXtensions) API
#include <math.h>
#include "Athena_Environment.h"
#include "Athena_Global.h"
#include "Athena_FileIO.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Environment.cpp - Environmental Audio Effects (EAX)
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Environmental audio effect system using Creative EAX technology
//		Simulates acoustic properties of different spaces (caves, halls, etc.)
//		Applies reverb, reflections, and absorption based on environment type
//
// Purpose:
//		- Create realistic acoustic environments for immersion
//		- Simulate how sound behaves in different spaces
//		- Apply reverb, echo, and absorption effects
//		- Support EAX hardware acceleration on compatible sound cards
//
// EAX Technology:
//		EAX (Environmental Audio eXtensions) by Creative Labs
//		Hardware-accelerated environmental effects for Sound Blaster cards
//		Simulates acoustic properties: room size, reflections, reverb, etc.
//		Used extensively in games from late 90s / early 2000s
//
// Environmental Parameters:
//		- Size: Room dimensions (affects reverb time)
//		- Diffusion: How evenly sound spreads in space
//		- Absorption: How much walls absorb sound (vs. reflect)
//		- Reflection volume/delay: Early reflections (echoes)
//		- Reverb volume/delay/decay: Late reverberation (tail)
//		- Rolloff: Distance attenuation factor
//
// Typical Environments:
//		- Generic (default): Moderate reverb
//		- Padded cell: Heavy absorption, no reverb
//		- Room: Small space, short reverb
//		- Bathroom: Tile reflections, bright reverb
//		- Living room: Medium absorption, soft reverb
//		- Stone corridor: Long reverb, lots of reflections
//		- Auditorium: Large space, long decay
//		- Cave: Very long reverb, spooky echoes
//		- Underwater: Muffled, dense reverb
//
// Implementation:
//		Environment class stores acoustic parameters
//		Parameters loaded from file or set programmatically
//		Applied to DirectSound3D listener via EAX property set
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	//=============================================================================
	// Default Environment Parameters
	//=============================================================================
	// Description:
	//		Default values for reflection and reverberation effects
	//		Used as fallback when no environment file loaded
	//
	// DEFAULT_REFLECTION:
	//		Volume: 0.8 (80% of dry signal)
	//		Delay: 7 ms (time before first reflection)
	//
	// DEFAULT_REVERBERATION:
	//		Volume: 1.02 (102% - slightly louder than dry)
	//		Delay: 11 ms (time before reverb tail starts)
	//		Decay: 1490 ms (1.49 seconds - medium reverb)
	//		HF Decay: 1236 ms (high frequencies decay faster)
	//
	//=============================================================================
	static const aalReflection DEFAULT_REFLECTION = { 0.8F, 7 };
	static const aalReverberation DEFAULT_REVERBERATION = { 1.02F, 11, 1490, 1236 };

	//=============================================================================
	// Environment Constructor
	//=============================================================================
	// Description:
	//		Initialize environment with default acoustic parameters
	//		Creates neutral/generic environment preset
	//
	// Default Values:
	//		name: NULL (unnamed environment)
	//		size: Default room size
	//		rolloff: 0.0 (no distance attenuation yet)
	//		diffusion: Default sound diffusion
	//		absorption: Default wall absorption
	//		reflect_volume/delay: Early reflection parameters
	//		reverb_volume/delay/decay/hf_decay: Reverb tail parameters
	//		callback: NULL (no environment change callback)
	//		lpksps: NULL (EAX property set - set later by audio system)
	//
	//=============================================================================
	Environment::Environment() :
		name(NULL),											// No name yet
		size(AAL_DEFAULT_ENVIRONMENT_SIZE),					// Default room size
		rolloff(0.0F),										// No rolloff initially
		diffusion(AAL_DEFAULT_ENVIRONMENT_DIFFUSION),		// Default diffusion
		absorption(AAL_DEFAULT_ENVIRONMENT_ABSORPTION),		// Default absorption
		reflect_volume(AAL_DEFAULT_ENVIRONMENT_REFLECTION_VOLUME),		// Reflection volume
		reflect_delay(aalFloat(AAL_DEFAULT_ENVIRONMENT_REFLECTION_DELAY)),	// Reflection delay
		reverb_volume(AAL_DEFAULT_ENVIRONMENT_REVERBERATION_VOLUME),	// Reverb volume
		reverb_delay(aalFloat(AAL_DEFAULT_ENVIRONMENT_REVERBERATION_DELAY)),	// Reverb delay
		reverb_decay(aalFloat(AAL_DEFAULT_ENVIRONMENT_REVERBERATION_DECAY)),	// Reverb decay time
		reverb_hf_decay(aalFloat(AAL_DEFAULT_ENVIRONMENT_REVERBERATION_HFDECAY)),	// HF decay ratio
		callback(NULL),										// No callback
		lpksps(NULL)										// EAX property set (set later)
	{
	}

	//=============================================================================
	// Environment Destructor
	//=============================================================================
	// Description:
	//		Free environment name string
	//		Other members (lpksps, callback) owned by audio system
	//
	//=============================================================================
	Environment::~Environment()
	{
		free(name);		// Free environment name string
	}

	//=============================================================================
	// FILE I/O METHODS
	//=============================================================================

	//=============================================================================
	// Load - Load Environment from File
	//=============================================================================
	// Description:
	//		Loads environmental acoustic parameters from binary file
	//		Environments stored as presets for different room types
	//
	// Parameters:
	//		_name: Environment filename (e.g., "cave.env", "hallway.env")
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_FILEIO if file not found or read fails
	//
	// File Format (binary):
	//		All values are 4-byte floats
	//		1. size (float) - Room size
	//		2. diffusion (float) - Sound diffusion
	//		3. absorption (float) - Wall absorption
	//		4. reflect_volume (float) - Reflection volume
	//		5. reflect_delay (float) - Reflection delay (ms)
	//		6. reverb_volume (float) - Reverb volume
	//		7. reverb_delay (float) - Reverb delay (ms)
	//		8. reverb_decay (float) - Reverb decay time (ms)
	//		9. reverb_hf_decay (float) - HF decay ratio
	//
	// Notes:
	//		Uses OpenResource for multi-path search
	//		Sets environment name to filename
	//
	//=============================================================================
	aalError Environment::Load(const char * _name)
	{
		// Open environment file (searches multiple paths)
		FILE * file = OpenResource(_name, environment_path);
		if (!file) return AAL_ERROR_FILEIO;

		// Read all environmental parameters (9 floats, 4 bytes each)
		if (!FileRead(&size, 4, 1, file) ||
		        !FileRead(&diffusion, 4, 1, file) ||
		        !FileRead(&absorption, 4, 1, file) ||
		        !FileRead(&reflect_volume, 4, 1, file) ||
		        !FileRead(&reflect_delay, 4, 1, file) ||
		        !FileRead(&reverb_volume, 4, 1, file) ||
		        !FileRead(&reverb_delay, 4, 1, file) ||
		        !FileRead(&reverb_decay, 4, 1, file) ||
		        !FileRead(&reverb_hf_decay, 4, 1, file))
		{
			FileClose(file);
			return AAL_ERROR_FILEIO;		// Read failed
		}

		FileClose(file);

		SetName(_name);		// Store environment name

		return AAL_OK;
	}

	//=============================================================================
	// ENVIRONMENT SETUP METHODS
	//=============================================================================

	//=============================================================================
	// SetName - Set Environment Name
	//=============================================================================
	// Description:
	//		Sets or clears environment name string
	//		Reallocates name buffer to fit new name
	//
	// Parameters:
	//		_name: New name string (or NULL to clear)
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_MEMORY if allocation fails
	//
	// Notes:
	//		Uses realloc to allow name changes without leaking
	//		If _name is NULL, frees existing name
	//
	//=============================================================================
	aalError Environment::SetName(const char * _name)
	{
		if (_name)
		{
			// Allocate new name buffer
			aalULong length(strlen(_name) + 1);		// +1 for null terminator
			aalVoid * ptr = realloc(name, length);
			if (!ptr) return AAL_ERROR_MEMORY;

			name = (char *)ptr;
			memcpy(name, _name, length);		// Copy name string
		}
		else
		{
			// Clear name (free and NULL)
			free(name);
			name = NULL;
		}

		return AAL_OK;
	}

	//=============================================================================
	// SetRolloffFactor - Set Distance Attenuation Factor
	//=============================================================================
	// Description:
	//		Sets how quickly sound attenuates with distance
	//		Higher values = faster attenuation, shorter hearing range
	//
	// Parameters:
	//		_factor: Rolloff factor (0.0 to 10.0)
	//			0.0 = No attenuation (sound same volume at any distance)
	//			1.0 = Natural attenuation (inverse distance law)
	//			>1.0 = Faster attenuation (shorter range)
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_SYSTEM if EAX property set fails
	//
	// Algorithm:
	//		1. Clamp factor to [0.0, 10.0] range
	//		2. If EAX available, apply to listener via property set
	//		3. Use deferred flag (applied at next Update call)
	//
	// Notes:
	//		Only applies if EAX initialized (lpksps != NULL)
	//		Deferred flag batches property changes for efficiency
	//
	//=============================================================================
	aalError Environment::SetRolloffFactor(const aalFloat & _factor)
	{
		// Clamp rolloff factor to valid range [0.0, 10.0]
		rolloff = _factor < 0.0F ? 0.0F  : _factor > 10.0F ? 10.0F : _factor;

		// If EAX property set available, apply rolloff to listener
		if (lpksps)
		{
			if (lpksps->Set(DSPROPSETID_EAX_ListenerProperties,
			                DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR | DSPROPERTY_EAXLISTENER_DEFERRED,
			                NULL, 0, &rolloff, sizeof(aalFloat)))
				return AAL_ERROR_SYSTEM;	// EAX call failed
		}

		return AAL_OK;
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================
