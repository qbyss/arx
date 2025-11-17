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
/*
           _.               _.    .    J)
        HNMMMM) .      L HNMMMM)  #H   H)     ()
       (MMMMMM) M)    (M(MMMMMM)  `H)._UL_   JM)
       `MF" `4) M)    NN`MF" `4)    JMMMMMM#.4F
        M       #)    M) M         (MF (0DANN.
        M       (M   .M  M  .NHH#L (N JMMMN.H#
        M       (M   (M  M   4##HF QQ(MF`"H#(M.
        M       `M   (N  M         #U(H(N (MJM)
       .M  __L   M)  M) .M  ___    4)U#NF (M0M)
       (MMMMMM   M)  M) (MMMMMM)     U#NHLJM(M`
       (MMMMN#   4) (M  (MMMMM#  _.  (H`HMM)JN
       (M""      (M (M  (M""   (MM)  (ML____M)
       (M        (M H)  (M     `"`  ..4MMMMM# D
       (M        `M M)  (M         JM)  """`  N#
       (M         MLM`  (M        #MF   (L    `F
       (M         MMM   (M        H`    (#
       (M         (MN   (M              (Q
       `M####H    (M)   `M####H         ``
        MMMMMM    `M`    NMMMMM
        `Q###F     "     `4###F   sebastien scieux @2001

*/
//////////////////////////////////////////////////////////////////////////////////////
// HERMES_pack_public.cpp - PAK Archive Reader with Compression & Encryption
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Low-level PAK archive reader implementing file extraction, decompression,
//		and decryption for Arx Fatalis resource packs.
//
// Purpose:
//		Provides complete PAK archive reading functionality:
//		- Loading and parsing PAK files
//		- Decrypting encrypted FAT (File Allocation Table)
//		- Decompressing files using PKZIP-style "explode" algorithm
//		- File I/O operations on PAK-contained files
//		- Extraction of PAK contents to disk
//
// Architecture:
//
//		EVE_LOADPACK - Single PAK Archive Reader
//			Represents one loaded PAK file
//			Contains: file handle, directory tree, FAT, encryption state
//			Provides: Open/Close, Read/ReadAlloc, file I/O operations
//
//		PACK_FILE - Virtual File Handle
//			Simulates FILE* for files inside PAK archives
//			Contains: file metadata pointer, current offset, active flag, ID
//			Allows transparent file I/O on archived files
//
//		Encryption System:
//			- XOR-based encryption with rotating key position
//			- Different keys for commercial game, demo, and debug builds
//			- Encrypts FAT (directory structure) but not always file data
//			- Per-character, per-short, per-int encryption functions
//
//		Compression:
//			- Uses PKZIP "explode" algorithm (implode compression format)
//			- param2 & PAK flag indicates if file is compressed
//			- Decompresses on-the-fly during Read operations
//			- Callback-based decompression (ReadData/WriteData callbacks)
//
// File Format:
//		[Header - 4 bytes] - Offset to FAT
//		[Data Blocks] - Compressed/uncompressed file data
//		[FAT - Variable Size]
//			- FAT size (4 bytes)
//			- For each directory:
//				* Directory path (encrypted null-terminated string)
//				* File count (encrypted int)
//				* For each file:
//					+ Filename (encrypted null-terminated string)
//					+ param - File offset in archive (encrypted int)
//					+ param2 - Flags (PAK bit = compressed) (encrypted int)
//					+ param3 - Uncompressed size (encrypted int)
//					+ taille - Compressed size (encrypted int)
//
// FAT Encryption Keys:
//		Commercial Game: "AVQF3FCKE50GRIAYXJP2AMEYO5QGA0JGIIH2NHBTVOA1VOGGU5H3GSSIARKPRQPQKKYEOIAQG1XRX0J4F5OEAEFI4DD3LL45VJTVOA1VOGGUKE50GRIAYX"
//		Commercial Demo: "NSIARKPRQPHBTE50GRIH3AYXJP2AMF3FCEYAVQO5QGA0JGIIH2AYXKVOA1VOGGU5GSQKKYEOIAQG1XRX0J4F5OEAEFI4DD3LL45VJTVOA1VOGGUKE50GRI"
//		Debug Build: "" (no encryption)
//
// Operations:
//		Open() - Load PAK, parse encrypted FAT, build directory tree
//		Close() - Close PAK file and free resources
//		Read() - Read file from PAK into pre-allocated buffer
//		ReadAlloc() - Read file and allocate buffer automatically
//		GetSize() - Get decompressed file size
//		fOpen/fClose/fRead/fSeek/fTell() - Standard file I/O on PAK files
//		WriteSousRepertoire() - Extract directory tree to disk (debug)
//
// Performance:
//		- Hash tables for O(1) file lookup within directories
//		- Seek caching (iSeekPak) avoids redundant seeks
//		- Stateful file handles (PACK_FILE array) for concurrent access
//		- Decompression buffer reuse (pcWorkBuff global)
//
// Encryption Algorithm:
//		XOR with rotating key:
//			encrypted_byte = (plaintext_byte XOR key[key_position]) >> shift
//			key_position = (key_position + 1) % key_length
//		Applied to: FAT strings, integers, directory structure
//		NOT applied to: File data itself (files stored compressed but not encrypted)
//
// Compression:
//		- PKZIP "explode" algorithm (predecessor to deflate)
//		- Files marked with PAK flag in param2 are compressed
//		- Decompression uses callback functions for streaming
//		- Supports partial reads from compressed files (complex buffering)
//
// Code: Sébastien Scieux @2001
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include "HERMES_pack_public.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define FINAL_COMMERCIAL_GAME		// Commercial release encryption key
//#define FINAL_COMMERCIAL_DEMO		// Demo version encryption key

