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
// HERMES_hachage.cpp - String Hash Table Implementation
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Hash table for fast string-to-pointer lookups using double hashing.
//		Used by HERMES for quick resource name resolution in PAK files.
//
// Purpose:
//		Provides O(1) average-case lookup for resource names (filenames).
//		Maps resource name strings to memory pointers (resource data/metadata).
//		Critical for performance when loading resources from PAK archives.
//
// Algorithm:
//		- Double hashing collision resolution (H1 primary, H2 secondary)
//		- Automatic table doubling when 75% full
//		- Case-insensitive string keys (converted to lowercase)
//		- Power-of-2 table sizes for fast modulo via bitwise AND
//
// Key Features:
//		- Dynamic resizing (doubles when 75% full)
//		- Collision tracking for performance analysis
//		- Case-insensitive lookups (all keys lowercased)
//		- Efficient double hashing probe sequence
//
// Hash Functions:
//		H1(key) = key (direct mapping)
//		H2(key) = (key >> 1) | 1 (always odd for better distribution)
//		Key = sum of (char * position * length) for each character
//
// Code: Mickael Pointier/Cyril Meynier
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include "HERMES_hachage.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//=============================================================================
// FUNCTION: CHachageString::CHachageString (Constructor)
//=============================================================================
// Description:
//		Constructs hash table with specified initial size.
//
// Parameters:
//		_iSize - Initial table size (should be power of 2 for optimal performance)
//
// Algorithm:
//		1. Allocate hash table array
//		2. Initialize all slots to NULL (empty)
//		3. Set size and bitmask for fast modulo
//		4. Initialize collision/fill counters
//
// Notes:
//		- Table size should be power of 2 for fast modulo via (hash & iMask)
//		- All entries initialized to NULL (lpszName = NULL indicates empty slot)
//		- iMask = iSize - 1 allows fast modulo: (hash % iSize) == (hash & iMask)
//
//=============================================================================
CHachageString::CHachageString(int _iSize)
{
	tTab = (T_HACHAGE_DATAS *)malloc(_iSize * sizeof(T_HACHAGE_DATAS));
	iNbCollisions = iNbNoInsert = 0;

	iSize = _iSize;
	iFill = 0;

	while (_iSize--)
	{
		tTab[_iSize].lpszName = NULL;
	}

	iMask = iSize - 1;
}

//=============================================================================
// FUNCTION: CHachageString::~CHachageString (Destructor)
//=============================================================================
// Description:
//		Destroys hash table and frees all allocated memory.
//
// Algorithm:
//		1. Free all stored string keys (strdup allocations)
//		2. Free hash table array itself
//
// Notes:
//		- Must free each lpszName string individually (allocated via strdup)
//		- pMem pointers NOT freed (caller owns pointed-to data)
//
//=============================================================================
CHachageString::~CHachageString()
{
	while (iSize--)
	{
		if (tTab[iSize].lpszName)
		{
			free((void *)tTab[iSize].lpszName);
			tTab[iSize].lpszName = NULL;
		}
	}

	free((void *)tTab);
}

//=============================================================================
// FUNCTION: CHachageString::AddString
//=============================================================================
// Description:
//		Adds a string-to-pointer mapping to the hash table.
//
// Parameters:
//		_lpszText - String key (filename/resource name) - converted to lowercase
//		_pMem - Pointer value to associate with string
//
// Returns:
//		true - String added successfully
//		false - Table full, could not add (after all probe attempts exhausted)
//
// Algorithm:
//		1. Convert string to lowercase for case-insensitive lookup
//		2. Check if table 75% full - if so, double size and rehash (TO DO)
//		3. Compute hash key from string
//		4. Compute H1 (primary hash) and H2 (secondary hash for probing)
//		5. Probe table using double hashing: pos = (H1 + i*H2) & iMask
//		6. Find first empty slot and insert string/pointer pair
//		7. Track collisions for performance analysis
//
// Notes:
//		- Strings duplicated via strdup (caller can free original)
//		- Lowercase conversion ensures case-insensitive lookups
//		- H2 is always odd to ensure full table coverage
//		- Table doubling not fully implemented (marked TO DO)
//		- If table fills completely, returns false
//
//=============================================================================
bool CHachageString::AddString(char * _lpszText, void * _pMem)
{
	char * lpszTextLow = _strlwr(_lpszText);

	if (iFill >= iSize * 0.75)
	{
		//TO DO: recr�e toute la table!!!!
		iSize <<= 1;
		iMask = iSize - 1;
		tTab = (T_HACHAGE_DATAS *)realloc(tTab, iSize * sizeof(T_HACHAGE_DATAS));
	}

	int iKey = GetKey(lpszTextLow);
	int	iH1 = FuncH1(iKey);
	int	iH2 = FuncH2(iKey);

	int	iNbSolution = 0;

	while (iNbSolution < iSize)
	{
		iH1 &= iMask;

		if (!tTab[iH1].lpszName)
		{
			tTab[iH1].lpszName = strdup(lpszTextLow);
			tTab[iH1].pMem = _pMem;
			iFill++;
			return true;
		}

		iNbCollisions++;
		iH1 += iH2;

		iNbSolution++;
	}

	iNbNoInsert++;
	return false;
}

