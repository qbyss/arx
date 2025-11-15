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
#include <math.h>
#include "Athena_Global.h"
#include "Athena_Sample.h"
#include "Athena_Ambiance.h"
#include "Athena_FileIO.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Ambiance.cpp - Ambient Sound Environment System
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Ambient soundscape system for complex multi-track audio environments
//		Ambiances contain multiple tracks with keyframe animation (see Track.cpp)
//		Provides layered, evolving audio environments for game locations
//
// Purpose:
//		- Create rich ambient soundscapes for game areas
//		- Layer multiple audio tracks (wind, water, birds, etc.)
//		- Animate track parameters over time (volume, pitch, position)
//		- Support 3D spatialized ambiances
//		- Enable smooth fading and crossfading between ambiances
//		- Load/save ambiance configurations from files
//
// Ambiance System Hierarchy:
//		Ambiance: Container for multiple tracks
//		├── Track 1: Wind (volume animates, looping)
//		├── Track 2: Birds (random chirps with delay variation)
//		├── Track 3: Water (3D positioned at stream location)
//		└── Track 4: Music (fades in/out based on player location)
//
// Track Composition:
//		Each track contains multiple keys (keyframes) - see Athena_Track.cpp
//		Keys define sound samples, timing, looping, and parameter animation
//		Tracks can be master (always play) or conditional
//		Tracks can be muted, paused, or faded independently
//
// Use Cases:
//		Forest Ambiance:
//		- Track 1: Wind rustling (continuous loop, volume varies)
//		- Track 2: Bird chirps (random timing with delays)
//		- Track 3: Distant stream (3D positioned)
//		- Track 4: Crickets (only at night, conditional)
//
//		Dungeon Ambiance:
//		- Track 1: Dripping water (echo, reverb)
//		- Track 2: Distant growls (random intervals)
//		- Track 3: Torch crackling (3D positioned at torches)
//		- Track 4: Ominous music (fades in near boss)
//
// File Format:
//		Ambiances loaded from binary .amb files
//		'GAMB' signature (0x424d4147)
//		Multiple versions supported (1.0.0.0 - 1.0.0.3)
//		Contains all track data, keys, and animation parameters
//
// Fading:
//		Smooth volume transitions for ambiance changes
//		FADE_INTERVAL: 50ms update frequency
//		Supports fade in, fade out, and crossfade
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	//=============================================================================
	// Ambiance File Format Constants
	//=============================================================================
	// GAMB: 'GAMB' signature (0x424d4147) - Identifies ambiance files
	// Versions 1.0.0.0 through 1.0.0.3 supported (incremental features)
	//=============================================================================
	static const aalULong AMBIANCE_FILE_SIGNATURE(0x424d4147);		//'GAMB'
	static const aalULong AMBIANCE_FILE_VERSION_1000(0x01000000);	// Version 1.0.0.0
	static const aalULong AMBIANCE_FILE_VERSION_1001(0x01000001);	// Version 1.0.0.1
	static const aalULong AMBIANCE_FILE_VERSION_1002(0x01000002);	// Version 1.0.0.2
	static const aalULong AMBIANCE_FILE_VERSION_1003(0x01000003);	// Version 1.0.0.3 (current)
	static const aalULong AMBIANCE_FILE_VERSION(AMBIANCE_FILE_VERSION_1003);

	//=============================================================================
	// Ambiance Timing Constants
	//=============================================================================
	// FADE_INTERVAL: Milliseconds between volume updates during fade
	// KEY_CONTINUE: Special value indicating key should continue indefinitely
	//=============================================================================
	static const aalULong FADE_INTERVAL(50);		// 50ms fade update interval
	static const aalULong KEY_CONTINUE(0xffffffff);	// Infinite key duration

	//=============================================================================
	// Track Flags
	//=============================================================================
	// TRACK_3D: Track uses 3D positioning
	// TRACK_REVERB: Track has environmental reverb
	// TRACK_MASTER: Always playing (not conditional)
	// TRACK_MUTED: Track muted (silent but still playing)
	// TRACK_PAUSED: Track paused (can be resumed)
	// TRACK_PREFETCHED: Track audio data preloaded
	//=============================================================================
	enum aalTrackFlag
	{
		TRACK_3D         = 0x00000001,
		TRACK_REVERB     = 0x00000002,
		TRACK_MASTER     = 0x00000004,
		TRACK_MUTED      = 0x00000008,
		TRACK_PAUSED     = 0x00000010,
		TRACK_PREFETCHED = 0x00000020
	};

	enum aalAmbianceFlag
	{
		IS_PLAYING       = 0x00000001,
		IS_PAUSED        = 0x00000002,
		IS_LOOPED        = 0x00000004,
		IS_FADED_UP      = 0x00000008,
		IS_FADED_DOWN    = 0x00000010
	};

	static aalVoid FreeTrack(Track & track);
	static aalVoid ResetSetting(KeySetting & setting);
	static aalFloat UpdateSetting(KeySetting & setting, const aalSLong & timez = 0);
	static aalVoid UpdateKeySynch(TrackKey & key);
	static aalVoid KeyPlay(Track & track, TrackKey & key);
	static aalVoid OnAmbianceSampleStart(aalVoid * inst, const aalSLong &, aalVoid * data);
	static aalVoid OnAmbianceSampleStop(aalVoid *, const aalSLong &, aalVoid * data);

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Constructor and destructor                                                //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////
	Ambiance::Ambiance() :
		flags(0), start(0), time(0),
		track_c(0), track_l(NULL)
	{
		*name = 0;
		channel.flags = 0;
	}

	Ambiance::~Ambiance()
	{
		if (track_l)
		{
			Track * track = &track_l[track_c];

			while (track > track_l) FreeTrack(*(--track));

			free(track_l);
		}
	}

	static aalError LoadAmbianceFileVersion_1000(FILE * file, Ambiance & amb)
	{
		//Read tracks configs
		for (aalULong i(0); i < amb.track_c; ++i)
		{
			char text[256];
			Track * track = &amb.track_l[i];
			TrackKey * key;
			Sample * sample = NULL;
			aalULong j(0);

			track->s_id = AAL_SFALSE;

			//Get track sample name
			do
			{
				if (!FileRead(&text[j], 1, 1, file)) return AAL_ERROR_FILEIO;
			}
			while (text[j++]);


			sample = new Sample;

			if (sample->Load(text))
			{
				delete sample;
				return AAL_ERROR_FILEIO;
			}

			if ((track->s_id = _sample.Add(sample)) == AAL_SFALSE)
			{
				delete sample;
				return AAL_ERROR_MEMORY;
			}

			sample->Catch();

			track->key_c = 1;

			key = track->key_l = (TrackKey *)malloc(sizeof(TrackKey));

			if (!track->key_l) return AAL_ERROR_MEMORY;

			memset(track->key_l, 0, sizeof(TrackKey));

			if (!FileRead(&key->start, 4, 1, file) ||
			        !FileRead(&key->loop, 4, 1, file) ||
			        !FileRead(&key->delay_min, 4, 1, file) ||
			        !FileRead(&key->delay_max, 4, 1, file) ||
			        !FileRead(&key->volume.min, 4, 1, file) ||
			        !FileRead(&key->volume.max, 4, 1, file) ||
			        !FileRead(&key->volume.interval, 4, 1, file) ||
			        !FileRead(&key->volume.flags, 4, 1, file) ||
			        !FileRead(&key->pitch.min, 4, 1, file) ||
			        !FileRead(&key->pitch.max, 4, 1, file) ||
			        !FileRead(&key->pitch.interval, 4, 1, file) ||
			        !FileRead(&key->pitch.flags, 4, 1, file) ||
			        !FileRead(&key->pan.min, 4, 1, file) ||
			        !FileRead(&key->pan.max, 4, 1, file) ||
			        !FileRead(&key->pan.interval, 4, 1, file) ||
			        !FileRead(&key->pan.flags, 4, 1, file) ||
			        !FileRead(&key->x.min, 4, 1, file) ||
			        !FileRead(&key->x.max, 4, 1, file) ||
			        !FileRead(&key->x.interval, 4, 1, file) ||
			        !FileRead(&key->x.flags, 4, 1, file) ||
			        !FileRead(&key->y.min, 4, 1, file) ||
			        !FileRead(&key->y.max, 4, 1, file) ||
			        !FileRead(&key->y.interval, 4, 1, file) ||
			        !FileRead(&key->y.flags, 4, 1, file) ||
			        !FileRead(&key->z.min, 4, 1, file) ||
			        !FileRead(&key->z.max, 4, 1, file) ||
			        !FileRead(&key->z.interval, 4, 1, file) ||
			        !FileRead(&key->z.flags, 4, 1, file))
				return AAL_ERROR_FILEIO;

			if (!FileRead(&track->flags, 4, 1, file)) return AAL_ERROR_FILEIO;

			track->flags &= ~(TRACK_MUTED | TRACK_PAUSED | TRACK_PREFETCHED);
		}

		return AAL_OK;
	}

	static aalError LoadAmbianceFileVersion_1001(FILE * file, Ambiance & amb)
	{
		for (aalULong i(0); i < amb.track_c; ++i)
		{
			char text[256];
			Track * track = &amb.track_l[i];
			Sample * sample = NULL;
			aalULong j(0);

			track->s_id = AAL_SFALSE;

			//Get track sample name
			do
			{
				if (!FileRead(&text[j], 1, 1, file)) return AAL_ERROR_FILEIO;
			}
			while (text[j++]);


			sample = new Sample;

			if (sample->Load(text))
			{
				delete sample;
				return AAL_ERROR_FILEIO;
			}

			if ((track->s_id = _sample.Add(sample)) == AAL_SFALSE)
			{
				delete sample;
				return AAL_ERROR_MEMORY;
			}

			sample->Catch();

			//Read flags and key count
			if (!FileRead(&track->flags, 4, 1, file) || !FileRead(&track->key_c, 4, 1, file))
				return AAL_ERROR_FILEIO;

			track->flags &= ~(TRACK_MUTED | TRACK_PAUSED | TRACK_PREFETCHED);

			if (track->key_c)
			{
				track->key_l = (TrackKey *)malloc(track->key_c * sizeof(TrackKey));

				if (!track->key_l) return AAL_ERROR_MEMORY;

				memset(track->key_l, 0, sizeof(TrackKey) * track->key_c);
			}

			//Read settings for each key
			for (j = 0; j < track->key_c; j++)
			{
				TrackKey * key = &track->key_l[j];

				if (!FileRead(&key->flags, 4, 1, file) ||
				        !FileRead(&key->start, 4, 1, file) ||
				        !FileRead(&key->loop, 4, 1, file) ||
				        !FileRead(&key->delay_min, 4, 1, file) ||
				        !FileRead(&key->delay_max, 4, 1, file) ||
				        !FileRead(&key->volume.min, 4, 1, file) ||
				        !FileRead(&key->volume.max, 4, 1, file) ||
				        !FileRead(&key->volume.interval, 4, 1, file) ||
				        !FileRead(&key->volume.flags, 4, 1, file) ||
				        !FileRead(&key->pitch.min, 4, 1, file) ||
				        !FileRead(&key->pitch.max, 4, 1, file) ||
				        !FileRead(&key->pitch.interval, 4, 1, file) ||
				        !FileRead(&key->pitch.flags, 4, 1, file) ||
				        !FileRead(&key->pan.min, 4, 1, file) ||
				        !FileRead(&key->pan.max, 4, 1, file) ||
				        !FileRead(&key->pan.interval, 4, 1, file) ||
				        !FileRead(&key->pan.flags, 4, 1, file) ||
				        !FileRead(&key->x.min, 4, 1, file) ||
				        !FileRead(&key->x.max, 4, 1, file) ||
				        !FileRead(&key->x.interval, 4, 1, file) ||
				        !FileRead(&key->x.flags, 4, 1, file) ||
				        !FileRead(&key->y.min, 4, 1, file) ||
				        !FileRead(&key->y.max, 4, 1, file) ||
				        !FileRead(&key->y.interval, 4, 1, file) ||
				        !FileRead(&key->y.flags, 4, 1, file) ||
				        !FileRead(&key->z.min, 4, 1, file) ||
				        !FileRead(&key->z.max, 4, 1, file) ||
				        !FileRead(&key->z.interval, 4, 1, file) ||
				        !FileRead(&key->z.flags, 4, 1, file))
					return AAL_ERROR_FILEIO;
			}
		}

		return AAL_OK;
	}

	static aalError LoadAmbianceFileVersion_1002(FILE * file, Ambiance & amb)
	{
		//Read tracks configs
		for (aalULong i(0); i < amb.track_c; i++)
		{
			char text[256];
			Sample * sample = NULL;
			Track * track = &amb.track_l[i];
			aalULong j(0);

			track->s_id = AAL_SFALSE;

			//Get track sample name
			do
			{
				if (!FileRead(&text[j], 1, 1, file)) return AAL_ERROR_FILEIO;
			}
			while (text[j++]);


			//Open sample
			sample = new Sample;

			if (sample->Load(text))
			{
				delete sample;
				return AAL_ERROR_FILEIO;
			}

			if ((track->s_id = _sample.Add(sample)) == AAL_SFALSE)
			{
				delete sample;
				return AAL_ERROR_MEMORY;
			}

			sample->Catch();

			//Get track name (!= sample name)
			j = 0;

			do
			{
				if (!FileRead(&text[j], 1, 1, file)) return AAL_ERROR_FILEIO;
			}
			while (text[j++]);


			if (j - 1)
			{
				track->name = (char *)malloc(j);

				if (!track->name) return AAL_ERROR_FILEIO;

				memcpy(track->name, text, j);
			}

			//Read flags and key count
			if (!FileRead(&track->flags, 4, 1, file) || !FileRead(&track->key_c, 4, 1, file))
				return AAL_ERROR_FILEIO;

			track->flags &= ~(TRACK_MUTED | TRACK_PAUSED | TRACK_PREFETCHED);

			if (track->key_c)
			{
				track->key_l = (TrackKey *)malloc(track->key_c * sizeof(TrackKey));

				if (!track->key_l) return AAL_ERROR_MEMORY;

				memset(track->key_l, 0, sizeof(TrackKey) * track->key_c);
			}

			//Read settings for each key
			for (j = 0; j < track->key_c; j++)
			{
				TrackKey * key = &track->key_l[j];

				if (!FileRead(&key->flags, 4, 1, file) ||
				        !FileRead(&key->start, 4, 1, file) ||
				        !FileRead(&key->loop, 4, 1, file) ||
				        !FileRead(&key->delay_min, 4, 1, file) ||
				        !FileRead(&key->delay_max, 4, 1, file) ||
				        !FileRead(&key->volume.min, 4, 1, file) ||
				        !FileRead(&key->volume.max, 4, 1, file) ||
				        !FileRead(&key->volume.interval, 4, 1, file) ||
				        !FileRead(&key->volume.flags, 4, 1, file) ||
				        !FileRead(&key->pitch.min, 4, 1, file) ||
				        !FileRead(&key->pitch.max, 4, 1, file) ||
				        !FileRead(&key->pitch.interval, 4, 1, file) ||
				        !FileRead(&key->pitch.flags, 4, 1, file) ||
				        !FileRead(&key->pan.min, 4, 1, file) ||
				        !FileRead(&key->pan.max, 4, 1, file) ||
				        !FileRead(&key->pan.interval, 4, 1, file) ||
				        !FileRead(&key->pan.flags, 4, 1, file) ||
				        !FileRead(&key->x.min, 4, 1, file) ||
				        !FileRead(&key->x.max, 4, 1, file) ||
				        !FileRead(&key->x.interval, 4, 1, file) ||
				        !FileRead(&key->x.flags, 4, 1, file) ||
				        !FileRead(&key->y.min, 4, 1, file) ||
				        !FileRead(&key->y.max, 4, 1, file) ||
				        !FileRead(&key->y.interval, 4, 1, file) ||
				        !FileRead(&key->y.flags, 4, 1, file) ||
				        !FileRead(&key->z.min, 4, 1, file) ||
				        !FileRead(&key->z.max, 4, 1, file) ||
				        !FileRead(&key->z.interval, 4, 1, file) ||
				        !FileRead(&key->z.flags, 4, 1, file))
					return AAL_ERROR_FILEIO;
			}
		}

		return AAL_OK;
	}

	static aalError LoadAmbianceFileVersion_1003(FILE * file, Ambiance & amb)
	{
		Track * track = &amb.track_l[amb.track_c];

		//Read tracks configs
		while (track > amb.track_l)
		{
			char text[256];
			Sample * sample = NULL;
			aalULong j(0);

			--track;

			track->s_id = AAL_SFALSE;

			//Get track sample name
			do
			{
				if (!FileRead(&text[j], 1, 1, file)) return AAL_ERROR_FILEIO;
			}
			while (text[j++]);

			//Open sample
			sample = new Sample;

			if (sample->Load(text))
			{
				delete sample;
				return AAL_ERROR_FILEIO;
			}

			if ((track->s_id = _sample.Add(sample)) == AAL_SFALSE)
			{
				delete sample;
				return AAL_ERROR_MEMORY;
			}

			sample->Catch();

			//Get track name (!= sample name)
			j = 0;

			do
			{
				if (!FileRead(&text[j], 1, 1, file)) return AAL_ERROR_FILEIO;
			}
			while (text[j++]);

			if (j - 1)
			{
				track->name = (char *)malloc(j);

				if (!track->name) return AAL_ERROR_FILEIO;

				memcpy(track->name, text, j);
			}

			//Read flags and key count
			if (!FileRead(&track->flags, 4, 1, file) || !FileRead(&track->key_c, 4, 1, file))
				return AAL_ERROR_FILEIO;

			track->flags &= ~(TRACK_MUTED | TRACK_PAUSED | TRACK_PREFETCHED);

			if (track->key_c)
			{
				track->key_l = (TrackKey *)malloc(track->key_c * sizeof(TrackKey));

				if (!track->key_l) return AAL_ERROR_MEMORY;

				memset(track->key_l, 0, sizeof(TrackKey) * track->key_c);
			}

			//Read settings for each key
			TrackKey * key = &track->key_l[track->key_c];

			while (key > track->key_l)
			{
				--key;

				if (!FileRead(&key->flags, 4, 1, file) ||
				        !FileRead(&key->start, 4, 1, file) ||
				        !FileRead(&key->loop, 4, 1, file) ||
				        !FileRead(&key->delay_min, 4, 1, file) ||
				        !FileRead(&key->delay_max, 4, 1, file) ||
				        !FileRead(&key->volume.min, 4, 1, file) ||
				        !FileRead(&key->volume.max, 4, 1, file) ||
				        !FileRead(&key->volume.interval, 4, 1, file) ||
				        !FileRead(&key->volume.flags, 4, 1, file) ||
				        !FileRead(&key->pitch.min, 4, 1, file) ||
				        !FileRead(&key->pitch.max, 4, 1, file) ||
				        !FileRead(&key->pitch.interval, 4, 1, file) ||
				        !FileRead(&key->pitch.flags, 4, 1, file) ||
				        !FileRead(&key->pan.min, 4, 1, file) ||
				        !FileRead(&key->pan.max, 4, 1, file) ||
				        !FileRead(&key->pan.interval, 4, 1, file) ||
				        !FileRead(&key->pan.flags, 4, 1, file) ||
				        !FileRead(&key->x.min, 4, 1, file) ||
				        !FileRead(&key->x.max, 4, 1, file) ||
				        !FileRead(&key->x.interval, 4, 1, file) ||
				        !FileRead(&key->x.flags, 4, 1, file) ||
				        !FileRead(&key->y.min, 4, 1, file) ||
				        !FileRead(&key->y.max, 4, 1, file) ||
				        !FileRead(&key->y.interval, 4, 1, file) ||
				        !FileRead(&key->y.flags, 4, 1, file) ||
				        !FileRead(&key->z.min, 4, 1, file) ||
				        !FileRead(&key->z.max, 4, 1, file) ||
				        !FileRead(&key->z.interval, 4, 1, file) ||
				        !FileRead(&key->z.flags, 4, 1, file))
					return AAL_ERROR_FILEIO;
			}
		}

		return AAL_OK;
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// File input/output                                                         //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	//=============================================================================
	// Load - Load Ambiance from File
	//=============================================================================
	// Description:
	//		Loads multi-track ambiance definition from file
	//		Ambiance files contain multiple tracks with keyframe data
	//
	// Parameters:
	//		_name: Ambiance filename (e.g., "forest.amb")
	//
	// Returns:
	//		AAL_OK: Successfully loaded
	//		AAL_ERROR_FILEIO: File not found or read error
	//		AAL_ERROR_FORMAT: Invalid file signature or version
	//		AAL_ERROR_MEMORY: Failed to allocate track structures
	//
	// File Format:
	//		[4 bytes] Signature (AMBIANCE_FILE_SIGNATURE)
	//		[4 bytes] Version (1000, 1001, 1002, 1003)
	//		[4 bytes] Track count
	//		[Variable] Track data (format varies by version)
	//
	// Algorithm:
	//		1. Free existing tracks if any (supports reload)
	//		2. Open ambiance file using resource path search
	//		3. Read and verify file signature/version
	//		4. Read track count and allocate track array
	//		5. Call version-specific loader (LoadAmbianceFileVersion_XXXX)
	//		6. Close file and store ambiance name
	//
	// Supported Versions:
	//		1000: Original format
	//		1001: Added features (exact details in version loader)
	//		1002: Enhanced format
	//		1003: Latest format
	//
	//=============================================================================
	aalError Ambiance::Load(const char * _name)
	{
		FILE * file = NULL;
		aalULong sign, version;
		aalError error;
		Track * track = NULL;

		if (track_l)
			track = &track_l[track_c];

		flags = start = time = 0;
		*name = 0;

		while (track > track_l) FreeTrack(*(--track));

		free(track_l), track_l = NULL, track_c = 0;

		file = OpenResource(_name, ambiance_path);

		if (!file) return AAL_ERROR_FILEIO;

		//Read file signature and version
		if (!FileRead(&sign, 4, 1, file) || !FileRead(&version, 4, 1, file))
		{
			FileClose(file);
			return AAL_ERROR_FILEIO;
		}

		//Check file signature
		if (sign != AMBIANCE_FILE_SIGNATURE || version > AMBIANCE_FILE_VERSION)
		{
			FileClose(file);
			return AAL_ERROR_FORMAT;
		}

		//Read track count and initialize track structures
		FileRead(&track_c, 4, 1, file);
		track_l = (Track *)malloc(sizeof(Track) * track_c);

		if (!track_l)
		{
			FileClose(file);
			return AAL_ERROR_MEMORY;
		}

		memset(track_l, 0, sizeof(Track) * track_c);

		switch (version)
		{
			case AMBIANCE_FILE_VERSION_1000 :
				error = LoadAmbianceFileVersion_1000(file, *this);
				break;
			case AMBIANCE_FILE_VERSION_1001 :
				error = LoadAmbianceFileVersion_1001(file, *this);
				break;
			case AMBIANCE_FILE_VERSION_1002 :
				error = LoadAmbianceFileVersion_1002(file, *this);
				break;
			case AMBIANCE_FILE_VERSION_1003 :
				error = LoadAmbianceFileVersion_1003(file, *this);
				break;
			default :
				error = AAL_ERROR;
		}


		FileClose(file);
		strcpy(name, _name);

		return error;
	}


	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Key status                                                                //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	aalError Ambiance::GetTrackKeyLength(const aalSLong & t_id, const aalSLong & k_id, aalULong & length)
	{
		aalFloat f_lgt(0.0F);
		aalSLong s_id;
		Sample * sample;

		if (IsNotTrackKey(t_id, k_id)) return AAL_ERROR_HANDLE;

		s_id = GetSampleID(track_l[t_id].s_id);

		if (_sample.IsNotValid(s_id))
		{
			length = 0;
			return AAL_OK;
		}

		sample = _sample[s_id];

		TrackKey * key = &track_l[t_id].key_l[k_id];

		if (key->pitch.interval)
		{
			aalFloat min, max, cur;
			aalFloat size;

			length = sample->length;
			size = aalFloat(length) * (key->loop + 1);

			min = key->pitch.min * sample->format.frequency * key->pitch.interval * 0.001F;
			max = key->pitch.max * sample->format.frequency * key->pitch.interval * 0.001F;

			cur = max;

			while (size > 0.0F)
			{
				f_lgt += key->pitch.interval;
				size -= cur = cur == min ? max : min;
			}

			if (size < 0.0F) f_lgt += key->pitch.interval * (size / cur);

			f_lgt *= 0.5F;
		}
		else
		{
			aalULong size, loop;

			sample->GetLength(size);

			loop = (key->loop + 1) / 2;
			f_lgt = (size * loop) * (1.0F / key->pitch.max);
			loop = (key->loop + 1) - loop;
			f_lgt += (size * loop) * (1.0F / key->pitch.min);
		}

		length = key->start + (key->loop + 1) * key->delay_max;
		length += aalULong(f_lgt);

		return AAL_OK;
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Track status                                                              //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	aalError Ambiance::GetTrackLength(const aalSLong & t_id, aalULong & length)
	{
		if (aalULong(t_id) >= track_c) return AAL_ERROR_HANDLE;

		length = 0;

		for (aalULong i(0); i < track_l[t_id].key_c; i++)
		{
			aalULong k_lgt;
			GetTrackKeyLength(t_id, i, k_lgt);
			length += k_lgt;
		}

		return AAL_OK;
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Setup                                                                     //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	aalError Ambiance::SetUserData(aalVoid * _data)
	{
		data = _data;

		return AAL_OK;
	}

	aalError Ambiance::SetVolume(const aalFloat & _volume)
	{
		if (!(channel.flags & AAL_FLAG_VOLUME)) return AAL_ERROR_INIT;

		channel.volume = (_volume > 1.0F) ? 1.0F : (_volume < 0.0F) ? 0.0F : _volume;

		if (flags & IS_PLAYING)
		{
			Track * track = &track_l[track_c];

			while (track > track_l)
			{
				--track;

				aalSLong s_id(GetSampleID(track->s_id));
				aalSLong i_id(GetInstanceID(track->s_id));

				if (_sample.IsValid(s_id) && _inst.IsValid(i_id) && _inst[i_id]->sample == _sample[s_id])
					_inst[i_id]->SetVolume(track->key_l[track->key_i].volume.cur * channel.volume);
			}
		}

		return AAL_OK;
	}

	aalError Ambiance::MuteTrack(const aalSLong & t_id, const aalUBool & mute)
	{
		Track * track = &track_l[t_id];

		if (IsNotTrack(t_id)) return AAL_ERROR_HANDLE;

		if (mute)
		{
			track->flags |= TRACK_MUTED;

			if (flags & IS_PLAYING)
			{
				aalSLong s_id(GetSampleID(track->s_id));

				if (_sample.IsValid(s_id))
				{
					aalSLong i_id(GetInstanceID(track->s_id));

					if (_inst.IsValid(i_id) && _inst[i_id]->sample == _sample[s_id])
						_inst[i_id]->Stop();
				}
			}
		}
		else
		{
			track->flags &= ~TRACK_MUTED;

			if (flags & IS_PLAYING) KeyPlay(*track, track->key_l[track->key_i = 0]);
		}

		return AAL_OK;
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Status                                                                    //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////
	aalError Ambiance::GetName(char * _name, const aalULong & max_char)
	{
		if (max_char < strlen(name) + 1) return AAL_ERROR_MEMORY;

		strcpy(_name, name);

		return AAL_OK;
	}

	aalError Ambiance::GetUserData(aalVoid ** _data)
	{
		*_data = data;

		return AAL_OK;
	}
	aalError Ambiance::GetTrackID(const char * _name, aalSLong & t_id)
	{
		if (!track_c)
		{
			t_id = AAL_SFALSE;
			return AAL_OK;
		}

		Track * track = &track_l[track_c];

		while (track > track_l)
		{
			--track;

			if (track->name && !_stricmp(track->name, _name)) break;
			else if (!_stricmp(_sample[GetSampleID(track->s_id)]->name, _name)) break;
		}

		t_id = track < track_l ? AAL_SFALSE : track - track_l;

		return AAL_OK;
	}

	aalError Ambiance::GetVolume(aalFloat & volume)
	{
		if (!(channel.flags & AAL_FLAG_VOLUME)) return AAL_ERROR_INIT;

		volume = channel.volume;

		return AAL_OK;
	}

	aalUBool Ambiance::IsPlaying()
	{
		return flags & IS_PLAYING ? AAL_UTRUE : AAL_UFALSE;
	}

	aalUBool Ambiance::IsPaused()
	{
		return flags & IS_PAUSED ? AAL_UTRUE : AAL_UFALSE;
	}

	aalUBool Ambiance::IsLooped()
	{
		return flags & IS_LOOPED ? AAL_UTRUE : AAL_UFALSE;
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Control                                                                   //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////

	//=============================================================================
	// Play - Start Ambiance Playback
	//=============================================================================
	// Description:
	//		Starts playing ambiance with all its tracks
	//		Supports optional fade-in and loop control
	//
	// Parameters:
	//		_channel: Playback channel (mixer, volume, flags)
	//		play_count: Number of times to play (0 = infinite loop)
	//		_fade_interval: Fade-in duration in milliseconds (0 = no fade)
	//
	// Returns:
	//		AAL_OK: Successfully started playback
	//
	// Algorithm:
	//		1. Stop if already playing
	//		2. Set looping flag based on play_count
	//		3. Setup fade-in if fade_interval specified:
	//		   - Start with volume 0
	//		   - Gradually increase to target volume over fade_interval
	//		4. For each track:
	//		   - Register start/stop callbacks on sample
	//		   - Initialize track keys (reset delays and sync)
	//		5. Set playing flag and store start time
	//
	// Fade-In:
	//		If fade_interval > 0:
	//		- Volume starts at 0.0
	//		- Fades up to channel.volume over fade_interval milliseconds
	//		- IS_FADED_UP flag set (processed in Update())
	//
	// Implementation Notes:
	//		- All tracks start simultaneously
	//		- Each track manages its own key playback timing
	//		- Callbacks (OnAmbianceSampleStart/Stop) trigger track keys
	//		- Looping ambiances play indefinitely until stopped
	//
	//=============================================================================
	aalError Ambiance::Play(const aalChannel & _channel, const aalULong & play_count, const aalULong & _fade_interval)
	{
		channel = _channel;

		if (flags & (IS_PLAYING | IS_PAUSED)) Stop();

		if (!play_count) flags |= IS_LOOPED;
		else flags &= ~IS_LOOPED;

		fade_interval = (aalFloat)_fade_interval;

		if (fade_interval)
		{
			flags |= IS_FADED_UP, flags &= ~IS_FADED_DOWN;
			fade_max = channel.volume;
			channel.volume = 0.0F;
			fade_time = 0.0F;
		}

		Track * track = &track_l[track_c];

		while (track > track_l)
		{
			aalSLong s_id;

			--track;

			s_id = GetSampleID(track->s_id);

			if (_sample.IsValid(s_id))
			{
				Sample * sample = _sample[s_id];

				if (!sample->callb_c)
				{
					sample->SetCallback(OnAmbianceSampleStart, track, 0, AAL_UNIT_BYTES);
					sample->SetCallback(OnAmbianceSampleStop, track, sample->length, AAL_UNIT_BYTES);
				}
			}

			//Init track keys
			TrackKey * key = &track->key_l[track->key_c];

			while (key > track->key_l)
			{
				--key;

				key->delay = key->delay_max;
				UpdateKeySynch(*key);
				key->n_start = key->start + key->delay;
				key->loopc = key->loop;

				ResetSetting(key->volume);
				ResetSetting(key->pitch);
				ResetSetting(key->pan);
				ResetSetting(key->x);
				ResetSetting(key->y);
				ResetSetting(key->z);
			}

			track->key_i = 0;
		}

		start = session_time;
		flags |= IS_PLAYING;

		const Mixer * mixer = _mixer[channel.mixer];

		while (mixer)
		{
			if (mixer->IsPaused())
			{
				flags |= IS_PAUSED;
				flags &= ~IS_PLAYING;
				break;
			}

			mixer = mixer->parent;
		}

		return AAL_OK;
	}

	//=============================================================================
	// Stop - Stop Ambiance Playback
	//=============================================================================
	// Description:
	//		Stops ambiance playback for all tracks
	//		Supports optional fade-out
	//
	// Parameters:
	//		_fade_interval: Fade-out duration in milliseconds (0 = instant stop)
	//
	// Returns:
	//		AAL_OK: Successfully stopped or initiated fade-out
	//
	// Algorithm:
	//		1. If not playing: return immediately
	//		2. If paused: resume first (to allow fade-out to work)
	//		3. If fade_interval specified:
	//		   - Set IS_FADED_DOWN flag
	//		   - Volume will fade to 0 over interval in Update()
	//		   - Return (actual stop happens when fade completes)
	//		4. If instant stop (fade_interval = 0):
	//		   - Clear IS_PLAYING flag
	//		   - Stop all track instances
	//
	// Fade-Out:
	//		If fade_interval > 0:
	//		- Volume fades from current to 0.0 over fade_interval ms
	//		- IS_FADED_DOWN flag set (processed in Update())
	//		- Stop occurs automatically when fade completes
	//
	// Implementation Notes:
	//		- Stops all playing instances from all tracks
	//		- If paused, resume before fading (paused sounds can't fade)
	//		- Fade-out is gradual, instant stop is immediate
	//
	//=============================================================================
	aalError Ambiance::Stop(const aalULong & _fade_interval)
	{
		if (!(flags & IS_PLAYING)) return AAL_OK;

		// Resume if paused (can't fade while paused)
		if (flags & IS_PAUSED) Resume();
		else
		{
			fade_interval = (aalFloat)_fade_interval;

			// Setup fade-out if requested
			if (fade_interval)
			{
				flags |= IS_FADED_DOWN, flags &= ~IS_FADED_UP;
				fade_time = 0;
				return AAL_OK;		// Fade will complete in Update()
			}
		}

		// Instant stop
		flags &= ~IS_PLAYING;
		time = 0;

		Track * track = &track_l[track_c];

		while (track > track_l)
		{
			--track;

			aalSLong s_id(GetSampleID(track->s_id));

			if (_sample.IsValid(s_id))
			{
				aalSLong i_id(GetInstanceID(track->s_id));

				if (_inst.IsValid(i_id) && _inst[i_id]->sample == _sample[s_id])
					_inst[i_id]->Stop();
			}

			s_id |= 0xffff0000;
		}

		return AAL_OK;
	}

	aalError Ambiance::Pause()
	{
		if (!(flags & IS_PLAYING)) return AAL_ERROR;

		time = session_time;
		flags &= ~IS_PLAYING;
		flags |= IS_PAUSED;

		Track * track = &track_l[track_c];

		while (track > track_l)
		{
			--track;

			aalSLong s_id(GetSampleID(track->s_id));

			if (_sample.IsValid(s_id))
			{
				aalSLong i_id(GetInstanceID(track->s_id));

				if (_inst.IsValid(i_id))
				{
					Instance * instance = _inst[i_id];

					if (instance->sample == _sample[s_id] && instance->IsPlaying())
					{
						instance->Pause();
						track->flags |= TRACK_PAUSED;
					}
				}
			}
		}

		return AAL_OK;
	}

	aalError Ambiance::Resume()
	{
		if (!(flags & IS_PAUSED)) return AAL_ERROR;

		Track * track = &track_l[track_c];

		while (track > track_l)
		{
			--track;

			if (track->flags & TRACK_PAUSED)
			{
				aalSLong s_id(GetSampleID(track->s_id));
				aalSLong i_id(GetInstanceID(track->s_id));

				if (_inst.IsValid(i_id) && _inst[i_id]->sample == _sample[s_id])
					_inst[i_id]->Resume();

				track->flags &= ~TRACK_PAUSED;
			}
		}

		flags &= ~IS_PAUSED;
		flags |= IS_PLAYING;
		start += session_time - time;
		time = session_time - start;

		return AAL_OK;
	}

	//=============================================================================
	// Update - Per-Frame Ambiance Update
	//=============================================================================
	// Description:
	//		Main update function called each frame for active ambiances
	//		Handles fading, track playback timing, and track updates
	//
	// Returns:
	//		AAL_OK: Update successful
	//
	// Algorithm:
	//		1. Skip if not playing
	//		2. Calculate time elapsed since last frame (interval)
	//		3. Update fade if fading:
	//		   - Fade-up: Increase volume from 0 to target
	//		   - Fade-down: Decrease volume to 0, then stop
	//		4. Update each track:
	//		   - Check if track instance still valid
	//		   - Update track's internal state
	//		   - Handle track-specific timing and parameters
	//
	// Fading System:
	//		IS_FADED_UP (fade-in):
	//			- Volume: 0.0 → fade_max (linear interpolation)
	//			- Duration: fade_interval milliseconds
	//			- Completes when volume reaches fade_max
	//
	//		IS_FADED_DOWN (fade-out):
	//			- Volume: fade_max → 0.0 (linear interpolation)
	//			- Duration: fade_interval milliseconds
	//			- Calls Stop() automatically when volume reaches 0
	//
	// Implementation Notes:
	//		- Called every frame by AAL_Update() for all active ambiances
	//		- time: Total playback time since Play() was called
	//		- interval: Time delta since last Update()
	//		- Each track manages its own keyframe playback independently
	//		- Volume conversion: LinearToLogVolume() for proper attenuation curve
	//
	//=============================================================================
	aalError Ambiance::Update()
	{
		aalULong interval;

		if (!(flags & IS_PLAYING)) return AAL_OK;

		// Calculate time delta since last update
		time += interval = session_time - start - time;

		// Process fade-in or fade-out
		if (fade_interval)
		{
			fade_time += interval;

			if (flags & IS_FADED_UP)
			{
				channel.volume = fade_max * (aalFloat(fade_time) / fade_interval);

				if (channel.volume >= fade_max)
				{
					channel.volume = fade_max;
					fade_interval = 0.0F;
				}

				channel.volume = LinearToLogVolume(channel.volume);
			}
			else if (flags & IS_FADED_DOWN)
			{
				channel.volume = fade_max - fade_max * fade_time / fade_interval;

				if (channel.volume <= 0.0F)
				{
					Stop();
					return AAL_OK;
				}

				channel.volume = LinearToLogVolume(channel.volume);
			}
		}

		//Update tracks
		Track * track = &track_l[track_c];

		while (track > track_l)
		{
			--track;

			aalSLong s_id(GetSampleID(track->s_id));

			if (_sample.IsNotValid(s_id) || track->flags & TRACK_MUTED) continue;

			if (track->key_i < track->key_c)
			{
				TrackKey * key = &track->key_l[track->key_i];

				//Run / update keys
				if (key->n_start <= interval) KeyPlay(*track, *key);
				else
				{
					key->n_start -= interval;

					aalSLong i_id(GetInstanceID(track->s_id));

					if (_inst.IsValid(i_id) && _inst[i_id]->sample == _sample[s_id])
					{
						Instance * instance = _inst[i_id];

						if (key->volume.interval)
						{
							aalFloat value(UpdateSetting(key->volume, time));

							if (channel.flags & AAL_FLAG_VOLUME) value *= channel.volume;

							instance->SetVolume(value);
						}
						else instance->SetVolume(key->volume.cur * channel.volume);

						if (key->pitch.interval) instance->SetPitch(UpdateSetting(key->pitch, time));

						if (track->flags & TRACK_3D)
						{
							aalVector position;
							position.x = key->x.interval ? UpdateSetting(key->x, time) : key->x.cur;
							position.y = key->y.interval ? UpdateSetting(key->y, time) : key->y.cur;
							position.z = key->z.interval ? UpdateSetting(key->z, time) : key->z.cur;

							if (channel.flags & AAL_FLAG_POSITION) position += channel.position;

							instance->SetPosition(position);
						}
						else instance->SetPan(UpdateSetting(key->pan, time));
					}
				}
			}
		}

		return AAL_OK;
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Macros!                                                                   //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////
	inline aalUBool Ambiance::IsNotTrack(const aalSLong & t_id)
	{
		return aalULong(t_id) >= track_c ? AAL_UTRUE : AAL_UFALSE;
	}

	inline aalUBool Ambiance::IsNotTrackKey(const aalSLong & t_id, const aalSLong & k_id)
	{
		return aalULong(t_id) >= track_c || aalULong(k_id) >= track_l[t_id].key_c ? AAL_UTRUE : AAL_UFALSE;
	}

	///////////////////////////////////////////////////////////////////////////////
	//                                                                           //
	// Static                                                                    //
	//                                                                           //
	///////////////////////////////////////////////////////////////////////////////
	static aalVoid FreeTrack(Track & track)
	{
		_sample.Delete(GetSampleID(track.s_id));
		free(track.name);
		free(track.key_l);
	}

	static aalVoid ResetSetting(KeySetting & setting)
	{
		setting.update = 0;

		if (setting.min != setting.max && setting.flags & AAL_KEY_SETTING_FLAG_RANDOM)
			setting.cur = aalFloat(setting.min + fmodf(ARX_CLEAN_WARN_CAST_FLOAT(Random()), setting.max - setting.min));
		else setting.cur = setting.min;

		setting.from = setting.min;
		setting.to = setting.max;
	}

	static aalFloat UpdateSetting(KeySetting & setting, const aalSLong & timez)
	{
		if (setting.min != setting.max)
		{
			aalSLong elapsed(timez - setting.update);

			if (elapsed >= aalSLong(setting.interval))
			{
				elapsed = 0;
				setting.update += setting.interval;

				if (setting.flags & AAL_KEY_SETTING_FLAG_RANDOM)
				{
					setting.from = setting.to;
					setting.to = aalFloat(setting.min + fmod(FRandom(), setting.max - setting.min));
				}
				else
				{
					if (setting.from == setting.min)
						setting.from = setting.max, setting.to = setting.min;
					else
						setting.from = setting.min, setting.to = setting.max;
				}

				setting.cur = setting.from;
			}

			if (setting.flags & AAL_KEY_SETTING_FLAG_INTERPOLATE)
				setting.cur = setting.from + aalFloat(elapsed) / setting.interval * (setting.to - setting.from);
		}

		return setting.cur;
	}

	static aalVoid UpdateKeySynch(TrackKey & key)
	{
		if (key.delay_min != key.delay_max)
		{
			key.delay = key.delay_max - key.delay;
			key.delay += key.delay_min + Random() % (key.delay_max - key.delay_min);
		}
		else key.delay = key.delay_min;
	}

	static aalVoid KeyPlay(Track & track, TrackKey & key)
	{
		aalSLong s_id(GetSampleID(track.s_id));
		aalSLong i_id(GetInstanceID(track.s_id));
		Instance * inst = NULL;

		if (_inst.IsNotValid(i_id) || _inst[i_id]->sample != _sample[s_id])
		{
			Ambiance * ambiance = _amb[track.a_id];
			aalChannel channel;

			channel.mixer = ambiance->channel.mixer;
			channel.flags = AAL_FLAG_CALLBACK | AAL_FLAG_VOLUME | AAL_FLAG_PITCH | AAL_FLAG_RELATIVE;
			channel.flags |= ambiance->channel.flags;
			channel.volume = key.volume.cur;

			if (ambiance->channel.flags & AAL_FLAG_VOLUME) channel.volume *= ambiance->channel.volume;

			channel.pitch = key.pitch.cur;

			if (track.flags & TRACK_3D)
			{
				channel.flags |= AAL_FLAG_POSITION;
				channel.position.x = key.x.cur;
				channel.position.y = key.y.cur;
				channel.position.z = key.z.cur;

				if (ambiance->channel.flags & AAL_FLAG_POSITION)
					channel.position += ambiance->channel.position;

				channel.flags |= ambiance->channel.flags & AAL_FLAG_REVERBERATION;
			}
			else
			{
				channel.flags |= AAL_FLAG_PAN;
				channel.pan = key.pan.cur;
			}

			inst = new Instance;

			if (inst->Init(_sample[s_id], channel) || (i_id = _inst.Add(inst)) == AAL_SFALSE)
			{
				delete inst;
				track.s_id |= 0xffff0000;
				return;
			}

			track.s_id = inst->id = (i_id << 16) | s_id;
		}
		else inst = _inst[i_id];


		if (!key.delay_min && !key.delay_max)
			inst->Play(key.loopc + 1);
		else
			inst->Play();

		key.n_start = KEY_CONTINUE;
	}

	static aalVoid OnAmbianceSampleStart(aalVoid * inst, const aalSLong &, aalVoid * data)
	{
		Instance * instance = (Instance *)inst;
		Track * track = (Track *)data;
		TrackKey * key = &track->key_l[track->key_i];
		aalFloat value;

		//aalUBool prefetch_done(AAL_UFALSE);
		if (track->flags & TRACK_PREFETCHED)
		{
			//	prefetch_done = AAL_UTRUE;
			track->flags &= ~TRACK_PREFETCHED;

			if (track->key_i == track->key_c) key = &track->key_l[track->key_i = 0];

			track->key_l[track->key_i].n_start = KEY_CONTINUE;
			track->key_l[track->key_i].loopc = track->key_l[track->key_i].loop;
		}

		//Prefetch
		if (!key->loopc && _amb[track->a_id]->flags & IS_LOOPED)
		{
			aalULong i(track->key_i + 1);

			if (i == track->key_c) i = 0;

			if (!track->key_l[i].start && !track->key_l[i].delay_min && !track->key_l[i].delay_max)
			{
				instance->Play(track->key_l[i].loop + 1);
				track->flags |= TRACK_PREFETCHED;
			}
		}

		value = UpdateSetting(key->volume);

		if (_amb[track->a_id]->channel.flags & AAL_FLAG_VOLUME)
			value *= _amb[track->a_id]->channel.volume;

		instance->SetVolume(value);

		instance->SetPitch(UpdateSetting(key->pitch));

		if (instance->channel.flags & AAL_FLAG_POSITION)
		{
			aalVector position;

			position.x = UpdateSetting(key->x);
			position.y = UpdateSetting(key->y);
			position.z = UpdateSetting(key->z);

			if (_amb[track->a_id]->channel.flags & AAL_FLAG_POSITION)
				position += _amb[track->a_id]->channel.position;

			instance->SetPosition(position);
		}
		else instance->SetPan(UpdateSetting(key->pan));
	}

	static aalVoid OnAmbianceSampleStop(aalVoid *, const aalSLong &, aalVoid * data)
	{
		Track * track = (Track *)data;
		Ambiance * ambiance = _amb[track->a_id];
		TrackKey * key = &track->key_l[track->key_i];

		if (!key->loopc--)
		{
			//Key end
			key->delay = key->delay_max;
			UpdateKeySynch(*key);
			key->n_start = key->start + key->delay;
			key->loopc = key->loop;
			key->pitch.update -= ambiance->time;

			if (++track->key_i == track->key_c)
			{
				//Track end
				if (track->flags & TRACK_MASTER)
				{
					//Ambiance end
					ambiance->time = 0;

					if (ambiance->flags & IS_LOOPED)
					{
						Track * track2 = &ambiance->track_l[ambiance->track_c];

						while (track2 > ambiance->track_l)
						{
							--track2;

							if (!(track2->flags & TRACK_PREFETCHED)) track2->key_i = 0;
						}

						ambiance->start = session_time;
					}
					else ambiance->flags &= ~IS_PLAYING;
				}
			}
		}
		else if (key->delay_min || key->delay_max)
		{
			UpdateKeySynch(*key);
			key->n_start = key->delay;
		}
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================