//#############################################################################
//#############################################################################
//                             EVE_LOADPACK Class
//#############################################################################
//#############################################################################

//=============================================================================
// FUNCTION: EVE_LOADPACK::EVE_LOADPACK (Constructor)
//=============================================================================
// Description:
//		Constructs PAK archive reader with encryption key initialization.
//
// Algorithm:
//		1. Initialize all pointers to NULL
//		2. Clear PACK_FILE handle array (PACK_MAX_FREAD slots)
//		3. Set encryption key based on build type:
//		   - FINAL_COMMERCIAL_GAME: Full game encryption key
//		   - FINAL_COMMERCIAL_DEMO: Demo version key
//		   - Debug: Empty key (no encryption)
//		4. Initialize encryption key position to 0
//
// Notes:
//		- PACK_FILE array allows up to PACK_MAX_FREAD concurrent open files
//		- Encryption key is 140+ character string for XOR obfuscation
//		- Different keys prevent demo PAKs from working in full game
//
//=============================================================================
EVE_LOADPACK::EVE_LOADPACK()
{
	lpszName = NULL;
	pfFile = NULL;
	pRoot = NULL;
	iSeekPak = 0;

	int iI = PACK_MAX_FREAD;

	while (--iI)
	{
		tPackFile[iI].bActif = false;
		tPackFile[iI].iID = 0;
		tPackFile[iI].iOffset = 0;
	}

	iPassKey = 0;
#ifdef FINAL_COMMERCIAL_GAME
	strcpy((char *)cKey, "AVQF3FCKE50GRIAYXJP2AMEYO5QGA0JGIIH2NHBTVOA1VOGGU5H3GSSIARKPRQPQKKYEOIAQG1XRX0J4F5OEAEFI4DD3LL45VJTVOA1VOGGUKE50GRIAYX");
#else
#ifdef FINAL_COMMERCIAL_DEMO
	strcpy((char *)cKey, "NSIARKPRQPHBTE50GRIH3AYXJP2AMF3FCEYAVQO5QGA0JGIIH2AYXKVOA1VOGGU5GSQKKYEOIAQG1XRX0J4F5OEAEFI4DD3LL45VJTVOA1VOGGUKE50GRI");
#else
	strcpy((char *)cKey, ""); //NO CRYPT
#endif
#endif

	pcFAT = NULL;
}

// EVE_LOADPACK::~EVE_LOADPACK - Destructor
//		Frees PAK name, closes file, deletes directory tree, frees FAT buffer
EVE_LOADPACK::~EVE_LOADPACK()
{
	if (lpszName)
	{
		free((void *)lpszName);
		lpszName = NULL;
	}

	if (pfFile) fclose(pfFile);

	if (pRoot) delete pRoot;

	if (pcFAT)
	{
		free((void *)pcFAT);
		pcFAT = NULL;
	}
}

//#############################################################################
//                    FAT PARSING HELPER FUNCTIONS
//#############################################################################

// ReadFAT_int - Read and decrypt 4-byte integer from FAT buffer
//		Advances pcFAT pointer, decrements iTailleFAT
int EVE_LOADPACK::ReadFAT_int()
{
	int i = *((int *)pcFAT);
	pcFAT += 4;
	iTailleFAT -= 4;

	UnCryptInt((unsigned int *)&i);

	return i;
}

// ReadFAT_string - Read and decrypt null-terminated string from FAT
//		Returns pointer to decrypted string in FAT buffer
//		Advances pcFAT pointer past string
char * EVE_LOADPACK::ReadFAT_string()
{
	char * t = pcFAT;
	int i = UnCryptString((unsigned char *)t) + 1;
	pcFAT += i;
	iTailleFAT -= i;

	return t;
}

