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
#include "Athena_FileIO.h"
#include "Athena_Global.h"
#include <Hermes_Pak.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_FileIO.cpp - File I/O Abstraction Layer (PAK vs Real Files)
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Abstraction layer for file I/O operations
//		Allows transparent switching between:
//		- Real file system (fopen, fread, etc.)
//		- PAK archive system (PAK_fopen, PAK_fread, etc.)
//
// Purpose:
//		- Single file I/O API regardless of source (disk or PAK)
//		- Runtime switching between packed and unpacked resources
//		- Enables shipping game with compressed PAK files
//		- Enables development with loose files
//
// Implementation:
//		Function pointers initialized to point to either:
//		- Standard C library functions (fopen, fread, etc.)
//		- HERMES PAK library functions (PAK_fopen, PAK_fread, etc.)
//
// Design Pattern:
//		Dependency Injection via function pointers
//		Strategy Pattern (selectable file I/O implementation)
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

extern long CURRENT_LOADMODE;		// Global load mode flag (defined in HERMES)

namespace ATHENA
{

	//=============================================================================
	// File I/O Function Pointers
	//=============================================================================
	// These function pointers are initialized to point to either standard C
	// file functions or PAK archive functions, based on AAL_FLAG_PACKEDRESOURCES
	//
	// Default to standard C library functions (initialized at declaration)
	//=============================================================================

	FILE *(* FileOpen)(const char * name, const char * mode) = fopen;
	int (* FileClose)(FILE * file) = fclose;
	size_t (* FileRead)(void * buffer, size_t size, size_t count, FILE * file) = fread;
	size_t (* FileWrite)(const void * buffer, size_t size, size_t count, FILE * file) = fwrite;
	int (* FileSeek)(FILE * file, long offset, int origin) = fseek;
	long(* FileTell)(FILE * file) = ftell;

	//=============================================================================
	// FileIOInit - Initialize File I/O Function Pointers
	//=============================================================================
	// Description:
	//		Sets up function pointers based on packed resources flag
	//		Called once during Athena initialization
	//
	// Behavior:
	//		If AAL_FLAG_PACKEDRESOURCES set:
	//			FileOpen = PAK_fopen (reads from PAK archives)
	//			FileRead = PAK_fread
	//			etc.
	//		Else:
	//			FileOpen = fopen (reads from disk)
	//			FileRead = fread
	//			etc.
	//
	// Notes:
	//		FileWrite always uses ::fwrite (PAKs are read-only)
	//		All subsequent FileOpen/FileRead calls use selected implementation
	//
	//=============================================================================
	aalVoid FileIOInit()
	{
		if (global_status & AAL_FLAG_PACKEDRESOURCES)		// Use PAK archives?
		{
			CURRENT_LOADMODE = LOAD_PACK;

			FileOpen = PAK_fopen;		// Point to PAK functions
			FileClose = PAK_fclose;
			FileRead = PAK_fread;
			FileWrite = ::fwrite;		// Write always uses real files
			FileSeek = PAK_fseek;
			FileTell = PAK_ftell;
		}
		else												// Use real files
		{
			CURRENT_LOADMODE = LOAD_TRUEFILE;

			FileOpen =	::fopen;		// Point to standard C functions
			FileClose = ::fclose;
			FileRead =	::fread;
			FileWrite = ::fwrite;
			FileSeek =	::fseek;
			FileTell =	::ftell;
		}
	}

	//=============================================================================
	// AddPack - Register PAK Archive for Loading
	//=============================================================================
	// Description:
	//		Adds PAK archive to HERMES PAK manager's search list
	//		Subsequent file opens will search this PAK
	//
	// Parameters:
	//		name: PAK archive filename
	//
	// Notes:
	//		Only effective if AAL_FLAG_PACKEDRESOURCES enabled
	//		PAK search order: last added PAK searched first
	//
	//=============================================================================
	aalVoid AddPack(const char * name)
	{
		PAK_SetLoadMode(global_status & AAL_FLAG_PACKEDRESOURCES ? LOAD_PACK : LOAD_TRUEFILE,
		                (char *)name);
	}

}//ATHENA::

//=============================================================================
// END OF FILE
//=============================================================================