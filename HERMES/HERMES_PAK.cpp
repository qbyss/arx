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
//////////////////////////////////////////////////////////////////////////////////////
//   @@        @@@        @@@                @@                           @@@@@     //
//   @@@       @@@@@@     @@@     @@        @@@@                         @@@  @@@   //
//   @@@       @@@@@@@    @@@    @@@@       @@@@      @@                @@@@        //
//   @@@       @@  @@@@   @@@  @@@@@       @@@@@@     @@@               @@@         //
//  @@@@@      @@  @@@@   @@@ @@@@@        @@@@@@@    @@@            @  @@@         //
//  @@@@@      @@  @@@@  @@@@@@@@         @@@@ @@@    @@@@@         @@ @@@@@@@      //
//  @@ @@@     @@  @@@@  @@@@@@@          @@@  @@@    @@@@@@        @@ @@@@         //
// @@@ @@@    @@@ @@@@   @@@@@            @@@@@@@@@   @@@@@@@      @@@ @@@@         //
// @@@ @@@@   @@@@@@@    @@@@@@           @@@  @@@@   @@@ @@@      @@@ @@@@         //
// @@@@@@@@   @@@@@      @@@@@@@@@@      @@@    @@@   @@@  @@@    @@@  @@@@@        //
// @@@  @@@@  @@@@       @@@  @@@@@@@    @@@    @@@   @@@@  @@@  @@@@  @@@@@        //
//@@@   @@@@  @@@@@      @@@      @@@@@@ @@     @@@   @@@@   @@@@@@@    @@@@@ @@@@@ //
//@@@   @@@@@ @@@@@     @@@@        @@@  @@      @@   @@@@   @@@@@@@    @@@@@@@@@   //
//@@@    @@@@ @@@@@@@   @@@@             @@      @@   @@@@    @@@@@      @@@@@      //
//@@@    @@@@ @@@@@@@   @@@@             @@      @@   @@@@    @@@@@       @@        //
//@@@    @@@  @@@ @@@@@                          @@            @@@                  //
//            @@@ @@@                           @@             @@        STUDIOS    //
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// HERMES_PAK.cpp - Virtual File System API (Disk Files + PAK Archives)
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Provides unified file I/O API that transparently accesses resources from
//		either disk files OR PAK archives, with configurable fallback behavior.
//
// Purpose:
//		Allows game to load resources from compressed PAK archives for release builds
//		while supporting loose files on disk for development/modding.
//		Provides seamless switching between file sources without code changes.
//
// Architecture:
//
//		PakManager - Multi-Archive Manager
//			Manages collection of loaded PAK archives (vector<EVE_LOADPACK*>)
//			Searches archives in order when looking up files
//			Provides unified API for reading from any loaded PAK
//
//		Load Modes (CURRENT_LOADMODE):
//			LOAD_TRUEFILE - Only load from disk files (development mode)
//			LOAD_PACK - Only load from PAK archives (release mode)
//			LOAD_PACK_THEN_TRUEFILE - Try PAK first, fall back to disk
//			LOAD_TRUEFILE_THEN_PACK - Try disk first, fall back to PAK
//
//		File API Functions (PAK_* wrappers):
//			All standard C file I/O functions have PAK_* equivalents:
//			- PAK_fopen() / PAK_fclose() - Open/close files
//			- PAK_fread() / PAK_fseek() / PAK_ftell() - Read/seek operations
//			- PAK_FileExist() / PAK_DirectoryExist() - Existence checks
//			- PAK_FileLoadMalloc() - Load entire file into memory
//
//		Internal Functions (_PAK_* helpers):
//			Each PAK_* function has a _PAK_* helper that accesses PAK archives
//			PAK_* functions switch between disk and _PAK_* based on CURRENT_LOADMODE
//
// Path Handling:
//		PAK_WORKDIR prefix stripped from paths before PAK lookup
//		Example: "c:\game\data\textures\wall.jpg" -> "textures\wall.jpg" in PAK
//		Allows game to use absolute paths while PAK uses relative paths
//
// File Loading Modes:
//		PAK_FileLoadMalloc() - Loads file, returns malloc'd buffer
//		PAK_FileLoadMallocZero() - Same but adds 2 null bytes at end (for strings)
//
// Debugging:
//		PAK_NotFoundInit() - Enable logging of missing files
//		PAK_NotFound() - Log file access failures for debugging
//		DrawDebugFile() - Debug visualization hook (disabled)
//
// Usage Pattern:
//		1. PAK_SetLoadMode() - Configure load behavior and open PAK archives
//		2. PAK_fopen() / PAK_FileLoadMalloc() - Load resources transparently
//		3. PAK_Close() - Cleanup when shutting down
//
// Example:
//		PAK_SetLoadMode(LOAD_TRUEFILE_THEN_PACK, "game.pak", "c:\\game\\data\\");
//		FILE* f = PAK_fopen("textures\\wall.jpg", "rb");  // Tries disk, then PAK
//		PAK_fread(buffer, 1, size, f);
//		PAK_fclose(f);
//		PAK_Close();
//
// Performance:
//		PAK archives use hash tables for O(1) file lookups
//		Multiple PAKs searched sequentially (first match wins)
//		Working directory prefix stripped once (cached length)
//
// Force-In-Pack Mode:
//		bForceInPack flag forces PAK-only access (ignores disk files)
//		Used for DRM/anti-cheat to prevent file replacement
//
// Code: Sébastien Scieux
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include "hermes_pak.h"
#include "hermesmain.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//#############################################################################
//                             GLOBAL VARIABLES
//#############################################################################