//=============================================================================
// FUNCTION: EVE_LOADPACK::Open
//=============================================================================
// Description:
//		Opens PAK archive and parses encrypted FAT to build directory tree.
//
// Parameters:
//		_pcName - Path to PAK file
//
// Returns:
//		true - PAK opened successfully
//		false - Failed to open PAK file
//
// Algorithm:
//		1. Open PAK file in binary read mode
//		2. Read FAT offset from header (first 4 bytes)
//		3. Seek to FAT and read FAT size
//		4. Load entire FAT into memory
//		5. Parse FAT:
//		   a. For each directory:
//		      - Read encrypted directory path
//		      - Add directory to tree
//		      - Read file count
//		      - Create hash table for files (next power-of-2 >= count * 1.33)
//		      - For each file:
//		        * Read encrypted filename
//		        * Read file metadata (offset, flags, sizes)
//		        * Add file to directory
//		6. Reset file pointer to start
//		7. Save PAK filename
//
// Notes:
//		- Entire FAT loaded into memory for fast parsing
//		- Hash tables sized to avoid overload (>75% triggers size doubling)
//		- Encryption key reset to 0 before parsing
//		- Directory tree built using EVE_REPERTOIRE structure
//
//=============================================================================
bool EVE_LOADPACK::Open(char * _pcName)
{
	pfFile = fopen(_pcName, "rb");

	if (!pfFile) return false;

	iPassKey = 0;

	pRoot = new EVE_REPERTOIRE(NULL, NULL);
	fread((void *)&iTailleFAT, 1, 4, pfFile);
	fseek(pfFile, iTailleFAT, SEEK_SET);
	fread((void *)&iTailleFAT, 1, 4, pfFile);		//taille de la FAT

	if (pcFAT)
	{
		free((void *)pcFAT);
		pcFAT = NULL;
	}

	pcFAT = (char *)malloc(iTailleFAT);
	char * pcFATCopy = pcFAT;
	fread((void *)pcFAT, iTailleFAT, 1, pfFile);

	while (iTailleFAT)
	{
		char * pcName = ReadFAT_string();

		EVE_REPERTOIRE * pRepertoire = pRoot;

		if (*pcName != 0)
		{
			pRoot->AddSousRepertoire((unsigned char *)pcName);
			pRepertoire = pRoot->GetSousRepertoire((unsigned char *)pcName);
		}
		else
		{
			pcName = NULL;
		}

		int iNbFiles = ReadFAT_int();

		if ((pRepertoire) &&
		        (iNbFiles) &&
		        !(pRepertoire->pHachage))
		{
			int iNbHache = 1;

			while (iNbHache < iNbFiles) iNbHache <<= 1;

			int iNbHacheTroisQuart = (iNbHache * 3) / 4;

			if (iNbFiles > iNbHacheTroisQuart) iNbHache <<= 1;

			pRepertoire->pHachage = new CHachageString(iNbHache);
		}

		while (iNbFiles--)
		{
			char * pcNameFile = ReadFAT_string();
			EVE_TFILE * pFile = pRoot->AddFileToSousRepertoire((unsigned char *)pcName, (unsigned char *)pcNameFile);
			pFile->param = ReadFAT_int();
			pFile->param2 = ReadFAT_int();
			pFile->param3 = ReadFAT_int();
			pFile->taille = ReadFAT_int();
		}
	}

	pcFAT = pcFATCopy;

	lpszName = strdup((const char *)_pcName);

	fseek(pfFile, 0, SEEK_SET);

	return true;
}

// EVE_LOADPACK::Close - Close PAK file and free directory tree
//		Closes file handle and deletes root directory (cascades to all subdirectories)
void EVE_LOADPACK::Close()
{
	if (pfFile)
	{
		fclose(pfFile);
		pfFile = NULL;
	}

	if (pRoot)
	{
		delete pRoot;
		pRoot = NULL;
	}
}

//#############################################################################
//                    DECOMPRESSION CALLBACK FUNCTIONS
//#############################################################################

// ReadData - Decompression callback: Read compressed data from PAK file
//		Used by explode() algorithm to fetch compressed input
static unsigned int ReadData(char * Buff, unsigned int * Size, void * Param)
{
	PAK_PARAM * pPP = (PAK_PARAM *)Param;

	int iRead = fread(Buff, 1, *Size, pPP->file);

	return (unsigned int)iRead;
}

// WriteData - Decompression callback: Write decompressed data to memory buffer
//		Used by explode() to output decompressed data
static void WriteData(char * Buff, unsigned int * Size, void * Param)
{
	PAK_PARAM * pPP = (PAK_PARAM *) Param;

	ARX_CHECK_NOT_NEG(pPP->lSize);
	long lSize = min(ARX_CAST_ULONG(pPP->lSize), *Size);

	memcpy((void *) pPP->mem, (const void *) Buff, lSize);
	pPP->mem   += lSize;
	pPP->lSize -= lSize;
}

char pcWorkBuff[EXP_BUFFER_SIZE];	// Global decompression work buffer

//#############################################################################
//                    FILE EXTRACTION FUNCTIONS
//#############################################################################

