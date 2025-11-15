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
// HERMESmain.cpp - HERMES Utility Functions & File I/O
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Collection of utility functions for HERMES resource management system.
//		Provides file I/O, path manipulation, directory operations, debugging,
//		memory tracking, and cross-process communication.
//
// Purpose:
//		Core utilities used throughout HERMES:
//		- File/path standardization and manipulation
//		- Directory tree operations (recursive delete, etc.)
//		- String utilities (case conversion, substring search)
//		- Date/time functions
//		- Debug console via Windows Registry (inter-process messaging)
//		- Memory allocation tracking (debug builds)
//		- File checksum/CRC validation
//		- Cross-process communication via Registry
//
// Key Functions:
//
//		Path & String Utilities:
//		- SAFEstrcpy() - Safe string copy with length limit
//		- IsIn() / NC_IsIn() - Substring search (case-sensitive/insensitive)
//		- File_Standardize() - Normalize paths (slashes, "..", uppercase)
//		- MakeUpcase() - Convert string to uppercase
//
//		Directory Operations:
//		- KillAllDirectory() - Recursive directory deletion
//		- DirectoryExist() - Check if directory exists
//		- CreateAllDirectories() - Create nested directory hierarchy
//
//		Date/Time:
//		- GetDate() - Get current system date/time
//
//		Debug Console:
//		- SendConsole() - Send debug message via Registry
//		- ConsoleSend() - Write message to Registry key
//		- ForceSendConsole() - Send regardless of debug level
//		- HERMES_GaiaCOM_Receive() - Read messages from Registry
//
//		Memory Tracking (Debug):
//		- HERMES_Register_Memory() - Track allocation
//		- HERMES_UnRegister_Memory() - Untrack allocation
//		- MakeMemoryText() - Generate memory usage report
//		- MemFree() - Free with tracking
//
//		File Validation:
//		- HERMES_CreateFileCheck() - Create checksum/CRC for file
//		- HERMES_VerifyFileCheck() - Verify file against checksum
//
//		File I/O Wrappers:
//		- FileExist() - Check if file exists
//		- FileLoadMalloc() - Load entire file into malloc'd buffer
//		- FileLoadMallocZero() - Load with null termination
//		- FileSaveKill() - Save buffer to file
//
// Debug Console System:
//		Uses Windows Registry for inter-process debug messaging
//		Registry Key: HKEY_CURRENT_USER\Software\Arkane_Studios\ASMODEE
//		Messages broadcast via WM_BROADCAST with custom message ID
//		Allows separate debug console process to display messages
//
// Memory Tracking:
//		When HERMES_KEEP_MEMORY_TRACE enabled:
//		- Tracks all allocations with source identifier
//		- Generates memory usage reports grouped by source
//		- Detects memory leaks
//		- Uses dynamic array (MemoTraces) to store allocation records
//
// Path Standardization:
//		File_Standardize() performs:
//		- Convert to uppercase
//		- Normalize slash direction
//		- Remove duplicate slashes
//		- Resolve ".." parent directory references
//		- Result: Canonical uppercase Windows path
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <shlobj.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <time.h>
#include <direct.h>			// _getcwd
#include "HERMESmain.h"
#include "HERMESNet.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//#############################################################################
//                             GLOBAL VARIABLES
//#############################################################################

char HermesBufferWork[_MAX_PATH];	// Work buffer for File_Standardize (avoids per-call malloc)

UINT GaiaWM = 0;					// Custom Windows message ID for debug console broadcasts
HWND MAIN_PROGRAM_HANDLE = NULL;	// Main application window handle
long DEBUGG = 1;					// Global debug enable flag

//#############################################################################
//                             STRING & PATH UTILITIES
//#############################################################################

// SAFEstrcpy - Safe string copy with maximum length limit
//		dest: Destination buffer
//		src: Source string
//		max: Maximum characters to copy
//		Truncates source if it exceeds max length to prevent buffer overruns
void SAFEstrcpy(char * dest, char * src, unsigned long max)
{
	if (strlen(src) > max)
	{
		memcpy(dest, src, max);		// Truncate to max length
	}
	else
	{
		strcpy(dest, src);			// Safe to copy entire string
	}
}

//=============================================================================
// IsIn - Case-Sensitive Substring Search
//=============================================================================
// Description:
//		Tests if 'str' appears anywhere within 'strin'
// Parameters:
//		strin: String to search within
//		str: Substring to search for
// Returns:
//		1 if found, 0 if not found
// Notes:
//		Case-sensitive (e.g., "Test" != "test")
//=============================================================================
unsigned char IsIn(char * strin, char * str)
{
	char * tmp;
	tmp = strstr(strin, str);		// Find substring

	if (tmp == NULL) return 0;		// Not found

	return 1;						// Found
}

//=============================================================================
// NC_IsIn - Case-Insensitive Substring Search (No Case)
//=============================================================================
// Description:
//		Tests if 'str' appears anywhere within 'strin' (ignoring case)
// Parameters:
//		strin: String to search within
//		str: Substring to search for
// Returns:
//		1 if found, 0 if not found
// Algorithm:
//		1. Copy both strings to temp buffers
//		2. Convert both to uppercase
//		3. Perform case-sensitive search on uppercase versions
//=============================================================================
unsigned char NC_IsIn(char * strin, char * str)
{
	char * tmp;
	char t1[4096];		// Temp buffer for uppercase strin
	char t2[4096];		// Temp buffer for uppercase str
	strcpy(t1, strin);
	strcpy(t2, str);
	MakeUpcase(t1);		// Convert to uppercase
	MakeUpcase(t2);
	tmp = strstr(t1, t2);	// Case-insensitive search (both uppercase)

	if (tmp == NULL) return 0;

	return 1;
}

//=============================================================================
// File_Standardize - Normalize File Path to Canonical Form
//=============================================================================
// Description:
//		Converts file path to standardized canonical form:
//		- Uppercase
//		- No duplicate slashes
//		- ".." parent references resolved
//
// Parameters:
//		from: Source path (can be mixed case, mixed slashes, have "..")
//		to: Output buffer for standardized path
//
// Algorithm:
//		PASS 1: Uppercase + remove duplicate slashes
//		1. Convert each character to uppercase
//		2. Skip consecutive slash characters (\ or /)
//		3. Write to temp buffer (HermesBufferWork)
//
//		PASS 2: Resolve ".." parent directory references
//		1. Find "\.." pattern
//		2. Backtrack to previous "\"
//		3. Remove "<dir>\.." substring
//		4. Repeat until all ".." resolved
//
// Example:
//		Input:  "c:\game/data\\..\textures\\wall.jpg"
//		Pass 1: "C:\GAME\DATA\..\TEXTURES\WALL.JPG"
//		Pass 2: "C:\GAME\TEXTURES\WALL.JPG"
//
//=============================================================================
void File_Standardize(char * from, char * to)
{
	long pos = 0;
	long pos2 = 0;
	long size = strlen(from);
	char * temp = HermesBufferWork;		// Global work buffer to avoid malloc

	// PASS 1: Uppercase + remove duplicate slashes
	while (pos < size)
	{
		// Skip duplicate slashes (keep only first)
		if (((from[pos] == '\\') || (from[pos] == '/')) && (pos != 0))
		{
			while ((pos < size - 1)
			        && ((from[pos+1] == '\\') || (from[pos+1] == '/')))
				pos++;		// Skip consecutive slashes
		}

		temp[pos2++]	= ARX_CLEAN_WARN_CAST_CHAR(toupper(from[pos]));	// Uppercase
		pos++;
	}

	temp[pos2] = 0;		// Null terminate

	// PASS 2: Resolve ".." parent directory references
again:
	;
	size = strlen(temp);
	pos = 0;

	while (pos < size - 2)
	{
		// Found "\.." pattern
		if ((temp[pos] == '\\') && (temp[pos+1] == '.') && (temp[pos+2] == '.'))
		{
			long fnd = pos;		// Remember start of "\.."

			// Backtrack to previous directory separator
			while (pos > 0)
			{
				pos--;

				if (temp[pos] == '\\')
				{
					// Remove "<dir>\.." by copying rest of string over it
					strcpy(temp + pos, temp + fnd + 3);
					goto again;		// Restart scan (path changed)
				}
			}
		}

		pos++;
	}

	strcpy(to, temp);	// Copy result to output
}

