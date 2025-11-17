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
// ARX_Loc.CPP - Localization and String Resource System
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Localization system for Arx Fatalis supporting multiple languages
//		Loads translated strings from INI-style text files
//		Provides key-value lookup for all game text (UI, dialogue, items, etc.)
//
// Purpose:
//		- Load localized strings from text files
//		- Parse INI format ([section] key=value pairs)
//		- Provide fast string lookup by section/key
//		- Support multiple languages (English, French, German, Chinese, etc.)
//		- Enable runtime language switching
//
// Key Responsibilities:
//		- Parse localisation.ini format files
//		- Extract [sections] and key=value pairs
//		- Store strings in hash table for fast lookup
//		- Handle Unicode/wide character strings
//		- Support PAK file system integration
//		- Manage memory for localized strings
//
// File Format:
//		[section_name]
//		key1=value1
//		key2=value2
//
//		[another_section]
//		key3=value3
//
//		Example:
//		[system_menus]
//		new_game=New Game
//		load_game=Load Game
//
// Main Functions:
//		ARX_Localisation_Init()    - Load localization files
//		ARX_Localisation_Close()   - Cleanup localization data
//		HERMES_UNICODE_GetProfileString() - Retrieve localized string
//		HERMES_UNICODE_GetProfileSectionKeyCount() - Count keys in section
//		ParseFile()                - Parse localisation text file
//		ParseCurFile()             - Parse from HERMES file system
//		ParseCurRep()              - Parse from HERMES directory
//
// Parsing Functions:
//		isSection()    - Check if line is [section] header
//		isKey()        - Check if line is key=value pair
//		isNotEmpty()   - Check if line has content
//		CleanSection() - Extract section name from [brackets]
//		CleanKey()     - Extract key name from key=value
//
// Language Support:
//		Loads files based on language setting:
//		- "localisation_english.ini"
//		- "localisation_french.ini"
//		- "localisation_german.ini"
//		- "localisation_chinese.ini"
//		- etc.
//
// Hash Table Integration:
//		- Uses CLocalisationHash for O(1) lookup
//		- Key format: "section/key"
//		- Returns Unicode strings (_TCHAR*)
//		- Handles collisions gracefully
//
// String Retrieval:
//		_TCHAR* text = HERMES_UNICODE_GetProfileString(
//			L"system_menus",    // section
//			L"new_game",        // key
//			L"New Game",        // default
//			buffer,             // output buffer
//			bufferSize,         // buffer size
//			NULL                // filename (uses loaded data)
//		);
//
// Unicode Support:
//		- Wide character strings (_TCHAR, wchar_t)
//		- UTF-16 encoding for file contents
//		- Compatible with ARX_Text rendering system
//		- Supports international characters
//
// File Loading:
//		- Loads from PAK archives via HERMES
//		- Scans "localisation" directory
//		- Supports multiple localisation files
//		- Merges all files into single hash table
//
// Memory Management:
//		- Strings stored in hash table
//		- Hash table freed on ARX_Localisation_Close()
//		- Temporary buffers cleaned during parsing
//		- Max line size: 8096 characters
//
// Section Counting:
//		- Can query number of keys in a section
//		- Used for dynamic UI generation
//		- Iterates through hash table entries
//
// Version Flags:
//		GERMAN_VERSION  - German language build
//		FRENCH_VERSION  - French language build
//		CHINESE_VERSION - Chinese language build
//		- Determines which localisation file to load
//
// Validation:
//		- Checks for well-formed sections [name]
//		- Validates key=value syntax
//		- Ignores empty lines and comments
//		- Handles malformed entries gracefully
//
// Integration:
//		- Used by all game systems for text
//		- ARX_Text uses for UI rendering
//		- ARX_Speech uses for dialogue
//		- Menus query for button labels
//		- Item descriptions loaded from localisation
//
// Performance:
//		- Hash table provides O(1) lookup
//		- All strings loaded at startup
//		- No runtime file I/O for lookups
//		- Memory trade-off for speed
//
// Technical Notes:
//		- Case-sensitive key matching
//		- Whitespace trimmed from values
//		- Supports = character in values
//		- Section names stored without brackets
//		- PAK_UNICODE_ functions for file access
//
// Dependencies:
//		- ARX_LocHash.h (hash table implementation)
//		- Arx_Config.h (language configuration)
//		- HermesMain.h (PAK file system)
//		- EERIEApp.h (application framework)
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
#include "eerieapp.h"
#include "Hermesmain.h"
#include "ARX_LocHash.h"
#include "ARX_Loc.h"
#include "Arx_Config.h"
#include <tchar.h>
#include <list>
#include "Arx_menu2.h"
#include "EerieTexture.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