//=============================================================================
// FUNCTION: EVE_LOADPACK::Read
//=============================================================================
// Description:
//		Reads file from PAK into pre-allocated buffer.
//
// Parameters:
//		_pcName - File path within PAK (e.g., "textures\\wall.jpg")
//		_mem - Pre-allocated buffer (must be >= file size)
//
// Returns:
//		true - File read successfully
//		false - File not found or error
//
// Algorithm:
//		1. Parse path into directory and filename components
//		2. Navigate directory tree to find file's directory
//		3. Lookup file in directory's hash table
//		4. Seek to file's offset in PAK
//		5. If compressed (param2 & PAK):
//		   - Use explode() with ReadData/WriteData callbacks
//		6. If uncompressed:
//		   - Direct fread into buffer
//		7. Update seek cache (iSeekPak)
//
// Notes:
//		- Caller must pre-allocate buffer (use GetSize)
//		- Compression detected via PAK flag in param2
//		- Hash table provides O(1) file lookup
//
//=============================================================================
bool EVE_LOADPACK::Read(char * _pcName, void * _mem)
{
	if ((!_pcName) ||
	        (!pRoot)) return false;

	char * pcDir = NULL;
	char * pcDir1 = (char *)EVEF_GetDirName((unsigned char *)_pcName);

	if (pcDir1)
	{
		pcDir = new char[strlen((const char *)pcDir1)+2];
		strcpy((char *)pcDir, (const char *)pcDir1);
		strcat((char *)pcDir, "\\");
		delete [] pcDir1;
	}

	char * pcFile = (char *)EVEF_GetFileName((unsigned char *)_pcName);

	EVE_REPERTOIRE * pDir;

	if (!pcDir)
	{
		pDir = pRoot;
	}
	else
	{
		pDir = pRoot->GetSousRepertoire((unsigned char *)pcDir);
	}

	if (!pDir)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return NULL;
	}

	if (!pDir->nbfiles)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return false;
	}

	EVE_TFILE * pTFiles = (EVE_TFILE *)pDir->pHachage->GetPtrWithString((char *)pcFile);

	if (pTFiles)
	{
		fseek(pfFile, pTFiles->param - iSeekPak, SEEK_CUR);

		if (pTFiles->param2 & PAK)
		{
			PAK_PARAM sPP;
			sPP.file = pfFile;
			sPP.mem = (char *)_mem;
			sPP.lSize = pTFiles->param3;
			explode(ReadData, WriteData, pcWorkBuff, &sPP);
		}
		else
		{
			fread(_mem, 1, pTFiles->taille, pfFile);
		}

		iSeekPak = ftell(pfFile);

		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return true;
	}

	if (pcDir) delete [] pcDir;

	if (pcFile) delete [] pcFile;

	return false;
}

// EVE_LOADPACK::ReadAlloc - Read file from PAK with automatic buffer allocation
//		Same as Read() but allocates buffer sized to file's decompressed size
//		Returns buffer pointer and size via _piTaille parameter
void * EVE_LOADPACK::ReadAlloc(char * _pcName, int * _piTaille)
{
	if ((!_pcName) ||
	        (!pRoot)) return NULL;

	char * pcDir = NULL;
	char * pcDir1 = (char *)EVEF_GetDirName((unsigned char *)_pcName);

	if (pcDir1)
	{
		pcDir = new char[strlen((const char *)pcDir1)+2];
		strcpy((char *)pcDir, (const char *)pcDir1);
		strcat((char *)pcDir, "\\");
		delete [] pcDir1;
	}

	char * pcFile = (char *)EVEF_GetFileName((unsigned char *)_pcName);

	EVE_REPERTOIRE * pDir;

	if (!pcDir)
	{
		pDir = pRoot;
	}
	else
	{
		pDir = pRoot->GetSousRepertoire((unsigned char *)pcDir);
	}

	if (!pDir)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return NULL;
	}

	if (!pDir->nbfiles)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return false;
	}

	EVE_TFILE * pTFiles = (EVE_TFILE *)pDir->pHachage->GetPtrWithString((char *)pcFile);

	if (pTFiles)
	{
		void * mem;
		fseek(pfFile, pTFiles->param - iSeekPak, SEEK_CUR);

		if (pTFiles->param2 & PAK)
		{
			mem = malloc(pTFiles->param3);
			*_piTaille = (int)pTFiles->param3;

			if (!mem)
			{
				if (pcDir) delete [] pcDir;

				if (pcFile) delete [] pcFile;

				return NULL;
			}

			PAK_PARAM sPP;
			sPP.file = pfFile;
			sPP.mem = (char *)mem;
			sPP.lSize = pTFiles->param3;
			explode(ReadData, WriteData, pcWorkBuff, &sPP);
		}
		else
		{
			mem = malloc(pTFiles->taille);
			*_piTaille = (int)pTFiles->taille;

			if (!mem)
			{
				if (pcDir) delete [] pcDir;

				if (pcFile) delete [] pcFile;

				return NULL;
			}

			fread(mem, 1, pTFiles->taille, pfFile);
		}

		iSeekPak = ftell(pfFile);

		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return mem;
	}

	if (pcDir) delete [] pcDir;

	if (pcFile) delete [] pcFile;

	return NULL;
}

//=============================================================================
// FUNCTION: EVE_LOADPACK::GetSize
//=============================================================================
// Description:
//		Gets decompressed size of file in PAK.
//
// Returns:
//		Decompressed file size in bytes
//		-1 if file not found
//
// Notes:
//		- Returns param3 (decompressed size) if compressed
//		- Returns taille (actual size) if uncompressed
//		- Does NOT read file data, just metadata
//
//=============================================================================
int EVE_LOADPACK::GetSize(char * _pcName)
{
	if ((!_pcName) ||
	        (!pRoot)) return -1;

	char * pcDir = NULL;
	char * pcDir1 = (char *)EVEF_GetDirName((unsigned char *)_pcName);

	if (pcDir1)
	{
		pcDir = new char[strlen((const char *)pcDir1)+2];
		strcpy((char *)pcDir, (const char *)pcDir1);
		strcat((char *)pcDir, "\\");
		delete [] pcDir1;
	}

	char * pcFile = (char *)EVEF_GetFileName((unsigned char *)_pcName);

	EVE_REPERTOIRE * pDir;

	if (!pcDir)
	{
		pDir = pRoot;
	}
	else
	{
		pDir = pRoot->GetSousRepertoire((unsigned char *)pcDir);
	}

	if (!pDir)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return -1;
	}

	if (!pDir->nbfiles)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return false;
	}

	EVE_TFILE * pTFiles = (EVE_TFILE *)pDir->pHachage->GetPtrWithString((char *)pcFile);

	if (pTFiles)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		if (pTFiles->param2 & PAK)
		{
			return pTFiles->param3;
		}
		else
		{
			return pTFiles->taille;
		}
	}

	if (pcDir) delete [] pcDir;

	if (pcFile) delete [] pcFile;

	return -1;
}

