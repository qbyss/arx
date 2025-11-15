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
#include <string.h>
#include "Athena_Resource.h"
#include "Athena_FileIO.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Resource.cpp - Audio Resource File Loading
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Resource file loading with multi-path fallback search
//		Searches multiple directory locations to find audio files
//
// Purpose:
//		Provides flexible audio file loading from multiple possible locations:
//		- Current directory (direct name)
//		- Root path + name
//		- Resource path + name
//		- Root path + resource path + name
//
// Search Order:
//		1. Try direct name (e.g., "sound.wav")
//		2. Try root_path + name (e.g., "C:\game\" + "sound.wav")
//		3. Try resource_path + name (e.g., "audio\" + "sound.wav")
//		4. Try root_path + resource_path + name (e.g., "C:\game\audio\" + "sound.wav")
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	extern char * root_path;		// Global root installation path (e.g., "C:\ArxFatalis\")

	//=============================================================================
	// OpenResource - Open Audio Resource File with Multi-Path Fallback
	//=============================================================================
	// Description:
	//		Opens audio file by trying multiple directory locations
	//		Implements fallback search across 4 possible path combinations
	//
	// Parameters:
	//		name: Filename (e.g., "ambient.wav")
	//		resource_path: Optional resource subdirectory (e.g., "sfx\")
	//
	// Returns:
	//		FILE* if found in any location
	//		NULL if file not found anywhere
	//
	// Search Strategy:
	//		Tries paths in priority order until file opens successfully
	//		Allows flexible resource organization (absolute/relative paths)
	//
	// Example:
	//		OpenResource("music.wav", "audio\\") searches:
	//		1. "music.wav"
	//		2. "C:\game\music.wav" (if root_path set)
	//		3. "audio\music.wav"
	//		4. "C:\game\audio\music.wav"
	//
	//=============================================================================
	FILE * OpenResource(const char * name, const char * resource_path)
	{
		FILE * file = FileOpen(name, "rb");		// Try 1: Direct name
		char text[256];

		if (!file && root_path)					// Try 2: root_path + name
		{
			strcpy(text, root_path);
			strcat(text, name);
			file = FileOpen(text, "rb");
		}

		if (!file && resource_path)				// Try 3: resource_path + name
		{
			strcpy(text, resource_path);
			strcat(text, name);
			file = FileOpen(text, "rb");
		}

		if (!file && root_path && resource_path)	// Try 4: root_path + resource_path + name
		{
			strcpy(text, root_path);
			strcat(text, resource_path);
			strcat(text, name);
			file = FileOpen(text, "rb");
		}

		return file;		// Return file handle or NULL
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================