//=============================================================================
// KillAllDirectory - Recursively Delete Directory and All Contents
//=============================================================================
// Description:
//		Recursively deletes directory tree (all files and subdirectories)
//
// Parameters:
//		path: Directory path to delete (must end with backslash)
//
// Algorithm:
//		1. Enumerate all entries in directory (_findfirst/_findnext)
//		2. Skip "." and ".." entries
//		3. For each subdirectory: recursively delete it, then remove dir
//		4. For each file: delete file
//		5. Finally remove the directory itself
//
// Notes:
//		DANGEROUS! Permanently deletes all files and subdirectories
//		Uses Win32 RemoveDirectory() and DeleteFile()
//
//=============================================================================
long KillAllDirectory(char * path)
{
	long idx;
	struct _finddata_t fl;
	char pathh[512];
	sprintf(pathh, "%s*.*", path);		// Search pattern: path\*.*

	if ((idx = _findfirst(pathh, &fl)) != -1)
	{
		do
		{
			if (fl.name[0] != '.')		// Skip "." and ".."
			{
				if (fl.attrib & _A_SUBDIR)		// Is directory?
				{
					sprintf(pathh, "%s%s\\", path, fl.name);
					KillAllDirectory(pathh);	// Recursive delete
					RemoveDirectory(pathh);		// Remove empty directory
				}
				else							// Is file?
				{
					sprintf(pathh, "%s%s", path, fl.name);
					DeleteFile(pathh);			// Delete file
				}
			}

		}
		while (_findnext(idx, &fl) != -1);

		_findclose(idx);
	}

	RemoveDirectory(path);		// Remove the directory itself
	return 1;
}

//#############################################################################
//                             DATE/TIME FUNCTIONS
//#############################################################################

//=============================================================================
// GetDate - Get Current System Date/Time
//=============================================================================
// Description:
//		Retrieves current system date and time
//
// Parameters:
//		hdt: Output HERMES_DATE_TIME structure
//
// Output Fields:
//		secs (0-59), mins (0-59), hours (0-23)
//		days (1-31), months (1-12), years (full year like 2002)
//
// Fallback:
//		If time retrieval fails, returns hardcoded date: May 32, 2002 00:00:00
//		(Note: May 32 is invalid - likely intentional sentinel value)
//
//=============================================================================
void GetDate(HERMES_DATE_TIME * hdt)
{
	struct tm * newtime;

	time_t long_time;

	time(&long_time);                  /* Get time as long integer. */
	newtime = localtime(&long_time);   /* Convert to local time. */

	if (newtime)
	{
		hdt->secs = newtime->tm_sec;
		hdt->mins = newtime->tm_min;
		hdt->hours = newtime->tm_hour;
		hdt->months = newtime->tm_mon + 1;		// tm_mon is 0-11, convert to 1-12
		hdt->days = newtime->tm_mday;
		hdt->years = newtime->tm_year + 1900;	// tm_year is years since 1900
	}
	else		// Fallback if time retrieval fails
	{
		hdt->secs = 0;
		hdt->mins = 0;
		hdt->hours = 0;
		hdt->months = 5;		// May
		hdt->days = 32;			// Invalid day (sentinel)
		hdt->years = 2002;
	}
}

//#############################################################################
//                             DEBUG CONSOLE SYSTEM
//#############################################################################

long DebugLvl[6];		// Debug level enable flags (levels 0-5)

// HERMES_InitDebug - Initialize debug console system
//		Enables all debug levels, disables global debug flag
void HERMES_InitDebug()
{
	DebugLvl[0] = 1;
	DebugLvl[1] = 1;
	DebugLvl[2] = 1;
	DebugLvl[3] = 1;
	DebugLvl[4] = 1;
	DebugLvl[5] = 1;
	DEBUGG = 0;			// Global debug disabled
}

// MakeUpcase - Convert string to uppercase (in-place)
void MakeUpcase(char * str)
{
	strupr(str);		// Win32 CRT function
}

HKEY    ConsoleKey = NULL;		// Registry key handle for debug console
#define CONSOLEKEY_KEY     TEXT("Software\\Arkane_Studios\\ASMODEE")

//=============================================================================
// ConsoleSend - Write Debug Message to Registry
//=============================================================================
// Description:
//		Writes debug message and metadata to Windows Registry
//		Used for inter-process communication with debug console
//
// Parameters:
//		dat: Debug message text
//		level: Debug level (0-5)
//		source: Window handle of sender
//		flag: Additional flags
//
// Registry Keys Written:
//		HKEY_CURRENT_USER\Software\Arkane_Studios\ASMODEE\ConsInfo (message)
//		HKEY_CURRENT_USER\Software\Arkane_Studios\ASMODEE\ConsHwnd (source window)
//		HKEY_CURRENT_USER\Software\Arkane_Studios\ASMODEE\ConsLevel (debug level)
//		HKEY_CURRENT_USER\Software\Arkane_Studios\ASMODEE\ConsFlag (flags)
//
//=============================================================================
void ConsoleSend(char * dat, long level, HWND source, long flag)
{
	RegCreateKeyEx(HKEY_CURRENT_USER, CONSOLEKEY_KEY, 0, NULL,
	               REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
	               &ConsoleKey, NULL);
	WriteRegKey(ConsoleKey, "ConsInfo", dat);				// Message text
	WriteRegKeyValue(ConsoleKey, "ConsHwnd", (DWORD)source);	// Source window
	WriteRegKeyValue(ConsoleKey, "ConsLevel", (DWORD)level);	// Debug level
	WriteRegKeyValue(ConsoleKey, "ConsFlag", (DWORD)flag);		// Flags
	RegCloseKey(ConsoleKey);
}

//=============================================================================
// SendConsole - Send Debug Message (With Level Filtering)
//=============================================================================
// Description:
//		Sends debug message to console if debug level is enabled
//		Writes to Registry and broadcasts Windows message to notify console
//
// Parameters:
//		dat: Debug message text
//		level: Debug level (1-5, 0 is special)
//		flag: Additional flags
//		source: Window handle of sender
//
// Filtering:
//		- Checks DebugLvl[0] - if true, return (master disable?)
//		- Checks level bounds (must be 1-5)
//		- Checks DebugLvl[level] - only send if enabled
//
// Notification:
//		Broadcasts custom Windows message (GaiaWM) to HWND_BROADCAST
//		Separate debug console process watches for this message
//
//=============================================================================
void SendConsole(char * dat, long level, long flag, HWND source)
{
	if (GaiaWM != 0)		// Debug console system initialized?
	{
		if (DebugLvl[0]) return;		// Master disable check

		if (level < 1) return;			// Invalid level
		if (level > 5) return;

		if (DebugLvl[level])			// Is this debug level enabled?
		{
			ConsoleSend(dat, level, source, flag);		// Write to Registry
			SendMessage(HWND_BROADCAST, GaiaWM, 33, 33);	// Notify console
		}
	}
}