//#############################################################################
//                    FILE I/O HANDLE FUNCTIONS
//		These functions provide FILE*-like interface for PAK-contained files
//#############################################################################

//=============================================================================
// FUNCTION: EVE_LOADPACK::fOpen
//=============================================================================
// Description:
//		Opens file in PAK and returns virtual file handle (PACK_FILE*).
//
// Returns:
//		PACK_FILE* handle for use with fRead/fSeek/fTell/fClose
//		NULL if file not found or all handles in use
//
// Algorithm:
//		1. Lookup file in directory tree
//		2. Find free PACK_FILE slot in tPackFile array
//		3. Initialize handle with file metadata pointer
//		4. Mark slot as active with unique ID
//		5. Return handle pointer
//
// Notes:
//		- Maximum PACK_MAX_FREAD concurrent open files
//		- Handle ID set to pcFAT address for validation
//		- Handle stores file offset separately from PAK file offset
//
//=============================================================================
PACK_FILE * EVE_LOADPACK::fOpen(const char * _pcName, const char * _pcMode)
{
	if ((!_pcName) ||
	        (!pRoot)) return NULL;

	char * pcDir = NULL;
	char * pcDir1 = (char *)EVEF_GetDirName((unsigned char *)_pcName);

	if (pcDir1)
	{
		pcDir = new char[strlen((const char *)pcDir1)+2];
		strcpy((char *)pcDir, (const char *)pcDir1);
		strcat((char *)pcDir, "\\");
		delete [] pcDir1;
	}

	char * pcFile = (char *)EVEF_GetFileName((unsigned char *)_pcName);

	EVE_REPERTOIRE * pDir;

	if (!pcDir)
	{
		pDir = pRoot;
	}
	else
	{
		pDir = pRoot->GetSousRepertoire((unsigned char *)pcDir);
	}

	if (!pDir)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return NULL;
	}

	if (!pDir->nbfiles)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		return false;
	}

	EVE_TFILE * pTFiles = (EVE_TFILE *)pDir->pHachage->GetPtrWithString((char *)pcFile);

	if (pTFiles)
	{
		if (pcDir) delete [] pcDir;

		if (pcFile) delete [] pcFile;

		int iNb = PACK_MAX_FREAD;

		while (--iNb)
		{
			if (!tPackFile[iNb].bActif)
			{
				tPackFile[iNb].iID = (int)pcFAT;
				tPackFile[iNb].bActif = true;
				tPackFile[iNb].iOffset = 0;
				tPackFile[iNb].pFile = pTFiles;
				return &tPackFile[iNb];
			}
		}

		return NULL;
	}

	if (pcDir) delete [] pcDir;

	if (pcFile) delete [] pcFile;

	return NULL;
}

// EVE_LOADPACK::fClose - Close PAK file handle
//		Validates handle and marks slot as inactive
//		Returns 0 on success, EOF on invalid handle
int EVE_LOADPACK::fClose(PACK_FILE * _pPackFile)
{
	if ((!_pPackFile) ||
	        (!_pPackFile->bActif) ||
	        (_pPackFile->iID != ((int)pcFAT))) return EOF;

	_pPackFile->bActif = false;
	return 0;
}

// ReadDataFRead - Decompression callback for partial reads from compressed files
//		Similar to ReadData but for fRead operations
static unsigned int ReadDataFRead(char * Buff, unsigned int * Size, void * Param)
{
	PAK_PARAM_FREAD * pPP = (PAK_PARAM_FREAD *)Param;

	int iRead = fread(Buff, 1, *Size, pPP->file);

	return (unsigned int)iRead;
}

// WriteDataFRead - Decompression callback for partial reads
//		Handles offset/limit logic for reading subset of decompressed data
//		Complex buffering to support fSeek + fRead on compressed files
static void WriteDataFRead(char * Buff, unsigned int * Size, void * Param)
{
	PAK_PARAM_FREAD * pPP = (PAK_PARAM_FREAD *)Param;

	if (pPP->iTailleW >= pPP->iTailleBase) return;

	pPP->iOffsetBase -= *Size;
	pPP->iOffsetCurr += *Size;

	if (pPP->iOffset < pPP->iOffsetCurr)
	{
		if (pPP->iOffsetBase < 0) pPP->iOffsetBase += *Size;

		int iSize = *Size - pPP->iOffsetBase;

		if (pPP->iTaille > iSize)
		{
			pPP->iTaille -= iSize;
		}
		else
		{
			iSize = pPP->iTaille;
		}

		if ((pPP->iTailleW + iSize) > pPP->iTailleFic)
		{
			iSize = pPP->iTailleFic - pPP->iTailleW;
		}

		pPP->iTailleW += iSize;

		memcpy((void *)pPP->mem, (const void *)(Buff + pPP->iOffsetBase), iSize);
		pPP->mem += iSize;
		pPP->iOffsetBase = 0;
	}
}