bool bForceInPack = false;				// Force PAK-only mode (disable disk file access)
long CURRENT_LOADMODE = LOAD_TRUEFILE;	// Active load mode (see enum in header)

PakManager * pPakManager = NULL;		// Global PAK manager singleton

char PAK_WORKDIR[256];					// Working directory prefix to strip from paths
ULONG g_pak_workdir_len = 0;			// Cached length of PAK_WORKDIR
char NOT_FOUND_FIC[256];				// File path for logging missing files
long WRITE_NOT_FOUND = 0;				// Flag: enable missing file logging

//#############################################################################
//                             DEBUG / LOGGING FUNCTIONS
//#############################################################################

//=============================================================================
// FUNCTION: PAK_NotFoundInit
//=============================================================================
// Description:
//		Initializes missing file logging system.
//
// Parameters:
//		fich - Path to log file where missing files will be written
//
// Notes:
//		- Creates/overwrites log file
//		- Sets WRITE_NOT_FOUND flag if file creation succeeds
//		- Used for debugging resource loading issues
//
//=============================================================================
void PAK_NotFoundInit(char * fich)
{
	strcpy(NOT_FOUND_FIC, fich);
	FILE * fic;

	if ((fic = fopen(NOT_FOUND_FIC, "w")) != NULL)
	{
		WRITE_NOT_FOUND = 1;
		fclose(fic);
	}
	else WRITE_NOT_FOUND = 0;
}

// PAK_NotFound - Log missing file to debug log
//		Returns true if logged successfully
bool PAK_NotFound(char * fich)
{
	FILE * fic;

	if (WRITE_NOT_FOUND)
	{
		if ((fic = fopen(NOT_FOUND_FIC, "a+")) != NULL)
		{
			fprintf(fic, "Not Found %s\n", fich);
			fclose(fic);
			return true;
		}
	}

	return false;
}

//#############################################################################
//                             INITIALIZATION FUNCTIONS
//#############################################################################