long FINAL_COMMERCIAL_DEMO_bis = 1;		// Commercial release flag

//=============================================================================
// ForceSendConsole - Send Debug Message (Bypass Level Filtering)
//=============================================================================
// Description:
//		Sends debug message unconditionally (unless commercial build)
//		Bypasses normal debug level filtering
//
// Parameters:
//		dat, level, flag, source: Same as SendConsole()
//
// Notes:
//		Only active in non-commercial builds (FINAL_COMMERCIAL_DEMO_bis == 0)
//		Used for critical debug messages that should always appear
//
//=============================================================================
void ForceSendConsole(char * dat, long level, long flag, HWND source)
{
	if (!FINAL_COMMERCIAL_DEMO_bis)		// Non-commercial build only
	{
		if (GaiaWM != 0)
		{
			ConsoleSend(dat, level, source, flag);			// Write to Registry
			SendMessage(HWND_BROADCAST, GaiaWM, 33, 33);	// Notify console
		}
	}
}

//#############################################################################
//                             INTER-PROCESS COMMUNICATION
//#############################################################################

HKEY    ComKey = NULL;		// Registry key handle for IPC
#define COMKEY_KEY     TEXT("Software\\Arkane_Studios\\GaiaCom")

//=============================================================================
// HERMES_GaiaCOM_Receive - Read IPC Message from Registry
//=============================================================================
// Description:
//		Reads inter-process communication message from Registry
//		Used for bidirectional communication between processes
//
// Returns:
//		Pointer to static buffer containing message string
//		(Caller should copy immediately, buffer reused on next call)
//
// Registry Key:
//		HKEY_CURRENT_USER\Software\Arkane_Studios\GaiaCom\ComInfo
//
//=============================================================================
char * HERMES_GaiaCOM_Receive()
{
	static char dat[1024];		// Static buffer (not thread-safe!)
	RegCreateKeyEx(HKEY_CURRENT_USER, COMKEY_KEY, 0, NULL,
	               REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
	               &ComKey, NULL);
	ReadRegKey(ComKey, "ComInfo",
	           dat, 512, "");		// Read message (max 512 chars)
	RegCloseKey(ComKey);
	return dat;
}

//#############################################################################
//                             MEMORY TRACKING SYSTEM
//#############################################################################
//
// Purpose:
//		Debug memory allocation tracking to detect leaks
//		Tracks all allocations with source identifier and size
//		Generates memory usage reports grouped by source
//
// Data Structure:
//		Dynamic array (MemoTraces) of MEMO_TRACE records
//		Each record contains: pointer, source string, size
//
// Enable:
//		Set HERMES_KEEP_MEMORY_TRACE = 1 to enable tracking
//		Currently disabled by default (even in debug builds)
//
//#############################################################################

#ifdef _DEBUG
long HERMES_KEEP_MEMORY_TRACE = 0;		// Memory tracking disabled (even in debug)
#else
long HERMES_KEEP_MEMORY_TRACE = 0;		// Memory tracking disabled (release)
#endif

#define SIZE_STRING_MEMO_TRACE 24		// Max length of source identifier string

// MEMO_TRACE - Memory allocation record
typedef struct
{
	void 	*		ptr;									// Allocated pointer
	char			string1[SIZE_STRING_MEMO_TRACE];	// Source identifier
	unsigned long	size;									// Allocation size (bytes)
} MEMO_TRACE;

MEMO_TRACE * MemoTraces = NULL;		// Dynamic array of allocation records
long nb_MemoTraces = 0;				// Number of tracked allocations

//=============================================================================
// HERMES_UnRegister_Memory - Remove Allocation from Tracking
//=============================================================================
// Description:
//		Removes allocation record when memory is freed
//		Shrinks MemoTraces array by removing entry
//
// Parameters:
//		adr: Pointer to freed memory
//
// Algorithm:
//		1. Linear search for matching pointer
//		2. If last entry, free entire array
//		3. Otherwise, create new smaller array
//		4. Copy all entries except deleted one
//		5. Replace old array with new one
//
// Notes:
//		O(n) removal (not optimized for performance)
//		On allocation failure, disables memory tracking
//
//=============================================================================
void HERMES_UnRegister_Memory(void * adr)
{
	long pos = -1;

	// Find matching pointer
	for (long i = 0; i < nb_MemoTraces; i++)
	{
		if (MemoTraces[i].ptr == adr)
		{
			pos = i;
			break;
		}
	}

	if (pos == -1) return;		// Not found (not tracked or double-free)

	// Last entry? Free entire array
	if (nb_MemoTraces == 1)
	{
		free(MemoTraces);
		MemoTraces = NULL;
		nb_MemoTraces = 0;
		return;
	}

	// Allocate smaller array (one less entry)
	MEMO_TRACE * tempo = (MEMO_TRACE *)malloc(sizeof(MEMO_TRACE) * (nb_MemoTraces - 1));

	if (tempo == NULL)		// Allocation failed - disable tracking
	{
		HERMES_KEEP_MEMORY_TRACE = 0;
		free(MemoTraces);
		return;
	}

	// Shift entries after deleted position
	for (int i = 0; i < nb_MemoTraces - 1; i++)
	{
		if (i >= pos)
		{
			memcpy(&MemoTraces[i], &MemoTraces[i+1], sizeof(MEMO_TRACE));
		}
	}

	// Copy to new array
	memcpy(tempo, MemoTraces, sizeof(MEMO_TRACE)*(nb_MemoTraces - 1));
	free(MemoTraces);
	MemoTraces = (MEMO_TRACE *)tempo;
	nb_MemoTraces--;
}

//=============================================================================
// MakeMemoryText - Generate Memory Usage Report
//=============================================================================
// Description:
//		Creates formatted text report of memory allocations grouped by source
//		Used for memory leak detection and profiling
//
// Parameters:
//		text: Output buffer for report (must be >= 64000 bytes)
//
// Returns:
//		Total memory allocated (bytes)
//
// Algorithm:
//		1. Group allocations by source identifier string
//		2. Sum total bytes for each unique source
//		3. Format as text: "<size_bytes> <source_name>"
//		4. Return grand total of all allocations
//
// Output Format:
//		       1024 TextureManager
//		      65536 MeshLoader
//		        512 <Unknown>
//
// Notes:
//		Groups allocations using O(n²) algorithm with ignore[] flags
//		Limits output to 64000 chars to prevent buffer overrun
//
//=============================================================================
unsigned long MakeMemoryText(char * text)
{
	if (HERMES_KEEP_MEMORY_TRACE == 0)
	{
		strcpy(text, "Memory Tracing OFF.");
		return 0;
	}

	BOOL * ignore;
	unsigned long TotMemory = 0;
	unsigned long TOTTotMemory = 0;
	char header[128];
	char theader[128];

	if (nb_MemoTraces == 0)
	{
		text[0] = 0;
		return 0;
	}

	ignore = (BOOL *)malloc(sizeof(BOOL) * nb_MemoTraces);		// Track which entries processed

	for (long i = 0; i < nb_MemoTraces; i++) ignore[i] = FALSE;

	strcpy(text, "");

	// Group allocations by source string
	for (int i = 0; i < nb_MemoTraces; i++)
	{
		if (!ignore[i])		// Not yet processed?
		{
			TotMemory = MemoTraces[i].size;

			if (MemoTraces[i].string1[0] != 0)
				strcpy(header, MemoTraces[i].string1);
			else strcpy(header, "<Unknown>");

			// Find all other allocations from same source
			for (long j = i + 1; j < nb_MemoTraces; j++)
			{
				if (!ignore[j])
				{
					if (MemoTraces[j].string1[0] != 0)
						strcpy(theader, MemoTraces[j].string1);
					else strcpy(theader, "<Unknown>");

					if (!strcmp(header, theader))		// Same source?
					{
						ignore[j] = TRUE;				// Mark as processed
						TotMemory += MemoTraces[j].size;	// Add to total
					}
				}
			}

			// Format: "     12345 SourceName\r\n"
			sprintf(theader, "%12u %s\r\n", TotMemory, header);

			// Append to report (check buffer size)
			if (strlen(text) + strlen(theader) + 4 < 64000)
			{
				strcat(text, theader);
			}

			TOTTotMemory += TotMemory;
		}
	}

	free(ignore);
	return TOTTotMemory;		// Return grand total bytes
}