//=============================================================================
// FUNCTION: EVE_LOADPACK::fRead
//=============================================================================
// Description:
//		Reads data from PAK file handle (compressed or uncompressed).
//
// Parameters:
//		_pMem - Output buffer
//		_iSize - Size of each element
//		_iCount - Number of elements
//		_pPackFile - PAK file handle from fOpen
//
// Returns:
//		Number of bytes read
//
// Algorithm:
//		If file is compressed:
//			- Decompress from current offset using complex buffering
//			- WriteDataFRead handles offset/limit logic
//		If file is uncompressed:
//			- Direct fread from current offset
//		Update handle's offset
//
// Notes:
//		- Supports partial reads from middle of compressed files
//		- Requires full decompression with buffering for offsets
//		- Much more complex than Read() due to streaming nature
//
//=============================================================================
int EVE_LOADPACK::fRead(void * _pMem, int _iSize, int _iCount, PACK_FILE * _pPackFile)
{
	if ((!_pPackFile) ||
	        (!_pPackFile->pFile) ||
	        (_pPackFile->iID != ((int) pcFAT))) return 0;

	int iTaille = _iSize * _iCount;

	if ((!_pPackFile) ||
	        (!iTaille) ||
	        (!_pPackFile->pFile)) return 0;

	EVE_TFILE * pTFiles = _pPackFile->pFile;

	if (pTFiles->param2 & PAK)
	{
		ARX_CHECK_NOT_NEG(_pPackFile->iOffset);
		if (ARX_CAST_UINT(_pPackFile->iOffset) >= _pPackFile->pFile->param3) return 0;

		fseek(pfFile, pTFiles->param - iSeekPak, SEEK_CUR);

		PAK_PARAM_FREAD sPP;
		sPP.file        = pfFile;
		sPP.mem         = (char *) _pMem;
		sPP.iOffsetCurr = 0;
		sPP.iOffset     = _pPackFile->iOffset;
		sPP.iOffsetBase = _pPackFile->iOffset;
		sPP.iTaille     = sPP.iTailleBase = iTaille;
		sPP.iTailleW    = 0;
		sPP.iTailleFic  = _pPackFile->pFile->param3;
		explode(ReadDataFRead, WriteDataFRead, pcWorkBuff, &sPP);
		iTaille         = sPP.iTailleW;
		iSeekPak        = ftell(pfFile);
	}
	else
	{
		ARX_CHECK_NOT_NEG(_pPackFile->iOffset);
		if (ARX_CAST_UINT(_pPackFile->iOffset) >= _pPackFile->pFile->taille) return 0;

		fseek(pfFile, pTFiles->param + _pPackFile->iOffset - iSeekPak, SEEK_CUR);

		ARX_CHECK_NOT_NEG(iTaille);

		if (pTFiles->taille < ARX_CAST_UINT(_pPackFile->iOffset + iTaille))
		{
			iTaille -= _pPackFile->iOffset + iTaille - pTFiles->taille;
		}

		fread(_pMem, 1, iTaille, pfFile);
		iSeekPak = ftell(pfFile);
	}

	_pPackFile->iOffset += iTaille;
	return iTaille;
}

// EVE_LOADPACK::fSeek - Seek to position in PAK file handle
//		Supports SEEK_SET, SEEK_END, SEEK_CUR
//		Updates handle's iOffset (logical position, not physical PAK offset)
//		Returns 0 on success, 1 on error (out of bounds)
int EVE_LOADPACK::fSeek(PACK_FILE * _pPackFile, unsigned long _lOffset, int _iOrigin)
{
	if ((!_pPackFile) ||
	        (!_pPackFile->pFile) ||
	        (_pPackFile->iID != ((int)pcFAT))) return 1;

	switch (_iOrigin)
	{
		case SEEK_SET:

			if (_lOffset < 0) return 1;

			if (_pPackFile->pFile->param2 & PAK)
			{
				if (_lOffset > _pPackFile->pFile->param3) return 1;

				_pPackFile->iOffset = _lOffset;
			}
			else
			{
				if (_lOffset > _pPackFile->pFile->taille) return 1;

				_pPackFile->iOffset = _lOffset;
			}

			break;
		case SEEK_END:

			if (_lOffset < 0) return 1;

			if (_pPackFile->pFile->param2 & PAK)
			{
				if (_lOffset > _pPackFile->pFile->param3) return 1;

				_pPackFile->iOffset = _pPackFile->pFile->param3 - _lOffset;
			}
			else
			{
				if (_lOffset > _pPackFile->pFile->taille) return 1;

				_pPackFile->iOffset = _pPackFile->pFile->taille - _lOffset;
			}

			break;
		case SEEK_CUR:

			if (_pPackFile->pFile->param2 & PAK)
			{
				int iOffset = _pPackFile->iOffset + _lOffset;

				if ((iOffset < 0) ||
			        (ARX_CAST_UINT(iOffset) > _pPackFile->pFile->param3))
				{
					return 1;
				}

				_pPackFile->iOffset = iOffset;
			}
			else
			{
				int iOffset = _pPackFile->iOffset + _lOffset;

				if ((iOffset < 0) ||
			        (ARX_CAST_UINT(iOffset) > _pPackFile->pFile->taille))
				{
					return 1;
				}

				_pPackFile->iOffset = iOffset;
			}

			break;
	}

	return 0;
}