//=============================================================================
// FUNCTION: PAK_SetLoadMode
//=============================================================================
// Description:
//		Configures resource loading mode and initializes PAK archives.
//
// Parameters:
//		mode - Load mode (LOAD_TRUEFILE, LOAD_PACK, LOAD_*_THEN_*, etc.)
//		pakfile - Path to PAK archive to load
//		workdir - Working directory prefix to strip from paths
//
// Notes:
//		- Always sets mode to LOAD_TRUEFILE_THEN_PACK (hardcoded override)
//		- Creates PakManager if it doesn't exist
//		- Removes and re-adds PAK to ensure clean state
//		- Caches working directory length for performance
//
//=============================================================================
void PAK_SetLoadMode(long mode, char * pakfile, char * workdir)
{

	mode = LOAD_TRUEFILE_THEN_PACK;
	// nust be remed for editor mode

	CURRENT_LOADMODE = mode;

	if (FileExist(pakfile))
	{
		if (!pPakManager) pPakManager = new PakManager();

		pPakManager->RemovePak(pakfile);
		pPakManager->AddPak(pakfile);
	}

	if (workdir)
	{
		strcpy(PAK_WORKDIR, workdir);
		g_pak_workdir_len = strlen(PAK_WORKDIR);
	}
	else
	{
		strcpy(PAK_WORKDIR, "");
		g_pak_workdir_len = 0;
	}

}

// PAK_Close - Shutdown PAK system and free resources
//		Deletes PakManager and all loaded PAK archives
void PAK_Close()
{
	if (pPakManager) delete pPakManager;

	pPakManager = NULL;
}

//#############################################################################
//                    INTERNAL HELPER FUNCTIONS (_PAK_*)
//		These functions access PAK archives directly (no disk fallback)
//		Called by public PAK_* functions based on CURRENT_LOADMODE
//#############################################################################

// _PAK_FileLoadMallocZero - Load file from PAK with null termination
//		Allocates buffer, reads file, adds 2 null bytes at end
//		Returns buffer pointer or NULL if not found
//		Strips PAK_WORKDIR prefix from path
void * _PAK_FileLoadMallocZero(char * name, long * SizeLoadMalloc)
{
#ifdef TEST_PACK_EDITOR
	strcpy(PAK_WORKDIR, "\\\\arkaneserver\\public\\arx\\");
	g_pak_workdir_len = strlen(PAK_WORKDIR);
#endif

	if (g_pak_workdir_len >= strlen(name))
	{
		if (SizeLoadMalloc) *SizeLoadMalloc = 0;

		return NULL;
	}

	int iTaille;
	iTaille = pPakManager->GetSize(name + g_pak_workdir_len);

	if (iTaille > 0)
	{
		char * mem = (char *)malloc(iTaille + 2);

		pPakManager->Read(name + g_pak_workdir_len, mem);

		mem[iTaille]   = 0;
		mem[iTaille+1] = 0;

		if (SizeLoadMalloc) *SizeLoadMalloc = iTaille + 2;

		return mem;
	}
	else
	{
		if (SizeLoadMalloc) *SizeLoadMalloc = iTaille;

		return NULL;
	}
}

// _PAK_FileLoadMalloc - Load file from PAK (no null termination)
//		Logs to PAK_NotFound if file not in any loaded PAK
void * _PAK_FileLoadMalloc(char * name, long * SizeLoadMalloc)
{
#ifdef TEST_PACK_EDITOR
	strcpy(PAK_WORKDIR, "\\\\arkaneserver\\public\\arx\\");
	g_pak_workdir_len = strlen(PAK_WORKDIR);
#endif

	if (g_pak_workdir_len >= strlen(name))
	{
		if (SizeLoadMalloc) *SizeLoadMalloc = 0;

		return NULL;
	}

	int iTaille = 0;
	void * mem = pPakManager->ReadAlloc(name + g_pak_workdir_len, &iTaille);

	if ((SizeLoadMalloc) && mem) *SizeLoadMalloc = iTaille;

	if (mem == NULL) PAK_NotFound(name);

	return mem;
}