//=============================================================================
// MemFree - Free Memory with Tracking
//=============================================================================
// Description:
//		Frees memory and removes from tracking system (if enabled)
//
// Parameters:
//		adr: Pointer to free
//
//=============================================================================
void MemFree(void * adr)
{
	if (HERMES_KEEP_MEMORY_TRACE)
		HERMES_UnRegister_Memory(adr);

	free(adr);
}

//#############################################################################
//                             FILE VALIDATION / CHECKSUM
//#############################################################################

//=============================================================================
// HERMES_CreateFileCheck - Create File Checksum/CRC Signature
//=============================================================================
// Description:
//		Creates checksum signature for file to detect tampering/corruption
//		Stores file metadata (timestamps, size) + block-based CRC
//
// Parameters:
//		name: File path
//		scheck: Output buffer for checksum data
//		size: Size of scheck buffer (bytes)
//		id: Identifier float (for versioning/validation)
//
// Returns:
//		false: Success, checksum created
//		true: Failure (file not found, can't open)
//
// Checksum Format (stored in scheck buffer as array of longs):
//		[0]    = id (float cast to long array)
//		[1]    = size of checksum buffer
//		[2-3]  = File creation time (FILETIME)
//		[4-5]  = File last write time (FILETIME)
//		[6]    = File size (bytes)
//		[7...] = File path string (null-terminated, padded to 4-byte boundary)
//		[...]  = Block checksums (256 bytes per block, simple additive CRC)
//
// Algorithm:
//		1. Get file attributes (timestamps)
//		2. Store metadata header
//		3. Read file in 256-byte blocks
//		4. For each block: sum all bytes (simple CRC)
//		5. Store CRC value for each block
//
// Notes:
//		Simple additive CRC (not cryptographic)
//		Used for detecting accidental corruption, not malicious tampering
//
//=============================================================================
long HERMES_CreateFileCheck(const char * name, char * scheck, const long & size, const float & id)
{
	WIN32_FILE_ATTRIBUTE_DATA attrib;
	FILE * file;
	long length(size >> 2), i = 7 * 4;		// length = size in longs, i = header size

	// Get file attributes (timestamps)
	if (!GetFileAttributesEx(name, GetFileExInfoStandard, &attrib)) return true;

	// Open file for reading
	if (!(file = fopen(name, "rb"))) return true;

	fseek(file, 0, SEEK_END);

	// Initialize checksum buffer
	memset(scheck, 0, size);
	((float *)scheck)[0] = id;									// Identifier
	((long *)scheck)[1] = size;									// Buffer size
	memcpy(&((long *)scheck)[2], &attrib.ftCreationTime, sizeof(FILETIME));		// Creation time
	memcpy(&((long *)scheck)[4], &attrib.ftLastWriteTime, sizeof(FILETIME));	// Modified time
	((long *)scheck)[6] = ftell(file);							// File size

	// Store filename (null-terminated, padded to 4-byte boundary)
	memcpy(&scheck[i], name, strlen(name) + 1);
	i += strlen(name) + 1;
	memset(&scheck[i], 0, i % 4);		// Pad to 4-byte boundary
	i += i % 4;
	i >>= 2;		// Convert to long index

	fseek(file, 0, SEEK_SET);

	// Generate block checksums (256 bytes per block)
	while (i < length)
	{
		long crc = 0;

		// Read 256 bytes and sum them (simple additive CRC)
		for (long j(0); j < 256; j++)
		{
			char read;

			if (!fread(&read, 1, 1, file)) break;

			crc += read;		// Simple additive checksum
		}

		((long *)scheck)[i++] = crc;		// Store block CRC

		if (feof(file))		// End of file? Zero remaining CRCs
		{
			memset(&((long *)scheck)[i], 0, (length - i) << 2);
			break;
		}
	}

	fclose(file);

	return false;		// Success
}

//#############################################################################
//                             FILE PATH MANIPULATION
//#############################################################################
//
// Purpose:
//		Path parsing and manipulation utilities using Win32 _splitpath/_makepath
//		Provides simple path component extraction and modification
//
// Global Buffers:
//		_drv, _dir, _name, _ext - Static buffers for path components
//		NOT THREAD-SAFE! Reused across all calls
//
//#############################################################################

char _drv[256];		// Drive letter (e.g., "C:")
char _dir[256];		// Directory path
char _name[256];	// Filename without extension
char _ext[256];		// Extension (e.g., ".txt")

// GetName - Extract filename (without extension) from path
//		Returns pointer to static buffer (copy immediately if needed)
char * GetName(char * str)
{
	_splitpath(str, _drv, _dir, _name, _ext);
	return _name;
}

// GetExt - Extract file extension from path (includes dot: ".txt")
//		Returns pointer to static buffer (copy immediately if needed)
char * GetExt(char * str)
{
	_splitpath(str, _drv, _dir, _name, _ext);
	return _ext;
}

// SetExt - Replace file extension (modifies str in-place)
//		str: Path to modify
//		new_ext: New extension (include dot: ".bak")
void SetExt(char * str, char * new_ext)
{
	_splitpath(str, _drv, _dir, _name, _ext);
	_makepath(str, _drv, _dir, _name, new_ext);
}

// AddToName - Append string to filename (before extension)
//		Example: "file.txt" + "_backup" = "file_backup.txt"
void AddToName(char * str, char * cat)
{
	_splitpath(str, _drv, _dir, _name, _ext);
	strcat(_name, cat);
	_makepath(str, _drv, _dir, _name, _ext);
}

// RemoveName - Remove filename, keep directory path only
//		Example: "C:\game\data\file.txt" -> "C:\game\data\"
void RemoveName(char * str)
{
	_splitpath(str, _drv, _dir, _name, _ext);
	_makepath(str, _drv, _dir, NULL, NULL);
}

//#############################################################################
//                             DIRECTORY OPERATIONS
//#############################################################################

//=============================================================================
// DirectoryExist - Check if Directory Exists
//=============================================================================
// Description:
//		Tests whether a directory exists using two methods:
//		1. Try _findfirst (may not work for all directory types)
//		2. Try changing to directory and back (fallback method)
//
// Parameters:
//		name: Directory path to test
//
// Returns:
//		1: Directory exists
//		0: Directory does not exist
//
// Notes:
//		Changes current directory temporarily (not thread-safe)
//
//=============================================================================
long DirectoryExist(char * name)
{
	long idx;
	struct _finddata_t fd;

	// Try _findfirst method
	if ((idx = _findfirst(name, &fd)) == -1)
	{
		_findclose(idx);
		char initial[256];
		_getcwd(initial, 255);		// Save current directory

		if (_chdir(name) == 0) // Can change to directory? It exists!
		{
			_chdir(initial);		// Restore original directory
			return 1;
		}

		_chdir(initial);
		return 0;		// Directory doesn't exist
	}

	_findclose(idx);
	return 1;		// Found via _findfirst
}

