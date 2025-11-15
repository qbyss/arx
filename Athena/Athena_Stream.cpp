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
#include "Athena_Stream.h"
#include "Athena_Stream_WAV.h"
//#include "Athena_Stream_ASF.h"		// ASF (Advanced Streaming Format) support commented out
#include "Athena_FileIO.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_Stream.cpp - Audio Stream Factory Functions
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Factory functions for creating and destroying audio stream objects
//		Provides abstraction layer for different streaming audio formats
//
// Purpose:
//		- Create audio stream objects from files
//		- Auto-detect format and instantiate appropriate stream class
//		- Manage stream lifecycle (creation/deletion)
//
// Supported Formats:
//		- WAV (StreamWAV) - Primary format, always enabled
//		- ASF (StreamASF) - Commented out, not used in final game
//
// Stream Classes:
//		Stream - Abstract base class
//		StreamWAV - WAV file streaming implementation
//		StreamASF - ASF file streaming (disabled)
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

namespace ATHENA
{

	//=============================================================================
	// CreateStream - Create Audio Stream Object from File
	//=============================================================================
	// Description:
	//		Opens audio file and creates appropriate stream object
	//		Currently only WAV format supported (StreamWAV)
	//
	// Parameters:
	//		name: Audio filename
	//
	// Returns:
	//		Stream object pointer on success
	//		NULL if file not found or stream creation failed
	//
	// Algorithm:
	//		1. Open file using multi-path search (OpenResource)
	//		2. Create StreamWAV object
	//		3. Initialize stream with file handle (SetStream)
	//		4. Return stream if successful, cleanup on failure
	//
	// Notes:
	//		Currently hardcoded to WAV format
	//		Could be extended to auto-detect format from file header
	//
	//=============================================================================
	Stream * CreateStream(const char * name)
	{
		FILE * file;
		Stream * stream = NULL;

		file = OpenResource(name, sample_path);		// Search multiple paths

		if (!file) return NULL;						// File not found

		FileSeek(file, 0, SEEK_SET);				// Rewind to start
		stream = new StreamWAV;						// Create WAV stream object

		if (stream->SetStream(file)) delete stream;	// Init failed? Delete stream
		else return stream;							// Success - return stream

		FileClose(file);							// Close file (only if failed)

		return NULL;
	}

	//=============================================================================
	// DeleteStream - Destroy Audio Stream Object
	//=============================================================================
	// Description:
	//		Properly releases stream resources and closes file
	//
	// Parameters:
	//		stream: Reference to stream pointer (set to NULL on return)
	//
	// Returns:
	//		AAL_OK on success
	//
	// Algorithm:
	//		1. Get file handle from stream
	//		2. Close file
	//		3. Delete stream object
	//		4. NULL out pointer (prevents dangling reference)
	//
	//=============================================================================
	aalError DeleteStream(Stream *&stream)
	{
		FILE * file;

		stream->GetStream(file);		// Get underlying file handle
		FileClose(file);				// Close file
		delete stream;					// Delete stream object
		stream = NULL;					// NULL out caller's pointer

		return AAL_OK;
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================
