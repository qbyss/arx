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
#include <math.h>
#include "Athena_Mixer.h"
#include "Athena_Global.h"
#include "Athena_Instance.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Mixer.cpp - Audio Mixer System (Hierarchical Volume Control)
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Audio mixer system for hierarchical volume control
//		Organizes sounds into groups with parent-child relationships
//		Allows batch control of volume, pause/resume for sound categories
//
// Purpose:
//		- Organize sounds into categories (SFX, music, voice, etc.)
//		- Control volume of entire categories at once
//		- Hierarchical mixing (child volumes multiplied by parent volumes)
//		- Batch pause/resume/stop operations
//		- User preference controls (master volume, music volume, etc.)
//
// Mixer Hierarchy Example:
//		Master (100%)
//		├── SFX (80%)
//		│   ├── Combat (50%)  -> Effective: 100% * 80% * 50% = 40%
//		│   └── Ambient (70%) -> Effective: 100% * 80% * 70% = 56%
//		├── Music (60%)       -> Effective: 100% * 60% = 60%
//		└── Voice (90%)       -> Effective: 100% * 90% = 90%
//
// Use Cases:
//		- Master volume control (adjusts all audio)
//		- Category volumes (SFX vs music vs voice)
//		- Situational ducking (lower music during dialog)
//		- Pause all sounds of specific type
//		- Settings menu volume sliders
//
// Implementation:
//		- Mixers form tree structure via parent pointers
//		- Each mixer has volume, pitch, pan settings
//		- Child volumes multiplied by parent volumes (cascading)
//		- Operations (pause/resume/stop) propagate to children
//		- Sounds (instances/ambiances) reference mixer for volume
//
// Design Pattern:
//		Composite Pattern (tree of mixers)
//		Observer Pattern (sounds observe mixer volume changes)
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	//=============================================================================
	// Mixer Status Flags
	//=============================================================================
	// Description:
	//		Bit flags for mixer state
	//
	// IS_PAUSED: Mixer is paused (all child sounds paused)
	//
	//=============================================================================
	static enum MixerFlag
	{
		IS_PAUSED = 0x00000001		// Mixer paused flag
	};

	//=============================================================================
	// Mixer Constructor
	//=============================================================================
	// Description:
	//		Initialize mixer with default values
	//		Creates neutral mixer (no volume/pitch/pan adjustment)
	//
	// Default Values:
	//		name: NULL (unnamed mixer)
	//		status: 0 (not paused)
	//		flags: 0 (no flags)
	//		volume: 1.0 (100% - no attenuation)
	//		pitch: 1.0 (original pitch)
	//		pan: 0.0 (center)
	//		parent: NULL (root mixer - no parent)
	//
	//=============================================================================
	Mixer::Mixer() :
		name(NULL),						// No name yet
		status(0),						// Not paused
		flags(0),						// No flags
		volume(AAL_DEFAULT_VOLUME),		// 100% volume (no attenuation)
		pitch(AAL_DEFAULT_PITCH),		// Original pitch (no change)
		pan(AAL_DEFAULT_PAN),			// Center pan
		parent(NULL)					// No parent (root mixer)
	{
	}

	//=============================================================================
	// Mixer Destructor
	//=============================================================================
	// Description:
	//		Cleanup mixer and all associated resources
	//		Deletes child mixers and stops all sounds using this mixer
	//
	// Algorithm:
	//		1. Delete all child mixers (mixers with this as parent)
	//		2. Delete all playing instances using this mixer
	//		3. Delete all playing ambiances using this mixer
	//		4. Free mixer name string
	//
	// Notes:
	//		Cascading delete - child mixers also delete their children
	//		Sounds are deleted (stopped) to prevent orphaned mixer references
	//
	//=============================================================================
	Mixer::~Mixer()
	{
		aalULong i;

		// Delete all child mixers (mixers parented to this mixer)
		for (i = 0; i < _mixer.Size(); i++)
			if (_mixer[i] && _mixer[i]->parent == this)
				_mixer.Delete(i);

		// Delete all playing instances using this mixer
		for (i = 0; i < _inst.Size(); i++)
			if (_inst[i] &&
			        _inst[i]->IsPlaying() &&
			        _mixer.IsValid(_inst[i]->channel.mixer) &&
			        _mixer[_inst[i]->channel.mixer] == this)
				_inst.Delete(i);

		// Delete all playing ambiances using this mixer
		for (i = 0; i < _amb.Size(); i++)
			if (_amb[i] &&
			        _amb[i]->IsPlaying() &&
			        _mixer.IsValid(_amb[i]->channel.mixer) &&
			        _mixer[_amb[i]->channel.mixer] == this)
				_amb.Delete(i);

		free(name);		// Free mixer name
	}

	//=============================================================================
	// MIXER SETUP METHODS
	//=============================================================================

	//=============================================================================
	// SetName - Set Mixer Name
	//=============================================================================
	// Description:
	//		Sets or clears mixer name (e.g., "SFX", "Music", "Voice")
	//
	// Parameters:
	//		_name: New name string (or NULL to clear)
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR_MEMORY if allocation fails
	//
	//=============================================================================
	aalError Mixer::SetName(const char * _name)
	{
		aalVoid * ptr;

		if (!_name)
		{
			// Clear name
			free(name), name = NULL;
			return AAL_OK;
		}

		// Allocate and copy new name
		ptr = realloc(name, strlen(_name) + 1);
		if (!ptr) return AAL_ERROR_MEMORY;

		name = (char *)ptr;
		strcpy(name, _name);

		return AAL_OK;
	}

	//=============================================================================
	// SetVolume - Set Mixer Volume
	//=============================================================================
	// Description:
	//		Sets mixer volume and propagates change to children and sounds
	//		Implements hierarchical volume control (cascading)
	//
	// Parameters:
	//		v: Volume (0.0 = silent, 1.0 = full)
	//
	// Returns:
	//		AAL_OK always
	//
	// Algorithm:
	//		1. Clamp volume to [0.0, 1.0] range
	//		2. Update all child mixers (triggers their volume recalculation)
	//		3. Update all sound instances using this mixer
	//
	// Notes:
	//		Cascading effect - changing parent volume affects all descendants
	//		Child volumes are multiplied by parent volumes to get effective volume
	//		Example: Parent 50% * Child 80% = 40% effective volume
	//
	//=============================================================================
	aalError Mixer::SetVolume(const aalFloat & v)
	{
		aalULong i;

		// Clamp volume to valid range [0.0, 1.0]
		volume = v > 1.0F ? 1.0F : v < 0.0F ? 0.0F : v;

		// Propagate volume change to all child mixers
		// Each child recalculates its effective volume (parent * child)
		for (i = 0; i < _mixer.Size(); i++)
			if (_mixer[i] && _mixer[i]->parent == this)
				_mixer[i]->SetVolume(_mixer[i]->volume);

		// Update all sound instances using this mixer
		// Sounds recalculate volume based on mixer chain
		for (i = 0; i < _inst.Size(); i++)
			if (_inst[i] && _mixer[_inst[i]->channel.mixer] == this)
				_inst[i]->SetVolume(_inst[i]->channel.volume);

		return AAL_OK;
	}

	//=============================================================================
	// SetParent - Set Parent Mixer for Hierarchy
	//=============================================================================
	// Description:
	//		Sets parent mixer to create hierarchical mixer tree
	//		Prevents circular parent references (would cause infinite loops)
	//
	// Parameters:
	//		_mixer: Parent mixer (or NULL for root mixer)
	//
	// Returns:
	//		AAL_OK on success
	//		AAL_ERROR if would create circular reference
	//
	// Algorithm:
	//		1. Walk up parent chain from new parent
	//		2. Check if 'this' appears in chain (circular reference)
	//		3. If no circular reference found, set parent
	//
	// Example Invalid Hierarchy (circular):
	//		A->parent = B
	//		B->parent = C
	//		C->parent = A  // ERROR - circular!
	//
	//=============================================================================
	aalError Mixer::SetParent(const Mixer * _mixer)
	{
		const Mixer * mixer = _mixer;

		// Walk up parent chain looking for circular reference
		while (mixer)
		{
			if (mixer == this) return AAL_ERROR;	// Circular reference detected!
			mixer = mixer->parent;					// Check next parent
		}

		parent = _mixer;	// No circular reference - safe to set parent
		return AAL_OK;
	}

	//=============================================================================
	// MIXER STATUS QUERY METHODS
	//=============================================================================

	//=============================================================================
	// GetVolume - Retrieve Mixer Volume
	//=============================================================================
	aalError Mixer::GetVolume(aalFloat & _volume) const
	{
		_volume = volume;		// Return mixer's volume setting
		return AAL_OK;
	}

	//=============================================================================
	// IsPaused - Check if Mixer is Paused
	//=============================================================================
	aalUBool Mixer::IsPaused() const
	{
		return status & IS_PAUSED ? AAL_UTRUE : AAL_UFALSE;
	}

	//=============================================================================
	// MIXER CONTROL METHODS
	//=============================================================================

	//=============================================================================
	// Stop - Stop All Sounds on This Mixer
	//=============================================================================
	// Description:
	//		Stops and deletes all sounds using this mixer
	//		Recursively stops all child mixers
	//		Clears paused flag
	//
	// Returns:
	//		AAL_OK always
	//
	// Algorithm:
	//		1. Stop all child mixers (recursive)
	//		2. Stop and delete all ambiances on this mixer
	//		3. Delete all sound instances on this mixer
	//		4. Clear paused flag
	//
	// Notes:
	//		This is destructive - sounds are deleted, not just paused
	//		Ambiances with AAL_FLAG_AUTOFREE are automatically freed
	//		Instances are always deleted when stopped
	//
	//=============================================================================
	aalError Mixer::Stop()
	{
		aalULong i;

		// Recursively stop all child mixers
		for (i = 0; i < _mixer.Size(); i++)
		{
			Mixer * mixer = _mixer[i];
			if (mixer && mixer->parent == this)
				mixer->Stop();
		}

		// Stop all ambiances on this mixer
		for (i = 0; i < _amb.Size(); i++)
		{
			Ambiance * ambiance = _amb[i];
			if (ambiance && _mixer[ambiance->channel.mixer] == this)
			{
				ambiance->Stop();	// Stop playback

				// Auto-delete ambiances marked for autofree
				if (ambiance->channel.flags & AAL_FLAG_AUTOFREE)
					_amb.Delete(i);
			}
		}

		// Delete all sound instances on this mixer
		for (i = 0; i < _inst.Size(); i++)
		{
			Instance * instance = _inst[i];
			if (instance && _mixer[instance->channel.mixer] == this)
				_inst.Delete(i);		// Delete (stops and frees)
		}

		status &= ~IS_PAUSED;	// Clear paused flag

		return AAL_OK;
	}

	//=============================================================================
	// Pause - Pause All Sounds on This Mixer
	//=============================================================================
	// Description:
	//		Pauses all sounds using this mixer
	//		Recursively pauses all child mixers
	//		Non-destructive (can be resumed)
	//
	// Returns:
	//		AAL_OK always
	//
	// Algorithm:
	//		1. Pause all child mixers (recursive)
	//		2. Pause all ambiances on this mixer
	//		3. Pause all sound instances on this mixer
	//		4. Set paused flag
	//
	// Notes:
	//		Non-destructive - sounds can be resumed
	//		Commonly used for game pause menus
	//
	//=============================================================================
	aalError Mixer::Pause()
	{
		aalULong i;

		// Recursively pause all child mixers
		for (i = 0; i < _mixer.Size(); i++)
			if (_mixer[i] && _mixer[i]->parent == this)
				_mixer[i]->Pause();

		// Pause all ambiances on this mixer
		for (i = 0; i < _amb.Size(); i++)
			if (_amb[i] && _mixer[_amb[i]->channel.mixer] == this)
				_amb[i]->Pause();

		// Pause all sound instances on this mixer
		for (i = 0; i < _inst.Size(); i++)
			if (_inst[i] && _mixer[_inst[i]->channel.mixer] == this)
				_inst[i]->Pause();

		status |= IS_PAUSED;	// Set paused flag
		return AAL_OK;
	}

	//=============================================================================
	// Resume - Resume All Paused Sounds on This Mixer
	//=============================================================================
	// Description:
	//		Resumes all paused sounds using this mixer
	//		Recursively resumes all child mixers
	//		Only resumes if mixer is currently paused
	//
	// Returns:
	//		AAL_OK always
	//
	// Algorithm:
	//		1. Check if paused (early exit if not)
	//		2. Resume all child mixers (recursive)
	//		3. Resume all ambiances on this mixer
	//		4. Resume all sound instances on this mixer
	//		5. Clear paused flag
	//
	// Notes:
	//		Inverse of Pause()
	//		Only affects sounds that were paused
	//
	//=============================================================================
	aalError Mixer::Resume()
	{
		if (!(status & IS_PAUSED)) return AAL_OK;	// Not paused - nothing to do

		aalULong i;

		// Recursively resume all child mixers
		for (i = 0; i < _mixer.Size(); i++)
			if (_mixer[i] && _mixer[i]->parent == this)
				_mixer[i]->Resume();

		// Resume all ambiances on this mixer
		for (i = 0; i < _amb.Size(); i++)
			if (_amb[i] && _mixer[_amb[i]->channel.mixer] == this)
				_amb[i]->Resume();

		// Resume all sound instances on this mixer
		for (i = 0; i < _inst.Size(); i++)
			if (_inst[i] && _mixer[_inst[i]->channel.mixer] == this)
				_inst[i]->Resume();

		status &= ~IS_PAUSED;	// Clear paused flag
		return AAL_OK;
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================