// EVE_LOADPACK::fTell - Get current position in PAK file handle
//		Returns logical offset within file (not physical PAK offset)
int EVE_LOADPACK::fTell(PACK_FILE * _pPackFile)
{
	if ((!_pPackFile) ||
	        (!_pPackFile->pFile) ||
	        (_pPackFile->iID != ((int)pcFAT))) return -1;

	return _pPackFile->iOffset;
}

//#############################################################################
//                    ENCRYPTION / DECRYPTION FUNCTIONS
//		XOR-based encryption with rotating key position
//		Used for FAT (directory structure) encryption
//#############################################################################

// CryptChar - Encrypt single character using XOR with rotating key
//		NOT USED (code appears incomplete - shift value is 0)
void EVE_LOADPACK::CryptChar(unsigned char * _pChar)
{
#ifdef CRYPT_OFF
	return;
#endif
	unsigned int iTailleKey = strlen((const char *) cKey);
	int iDecalage = 0;
	
	*_pChar = ARX_CLEAN_WARN_CAST_UCHAR(((*_pChar) ^ cKey[iPassKey]) >> iDecalage);

	iPassKey++;

	if (iPassKey >= iTailleKey) iPassKey = 0;
}

// UnCryptChar - Decrypt single character using XOR with rotating key
//		XOR with current key character, advance key position
void EVE_LOADPACK::UnCryptChar(unsigned char * _pChar)
{
#ifdef CRYPT_OFF
	return;
#endif

	unsigned int iTailleKey = strlen((const char *) cKey);

	int iDecalage = 0;
	*_pChar = ARX_CLEAN_WARN_CAST_UCHAR(((*_pChar) ^ cKey[iPassKey]) << iDecalage);

	iPassKey++;

	if (iPassKey >= iTailleKey) iPassKey = 0;
}

// CryptString - Encrypt null-terminated string character-by-character
void EVE_LOADPACK::CryptString(unsigned char * _pTxt)
{
	unsigned char * pTxtCopy = (unsigned char *)_pTxt;
	int iTaille = strlen((const char *)_pTxt) + 1;

	while (iTaille--)
	{
		CryptChar(pTxtCopy);
		pTxtCopy++;
	}
}

// UnCryptString - Decrypt null-terminated string character-by-character
//		Returns length of string (excluding null terminator)
int EVE_LOADPACK::UnCryptString(unsigned char * _pTxt)
{
	unsigned char * pTxtCopy = (unsigned char *)_pTxt;

	int iNbChar = 0;

	while (1)
	{
		UnCryptChar(pTxtCopy);

		if (!*pTxtCopy)
		{
			break;
		}

		pTxtCopy++;
		iNbChar++;
	}

	return iNbChar;
}

// CryptShort - Encrypt 2-byte short by encrypting each byte separately
void EVE_LOADPACK::CryptShort(unsigned short * _pShort)
{
	unsigned char cA, cB;
	cA = ARX_CLEAN_WARN_CAST_UCHAR((*_pShort) & 0xFF);
	cB = ARX_CLEAN_WARN_CAST_UCHAR(((*_pShort) >> 8) & 0xFF);

	CryptChar(&cA);
	CryptChar(&cB);
	*_pShort = cA | (cB << 8);
}

// UnCryptShort - Decrypt 2-byte short by decrypting each byte separately
void EVE_LOADPACK::UnCryptShort(unsigned short * _pShort)
{
	unsigned char cA, cB;
	cA = ARX_CLEAN_WARN_CAST_UCHAR((*_pShort) & 0xFF);
	cB = ARX_CLEAN_WARN_CAST_UCHAR(((*_pShort) >> 8) & 0xFF);

	UnCryptChar(&cA);
	UnCryptChar(&cB);
	*_pShort = cA | (cB << 8);
}

// CryptInt - Encrypt 4-byte int by encrypting each short separately
void EVE_LOADPACK::CryptInt(unsigned int * _iInt)
{
	unsigned short sA, sB;
	sA = (*_iInt) & 0xFFFF;
	sB = ((*_iInt) >> 16) & 0xFFFF;

	CryptShort(&sA);
	CryptShort(&sB);
	*_iInt = sA | (sB << 16);
}

// UnCryptInt - Decrypt 4-byte int by decrypting each short separately
void EVE_LOADPACK::UnCryptInt(unsigned int * _iInt)
{
	unsigned short sA, sB;
	sA = (*_iInt) & 0xFFFF;
	sB = ((*_iInt) >> 16) & 0xFFFF;

	UnCryptShort(&sA);
	UnCryptShort(&sB);
	*_iInt = sA | (sB << 16);
}

//#############################################################################
//                    EXTRACTION UTILITIES (DEBUG/DEVELOPMENT)
//		Functions to extract PAK contents to disk for debugging
//#############################################################################