// _PAK_DirectoryExist - Check if directory exists in any loaded PAK
//		Searches all PAK archives for matching directory
//		Returns true if found in at least one PAK
long _PAK_DirectoryExist(char * name)
{
#ifdef TEST_PACK_EDITOR
	strcpy(PAK_WORKDIR, "\\\\arkaneserver\\public\\arx\\");
	g_pak_workdir_len = strlen(PAK_WORKDIR);
#endif

	long leng = strlen(name);

	if (ARX_CAST_LONG(g_pak_workdir_len) >= leng)
	{
		return false;
	}

	char temp[256];
	strcpy(temp, name + g_pak_workdir_len);
	long l = leng - g_pak_workdir_len ;
	if (temp[l] != '\\') strcat(temp, "\\");

	vector<EVE_REPERTOIRE *>* pvRepertoire;
	pvRepertoire = pPakManager->ExistDirectory(temp);

	if (!pvRepertoire->size())
	{
		pvRepertoire->clear();
		delete pvRepertoire;
		return false;
	}

	pvRepertoire->clear();
	delete pvRepertoire;
	return true;
}

//#############################################################################
//                    PUBLIC API FUNCTIONS (PAK_*)
//		All functions follow same pattern:
//		- Switch on CURRENT_LOADMODE to determine source priority
//		- LOAD_TRUEFILE: Disk only
//		- LOAD_PACK: PAK only
//		- LOAD_PACK_THEN_TRUEFILE: Try PAK, fallback to disk
//		- LOAD_TRUEFILE_THEN_PACK: Try disk, fallback to PAK
//		- bForceInPack flag forces PAK-only access in TRUEFILE_THEN_PACK mode
//#############################################################################

// PAK_DirectoryExist - Check if directory exists (disk or PAK based on mode)
long PAK_DirectoryExist(char * name)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif

	long ret = 0;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret = DirectoryExist(name);
			break;
		case LOAD_PACK:
			ret = _PAK_DirectoryExist(name);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret = _PAK_DirectoryExist(name);

			if (!ret)
				ret = DirectoryExist(name);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (bForceInPack)
			{
				ret = _PAK_DirectoryExist(name);
			}
			else
			{
				ret = DirectoryExist(name);

				if (!ret)
					ret = _PAK_DirectoryExist(name);
			}

			break;
	}

	return ret;
}

// _PAK_FileExist - Check if file exists in any loaded PAK archive
long _PAK_FileExist(char * name)
{
#ifdef TEST_PACK_EDITOR
	strcpy(PAK_WORKDIR, "\\\\arkaneserver\\public\\arx\\");
	g_pak_workdir_len = strlen(PAK_WORKDIR);
#endif

	if (g_pak_workdir_len >= strlen(name))
	{
		return false;
	}

	char path[256];
	strcpy(path, name + g_pak_workdir_len);

	if (pPakManager->ExistFile(path)) return 1;

	PAK_NotFound(name);
	return 0;
}

// PAK_FileExist - Check if file exists (disk or PAK based on mode)
long PAK_FileExist(char * name)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif
	long ret = 0;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret	=	FileExist(name);
			break;
		case LOAD_PACK:
			ret	=	_PAK_FileExist(name);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret	=	_PAK_FileExist(name);

			if (!ret)
				ret	=	FileExist(name);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (bForceInPack)
			{
				ret	=	_PAK_FileExist(name);
			}
			else
			{
				ret	=	FileExist(name);

				if (!ret)
					ret	= _PAK_FileExist(name);
			}

			break;
	}

	return ret;
}

// PAK_FileLoadMalloc - Load entire file into malloc'd buffer (disk or PAK)
void * PAK_FileLoadMalloc(char * name, long * SizeLoadMalloc)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif

	void * ret = NULL;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret	=	FileLoadMalloc(name, SizeLoadMalloc);
			break;
		case LOAD_PACK:
			ret	=	_PAK_FileLoadMalloc(name, SizeLoadMalloc);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret	=	_PAK_FileLoadMalloc(name, SizeLoadMalloc);

			if (ret == NULL)
				if (PAK_FileExist(name))
					ret = FileLoadMalloc(name, SizeLoadMalloc);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (bForceInPack)
			{
				ret	=	_PAK_FileLoadMalloc(name, SizeLoadMalloc);
			}
			else
			{
				ret	=	FileLoadMalloc(name, SizeLoadMalloc);

				if (ret == NULL)
					ret	= _PAK_FileLoadMalloc(name, SizeLoadMalloc);
			}

			break;
	}

	return ret;
}