//=============================================================================
// CreateFullPath - Create Nested Directory Hierarchy
//=============================================================================
// Description:
//		Creates all directories in path (like "mkdir -p" on Unix)
//		Creates parent directories as needed
//
// Parameters:
//		path: Full directory path to create
//
// Returns:
//		TRUE: Success (all directories created or already exist)
//		FALSE: Failure (invalid path or creation failed)
//
// Algorithm:
//		1. Split path into components
//		2. Iterate through each directory level
//		3. Create each level using _mkdir (fails silently if exists)
//		4. Verify final path exists
//
// Example:
//		CreateFullPath("C:\\game\\data\\levels\\")
//		Creates: C:\game\, C:\game\data\, C:\game\data\levels\
//
//=============================================================================
BOOL CreateFullPath(char * path)
{

	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath(path, drive, dir, fname, ext);

	if (strlen(dir) == 0) return FALSE;		// No directory component

	char curpath[256];
	strcpy(curpath, drive);
	strcat(curpath, "\\");			// Start with "C:\"
	long start = 1;					// Skip leading backslash in dir
	long pos = 1;
	long size = strlen(dir);

	// Iterate through each directory level
	while (pos < size)
	{
		if ((dir[pos] == '\\') || (dir[pos] == '/'))	// Found separator?
		{
			dir[pos] = 0;		// Temporarily null-terminate
			memcpy(curpath + strlen(curpath), dir + start, pos - start + 1);
			strcat(curpath, "\\");
			_mkdir(curpath);	// Create directory (fails silently if exists)
			start = pos + 1;
		}

		pos++;
	}

	// Verify final path was created successfully
	if (DirectoryExist(path)) return TRUE;

	return FALSE;
}

//#############################################################################
//                             LOW-LEVEL FILE I/O WRAPPERS
//#############################################################################
//
// Purpose:
//		Thin wrappers around Win32 CRT file I/O functions (_open, _read, etc.)
//		Provides 1-based handle indexing (0 = error, 1+ = valid handle)
//
// Handle Convention:
//		External handles: 1-based (0 indicates error)
//		Internal handles: 0-based (converted via handle-1)
//		This allows 0 to be used as error sentinel value
//
//#############################################################################

// FileExist - Test if file exists and is readable
long FileExist(char * name)
{
	long i;

	if ((i = FileOpenRead(name)) == 0) return 0;		// Can't open = doesn't exist

	FileCloseRead(i);
	return 1;		// Exists
}

// FileOpenRead - Open file for reading (binary mode)
//		Returns: 0 on error, handle (1+) on success
long	FileOpenRead(char * name)
{
	long	handle;
	handle = _open((const char *)name, (int)_O_BINARY | _O_RDONLY);

	if (handle < 0)	return(0);		// Failed

	return(handle + 1);		// Convert to 1-based handle
}

// FileSizeHandle - Get current file position (used as size after SEEK_END)
//		Returns file position in bytes
long	FileSizeHandle(long handle)
{
	return(_tell((int)handle - 1));		// Convert to 0-based handle
}

// FileOpenWrite - Open/create file for writing (binary mode)
//		Creates new file or truncates existing file
//		Returns: 0 on error, handle (1+) on success
long	FileOpenWrite(char * name)
{
	int	handle;

	// Create/truncate file
	handle = _open((const char *)name, (int)_O_BINARY | _O_CREAT | _O_TRUNC, (int)_S_IWRITE);

	if (handle < 0)	return(0);		// Failed

	_close(handle);		// Close and reopen (why? possibly to flush creation)

	// Reopen for writing
	handle = _open((const char *)name, (int)_O_BINARY | _O_WRONLY);

	if (handle < 0)	return(0);		// Failed

	return(handle + 1);		// Convert to 1-based handle
}

// FileCloseRead - Close file opened for reading
long	FileCloseRead(long handle)
{
	return(_close((int)handle - 1));
}

// FileCloseWrite - Close file opened for writing (with commit to disk)
long	FileCloseWrite(long handle)
{
	_commit((int)handle - 1);		// Flush to disk
	return(_close((int)handle - 1));
}

// FileRead - Read bytes from file
//		Returns: Number of bytes actually read
long	FileRead(long handle, void * adr, long size)
{
	return(_read(handle - 1, adr, size));
}

// FileWrite - Write bytes to file
//		Returns: Number of bytes actually written
long	FileWrite(long handle, void * adr, long size)
{
	return(_write(handle - 1, adr, size));
}

// FileSeek - Set file position
//		mode: FILE_SEEK_START, FILE_SEEK_CURRENT, FILE_SEEK_END
//		Returns: New file position
long	FileSeek(long handle, long offset, long mode)
{
	return(_lseek((int)handle - 1, (long)offset, (int)mode));
}

// ExitApp - Graceful application shutdown
//		Sends WM_CLOSE to main window before exiting
void ExitApp(int v)
{
	if (MAIN_PROGRAM_HANDLE != NULL)
		SendMessage(MAIN_PROGRAM_HANDLE, WM_CLOSE, 0, 0);		// Request graceful close

	exit(v);		// Hard exit
}

//#############################################################################
//                             HIGH-LEVEL FILE LOADING
//#############################################################################

//=============================================================================
// FileLoadMallocZero - Load Entire File into Malloc'd Buffer (Null-Terminated)
//=============================================================================
// Description:
//		Loads complete file into dynamically allocated buffer
//		Appends two null bytes for safe text file processing
//
// Parameters:
//		name: File path to load
//		SizeLoadMalloc: Output - size of allocated buffer (includes +2 bytes)
//
// Returns:
//		Pointer to allocated buffer containing file data + 2 null bytes
//		NULL on failure
//
// Error Handling:
//		Shows MessageBox with Abort/Retry/Ignore on errors:
//		- File not found
//		- Memory allocation failure
//		- Read size mismatch
//
// Notes:
//		Caller must free() returned pointer
//		Buffer is file_size + 2 bytes (two null terminators)
//		Useful for loading text files as strings
//
//=============================================================================
void	* FileLoadMallocZero(char * name, long * SizeLoadMalloc)
{
	long	handle;
	long	size1, size2;
	unsigned char	* adr;

retry:		// Retry label for error handling
	;
	handle = FileOpenRead(name);

	if (!handle)		// File open failed
	{
		char str[256];
		strcpy(str, name);
		int mb;
		mb = ShowError("FileLoadMalloc", str, 4);		// Show error dialog

		switch (mb)
		{
			case IDABORT:
				ExitApp(1);
				break;
			case IDRETRY:
				goto  retry;		// Try again
				break;
			case IDIGNORE:
				if (SizeLoadMalloc != NULL) *SizeLoadMalloc = 0;
				return(NULL);
				break;
		}
	}

	// Get file size
	FileSeek(handle, 0, FILE_SEEK_END);
	size1 = FileSizeHandle(handle) + 2;		// +2 for null terminators
	adr = (unsigned char *)malloc(size1);

	if (!adr)		// Allocation failed
	{
		int mb;
		mb = ShowError("FileLoadMalloc", name, 4);

		switch (mb)
		{
			case IDABORT:
				ExitApp(1);
				break;
			case IDRETRY:
				FileCloseRead(handle);
				goto  retry;
				break;
			case IDIGNORE:
				FileCloseRead(handle);
				if (SizeLoadMalloc != NULL) *SizeLoadMalloc = 0;
				return(0);
				break;
		}
	}

	// Read file data
	FileSeek(handle, 0, FILE_SEEK_START);
	size2 = FileRead(handle, adr, size1 - 2);		// Read file_size bytes
	FileCloseRead(handle);

	if (size1 != size2 + 2)		// Read size mismatch
	{
		free(adr);
		int mb;
		mb = ShowError("FileLoadMalloc", name, 4);

		switch (mb)
		{
			case IDABORT:
				ExitApp(1);
				break;
			case IDRETRY:
				goto  retry;
				break;
			case IDIGNORE:
				if (SizeLoadMalloc != NULL) *SizeLoadMalloc = 0;
				return(0);
				break;
		}
	}

	if (SizeLoadMalloc != NULL) *SizeLoadMalloc = size1;

	adr[size1-1] = 0;		// Null terminate
	adr[size1-2] = 0;		// Double null (for safety)
	return(adr);
}