//=============================================================================
// FUNCTION: CHachageString::GetPtrWithString
//=============================================================================
// Description:
//		Looks up a string in the hash table and returns associated pointer.
//
// Parameters:
//		_lpszText - String key to search for (converted to lowercase)
//
// Returns:
//		void* - Pointer associated with string if found
//		NULL - String not in table
//
// Algorithm:
//		1. Convert string to lowercase for case-insensitive search
//		2. Compute hash key from string
//		3. Compute H1 and H2 hash functions
//		4. Probe table using same double hashing sequence as AddString
//		5. Compare string at each probed slot
//		6. Return pointer if string matches, NULL if not found
//
// Notes:
//		- Uses same probing sequence as AddString for consistency
//		- Case-insensitive comparison via lowercase conversion
//		- Returns NULL if string not found after probing entire table
//
//=============================================================================
void * CHachageString::GetPtrWithString(char * _lpszText)
{
	char * lpszTextLow = _strlwr(_lpszText);

	int iKey = GetKey(lpszTextLow);
	int	iH1 = FuncH1(iKey);
	int	iH2 = FuncH2(iKey);

	int	iNbSolution = 0;

	while (iNbSolution < iSize)
	{
		iH1 &= iMask;

		if (tTab[iH1].lpszName)
		{
			if (!strcmp((const char *)lpszTextLow, (const char *)tTab[iH1].lpszName))
			{
				return tTab[iH1].pMem;
			}
		}

		iH1 += iH2;
		iNbSolution++;
	}

	return NULL;
}

//=============================================================================
// FUNCTION: CHachageString::FuncH1
//=============================================================================
// Description:
//		Primary hash function (H1) - direct key mapping.
//
// Parameters:
//		_iKey - Integer key computed from string
//
// Returns:
//		Primary hash value (same as key)
//
// Notes:
//		- Simple identity function
//		- Used as starting position for probing
//		- Combined with iMask for table index: (H1 & iMask)
//
//=============================================================================
int CHachageString::FuncH1(int _iKey)
{
	return _iKey;
}

//=============================================================================
// FUNCTION: CHachageString::FuncH2
//=============================================================================
// Description:
//		Secondary hash function (H2) - computes probe step size.
//
// Parameters:
//		_iKey - Integer key computed from string
//
// Returns:
//		Secondary hash value (always odd number)
//
// Algorithm:
//		Returns (key >> 1) | 1, which:
//		- Right-shifts key by 1 (divides by 2)
//		- ORs with 1 to ensure result is always odd
//
// Notes:
//		- Odd step size ensures full table coverage (coprime with power-of-2 size)
//		- Prevents probe cycles that would skip table entries
//		- Critical for double hashing correctness
//
//=============================================================================
int	CHachageString::FuncH2(int _iKey)
{
	return ((_iKey >> 1) | 1);
}

//=============================================================================
// FUNCTION: CHachageString::GetKey
//=============================================================================
// Description:
//		Computes integer hash key from string.
//
// Parameters:
//		_lpszText - String to hash (should be lowercase for consistency)
//
// Returns:
//		Integer hash key
//
// Algorithm:
//		For each character at position i:
//			key += char * (i+1) + char * length
//		This weights characters by both position and string length
//
// Notes:
//		- Position weighting ensures anagrams have different hashes
//		- Length weighting provides additional distribution
//		- Not cryptographically secure (not needed for hash table)
//		- Optimized for speed over collision resistance
//
//=============================================================================
int	CHachageString::GetKey(char * _lpszText)
{
	int iKey = 0;
	int iLenght = strlen((const char *)_lpszText);
	int iLenght2 = iLenght;

	while (iLenght--)
	{
		iKey += _lpszText[iLenght] * (iLenght + 1) + _lpszText[iLenght] * iLenght2;
	}

	return iKey;
}