// PAK_FileLoadMallocZero - Load file with null termination (+2 bytes for strings)
void * PAK_FileLoadMallocZero(char * name, long * SizeLoadMalloc)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif

	void * ret = NULL;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret	=	FileLoadMallocZero(name, SizeLoadMalloc);
			break;
		case LOAD_PACK:
			ret	=	_PAK_FileLoadMallocZero(name, SizeLoadMalloc);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret	=	_PAK_FileLoadMallocZero(name, SizeLoadMalloc);

			if (ret == NULL)
				if (PAK_FileExist(name))
					ret = FileLoadMallocZero(name, SizeLoadMalloc);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (bForceInPack)
			{
				ret	=	_PAK_FileLoadMallocZero(name, SizeLoadMalloc);
			}
			else
			{
				ret	=	FileLoadMallocZero(name, SizeLoadMalloc);

				if (ret == NULL)
					ret	=	_PAK_FileLoadMallocZero(name, SizeLoadMalloc);
			}

			break;
	}

	return ret;
}

//#############################################################################
//                    FILE I/O WRAPPERS (fopen, fclose, fread, etc.)
//		These functions provide drop-in replacements for standard C file I/O
//		Can work with both real FILE* and virtual PACK_FILE* handles
//#############################################################################

long PAK_ftell(FILE * stream);

// _PAK_ftell - Get current position in PAK file
long _PAK_ftell(FILE * stream)
{
	return pPakManager->fTell((PACK_FILE *)stream);
}

// PAK_ftell - Get file position (disk or PAK based on mode)
long PAK_ftell(FILE * stream)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif

	long ret = 0;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret = ftell(stream);
			break;
		case LOAD_PACK:
			ret = _PAK_ftell(stream);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret = _PAK_ftell(stream);

			if (ret < 0)
				ret = ftell(stream);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (ferror(stream) && (!bForceInPack))
			{
				ret = ftell(stream);
			}
			else
			{
				ret = _PAK_ftell(stream);
			}

			break;
	}

	return ret;
}

// _PAK_fopen - Open file from PAK archive
//		Returns PACK_FILE* cast to FILE* for transparent usage
FILE * _PAK_fopen(const char * filename, const char * mode)
{
#ifdef TEST_PACK_EDITOR
	strcpy(PAK_WORKDIR, "\\\\arkaneserver\\public\\arx\\");
	g_pak_workdir_len = strlen(PAK_WORKDIR);
#endif

	if (g_pak_workdir_len >= strlen(filename))
	{
		return NULL;
	}

	return (FILE *)pPakManager->fOpen((char *)(filename + g_pak_workdir_len));
}

// PAK_fopen - Open file (disk or PAK based on mode)
FILE * PAK_fopen(const char * filename, const char * mode)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif

	FILE * ret = NULL;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret = fopen(filename, mode);
			break;
		case LOAD_PACK:
			ret = _PAK_fopen(filename, mode);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret = _PAK_fopen(filename, mode);

			if (ret == NULL)
				ret = fopen(filename, mode);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (bForceInPack)
			{
				ret = _PAK_fopen(filename, mode);
			}
			else
			{
				ret = fopen(filename, mode);

				if (ret == NULL)
					ret = _PAK_fopen(filename, mode);
			}

			break;
	}

	return ret;
}

// _PAK_fclose - Close file in PAK archive
int _PAK_fclose(FILE * stream)
{
	return pPakManager->fClose((PACK_FILE *)stream);
}

