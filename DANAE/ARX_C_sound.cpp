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
// ARX_C_sound.CPP - Cinematic Sound Management System
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Sound resource management and playback for cinematic sequences
//		Handles localized audio paths, sound loading, and synchronized playback
//		Manages sound resources for cutscenes with multi-language support
//
// Purpose:
//		- Manage sound resources for cinematics
//		- Handle localized audio file paths
//		- Synchronize sound playback with keyframes
//		- Provide language-specific sound variants
//		- Integrate with main ARX sound system
//
// Key Responsibilities:
//		- Sound resource allocation/deallocation
//		- Path normalization for localized audio
//		- Language selection and path mapping
//		- Sound playback at cinematic keyframes
//		- Sound handle management
//		- Resource cleanup
//
// Global Data:
//		C_SOUND TabSound[MAX_SOUND]:
//			- Array of cinematic sound resources
//			- Fixed-size pool allocation
//			- Each entry contains:
//				* actif: Active flag + language ID (upper byte)
//				* dir: Source directory path
//				* name: Sound filename
//				* sound: Final resolved file path
//				* load: Load status flag
//				* idhandle: ARX_SOUND system handle
//
//		NbSound:
//			- Count of currently loaded sounds
//			- Used for resource tracking
//
//		LSoundChoose:
//			- Language selection (upper byte)
//			- Values: C_LANGUAGE_ENGLISH << 8, etc.
//			- Filters sounds by language
//
// Core Functions:
//		InitSound(CINEMATIQUE* c):
//			- Initializes sound table to empty state
//			- Sets all handles to ARX_SOUND_INVALID_RESOURCE
//			- Zeros all C_SOUND structures
//			- Resets NbSound counter
//			- Called when starting new cinematic
//
//		GetFreeSound(int* num):
//			- Searches for unused slot in TabSound[]
//			- Returns pointer to free C_SOUND entry
//			- Sets num to index in array
//			- Returns NULL if table full
//
//		DeleteFreeSound(int num):
//			- Removes sound from slot num
//			- Frees allocated strings (dir, name, sound)
//			- Marks slot inactive
//			- Decrements NbSound counter
//			- Returns FALSE if slot already inactive
//
//		DeleteAllSound():
//			- Iterates through all MAX_SOUND slots
//			- Calls DeleteFreeSound() on each
//			- Used when clearing cinematic
//			- Ensures complete cleanup
//
//		ExistSound(char* dir, char* name):
//			- Checks if sound already loaded
//			- Searches by directory and filename
//			- Filters by current language (LSoundChoose)
//			- Returns index if found, -1 otherwise
//			- Prevents duplicate loading
//
//		AddSoundToList(char* dir, char* name, int id, int pos):
//			- Main function for loading cinematic sounds
//			- Checks for existing sound (avoids duplicates)
//			- Allocates C_SOUND slot (reuses id if >= 0)
//			- Copies directory and filename
//			- Builds final path with localization
//			- Calls PatchReplace() for path normalization
//			- Stores uppercase path in cs->sound
//			- Marks as loaded and active
//			- Returns slot index or -1 on failure
//
//		PlaySoundKeyFramer(int id):
//			- Plays sound at cinematic keyframe
//			- Calls ARX_SOUND_PlayCinematic()
//			- Strips working directory prefix (g_pak_workdir_len)
//			- Stores handle in cs->idhandle
//			- Returns FALSE if slot inactive
//
//		StopSoundKeyFramer():
//			- Stops all cinematic sounds
//			- Iterates through TabSound[] array
//			- Calls ARX_SOUND_Stop() on active sounds
//			- Resets handles to ARX_SOUND_INVALID_RESOURCE
//			- Called when cinematic ends or skips
//
// Path Processing:
//		PatchReplace():
//			- Complex path normalization for localization
//			- Replaces "uk" with "english\\"
//			- Replaces "fr" with "francais\\"
//			- Removes absolute path prefix (ClearAbsDirectory)
//			- Adds working directory (AddDirectory)
//			- Removes "sfx\\" from "sfx\\speech\\" paths
//			- Inserts localization folder in speech paths
//			- Final format: "speech\\<locale>\\filename.wav"
//
//		CutAndAddString(char* pText, char* pDebText):
//			- Searches for pDebText substring in pText
//			- Concatenates remaining text to AllTxt
//			- Case-insensitive search (strnicmp)
//			- Used for extracting partial paths
//
// Language Mapping:
//		Language codes converted to folders:
//			"uk" -> "english\\"
//			"fr" -> "francais\\"
//
//		Path transformation example:
//			Input:  "sfx\\speech\\uk\\dialogue.wav"
//			Output: "<workdir>\\speech\\english\\dialogue.wav"
//
//		LSoundChoose encoding:
//			Upper byte: Language ID
//			Lower byte: Unused
//			Example: C_LANGUAGE_ENGLISH << 8
//
// Path Building Logic:
//		For SFX sounds:
//			- Start with "\\\\Arkaneserver\\public\\ARX\\"
//			- Cut and add from "sfx" directory
//			- Append filename
//			- Apply PatchReplace() normalization
//			- Convert to uppercase
//
//		For speech sounds:
//			- Format: "<workdir>\\speech\\<locale>\\<filename>"
//			- Uses Project.localisationpath
//			- No uppercase conversion
//
// C_SOUND Structure Fields:
//		actif (short):
//			- Lower byte: Active flag (0 = inactive, 1 = active)
//			- Upper byte: Language ID
//			- Combined: actif = 1 | LSoundChoose
//
//		dir (char*):
//			- Source directory path
//			- Dynamically allocated
//			- Freed in DeleteFreeSound()
//
//		name (char*):
//			- Sound filename
//			- Dynamically allocated
//			- Freed in DeleteFreeSound()
//
//		sound (char*):
//			- Final resolved file path
//			- Used for ARX_SOUND_PlayCinematic()
//			- Uppercase for SFX, mixed case for speech
//			- Dynamically allocated (strdup)
//
//		load (int):
//			- Load status flag
//			- Set to 1 when sound added
//			- Checked to prevent overwriting loaded sounds
//
//		idhandle:
//			- ARX sound system resource handle
//			- Set by ARX_SOUND_PlayCinematic()
//			- Used by ARX_SOUND_Stop()
//			- ARX_SOUND_INVALID_RESOURCE when not playing
//
// Memory Management:
//		Allocations:
//			- cs->dir: malloc(strlen(dir) + 1)
//			- cs->name: malloc(strlen(name) + 1)
//			- cs->sound: strdup(AllTxt) or strdup(szTemp)
//
//		Deallocations:
//			- All freed in DeleteFreeSound()
//			- Handles NULL pointers safely
//			- No memory leaks when properly cleaned
//
// Integration with ARX_SOUND:
//		ARX_SOUND_PlayCinematic():
//			- Plays sound exclusively for cinematics
//			- Different from gameplay sounds
//			- Returns sound handle
//
//		ARX_SOUND_Stop():
//			- Stops playing sound by handle
//			- Safe to call on invalid handles
//
//		ARX_SOUND_INVALID_RESOURCE:
//			- Sentinel value for invalid handles
//			- Used to mark unloaded/stopped sounds
//
// Global Variables (extern):
//		AllTxt[32767]:
//			- Large buffer for path construction
//			- Used by PatchReplace() and path builders
//			- Shared with other cinematic systems
//
//		DirectoryAbs:
//			- Absolute working directory
//			- Added by PatchReplace()
//
//		g_pak_workdir_len:
//			- Length of PAK working directory prefix
//			- Stripped from paths in PlaySoundKeyFramer()
//
//		Project.workingdir:
//			- Game working directory
//			- Base path for all resources
//
//		Project.localisationpath:
//			- Current language folder name
//			- Example: "english", "francais"
//
// Use Cases:
//		- Playing localized dialogue in cutscenes
//		- Sound effects synchronized to keyframes
//		- Multi-language cinematic support
//		- Voice acting for story sequences
//		- Sound cue triggering at specific frames
//
// Workflow:
//		1. InitSound() - clear sound table
//		2. AddSoundToList() - load sounds for cinematic
//		3. Cinematic keyframe reached
//		4. PlaySoundKeyFramer() - play sound
//		5. Cinematic ends
//		6. StopSoundKeyFramer() - stop all sounds
//		7. DeleteAllSound() - cleanup resources
//
// Error Handling:
//		- Returns NULL/FALSE on allocation failure
//		- Checks for inactive slots
//		- Prevents duplicate loading
//		- Validates sound handles
//		- Safe cleanup on errors
//
// Technical Notes:
//		- Fixed-size sound pool (MAX_SOUND)
//		- Case-insensitive path matching
//		- String duplication for safety
//		- Path normalization for cross-platform
//		- Upper byte of actif stores language
//		- Lower byte stores active state
//
// Limitations:
//		- MAX_SOUND simultaneous sounds
//		- Single language active at a time
//		- No streaming (all loaded)
//		- No volume/pan control exposed
//		- No sound priority system
//
// Dependencies:
//		- arx_c_cinematique.h (CINEMATIQUE class)
//		- arx_sound.h (ARX_SOUND_* functions)
//		- String manipulation (strcpy, strdup, etc.)
//		- Memory allocation (malloc, free)
//		- Path utilities (ClearAbsDirectory, AddDirectory)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include "arx_c_cinematique.h"
#include "resource.h"
#include "arx_sound.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