//=============================================================================
// FUNCTION: EVE_LOADPACK::WriteSousRepertoire
//=============================================================================
// Description:
//		Recursively extracts directory tree from PAK to disk.
//
// Parameters:
//		pcAbs - Absolute path where to extract files
//		r - Directory node to extract
//
// Algorithm:
//		1. Build full directory path
//		2. Create directory on disk
//		3. For each file in directory:
//		   - Read file from PAK
//		   - Write to disk
//		4. Recursively process subdirectories
//
// Notes:
//		- Debug/development tool for inspecting PAK contents
//		- Uses MessageBox for error reporting (Windows)
//		- Prints extracted paths to console
//
//=============================================================================
void EVE_LOADPACK::WriteSousRepertoire(char * pcAbs, EVE_REPERTOIRE * r)
{
	char EveTxtFile[256];
	strcpy((char *)EveTxtFile, pcAbs);

	if (r)
	{
		r->ConstructFullNameRepertoire(EveTxtFile);
	}

	CreateDirectory((const char *)EveTxtFile, NULL);

	EVE_TFILE * f = r->fichiers;
	int nb = r->nbfiles;

	while (nb--)
	{
		char	tTxt[512];
		strcpy(tTxt, EveTxtFile);
		strcat(tTxt, (const char *)f->name);
		int		iTaille;
		void	* pDat = this->ReadAlloc(tTxt + strlen((const char *)pcAbs), &iTaille);

		if (pDat)
		{
			printf("%s\n", tTxt);
			FILE * file;
			file = fopen(tTxt, "wb");

			if (file)
			{
				fwrite(pDat, 1, iTaille, file);
				fclose(file);
			}

			free((void *)pDat);
			f = f->fnext;
		}
		else
		{
			MessageBox(NULL, tTxt, "No Found!!", 0);
		}
	}


	EVE_REPERTOIRE * brep = r->fils;
	nb = r->nbsousreps;

	while (nb--)
	{
		EVE_REPERTOIRE * brepnext = brep->brothernext;
		WriteSousRepertoire(pcAbs, brep);
		brep = brepnext;
	}
}

//=============================================================================
// FUNCTION: EVE_LOADPACK::WriteSousRepertoireZarbi
//=============================================================================
// Description:
//		Test/validation version of WriteSousRepertoire - extracts PAK with
//		fRead validation (reads files in random-sized chunks to test fRead).
//
// Purpose:
//		- Validates fRead implementation on compressed files
//		- Tests partial reads and seeking in PAK files
//		- Compares extracted data with direct Read() output
//
// Algorithm:
//		Same as WriteSousRepertoire but uses fOpen/fRead/fClose
//		with random chunk sizes instead of direct Read()
//
// Notes:
//		- "Zarbi" = French slang for "weird" (testing/debug function)
//		- Useful for validating streaming decompression logic
//		- Slower than WriteSousRepertoire due to random I/O
//
//=============================================================================
void EVE_LOADPACK::WriteSousRepertoireZarbi(char * pcAbs, EVE_REPERTOIRE * r)
{
	char EveTxtFile[256];
	strcpy((char *)EveTxtFile, pcAbs);

	if (r)
	{
		r->ConstructFullNameRepertoire(EveTxtFile);
	}

	CreateDirectory((const char *)EveTxtFile, NULL);

	EVE_TFILE * f = r->fichiers;
	int nb = r->nbfiles;

	while (nb--)
	{
		char	tTxt[512];
		strcpy(tTxt, EveTxtFile);
		strcat(tTxt, (const char *)f->name);
		int		iTaille;

		void	* pDat = this->ReadAlloc(tTxt + strlen((const char *)pcAbs), &iTaille);
		int iTaille2 = iTaille;

		if (pDat)
		{
			printf("%s\n", tTxt);
			PACK_FILE * pPf = fOpen(tTxt + strlen((const char *)pcAbs), "rb");

			if (!pPf)
			{
				MessageBox(NULL, tTxt, "ERROR fopen!!", 0);
			}
			else
			{
				int nb2;
				char * pcDat = (char *)pDat;

				while (iTaille)
				{
					printf("%d\r", iTaille);

					if (iTaille < 50)
					{
						nb2 = iTaille;
					}
					else
					{
						nb2 = rand() % iTaille;

						if (!nb2) continue;
					}

					iTaille -= nb2;
					int nb3 = fRead(pcDat, 1, nb2, pPf);
					pcDat += nb2;

					if (nb3 != nb2)
					{
						MessageBox(NULL, tTxt, "ERROR fread!!", 0);
					}
				}

				printf("%d\n", iTaille);
				fClose(pPf);

				FILE * file;
				file = fopen(tTxt, "wb");

				if (file)
				{
					fwrite(pDat, 1, iTaille2, file);
					fclose(file);
				}

				free((void *)pDat);
				f = f->fnext;
			}
		}
		else
		{
			MessageBox(NULL, tTxt, "No Found!!", 0);
		}
	}


	EVE_REPERTOIRE * brep = r->fils;
	nb = r->nbsousreps;

	while (nb--)
	{
		EVE_REPERTOIRE * brepnext = brep->brothernext;
		WriteSousRepertoireZarbi(pcAbs, brep);
		brep = brepnext;
	}
}
