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
#include "Athena_Track.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Track.cpp - Audio Track Keyframe System
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Audio track system with keyframe-based parameter animation
//		Enables dynamic audio playback with time-varying parameters
//		Used for complex soundscapes with evolving characteristics
//
// Purpose:
//		- Create audio sequences with multiple keyframes (keys)
//		- Animate audio parameters over time (volume, pitch, pan, position)
//		- Support looping keys with randomized variation
//		- Implement dynamic soundscapes (wind, ambiences, etc.)
//		- Time-based audio parameter modulation
//
// Track Concept:
//		A Track is an audio sequence composed of multiple keys (keyframes)
//		Each key defines audio parameters at a specific point in time:
//		- Start time: When the key begins playing
//		- Sample: Which sound sample to play
//		- Loop count: How many times to repeat the sample
//		- Delay: Random delay between loops (min/max range)
//		- Parameters: Volume, pitch, pan, 3D position (x,y,z)
//
// Parameter Animation:
//		Each parameter (volume, pitch, pan, position) has:
//		- min/max: Value range for randomization
//		- interval: Time between parameter changes (ms)
//		- flags: Control flags (random, interpolate, etc.)
//		- cur: Current value (interpolated/randomized)
//
// Example Use Case - Footsteps Track:
//		Key 0: Start=0ms, Sample=footstep1, Volume=0.8-1.0, Pitch=0.9-1.1
//		Key 1: Start=500ms, Sample=footstep2, Volume=0.7-0.9, Pitch=1.0-1.2
//		Key 2: Start=1000ms, Sample=footstep1, Volume=0.8-1.0, Pitch=0.9-1.1
//		Result: Varying footstep sounds with randomized volume/pitch
//
// Example Use Case - Wind Ambiance Track:
//		Key 0: Start=0ms, Sample=wind_gust, Loop=infinite
//		       Volume: 0.3-0.8, Interval=2000ms (2 sec)
//		       Pitch: 0.8-1.2, Interval=3000ms (3 sec)
//		Result: Wind that dynamically varies in volume and pitch
//
// Design Pattern:
//		Keyframe Animation Pattern (from animation/video editing)
//		Composite Pattern (track contains multiple keys)
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	//=============================================================================
	// KEY SETUP METHODS
	//=============================================================================
	// Description:
	//		Methods for configuring individual keys within a track
	//		Each key can have different timing, looping, and audio parameters
	//
	//=============================================================================

	//=============================================================================
	// SetKeyStart - Set Key Start Time
	//=============================================================================
	// Description:
	//		Sets when this key begins playing relative to track start
	//
	// Parameters:
	//		k_id: Key index (0 to key_c-1)
	//		start: Start time in milliseconds from track beginning
	//
	// Returns:
	//		ATHENA_OK on success
	//		ATHENA_ERROR_HANDLE if invalid key ID
	//
	// Example:
	//		SetKeyStart(0, 0);      // Key 0 starts immediately
	//		SetKeyStart(1, 500);    // Key 1 starts after 500ms
	//		SetKeyStart(2, 1000);   // Key 2 starts after 1 second
	//
	//=============================================================================
	ATHENAError Track::SetKeyStart(const SLong & k_id, const ULong & start)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;	// Validate key index

		key_l[k_id].start = start;		// Set start time

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyLoop - Set Key Loop Count
	//=============================================================================
	// Description:
	//		Sets how many times this key's sample should loop
	//
	// Parameters:
	//		k_id: Key index
	//		loop: Loop count (0 = play once, 1 = play twice, etc.)
	//
	// Example:
	//		SetKeyLoop(0, 0);    // Play once, no loop
	//		SetKeyLoop(1, 3);    // Play 4 times total (original + 3 loops)
	//		SetKeyLoop(2, 999);  // Loop many times (pseudo-infinite)
	//
	//=============================================================================
	ATHENAError Track::SetKeyLoop(const SLong & k_id, const ULong & loop)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		key_l[k_id].loop = loop;		// Set loop count

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyDelay - Set Random Delay Between Loops
	//=============================================================================
	// Description:
	//		Sets random delay range between loop iterations
	//		Adds variation to prevent mechanical repetition
	//
	// Parameters:
	//		k_id: Key index
	//		min: Minimum delay in milliseconds
	//		max: Maximum delay in milliseconds
	//
	// Algorithm:
	//		Actual delay = random value between [min, max]
	//		Applied between each loop iteration
	//
	// Example:
	//		SetKeyDelay(0, 100, 300);  // Random delay 100-300ms between loops
	//		SetKeyDelay(1, 0, 0);      // No delay (immediate loop)
	//
	//=============================================================================
	ATHENAError Track::SetKeyDelay(const SLong & k_id, const ULong & min, const ULong & max)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		key_l[k_id].delay_min = min;	// Set minimum delay
		key_l[k_id].delay_max = max;	// Set maximum delay

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyVolume - Set Key Volume Animation Parameters
	//=============================================================================
	// Description:
	//		Sets volume variation parameters for this key
	//		Enables dynamic volume changes during playback
	//
	// Parameters:
	//		k_id: Key index
	//		setting: Volume animation settings
	//			min: Minimum volume (0.0 = silent, 1.0 = full)
	//			max: Maximum volume
	//			interval: Time between volume changes (ms)
	//			flags: Animation flags (random, interpolate, etc.)
	//
	// Algorithm:
	//		Every 'interval' milliseconds:
	//		- New volume = random/interpolated value between [min, max]
	//		- If key currently playing, update instance volume immediately
	//
	// Example:
	//		setting = {0.5, 1.0, 2000, RANDOM};  // Volume varies 0.5-1.0 every 2 sec
	//
	//=============================================================================
	ATHENAError Track::SetKeyVolume(const SLong & k_id, const ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		// Store volume animation parameters
		key->volume.min = setting.min;
		key->volume.max = setting.max;
		key->volume.interval = setting.interval;
		key->volume.flags = setting.flags;

		// If this key is currently playing, update volume immediately
		if (flags & (IS_PLAYING | IS_PAUSED) && key_i == ULong(k_id))
		{
			SLong i_id(GetInstanceID(s_id));
			key->volume.cur = (setting.min + setting.max) / 2.0F;	// Set to midpoint

			if (_inst.IsValid(i_id)) _inst[i_id]->SetVolume(key->volume.cur);
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyPitch - Set Key Pitch Animation Parameters
	//=============================================================================
	// Description:
	//		Sets pitch variation parameters for this key
	//		Enables dynamic pitch changes (playback speed)
	//
	// Parameters:
	//		k_id: Key index
	//		setting: Pitch animation settings
	//			min: Minimum pitch (0.5 = half speed/octave down, 1.0 = normal)
	//			max: Maximum pitch (2.0 = double speed/octave up)
	//			interval: Time between pitch changes (ms)
	//			flags: Animation flags
	//
	// Algorithm:
	//		Every 'interval' milliseconds:
	//		- New pitch = random/interpolated value between [min, max]
	//		- If key currently playing, update instance pitch immediately
	//
	// Notes:
	//		Pitch affects playback speed AND perceived frequency
	//		Pitch 2.0 = plays twice as fast, sounds one octave higher
	//		Pitch 0.5 = plays half speed, sounds one octave lower
	//
	// Example:
	//		setting = {0.9, 1.1, 1000, RANDOM};  // Pitch varies ±10% every 1 sec
	//
	//=============================================================================
	ATHENAError Track::SetKeyPitch(const SLong & k_id, const ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		// Store pitch animation parameters
		key->pitch.min = setting.min;
		key->pitch.max = setting.max;
		key->pitch.interval = setting.interval;
		key->pitch.flags = setting.flags;

		// If this key is currently playing, update pitch immediately
		if (flags & (IS_PLAYING | IS_PAUSED) && key_i == ULong(k_id))
		{
			SLong i_id(GetInstanceID(s_id));
			key->pitch.cur = (setting.min + setting.max) / 2.0F;	// Set to midpoint

			if (_inst.IsValid(i_id)) _inst[i_id]->SetPitch(key->pitch.cur);
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyPan - Set Key Stereo Pan Animation Parameters
	//=============================================================================
	// Description:
	//		Sets stereo panning variation parameters for this key
	//		Controls left/right speaker balance (2D audio only)
	//
	// Parameters:
	//		k_id: Key index
	//		setting: Pan animation settings
	//			min: Minimum pan (-1.0 = full left, 0.0 = center)
	//			max: Maximum pan (0.0 = center, 1.0 = full right)
	//			interval: Time between pan changes (ms)
	//			flags: Animation flags
	//
	// Notes:
	//		Pan only applies to 2D sounds (non-positional)
	//		For 3D sounds, use SetKeyPositionX/Y/Z instead
	//
	// Example:
	//		setting = {-0.5, 0.5, 3000, RANDOM};  // Pan wanders L-R every 3 sec
	//
	//=============================================================================
	ATHENAError Track::SetKeyPan(const SLong & k_id, const ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		Track * track = &track_l[t_id];
		TrackKey * key = &key_l[k_id];

		// Store pan animation parameters
		key->pan.min = setting.min;
		key->pan.max = setting.max;
		key->pan.interval = setting.interval;
		key->pan.flags = setting.flags;

		// If this key is currently playing, update pan immediately
		if (flags & (IS_PLAYING | IS_PAUSED) && key_i == ULong(k_id))
		{
			SLong i_id(GetInstanceID(s_id));
			key->pan.cur = (setting.min + setting.max) / 2.0F;	// Set to midpoint

			if (_inst.IsValid(i_id)) _inst[i_id]->SetPan(key->pan.cur);
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyPositionX - Set Key X-Axis Position Animation Parameters
	//=============================================================================
	// Description:
	//		Sets X-axis (left/right) position variation for 3D audio
	//		Enables moving sounds in 3D space
	//
	// Parameters:
	//		k_id: Key index
	//		setting: X position animation settings
	//			min/max: X coordinate range (world space units)
	//			interval: Time between position changes (ms)
	//			flags: Animation flags
	//
	// Notes:
	//		Only applies to 3D positional sounds (TRACK_3D flag set)
	//		Coordinates are in game world space
	//		Combined with Y/Z for full 3D movement
	//
	// Example:
	//		setting = {-100, 100, 5000, INTERPOLATE};  // Moves L-R over 5 sec
	//
	//=============================================================================
	ATHENAError Track::SetKeyPositionX(const SLong & k_id, const ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		// Store X position animation parameters
		key->x.min = setting.min;
		key->x.max = setting.max;
		key->x.interval = setting.interval;
		key->x.flags = setting.flags;

		// If this key is currently playing, update position immediately
		if (flags & (IS_PLAYING | IS_PAUSED) && key_i == ULong(k_id))
		{
			SLong i_id(GetInstanceID(s_id));

			if (_inst.IsValid(i_id))
			{
				ATHENAVector position;

				_inst[i_id]->GetPosition(position);		// Get current position
				position.x = key->x.cur = (setting.min + setting.max) / 2.0F;	// Update X
				_inst[i_id]->SetPosition(position);		// Apply new position
			}
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyPositionY - Set Key Y-Axis Position Animation Parameters
	//=============================================================================
	// Description:
	//		Sets Y-axis (up/down) position variation for 3D audio
	//		Enables vertical movement of sounds in 3D space
	//
	// Parameters:
	//		k_id: Key index
	//		setting: Y position animation settings (see SetKeyPositionX)
	//
	// Notes:
	//		Y-axis typically represents height/elevation in game world
	//		Combined with X/Z for full 3D positioning
	//
	//=============================================================================
	ATHENAError Track::SetKeyPositionY(const SLong & k_id, const ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		Track * track = &track_l[t_id];
		TrackKey * key = &key_l[k_id];

		// Store Y position animation parameters
		key->y.min = setting.min;
		key->y.max = setting.max;
		key->y.interval = setting.interval;
		key->y.flags = setting.flags;

		// If this key is currently playing, update position immediately
		if (flags & (IS_PLAYING | IS_PAUSED) && key_i == ULong(k_id))
		{
			SLong i_id(GetInstanceID(s_id));

			if (_inst.IsValid(i_id))
			{
				ATHENAVector position;

				_inst[i_id]->GetPosition(position);		// Get current position
				position.y = key->y.cur = (setting.min + setting.max) / 2.0F;	// Update Y
				_inst[i_id]->SetPosition(position);		// Apply new position
			}
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyPositionZ - Set Key Z-Axis Position Animation Parameters
	//=============================================================================
	// Description:
	//		Sets Z-axis (forward/back) position variation for 3D audio
	//		Enables depth movement of sounds in 3D space
	//
	// Parameters:
	//		k_id: Key index
	//		setting: Z position animation settings (see SetKeyPositionX)
	//
	// Notes:
	//		Z-axis typically represents depth/distance in game world
	//		Combined with X/Y for full 3D positioning
	//
	//=============================================================================
	ATHENAError Track::SetKeyPositionZ(const SLong & k_id, const ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		// Store Z position animation parameters
		key->z.min = setting.min;
		key->z.max = setting.max;
		key->z.interval = setting.interval;
		key->z.flags = setting.flags;

		// If this key is currently playing, update position immediately
		if (flags & (IS_PLAYING | IS_PAUSED) && key_i == ULong(k_id))
		{
			SLong i_id(GetInstanceID(s_id));

			if (_inst.IsValid(i_id))
			{
				ATHENAVector position;

				_inst[i_id]->GetPosition(position);		// Get current position
				position.z = key->z.cur = (setting.min + setting.max) / 2.0F;	// Update Z
				_inst[i_id]->SetPosition(position);		// Apply new position
			}
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// KEY STATUS QUERY METHODS
	//=============================================================================
	// Description:
	//		Methods for retrieving key configuration parameters
	//		Mirror the SetKey methods for complete get/set interface
	//
	//=============================================================================

	//=============================================================================
	// GetKeyStart - Retrieve Key Start Time
	//=============================================================================
	ATHENAError Track::GetKeyStart(const SLong & k_id, ULong & start)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		start = key_l[k_id].start;		// Return start time

		return ATHENA_OK;
	}

	// GetKeyLoop - Retrieve key loop count
	ATHENAError Track::GetKeyLoop(const SLong & k_id, ULong & loop)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		loop = key_l[k_id].loop;		// Return loop count

		return ATHENA_OK;
	}

	// GetKeyDelay - Retrieve key delay range
	ATHENAError Track::GetKeyDelay(const SLong & k_id, ULong & min, ULong & max)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		min = key->delay_min;		// Return minimum delay
		max = key->delay_max;		// Return maximum delay

		return ATHENA_OK;
	}

	// GetKeyVolume - Retrieve key volume animation parameters
	ATHENAError Track::GetKeyVolume(const SLong & k_id, ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		setting.min = key->volume.min;
		setting.max = key->volume.max;
		setting.interval = key->volume.interval;
		setting.flags = key->volume.flags;

		return ATHENA_OK;
	}

	// GetKeyPitch - Retrieve key pitch animation parameters
	ATHENAError Track::GetKeyPitch(const SLong & k_id, ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		setting.min = key->pitch.min;
		setting.max = key->pitch.max;
		setting.interval = key->pitch.interval;
		setting.flags = key->pitch.flags;

		return ATHENA_OK;
	}

	// GetKeyPan - Retrieve key pan animation parameters
	ATHENAError Track::GetKeyPan(const SLong & k_id, ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		setting.min = key->pan.min;
		setting.max = key->pan.max;
		setting.interval = key->pan.interval;
		setting.flags = key->pan.flags;

		return ATHENA_OK;
	}

	// GetKeyPositionX - Retrieve key X position animation parameters
	ATHENAError Track::GetKeyPositionX(const SLong & k_id, ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		setting.min = key->x.min;
		setting.max = key->x.max;
		setting.interval = key->x.interval;
		setting.flags = key->x.flags;

		return ATHENA_OK;
	}

	// GetKeyPositionY - Retrieve key Y position animation parameters
	ATHENAError Track::GetKeyPositionY(const SLong & k_id, ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		setting.min = key->y.min;
		setting.max = key->y.max;
		setting.interval = key->y.interval;
		setting.flags = key->y.flags;

		return ATHENA_OK;
	}

	// GetKeyPositionZ - Retrieve key Z position animation parameters
	ATHENAError Track::GetKeyPositionZ(const SLong & k_id, ATHENAKeySetting & setting)
	{
		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		TrackKey * key = &key_l[k_id];

		setting.min = key->z.min;
		setting.max = key->z.max;
		setting.interval = key->z.interval;
		setting.flags = key->z.flags;

		return ATHENA_OK;
	}

	//=============================================================================
	// GetKeyLength - Calculate Total Key Playback Duration
	//=============================================================================
	// Description:
	//		Calculates total time this key will play including all loops
	//		Accounts for variable pitch affecting playback duration
	//
	// Parameters:
	//		k_id: Key index
	//		length: [out] Total duration in milliseconds
	//
	// Algorithm:
	//		1. Get sample length in bytes
	//		2. If pitch varies with interval:
	//		   - Calculate alternating pitch durations (min/max/min/max...)
	//		   - Sum all interval durations
	//		3. Otherwise:
	//		   - Half loops at min pitch, half at max pitch
	//		4. Add start delay + loop delays
	//
	// Notes:
	//		Higher pitch = faster playback = shorter duration
	//		Pitch 2.0 plays in half the time of pitch 1.0
	//		Complex calculation for accurate timing with pitch variation
	//
	//=============================================================================
	ATHENAError Track::GetKeyLength(const SLong & k_id, ULong & length)
	{
		Float f_lgt(0.0F);
		SLong s_id;
		Sample * sample;

		if (ULong(k_id) >= key_c) return ATHENA_ERROR_HANDLE;

		s_id = GetSampleID(s_id);

		if (_sample.IsNotValid(s_id))
		{
			length = 0;		// No sample = zero length
			return ATHENA_OK;
		}

		sample = _sample[s_id];

		TrackKey * key = &key_l[k_id];

		// If pitch changes at intervals, calculate alternating durations
		if (key->pitch.interval)
		{
			Float min, max, cur;
			Float size;

			length = sample->length;	// Sample size in bytes
			size = Float(length) * (key->loop + 1);		// Total bytes to play

			// Calculate bytes consumed per interval at each pitch
			min = key->pitch.min * sample->format.frequency * key->pitch.interval * 0.001F;
			max = key->pitch.max * sample->format.frequency * key->pitch.interval * 0.001F;

			cur = max;

			// Alternate between min and max pitch, accumulating time
			while (size > 0.0F)
			{
				f_lgt += key->pitch.interval;		// Add interval duration
				size -= cur = cur == min ? max : min;	// Consume bytes, toggle pitch
			}

			// Adjust for partial final interval
			if (size < 0.0F) f_lgt += key->pitch.interval * (size / cur);

			f_lgt *= 0.5F;	// Average the alternating durations
		}
		else
		{
			// No pitch variation - split loops between min and max pitch
			ULong size, loop;

			sample->GetLength(size);

			// Half the loops at max pitch (faster = shorter)
			loop = (key->loop + 1) / 2;
			f_lgt = (size * loop) * (1.0F / key->pitch.max);

			// Other half at min pitch (slower = longer)
			loop = (key->loop + 1) - loop;
			f_lgt += (size * loop) * (1.0F / key->pitch.min);
		}

		// Total duration = start delay + loop delays + playback time
		length = key->start + (key->loop + 1) * key->delay_max;
		length += ULong(f_lgt);

		return ATHENA_OK;
	}

	//=============================================================================
	// GetKeyLoopLength - Calculate Single Loop Duration
	//=============================================================================
	// Description:
	//		Calculates duration of a specific loop iteration within a key
	//		Used for precise timing of individual loop cycles
	//
	// Parameters:
	//		k_id: Key index
	//		loop_i: Loop iteration index (0 to key->loop)
	//		length: [out] Loop duration in milliseconds
	//
	// Notes:
	//		Even/odd loop indices may have different durations if pitch alternates
	//		Loop 0 might use pitch.max, loop 1 uses pitch.min, etc.
	//
	//=============================================================================
	ATHENAError Track::GetKeyLoopLength(const SLong & k_id, const ULong & loop_i, ULong & length)
	{
		Float f_lgt(0.0F);

		if (ULong(k_id) >= key_c || loop_i > key_l[k_id].loop) return ATHENA_ERROR_HANDLE;

		SLong s_id(GetSampleID(s_id));

		if (_sample.IsNotValid(s_id))
		{
			length = 0;		// No sample = zero length
			return ATHENA_OK;
		}

		Sample * sample = _sample[s_id];
		TrackKey * key = &key_l[k_id];

		// If pitch alternates at intervals, calculate duration
		if (key->pitch.interval)
		{
			Float min, max, cur;
			Float size;

			length = sample->length;	// Length in bytes
			size = Float(length) * (key->loop + 1);

			// Calculate bytes per interval at each pitch
			min = key->pitch.min * sample->format.frequency * key->pitch.interval * 0.001F;
			max = key->pitch.max * sample->format.frequency * key->pitch.interval * 0.001F;

			cur = max;

			// Accumulate duration over all intervals
			while (size > 0.0F)
			{
				f_lgt += key->pitch.interval;
				size -= cur = cur == min ? max : min;	// Toggle pitch
			}

			// Adjust for partial final interval
			if (size < 0.0F) f_lgt += key->pitch.interval * (size / cur);
		}
		else
		{
			// Use pitch based on even/odd loop index
			sample->GetLength(length);

			// Odd loops use max pitch, even loops use min pitch
			length = ULong((loop_i & 0x00000001 ? key->pitch.max : key->pitch.min) * length);
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// TRACK SETUP METHODS
	//=============================================================================
	// Description:
	//		Methods for configuring track-level properties
	//		Affects all keys within the track
	//
	//=============================================================================

	//=============================================================================
	// SetName - Set Track Name
	//=============================================================================
	// Description:
	//		Sets or clears track name (used for identification/debugging)
	//
	// Parameters:
	//		_name: Track name string (or empty string to clear)
	//
	// Notes:
	//		If no name set, track uses sample name as fallback
	//
	//=============================================================================
	ATHENAError Track::SetName(const char * _name)
	{
		Void * ptr;

		if (!strlen(_name)) free(name), name = NULL;	// Clear if empty string
		else
		{
			// Allocate and copy new name
			ptr = realloc(name, strlen(_name) + 1);

			if (!ptr) return ATHENA_ERROR_MEMORY;

			name = (char *)ptr;
			strcpy(name, _name);
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// SetMode - Set Track Audio Mode Flags
	//=============================================================================
	// Description:
	//		Configures track mode flags (3D positioning, reverb, etc.)
	//
	// Parameters:
	//		_flags: Mode flags (ATHENA_FLAG_POSITION, ATHENA_FLAG_REVERBERATION)
	//
	// Flags:
	//		ATHENA_FLAG_POSITION: Enable 3D positioning (TRACK_3D)
	//		ATHENA_FLAG_REVERBERATION: Enable environmental reverb (TRACK_REVERB)
	//
	//=============================================================================
	ATHENAError Track::SetMode(const ULong & _flags)
	{
		// Set/clear 3D positioning flag
		if (_flags & ATHENA_FLAG_POSITION)
			flags |= TRACK_3D;
		else
			flags &= ~TRACK_3D;

		// Set/clear reverb flag
		if (_flags & ATHENA_FLAG_REVERBERATION)
			flags |= TRACK_REVERB;
		else
			flags &= ~TRACK_REVERB;

		return ATHENA_OK;
	}

	//=============================================================================
	// SetSample - Set Sample for Track Keys
	//=============================================================================
	// Description:
	//		Associates a sample with this track's keys
	//		All keys use this sample unless overridden
	//
	// Parameters:
	//		s_id: Sample ID (handle to sample resource)
	//
	// Notes:
	//		Sample ID is packed with 0xffff0000 for identification
	//		Invalid sample ID results in no sound playback
	//
	//=============================================================================
	ATHENAError Track::SetSample(const SLong & s_id)
	{
		if (_sample.IsNotValid(GetSampleID(s_id))) s_id = SFALSE;	// Clear if invalid
		else s_id = s_id | 0xffff0000;		// Pack sample ID

		return ATHENA_OK;
	}

	//=============================================================================
	// SetKeyCount - Allocate/Resize Key Array
	//=============================================================================
	// Description:
	//		Sets number of keys (keyframes) in this track
	//		Allocates/reallocates key array to requested size
	//
	// Parameters:
	//		count: Number of keys to allocate
	//
	// Algorithm:
	//		1. If count unchanged, return immediately
	//		2. Reallocate key array to new size
	//		3. If growing, zero new keys and set default values
	//		4. Update key count
	//
	// Notes:
	//		New keys initialized with default volume/pitch (1.0)
	//		Shrinking the array discards keys beyond new count
	//
	//=============================================================================
	ATHENAError Track::SetKeyCount(const ULong & count)
	{
		Void * ptr;

		if (count == key_c) return ATHENA_OK;	// No change needed

		// Reallocate key array
		ptr = realloc(key_l, count * sizeof(TrackKey));

		if (count && !ptr) return ATHENA_ERROR_MEMORY;

		key_l = (TrackKey *)ptr;

		// If growing, initialize new keys
		if (count > key_c)
		{
			// Zero out new keys
			memset(&key_l[key_c], 0, (count - key_c) * sizeof(TrackKey));

			TrackKey * key = &key_l[count];
			TrackKey * key_d = &key_l[key_c];

			// Set default volume/pitch for new keys
			while (key > key_d)
			{
				--key;
				key->volume.min = key->volume.max = key->pitch.min = key->pitch.max = ATHENA_DEFAULT_VOLUME;
			}
		}

		key_c = count;	// Update key count

		return ATHENA_OK;
	}

	//=============================================================================
	// TRACK STATUS QUERY METHODS
	//=============================================================================
	// Description:
	//		Methods for retrieving track configuration and state
	//
	//=============================================================================

	//=============================================================================
	// GetName - Retrieve Track Name
	//=============================================================================
	// Description:
	//		Gets track name (or sample name as fallback)
	//
	// Parameters:
	//		_name: [out] Buffer for name string
	//		max_char: Maximum characters to copy
	//
	// Notes:
	//		If track has no name, uses associated sample's name
	//		If no sample, sets empty string
	//
	//=============================================================================
	ATHENAError Track::GetName(char * _name, const ULong & max_char)
	{
		if (name) strncpy(_name, name, max_char);	// Copy track name if set
		else
		{
			// Fallback to sample name
			SLong s_id(GetSampleID(s_id));

			if (_sample.IsValid(s_id)) strncpy(_name, _sample[s_id]->name, max_char);
			else *_name = 0;	// No sample = empty string
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// GetMode - Retrieve Track Mode Flags
	//=============================================================================
	// Description:
	//		Returns track mode flags (3D, reverb, etc.)
	//
	// Parameters:
	//		_flags: [out] Mode flags (ATHENA_FLAG_POSITION, etc.)
	//
	//=============================================================================
	ATHENAError Track::GetMode(ULong & _flags)
	{
		_flags = 0;

		if (flags & TRACK_3D) _flags |= ATHENA_FLAG_POSITION;	// 3D positioning enabled

		if (flags & TRACK_REVERB) _flags |= ATHENA_FLAG_REVERBERATION;	// Reverb enabled

		return ATHENA_OK;
	}

	//=============================================================================
	// GetSample - Retrieve Associated Sample ID
	//=============================================================================
	// Description:
	//		Returns sample ID used by track keys
	//
	// Parameters:
	//		sample_id: [out] Sample ID (packed format)
	//
	//=============================================================================
	ATHENAError Track::GetSample(SLong & sample_id)
	{
		sample_id = s_id | 0xffff0000;	// Return packed sample ID

		return ATHENA_OK;
	}

	//=============================================================================
	// GetLength - Calculate Total Track Duration
	//=============================================================================
	// Description:
	//		Calculates total playback time for entire track
	//		Sums durations of all keys sequentially
	//
	// Parameters:
	//		length: [out] Total duration in milliseconds
	//
	// Algorithm:
	//		Iterate through all keys, sum their individual lengths
	//
	//=============================================================================
	ATHENAError Track::GetLength(ULong & length)
	{
		length = 0;

		// Sum lengths of all keys
		for (ULong i(0); i < key_c; i++)
		{
			ULong k_lgt;
			GetKeyLength(t_id, i, k_lgt);	// Get key duration
			length += k_lgt;				// Accumulate total
		}

		return ATHENA_OK;
	}

	//=============================================================================
	// GetKeyCount - Retrieve Number of Keys
	//=============================================================================
	// Description:
	//		Returns count of keys (keyframes) in this track
	//
	// Parameters:
	//		count: [out] Number of keys
	//
	//=============================================================================
	ATHENAError Track::GetKeyCount(ULong & count)
	{
		count = key_c;	// Return key count

		return ATHENA_OK;
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================