/*-----------------------------------------------------------*/
C_SOUND		TabSound[MAX_SOUND];
int			NbSound;
/*-----------------------------------------------------------*/
extern char AllTxt[];
extern HWND HwndPere;
extern char DirectoryChoose[];
extern int	LSoundChoose;

extern char		DirectoryAbs[];

extern ULONG g_pak_workdir_len;

void ClearAbsDirectory(char * pT, char * d);
void AddDirectory(char * pT, char * dir);
/*-----------------------------------------------------------*/
void InitSound(CINEMATIQUE * c)
{
	C_SOUND	*	ts;
	int			nb;

	ts = TabSound;
	nb = MAX_SOUND;

	while (nb)
	{
		memset((void *)ts, 0, sizeof(C_SOUND));
		ts->idhandle = ARX_SOUND_INVALID_RESOURCE;
		ts++;
		nb--;
	}

	NbSound = 0;
}
/*-----------------------------------------------------------*/
C_SOUND * GetFreeSound(int * num)
{
	C_SOUND	*	ts;
	int			nb;

	ts = TabSound;
	nb = MAX_SOUND;

	while (nb)
	{
		if (!ts->actif)
		{
			*num = MAX_SOUND - nb;
			return ts;
		}

		ts++;
		nb--;
	}

	return NULL;
}
/*-----------------------------------------------------------*/
BOOL DeleteFreeSound(int num)
{
	C_SOUND	*	cs;
	int			l;

	cs = &TabSound[num];

	if (!cs->actif) return FALSE;

	l = 0;

	if (cs->dir)
	{
		free((void *)cs->dir);
		cs->dir = NULL;
	}

	if (cs->name)
	{
		free((void *)cs->name);
		cs->name = NULL;
	}

	if (cs->sound)
	{
		free((void *)cs->sound);
		cs->sound = NULL;
	}

	cs->actif = 0;
	NbSound--;

	return TRUE;
}
/*-----------------------------------------------------------*/
void DeleteAllSound(void)
{
	int	nb;

	nb = MAX_SOUND;

	while (nb)
	{
		DeleteFreeSound(MAX_SOUND - nb);
		nb--;
	}
}
/*-----------------------------------------------------------*/
void CutAndAddString(char * pText, char * pDebText)
{
	int	i = strlen(pText);
	int j = strlen(pDebText);
	bool bOk = false;

	while (i--)
	{
		if (!strnicmp(pText, pDebText, j))
		{
			bOk = true;
			break;
		}

		pText++;
	}

	if (bOk)
	{
		strcat(AllTxt, pText);
	}
}
/*-----------------------------------------------------------*/
int ExistSound(char * dir, char * name)
{
	C_SOUND * cs;

	cs = TabSound;
	int nb = MAX_SOUND;

	while (nb)
	{
		if ((cs->actif) &&
		        ((cs->actif & 0xFF00) == LSoundChoose))
		{
			if (!stricmp(dir, cs->dir))
			{
				if (!stricmp(name, cs->name))
				{
					return MAX_SOUND - nb;
				}
			}
		}

		cs++;
		nb--;
	}

	return -1;
}