// PAK_fclose - Close file (disk or PAK based on mode)
int PAK_fclose(FILE * stream)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif

	int ret = 0;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret = fclose(stream);
			break;
		case LOAD_PACK:
			ret = _PAK_fclose(stream);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret = _PAK_fclose(stream);

			if (ret == EOF)
				ret = fclose(stream);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (ferror(stream) && (!bForceInPack))
			{
				ret = fclose(stream);
			}
			else
			{
				ret = _PAK_fclose(stream);
			}

			break;
	}

	return ret;
}

// _PAK_fread - Read data from PAK file
size_t _PAK_fread(void * buffer, size_t size, size_t count, FILE * stream)
{
	return pPakManager->fRead(buffer, size, count, (PACK_FILE *)stream);
}

// PAK_fread - Read data from file (disk or PAK based on mode)
size_t PAK_fread(void * buffer, size_t size, size_t count, FILE * stream)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif

	size_t ret = 0;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret = fread(buffer, size, count, stream);
			break;
		case LOAD_PACK:
			ret = _PAK_fread(buffer, size, count, stream);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret = _PAK_fread(buffer, size, count, stream);

			if (ret == NULL)
				ret = fread(buffer, size, count, stream);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (ferror(stream) && (!bForceInPack))
			{
				ret = fread(buffer, size, count, stream);
			}
			else
			{
				ret = _PAK_fread(buffer, size, count, stream);
			}

			break;
	}

	return ret;
}

// _PAK_fseek - Seek to position in PAK file
int _PAK_fseek(FILE * fic, long offset, int origin)
{
	return pPakManager->fSeek((PACK_FILE *)fic, offset, origin);
}

// PAK_fseek - Seek in file (disk or PAK based on mode)
int PAK_fseek(FILE * fic, long offset, int origin)
{
#ifdef TEST_PACK_EDITOR
	CURRENT_LOADMODE = LOAD_PACK_THEN_TRUEFILE;
#endif

	int ret = 1;

	switch (CURRENT_LOADMODE)
	{
		case LOAD_TRUEFILE:
			ret = fseek(fic, offset, origin);
			break;
		case LOAD_PACK:
			ret = _PAK_fseek(fic, offset, origin);
			break;
		case LOAD_PACK_THEN_TRUEFILE:
			ret = _PAK_fseek(fic, offset, origin);

			if (ret == 1)
				ret = fseek(fic, offset, origin);

			break;
		case LOAD_TRUEFILE_THEN_PACK:

			if (ferror(fic) && (!bForceInPack))
			{
				ret = fseek(fic, offset, origin);
			}
			else
			{
				ret = _PAK_fseek(fic, offset, origin);
			}

			break;
	}

	return ret;
}

//#############################################################################
//#############################################################################
//                             PakManager Class
//#############################################################################
//#############################################################################

// PakManager::PakManager - Constructor
//		Initializes empty PAK collection
PakManager::PakManager()
{
	vLoadPak.clear();
}

// PakManager::~PakManager - Destructor
//		Closes and deletes all loaded PAK archives
PakManager::~PakManager()
{
	vector<EVE_LOADPACK *>::iterator i;

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		delete(*i);
	}

	vLoadPak.clear();
}

//=============================================================================
// FUNCTION: PakManager::AddPak
//=============================================================================
// Description:
//		Loads a PAK archive and adds it to the collection.
//
// Parameters:
//		_lpszName - Path to PAK file to load
//
// Returns:
//		true - PAK loaded successfully
//		false - PAK failed to load (file not found or corrupted)
//
// Notes:
//		- Creates new EVE_LOADPACK instance
//		- Opens PAK and parses directory tree
//		- Adds to vector for subsequent searches
//		- Later PAKs in vector are searched before earlier ones
//
//=============================================================================
bool PakManager::AddPak(char * _lpszName)
{
	EVE_LOADPACK * pLoadPak = new EVE_LOADPACK();
	pLoadPak->Open(_lpszName);

	if (!pLoadPak->pRoot)
	{
		delete pLoadPak;
		return false;
	}

	vLoadPak.push_back(pLoadPak);
	return true;
}