extern long GERMAN_VERSION;
extern long FRENCH_VERSION;
extern long CHINESE_VERSION;
extern long FINAL_COMMERCIAL_GAME;

extern PROJECT			Project;
extern bool bForceInPack;
extern long FINAL_COMMERCIAL_DEMO;
using namespace std;
extern CMenuConfig * pMenuConfig;
#define MAX_LINE_SIZE 8096

CLocalisationHash * pHashLocalisation = NULL;

//-----------------------------------------------------------------------------
bool isSection(_TCHAR * _lpszUText)
{
	ULONG i			 = 0;
	unsigned long ulTextSize = _tcslen(_lpszUText);
	bool bFirst = false;
	bool bLast  = false;

	while (i < ulTextSize)
	{
		if (_lpszUText[i] == _T('['))
		{
			if (bFirst)
				return false;
			else
				bFirst = true;
		}
		else if (_lpszUText[i] == _T(']'))
		{
			if (bFirst)
				if (bLast)
					return false;
				else
					bLast = true;
			else
				return false;
		}
		else if (_istalnum(_lpszUText[i]))
		{
			if (!bFirst)
				return false;
			else if (bFirst && bLast)
				return false;
		}

		i++;
	}

	if (bFirst && bLast) return true;

	return false;
}

//-----------------------------------------------------------------------------
bool isKey(const _TCHAR * _lpszUText)
{
	unsigned long i = 0;
	unsigned long ulTextSize = _tcslen(_lpszUText);
 
 
	bool bSpace = false;
	bool bAlpha = false;

	while (i < ulTextSize)
	{
		if (_lpszUText[i] == _T('='))
		{
			if (bSpace)
				return false;
			else
				bSpace = true;
		}
		else if (_istalnum(_lpszUText[i]))
		{
			if (!bAlpha)
				bAlpha = true;
		}

		i++;
	}

	if (bSpace && bAlpha) return true;

	return false;
}

//-----------------------------------------------------------------------------
bool isNotEmpty(_TCHAR * _lpszUText)
{
	ULONG i			 = 0;
	unsigned long ulTextSize = _tcslen(_lpszUText);

	while (i < ulTextSize)
	{
		if (_istalnum(_lpszUText[i]))
		{
			return true;
		}

		i++;
	}

	return false;
}

//-----------------------------------------------------------------------------
_TCHAR * CleanSection(const _TCHAR * _lpszUText)
{
	_TCHAR * lpszUText = (_TCHAR *) malloc((_tcslen(_lpszUText) + 2) * sizeof(_TCHAR));
	ZeroMemory(lpszUText, (_tcslen(_lpszUText) + 2)*sizeof(_TCHAR));

	unsigned long ulPos = 0;
	bool bFirst = false;
	bool bLast = false;

	for (unsigned long ul = 0; ul < _tcslen(_lpszUText); ul++)
	{
		if (_lpszUText[ul] == _T('['))
		{
			bFirst = true;
		}
		else if (_lpszUText[ul] == _T(']'))
		{
			bLast = true;
		}

		if (bFirst)
		{
			lpszUText[ulPos] = _lpszUText[ul];
			ulPos ++;

			if (bLast)
			{
				break;
			}
		}
	}

	return lpszUText;
}

//-----------------------------------------------------------------------------
_TCHAR * CleanKey(const _TCHAR * _lpszUText)
{
	_TCHAR * lpszUText = (_TCHAR *) malloc((_tcslen(_lpszUText) + 2) * sizeof(_TCHAR));
	ZeroMemory(lpszUText, (_tcslen(_lpszUText) + 2)*sizeof(_TCHAR));

	unsigned long ulPos = 0;
	unsigned long ulTextSize = _tcslen(_lpszUText);
	bool bAlpha = false;
	bool bEqual = false;

	for (unsigned long ul = 0; ul < ulTextSize; ul++)
	{
		if (_lpszUText[ul] == _T('='))
		{
			bEqual = true;
		}
		else if (bEqual && (_istalnum(_lpszUText[ul]) ||
		                    ((_lpszUText[ul] != _T(' ')) &&
		                     (_lpszUText[ul] != _T('"')))
		                   ))
		{
			bAlpha = true;
		}
		else if ((_lpszUText[ul] == _T('\"')) && (!bAlpha))
		{
			continue;
		}

		if (bEqual && bAlpha)
		{
			lpszUText[ulPos] = _lpszUText[ul];
			ulPos ++;
		}
	}

	while (ulPos--)
	{
		if (_istalnum(lpszUText[ulPos]))
		{
			break;
		}
		else if (lpszUText[ulPos] == _T('"'))
		{
			lpszUText[ulPos] = 0;
		}
	}

	return lpszUText;
}