//=============================================================================
// FileLoadMalloc - Load Entire File into Malloc'd Buffer (No Null Termination)
//=============================================================================
// Description:
//		Like FileLoadMallocZero but without null termination
//		For loading binary files
//
// Parameters:
//		name: File path
//		SizeLoadMalloc: Output - exact file size
//
// Returns:
//		Pointer to allocated buffer, NULL on failure
//
//=============================================================================
void	* FileLoadMalloc(char * name, long * SizeLoadMalloc)
{
	long	handle;
	long	size1, size2;
	unsigned char	* adr;

retry:
	handle = FileOpenRead(name);

	if (!handle)		// File open failed
	{
		char str[256];
		strcpy(str, name);
		int mb;
		mb = ShowError("FileLoadMalloc", str, 4);

		switch (mb)
		{
			case IDABORT:
				ExitApp(1);
				break;
			case IDRETRY:
				goto  retry;
				break;
			case IDIGNORE:
				if (SizeLoadMalloc != NULL) *SizeLoadMalloc = 0;
				return(NULL);
				break;
		}

	}

	FileSeek(handle, 0, FILE_SEEK_END);
	size1 = FileSizeHandle(handle);		// Exact file size (no +2)
	adr = (unsigned char *)malloc(size1);

	if (!adr)		// Allocation failed
	{
		int mb;
		mb = ShowError("FileLoadMalloc", name, 4);

		switch (mb)
		{
			case IDABORT:
				ExitApp(1);
				break;
			case IDRETRY:
				FileCloseRead(handle);
				goto  retry;
				break;
			case IDIGNORE:
				FileCloseRead(handle);

				if (SizeLoadMalloc != NULL) *SizeLoadMalloc = 0;

				return(0);
				break;
		}
	}

	// Read file data (exact size, no null termination)
	FileSeek(handle, 0, FILE_SEEK_START);
	size2 = FileRead(handle, adr, size1);		// Read entire file
	FileCloseRead(handle);

	if (size1 != size2)		// Read size mismatch
	{
		free(adr);
		int mb;
		mb = ShowError("FileLoadMalloc", name, 4);

		switch (mb)
		{
			case IDABORT:
				ExitApp(1);
				break;
			case IDRETRY:
				goto  retry;
				break;
			case IDIGNORE:
				if (SizeLoadMalloc != NULL) *SizeLoadMalloc = 0;
				return(0);
				break;
		}

	}

	if (SizeLoadMalloc != NULL) *SizeLoadMalloc = size1;

	return(adr);		// Return buffer (NO null termination - binary data)
}

//#############################################################################
//                             FILE DIALOG FUNCTIONS
//#############################################################################
//
// Purpose:
//		Win32 common dialog wrappers for Open/Save file dialogs
//		and folder browsing
//
//#############################################################################

char	LastFolder[_MAX_PATH];		// Last folder browsed (global state)
static OPENFILENAME ofn;			// Reusable OPENFILENAME structure

// HERMESFolderBrowse - Show folder browser dialog
//		str: Dialog title
//		Returns: TRUE if folder selected, FALSE if cancelled
//		Result stored in LastFolder global
bool HERMESFolderBrowse(char * str)
{
	BROWSEINFO		bi;
	LPITEMIDLIST	liil;

	// Setup BROWSEINFO structure
	bi.hwndOwner	= NULL;				// No owner window
	bi.pidlRoot		= NULL;				// Start at desktop
	bi.pszDisplayName = LastFolder;
	bi.lpszTitle	= str;				// Dialog title
	bi.ulFlags		= 0;
	bi.lpfn			= NULL;				// No callback
	bi.lParam		= 0;
	bi.iImage		= 0;

	liil = SHBrowseForFolder(&bi);		// Show folder browser

	if (liil)		// User selected folder?
	{
		if (SHGetPathFromIDList(liil, LastFolder))	return TRUE;	// Convert to path
		else return FALSE;
	}
	else return FALSE;		// User cancelled
}

// HERMESFolderSelector - Show folder selector and return path
//		file_name: Output buffer for selected folder path (with trailing backslash)
//		title: Dialog title
//		Returns: TRUE if folder selected, FALSE if cancelled
bool HERMESFolderSelector(char * file_name, char * title)
{
	if (HERMESFolderBrowse(title))
	{
		sprintf(file_name, "%s\\", LastFolder);		// Append trailing backslash
		return TRUE;
	}
	else
	{
		strcpy(file_name, " ");		// Empty result
		return FALSE;
	}
}

// HERMES_WFSelectorCommon - Internal: Show Open/Save file dialog
//		Common implementation for Open and Save dialogs
//		flag_operation: 1=Open dialog, 0=Save dialog
BOOL HERMES_WFSelectorCommon(PSTR pstrFileName, PSTR pstrTitleName, char * filter, long flag, long flag_operation, long max_car, HWND hWnd)
{
	LONG	value;
	char	cwd[_MAX_PATH];

	// Initialize OPENFILENAME structure
	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hInstance			= NULL;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= _MAX_FNAME + _MAX_EXT;
	ofn.nFileOffset		= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt		= "txt";
	ofn.lCustData			= 0L;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;

	// Set parameters
	ofn.lpstrFilter			= filter;
	ofn.hwndOwner			= hWnd;
	ofn.lpstrFile			= pstrFileName;
	ofn.lpstrTitle			= pstrTitleName;
	ofn.Flags				= flag;

	_getcwd(cwd, _MAX_PATH);
	ofn.lpstrInitialDir = cwd;		// Start in current directory
	ofn.nMaxFile = max_car;

	if (flag_operation)		// Open dialog?
	{
		value = GetOpenFileName(&ofn);
	}
	else					// Save dialog
	{
		value = GetSaveFileName(&ofn);
	}

	return value;
}

// HERMESFileSelectorOpen - Show "Open File" dialog
int HERMESFileSelectorOpen(PSTR pstrFileName, PSTR pstrTitleName, char * filter, HWND hWnd)
{
	return HERMES_WFSelectorCommon(pstrFileName, pstrTitleName, filter, OFN_HIDEREADONLY | OFN_CREATEPROMPT, 1, _MAX_PATH, hWnd);
}