// PakManager::RemovePak - Remove PAK from collection by name
//		Searches for PAK with matching filename, deletes and removes from vector
bool PakManager::RemovePak(char * _lpszName)
{
	vector<EVE_LOADPACK *>::iterator i;

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		EVE_LOADPACK * pLoadPak = *i;

		if (pLoadPak)
		{
			if (!stricmp((const char *)_lpszName, pLoadPak->lpszName))
			{
				delete(*i);
				vLoadPak.erase(i);
				return true;
			}
		}
	}

	return false;
}

// DrawDebugFile - Debug hook for visualizing file accesses (disabled)
static void DrawDebugFile(char * _lpszName)
{
	return;

}

// PakManager::Read - Read file from any loaded PAK into pre-allocated buffer
//		Searches all PAKs in order, returns true if found and read successfully
bool PakManager::Read(char * _lpszName, void * _pMem)
{
	vector<EVE_LOADPACK *>::iterator i;

	if ((_lpszName[0] == '\\') ||
	        (_lpszName[0] == '//'))
	{
		_lpszName++;
	}

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		if ((*i)->Read(_lpszName, _pMem))
		{
			return true;
		}
	}

	DrawDebugFile(_lpszName);

	return false;
}

// PakManager::ReadAlloc - Read file and allocate buffer automatically
//		Searches all PAKs, allocates buffer, reads file, returns buffer pointer
void * PakManager::ReadAlloc(char * _lpszName, int * _piTaille)
{
	vector<EVE_LOADPACK *>::iterator i;

	if ((_lpszName[0] == '\\') ||
	        (_lpszName[0] == '//'))
	{
		_lpszName++;
	}

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		void * pMem;

		if ((pMem = (*i)->ReadAlloc(_lpszName, _piTaille)))
		{
			return pMem;
		}
	}

	DrawDebugFile(_lpszName);
	return NULL;
}

// PakManager::GetSize - Get file size from any loaded PAK
//		Returns size in bytes if found, -1 if not found
int PakManager::GetSize(char * _lpszName)
{
	vector<EVE_LOADPACK *>::iterator i;

	if ((_lpszName[0] == '\\') ||
	        (_lpszName[0] == '//'))
	{
		_lpszName++;
	}

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		int iTaille;

		if ((iTaille = (*i)->GetSize(_lpszName)) > 0)
		{
			return iTaille;
		}
	}

	DrawDebugFile(_lpszName);
	return -1;
}

// PakManager::fOpen - Open file from any loaded PAK
//		Returns PACK_FILE handle for subsequent fRead/fSeek/fClose operations
PACK_FILE * PakManager::fOpen(char * _lpszName)
{
	vector<EVE_LOADPACK *>::iterator i;

	if ((_lpszName[0] == '\\') ||
	        (_lpszName[0] == '//'))
	{
		_lpszName++;
	}

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		PACK_FILE * pPakFile;

		if ((pPakFile = (*i)->fOpen((const char *)_lpszName, "rb")))
		{
			return pPakFile;
		}
	}

	DrawDebugFile(_lpszName);
	return NULL;
}

// PakManager::fClose - Close PACK_FILE handle
//		Searches all PAKs for matching handle, closes if found
int PakManager::fClose(PACK_FILE * _pPakFile)
{
	vector<EVE_LOADPACK *>::iterator i;


	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		if ((*i)->fClose(_pPakFile) != EOF)
		{
			return 0;
		}
	}

	return EOF;
}

// PakManager::fRead - Read data from PACK_FILE
//		Delegates to appropriate EVE_LOADPACK based on file handle
int PakManager::fRead(void * _pMem, int _iSize, int _iCount, PACK_FILE * _pPackFile)
{
	vector<EVE_LOADPACK *>::iterator i;

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		int iTaille;

		if ((iTaille = (*i)->fRead(_pMem, _iSize, _iCount, _pPackFile)))
		{
			return iTaille;
		}
	}

	return 0;
}