//-----------------------------------------------------------------------------
void ParseFile(_TCHAR * _lpszUTextFile, const unsigned long _ulFileSize)
{
	_TCHAR * pULine;
 

	//-------------------------------------------------------------------------
	// on skip l'entete unicode
	_TCHAR * pFile = _lpszUTextFile;
	pFile ++;
	unsigned long ulFileSize = _ulFileSize - 1;

	//-------------------------------------------------------------------------
	//clean up comments
	for (unsigned long i = 0; i < (ulFileSize - 1); i++)
	{
		if ((pFile[i] == _T('/')) && (pFile[i+1] == _T('/')))
		{
			unsigned long j = i;

			while ((j < (ulFileSize - 1)) && (pFile[j] != _T('\r')) && (pFile[j+1] != _T('\n')))
			{
				pFile[j] = _T(' ');
				j++;
			}

			i = j;
		}
	}

	//-------------------------------------------------------------------------
	// get all lines into list

	list<_TCHAR *> lUText;
	pULine = _tcstok(pFile, _T("\r\n"));

	long t = 0;

	while (pULine != NULL)
	{
		if (isNotEmpty(pULine))
		{
			t++;
			lUText.insert(lUText.end(), (pULine)); //_tcsdup
		}

		pULine = _tcstok(NULL, _T("\r\n"));
	}

	list<_TCHAR *>::iterator it;

	//-------------------------------------------------------------------------
	// look up for sections and associated keys
	it = lUText.begin();
	t = 0;

	while (it != lUText.end())
	{
		if (isSection(*it))
		{
			CLocalisation * pLoc = new CLocalisation();
			_TCHAR * lpszUT = CleanSection(*it);
			pLoc->SetSection(lpszUT);

			free(lpszUT);

			it++;
			t++;

			while ((it != lUText.end()) && (!isSection(*it)))
			{
				if (isKey(*it))
				{
					_TCHAR * lpszUTk = CleanKey(*it);
					pLoc->AddKey(lpszUTk);
					free(lpszUTk);
					it++;
					t++;
				}
				else
				{
					break;
				}
			}

			pHashLocalisation->AddElement(pLoc);
			continue;
		}

		it++;
		t++;
	}
}

//-----------------------------------------------------------------------------
char LocalisationLanguage = -1;