// HERMESFileSelectorSave - Show "Save File" dialog
int HERMESFileSelectorSave(PSTR pstrFileName, PSTR pstrTitleName, char * filter, HWND hWnd)
{
	return HERMES_WFSelectorCommon(pstrFileName, pstrTitleName, filter, OFN_OVERWRITEPROMPT, 0, _MAX_PATH, hWnd);
}

//#############################################################################
//                             PKZIP COMPRESSION/DECOMPRESSION
//#############################################################################
//
// Purpose:
//		PKZIP-style implode/explode compression wrappers
//		Uses external implode.lib library (PKWare compression algorithm)
//
// Functions:
//		implode() - Compress data (external library function)
//		explode() - Decompress data (external library function)
//		Callback-based I/O for streaming compression
//
// Note:
//		WorkBuff: Global work buffer required by compression library
//
//#############################################################################

char WorkBuff[CMP_BUFFER_SIZE];		// Work buffer for compression library

//=============================================================================
// Compression I/O Callbacks
//=============================================================================
// These callback functions are passed to implode()/explode()
// They handle reading source data and writing destination data
// Param points to PARAM structure containing source/dest buffers and offsets
//=============================================================================

// ReadUnCompressed - Read callback for implode() (source data)
//		Reads uncompressed data that will be compressed
//		Returns: Number of bytes read (0 = end of data)
unsigned int
ReadUnCompressed(char * buff, unsigned int * size, void * Param)
{
	PARAM * Ptr = (PARAM *) Param;

	if (Ptr->UnCompressedSize == 0L)		// No more data?
	{
		return(0);		// Terminate compression
	}

	// Limit read size to remaining data
	if (Ptr->UnCompressedSize < (unsigned long)*size)
	{
		*size = (unsigned int)Ptr->UnCompressedSize;
	}

	// Copy from source buffer
	memcpy(buff, Ptr->pSource + Ptr->SourceOffset, *size);
	Ptr->SourceOffset += (unsigned long) * size;
	Ptr->UnCompressedSize -= (unsigned long) * size;

	return(*size);
}

// ReadCompressed - Read callback for explode() (compressed source data)
//		Reads compressed data that will be decompressed
unsigned int
ReadCompressed(char * buff, unsigned int * size, void * Param)
{
	PARAM * Ptr = (PARAM *) Param;

	if (Ptr->CompressedSize == 0L)		// No more data?
	{
		return(0);		// Terminate decompression
	}

	// Limit read size to remaining data
	if (Ptr->CompressedSize < (unsigned long)*size)
	{
		*size = (unsigned int)Ptr->CompressedSize;
	}

	// Copy from source buffer
	memcpy(buff, Ptr->pSource + Ptr->SourceOffset, *size);
	Ptr->SourceOffset += (unsigned long) * size;
	Ptr->CompressedSize -= (unsigned long) * size;

	return(*size);
}

// WriteCompressed - Write callback for implode() (compressed output)
//		Writes compressed data output
//		Auto-grows buffer as needed
void
WriteCompressed(char * buff, unsigned int * size, void * Param)
{
	PARAM * Ptr = (PARAM *) Param;

	// Grow buffer if needed
	if (Ptr->CompressedSize + (unsigned long)*size > Ptr->BufferSize)
	{
		Ptr->BufferSize = Ptr->CompressedSize + (unsigned long) * size;
		Ptr->pDestination = (char *)realloc(Ptr->pDestination, Ptr->BufferSize);
	}

	if (Ptr->pDestination)
	{
		memcpy(Ptr->pDestination + Ptr->DestinationOffset, buff, *size);
		Ptr->DestinationOffset += (unsigned long) * size;
		Ptr->CompressedSize += (unsigned long) * size;
	}
}

// WriteUnCompressed - Write callback for explode() (decompressed output)
//		Writes decompressed data output
//		Auto-grows buffer as needed (generous 10x growth)
void
WriteUnCompressed(char * buff, unsigned int * size, void * Param)
{
	PARAM * Ptr = (PARAM *) Param;

	// Grow buffer if needed (10x for efficiency)
	if (Ptr->UnCompressedSize + (unsigned long)*size > Ptr->BufferSize)
	{
		Ptr->BufferSize = Ptr->UnCompressedSize + ((unsigned long) * size) * 10;
		Ptr->pDestination = (char *)realloc(Ptr->pDestination, Ptr->BufferSize);
	}

	if (Ptr->pDestination)
	{
		memcpy(Ptr->pDestination + Ptr->DestinationOffset, buff, *size);
		Ptr->DestinationOffset += (unsigned long) * size;
		Ptr->UnCompressedSize += (unsigned long) * size;
	}
}

// WriteUnCompressedNoAlloc - Write callback for explode() (pre-allocated buffer)
//		Like WriteUnCompressed but doesn't realloc (buffer pre-allocated)
void
WriteUnCompressedNoAlloc(char * buff, unsigned int * size, void * Param)
{
	PARAM * Ptr = (PARAM *) Param;

	// Update buffer size tracking (but don't realloc)
	if (Ptr->UnCompressedSize + (unsigned long)*size > Ptr->BufferSize)
	{
		Ptr->BufferSize = Ptr->UnCompressedSize + ((unsigned long) * size) * 10;
	}

	if (Ptr->pDestination)
	{
		memcpy(Ptr->pDestination + Ptr->DestinationOffset, buff, *size);
		Ptr->DestinationOffset += (unsigned long) * size;
		Ptr->UnCompressedSize += (unsigned long) * size;
	}
}

//=============================================================================
// STD_Implode - Compress Buffer Using PKZIP Algorithm
//=============================================================================
// Description:
//		Compresses data using PKZIP implode algorithm
//		Auto-selects dictionary size based on input size
//
// Parameters:
//		from: Source data buffer
//		from_size: Source data size (bytes)
//		to_size: Output - compressed size
//
// Returns:
//		Pointer to malloc'd buffer containing compressed data
//		NULL on failure
//
// Dictionary Sizes:
//		<= 32KB: 1024 byte dictionary
//		<= 128KB: 2048 byte dictionary
//		> 128KB: 4096 byte dictionary
//
//=============================================================================
char * STD_Implode(char * from, long from_size, long * to_size)
{
	if (WorkBuff == NULL)		// Work buffer not initialized
	{
		return NULL;
	}

	unsigned int type  = CMP_BINARY;		// Binary compression
	unsigned int dsize;						// Dictionary size

	// Select dictionary size based on input size
	if (from_size <= 32768)
		dsize = 1024;
	else if (from_size <= 131072)
		dsize = 2048;
	else dsize = 4096;

	PARAM Param;
	memset(&Param, 0, sizeof(PARAM));

	Param.pSource = from;
	Param.pDestination = (char *)malloc(from_size);		// Initial buffer (will grow if needed)

	bool bFirst = true;		// First attempt flag

	while (1)
	{
		// Reset parameters for compression
		Param.CompressedSize = 0;
		Param.UnCompressedSize = from_size;
		Param.BufferSize = Param.UnCompressedSize;
		Param.SourceOffset      = 0L;
		Param.DestinationOffset = 0L;
		Param.Crc               = (unsigned long) - 1;
		long lResult = implode(ReadUnCompressed, WriteCompressed, WorkBuff, &Param, &type, &dsize);

		if (!lResult)		// Success
		{
			break;
		}

		if (bFirst)		// First attempt failed - retry once
		{
			bFirst = false;
			continue;
		}

		// Show error dialog
		lResult = MessageBox(NULL, "Failed while saving...", "Save Error", MB_RETRYCANCEL | MB_ICONERROR);

		if (lResult == IDCANCEL)
		{
			break;
		}
	}

	*to_size = Param.CompressedSize;
	return Param.pDestination;		// Caller must free()
}