/*-----------------------------------------------------------*/
void PatchReplace()
{
	char CopyTxt[256];
	int j = strlen(AllTxt);
	char * pT = AllTxt;

	while (j--)
	{
		if (!strnicmp(pT, "uk", strlen("uk")))
		{
			*pT = 0;
			strcpy(CopyTxt, pT + 3);
			strcat(AllTxt, "english\\");
			strcat(AllTxt, CopyTxt);
			break;
		}

		if (!strnicmp(pT, "fr", strlen("fr")))
		{
			*pT = 0;
			strcpy(CopyTxt, pT + 3);
			strcat(AllTxt, "francais\\");
			strcat(AllTxt, CopyTxt);
			break;
		}

		pT++;
	}

	ClearAbsDirectory(AllTxt, "arx\\");
	AddDirectory(AllTxt, DirectoryAbs);

	//on enleve "sfx"
	bool bFound = false;
	pT = AllTxt;
	j = strlen((const char *)pT);

	while (j)
	{
		if (!strnicmp((const char *)pT, "sfx\\speech\\", strlen((const char *)"sfx\\speech\\")))
		{
			bFound = true;
			break;
		}

		j--;
		pT++;
	}

	if (bFound)
	{
		memmove((void *)pT, (const void *)(pT + 4), strlen((const char *)(pT + 4)) + 1);
	}

	//UNIQUEMENT EN MODE GAME!!!!!!
	char * pcTxt = strstr(AllTxt, "speech\\");

	if (pcTxt)
	{
		pcTxt += strlen("speech\\");
		char * pcTxt2 = strdup(pcTxt);
		char * pcTxt3 = pcTxt2;

		while (*pcTxt3 != '\\')
		{
			pcTxt3++;
		}

		*pcTxt = 0;
		strcat(pcTxt, Project.localisationpath);
		strcat(pcTxt, "\\");
		strcat(pcTxt, pcTxt3 + 1);

		free((void *)pcTxt2);
	}
}