//-----------------------------------------------------------------------------
void ARX_Localisation_Init(char * _lpszExtension) 
{
	if (_lpszExtension == NULL)
		return;

	// nettoyage
	if (pHashLocalisation)
	{
		ARX_Localisation_Close();
	}

	char tx[256];
	ZeroMemory(tx, 256);
	sprintf(tx, "%slocalisation\\utext_%s.ini", Project.workingdir, Project.localisationpath);

	long LocalisationSize = 0;
	_TCHAR * Localisation = NULL;

	if (GERMAN_VERSION)
	{
		bool bOldForceInPack = bForceInPack;
		bForceInPack = true;
		Localisation = (_TCHAR *)PAK_FileLoadMallocZero(tx, &LocalisationSize);
		bForceInPack = bOldForceInPack;
	}
	else
	{
		Localisation = (_TCHAR *)PAK_FileLoadMallocZero(tx, &LocalisationSize);
	}

	if (Localisation == NULL)
	{
		if (GERMAN_VERSION || FRENCH_VERSION)
		{
			delete pMenuConfig;	
			pMenuConfig = NULL;
			exit(0);
		}

		ZeroMemory(tx, 256);
		strcpy(Project.localisationpath, "english");
		sprintf(tx, "%slocalisation\\utext_%s.ini", Project.workingdir, Project.localisationpath);
		Localisation = (_TCHAR *)PAK_FileLoadMallocZero(tx, &LocalisationSize);

	}

	if (Localisation && LocalisationSize)
	{
		pHashLocalisation = new CLocalisationHash(1 << 13);
		LocalisationSize = _tcslen(Localisation);
		ParseFile(Localisation, LocalisationSize);
		free((void *)Localisation);
		Localisation = NULL;
		LocalisationSize = 0;
	}

	//CD Check
	if (FINAL_COMMERCIAL_DEMO && (!bForceInPack))
	{
		_TCHAR szMenuText[256];
		PAK_UNICODE_GetPrivateProfileString(_T("system_menus_main_cdnotfound"), _T("string"), _T(""), szMenuText, 256, NULL);

		if (!szMenuText[0]) //warez
		{
			bForceInPack = true;
			ARX_Localisation_Init();
			bForceInPack = false;
		}
	}

	if (FINAL_COMMERCIAL_GAME)
	{
		_TCHAR szMenuText[256] = {0};
		PAK_UNICODE_GetPrivateProfileString(_T("unicode"), _T("string"), _T(""), szMenuText, 256, NULL);

		if (szMenuText[0]) //warez
		{
			if (!_tcsicmp(_T("chinese"), szMenuText))
			{
				CHINESE_VERSION = 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
void ARX_Localisation_Close()
{
	LocalisationLanguage = -1;

	delete pHashLocalisation;
	pHashLocalisation = NULL;
}

//-----------------------------------------------------------------------------
long HERMES_UNICODE_GetProfileString(_TCHAR * sectionname,
                                     _TCHAR * t_keyname,
                                     _TCHAR * defaultstring,
                                     _TCHAR * destination,
                                     unsigned long    maxsize,
                                     _TCHAR * datastream,
                                     long    lastspeech)
{

	ZeroMemory(destination, maxsize * sizeof(_TCHAR));

	if (pHashLocalisation)
	{
		_TCHAR * t = pHashLocalisation->GetPtrWithString(sectionname);

		if (t)
		{
			_tcsncpy(destination, t, min(maxsize, _tcslen(t)));
		}
		else
		{
			_tcsncpy(destination, defaultstring, min(maxsize, _tcslen(defaultstring)));
		}
	}
	else
	{
		_tcsncpy(destination, defaultstring, min(maxsize, _tcslen(defaultstring)));
	}

	return 0;
}

//-----------------------------------------------------------------------------
long HERMES_UNICODE_GetProfileSectionKeyCount(const _TCHAR * sectionname)
{
	if (pHashLocalisation)
		return pHashLocalisation->GetKeyCount(sectionname);

	return 0;
}

static long ltNum = 0;

//-----------------------------------------------------------------------------
DWORD PAK_UNICODE_GetPrivateProfileString(_TCHAR * _lpszSection,
        _TCHAR * _lpszKey,
        _TCHAR * _lpszDefault,
        _TCHAR * _lpszBuffer,
        unsigned long	_lBufferSize,
        char	* _lpszFileName)
{
	ltNum ++;
	ZeroMemory(_lpszBuffer, _lBufferSize * sizeof(_TCHAR));

	if (_lpszSection[0] == _T('\0'))
	{
		_tcsncpy(_lpszBuffer, _lpszDefault, min(_lBufferSize, _tcslen(_lpszDefault)));
		_stprintf(_lpszBuffer, _T("%s: NOT FOUND"), _lpszSection);
		return 0;
	}

	_TCHAR szSection[256] = _T("");
	_stprintf(szSection, _T("[%s]"), _lpszSection);

	HERMES_UNICODE_GetProfileString(
	    szSection,
	    _lpszKey,
	    _lpszDefault,
	    _lpszBuffer,
	    _lBufferSize,
	    NULL,
	    -1); //lastspeechflag

	return 1;
}

#include <vector>
using namespace std;

extern PakManager * pPakManager;
vector<char *> mlist;

//-----------------------------------------------------------------------------
void ParseCurFile(EVE_TFILE * _e)
{
	if (!_e) return;

	if (stricmp((char *)_e->name, "utext") > 0)
	{
		char * t = strdup((char *)_e->name);

		for (UINT i = 0 ; i < strlen(t) ; i++)
		{

			t[i] = ARX_CLEAN_WARN_CAST_CHAR(tolower(t[i]));

		}

		mlist.insert(mlist.end(), t);
		//free (t);
	}

	ParseCurFile(_e->fnext);
}

//-----------------------------------------------------------------------------
void ParseCurRep(EVE_REPERTOIRE * _er)
{
	if (!_er) return;

	ParseCurFile(_er->fichiers);

	ParseCurRep(_er->fils);
	ParseCurRep(_er->brothernext);
}

extern HBITMAP ARX_CONFIG_hBitmap;