//=============================================================================
// STD_Explode - Decompress PKZIP-Compressed Buffer
//=============================================================================
// Description:
//		Decompresses PKZIP implode-compressed data
//		Allocates output buffer automatically
//
// Parameters:
//		from: Compressed data buffer
//		from_size: Compressed size (bytes)
//		to_size: Output - decompressed size
//
// Returns:
//		Pointer to malloc'd buffer containing decompressed data
//		NULL on error
//
//=============================================================================
char * STD_Explode(char * from, long from_size, long * to_size)
{
	if (WorkBuff == NULL)
	{
		return NULL;
	}

	PARAM Param;
	memset(&Param, 0, sizeof(PARAM));
	Param.BufferSize = 0;
	Param.pSource = from;
	Param.pDestination = NULL;		// Will be malloc'd by WriteUnCompressed callback
	Param.CompressedSize = from_size;

	Param.Crc               = (unsigned long) - 1;
	unsigned int error = explode(ReadCompressed, WriteUnCompressed, WorkBuff, &Param);

	if (error)		// Decompression failed
	{
		*to_size = 0;
		return NULL;
	}

	*to_size = Param.UnCompressedSize;
	return Param.pDestination;		// Caller must free()
}

//=============================================================================
// STD_ExplodeNoAlloc - Decompress Into Pre-Allocated Buffer
//=============================================================================
// Description:
//		Like STD_Explode but decompresses into caller-provided buffer
//		No allocation - caller must ensure buffer is large enough
//
// Parameters:
//		from: Compressed data
//		from_size: Compressed size
//		to: Pre-allocated output buffer
//		to_size: Output - decompressed size
//
//=============================================================================
void STD_ExplodeNoAlloc(char * from, long from_size, char * to, long * to_size)
{
	if (WorkBuff == NULL)
	{
		return;
	}

	PARAM Param;
	memset(&Param, 0, sizeof(PARAM));
	Param.BufferSize = 0;
	Param.pSource = from;
	Param.pDestination = to;		// Pre-allocated buffer
	Param.CompressedSize = from_size;
	Param.Crc               = (unsigned long) - 1;
	unsigned int error = explode(ReadCompressed, WriteUnCompressedNoAlloc, WorkBuff, &Param);

	if (error)
	{
		*to_size = 0;
		return;
	}

	*to_size = Param.UnCompressedSize;
}

//#############################################################################
//                             ERROR LOGGING
//#############################################################################

// hrnd() - Random float [0.0, 1.0) (unused?)
#define hrnd()  (((FLOAT)rand() ) * 0.00003051850947599f)

char ERROR_Log_FIC[256];	// Error log filename
long ERROR_Log_Write = 0;	// Error logging enabled flag

// ERROR_Log_Init - Initialize error log file
//		fich: Log filename
//		Creates empty log file, enables logging if successful
void ERROR_Log_Init(char * fich)
{
	strcpy(ERROR_Log_FIC, fich);
	FILE * fic;

	if ((fic = fopen(ERROR_Log_FIC, "w")) != NULL)		// Create empty file
	{
		ERROR_Log_Write = 1;		// Enable logging
		fclose(fic);
	}
	else ERROR_Log_Write = 0;		// Disable logging (can't create file)
}

// ERROR_Log - Append error message to log
//		fich: Error message
//		Returns: true if logged, false if logging disabled or failed
bool ERROR_Log(char * fich)
{
	FILE * fic;

	if (ERROR_Log_Write)		// Logging enabled?
	{
		if ((fic = fopen(ERROR_Log_FIC, "a+")) != NULL)		// Append mode
		{
			fprintf(fic, "Error: %s\n", fich);
			fclose(fic);
			return true;
		}
	}

	return false;
}

//#############################################################################
//                             MEMORY SECURITY / EMERGENCY ALLOCATION
//#############################################################################
//
// Purpose:
//		Reserve emergency memory buffer that can be freed during out-of-memory
//		Allows showing error dialog and graceful shutdown when malloc fails
//
//#############################################################################

char * HERMES_MEMORY_SECURITY = NULL;		// Emergency memory reserve

// HERMES_Memory_Security_On - Allocate emergency memory reserve
//		size: Reserve size (minimum 128KB)
//		Allocates buffer that can be freed during out-of-memory conditions
void HERMES_Memory_Security_On(long size)
{
	if (size < 128000) size = 128000;		// Minimum 128KB reserve

	HERMES_MEMORY_SECURITY = (char *)malloc(size);
}

// HERMES_Memory_Security_Off - Free emergency memory reserve
void HERMES_Memory_Security_Off()
{
	if (HERMES_MEMORY_SECURITY)
		free(HERMES_MEMORY_SECURITY);

	HERMES_MEMORY_SECURITY = NULL;
}

//=============================================================================
// HERMES_Memory_Emergency_Out - Out-of-Memory Error Handler
//=============================================================================
// Description:
//		Called when malloc fails
//		Frees emergency reserve, shows error dialog
//
// Parameters:
//		size: Failed allocation size
//		info: Additional error context
//
// Returns:
//		1: User chose Retry
//		0: Exit application (doesn't return)
//
// Notes:
//		Always exits on Cancel or if no emergency buffer available
//
//=============================================================================
long HERMES_Memory_Emergency_Out(long size, char * info)
{
	// Free emergency reserve to allow error dialog allocation
	if (HERMES_MEMORY_SECURITY)
		free(HERMES_MEMORY_SECURITY);

	HERMES_MEMORY_SECURITY = NULL;
	char out[512];

	// Format error message
	if (info)
	{
		if (size > 0)
			sprintf(out, "FATAL ERROR: Unable To Allocate %d bytes... %s", size, info);
		else
			sprintf(out, "FATAL ERROR: Unable To Allocate Memory... %s", info);
	}
	else if (size > 0)
		sprintf(out, "FATAL ERROR: Unable To Allocate %d bytes...", size);
	else
		sprintf(out, "FATAL ERROR: Unable To Allocate Memory...");

	// Show error dialog
	int re = MessageBox(NULL, out, "ARX Fatalis", MB_RETRYCANCEL | MB_ICONSTOP);

	if ((re == IDRETRY) && (size > 0))
		return 1;		// Retry

	exit(0);		// Exit application
	return 0;
}

//#############################################################################
//                             PERFORMANCE BENCHMARKING
//#############################################################################
//
// Purpose:
//		High-resolution performance timing using QueryPerformanceCounter
//		For profiling code sections
//
//#############################################################################

LARGE_INTEGER	start_chrono;		// Benchmark start timestamp
long NEED_BENCH = 0;				// Benchmarking enabled flag

// StartBench - Begin performance measurement
//		Captures high-resolution timestamp
void StartBench()
{
	if (NEED_BENCH)
	{
		QueryPerformanceCounter(&start_chrono);
	}
}

// EndBench - End performance measurement
//		Returns: Elapsed CPU ticks (high-resolution)
//		Convert to time using QueryPerformanceFrequency
unsigned long EndBench()
{
	if (NEED_BENCH)
	{
		LARGE_INTEGER	end_chrono;
		QueryPerformanceCounter(&end_chrono);
		unsigned long ret = (unsigned long)(end_chrono.QuadPart - start_chrono.QuadPart);
		return ret;		// Return elapsed ticks
	}

	return 0;
}

//=============================================================================
// END OF FILE
//=============================================================================
 