/*-----------------------------------------------------------*/
int AddSoundToList(char * dir, char * name, int id, int pos)
{
	C_SOUND * cs;
	int		num;

	if ((num = ExistSound(dir, name)) >= 0)
	{
		return num;
	}

	if (id >= 0)
	{
		cs = &TabSound[id];

		if (!cs->actif || cs->load) return -1;

		free((void *)cs->name);
		free((void *)cs->dir);
		NbSound--;
	}
	else
	{
		cs = GetFreeSound(&num);

		if (!cs) return -1;
	}

	cs->dir = (char *)malloc(strlen(dir) + 1);

	if (!cs->dir) return -1;

	strcpy(cs->dir, dir);

	cs->name = (char *)malloc(strlen(name) + 1);

	if (!cs->name)
	{
		free((void *)cs->dir);
		return -1;
	}

	strcpy(cs->name, name);

	strcpy(AllTxt, "\\\\Arkaneserver\\public\\ARX\\");
	CutAndAddString(dir, "sfx");
	strcat(AllTxt, name);
	PatchReplace();

	strupr(AllTxt);

	if (strstr(AllTxt, "SFX"))
	{
		cs->sound = strdup(AllTxt);
	}
	else
	{
		char szTemp[1024];
		ZeroMemory(szTemp, 1024);

		sprintf(szTemp, "%sspeech\\%s\\%s", Project.workingdir, Project.localisationpath, name);
		cs->sound = strdup(szTemp);
	}

	cs->load = 1;



	int iActif = 1 | LSoundChoose;
	ARX_CHECK_SHORT(iActif);

	cs->actif = ARX_CLEAN_WARN_CAST_SHORT(iActif);


	NbSound++;
	return num;
}
/*-----------------------------------------------------------*/
BOOL PlaySoundKeyFramer(int id)
{
	C_SOUND * cs;

	cs = &TabSound[id];

	if (!cs->actif) return FALSE;

	cs->idhandle = ARX_SOUND_PlayCinematic(cs->sound + g_pak_workdir_len);

	return TRUE;
}
/*-----------------------------------------------------------*/
void StopSoundKeyFramer(void)
{
	C_SOUND	*	ts;
	int			nb;

	ts = TabSound;
	nb = MAX_SOUND;

	while (nb)
	{
		if (ts->actif)
		{
			ARX_SOUND_Stop(ts->idhandle);
			ts->idhandle = ARX_SOUND_INVALID_RESOURCE;
		}

		ts++;
		nb--;
	}
}