// PakManager::fSeek - Seek to position in PACK_FILE
int PakManager::fSeek(PACK_FILE * _pPackFile, int _iSeek, int _iMode)
{
	vector<EVE_LOADPACK *>::iterator i;

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		if (!((*i)->fSeek(_pPackFile, _iSeek, _iMode)))
		{
			return 0;
		}
	}

	return 1;
}

// PakManager::fTell - Get current position in PACK_FILE
int PakManager::fTell(PACK_FILE * _pPackFile)
{
	vector<EVE_LOADPACK *>::iterator i;

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		int iOffset;

		if ((iOffset = (*i)->fTell(_pPackFile)) >= 0)
		{
			return iOffset;
		}
	}

	return -1;
}

//=============================================================================
// FUNCTION: PakManager::ExistDirectory
//=============================================================================
// Description:
//		Checks if directory exists in any loaded PAK.
//
// Returns:
//		Vector of matching EVE_REPERTOIRE pointers from all PAKs
//		Empty vector if directory not found
//
// Notes:
//		- Caller must delete returned vector
//		- May return multiple results if same path in multiple PAKs
//
//=============================================================================
vector<EVE_REPERTOIRE *>* PakManager::ExistDirectory(char * _lpszName)
{
	vector<EVE_LOADPACK *>::iterator i;

	if ((_lpszName[0] == '\\') ||
	        (_lpszName[0] == '//'))
	{
		_lpszName++;
	}

	vector<EVE_REPERTOIRE *> *pvRepertoire = new vector<EVE_REPERTOIRE *>;
	pvRepertoire->clear();

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		EVE_REPERTOIRE * pRep;

		if ((pRep = (*i)->pRoot->GetSousRepertoire((unsigned char *)_lpszName)))
		{
			pvRepertoire->insert(pvRepertoire->end(), pRep);
		}
	}
	return pvRepertoire;
}

//=============================================================================
// FUNCTION: PakManager::ExistFile
//=============================================================================
// Description:
//		Checks if file exists in any loaded PAK.
//
// Returns:
//		true - File found in at least one PAK
//		false - File not found in any PAK
//
// Algorithm:
//		1. Extract directory path and filename from full path
//		2. For each loaded PAK:
//		   a. Find directory in PAK's tree
//		   b. Search directory's hash table for filename
//		   c. Return true if found
//		3. Return false if not found in any PAK
//
// Notes:
//		- Uses hash table for O(1) filename lookup within directory
//		- Searches PAKs in order (first match wins)
//
//=============================================================================
bool PakManager::ExistFile(char * _lpszName)
{
	vector<EVE_LOADPACK *>::iterator i;

	if ((_lpszName[0] == '\\') ||
	        (_lpszName[0] == '//'))
	{
		_lpszName++;
	}


	char * pcDir = NULL;
	char * pcDir1 = (char *)EVEF_GetDirName((unsigned char *)_lpszName);

	if (pcDir1)
	{
		pcDir = new char[strlen((const char *)pcDir1)+2];
		strcpy((char *)pcDir, (const char *)pcDir1);
		strcat((char *)pcDir, "\\");
		delete [] pcDir1;
	}

	char * pcFile = (char *)EVEF_GetFileName((unsigned char *)_lpszName);

	for (i = vLoadPak.begin(); i < vLoadPak.end(); i++)
	{
		EVE_REPERTOIRE * pRep;

		if ((pRep = (*i)->pRoot->GetSousRepertoire((unsigned char *)pcDir)))
		{
			if (pRep->nbfiles)
			{
				EVE_TFILE * pTFiles = (EVE_TFILE *)pRep->pHachage->GetPtrWithString((char *)pcFile);

				if (pTFiles)
				{
					delete [] pcFile;
					delete [] pcDir;
					return true;
				}
			}
		}
	}

	DrawDebugFile(_lpszName);

	delete [] pcDir;
	delete [] pcFile;
	return false;
}
