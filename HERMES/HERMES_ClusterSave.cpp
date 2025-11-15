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
// HERMES_ClusterSave.cpp - Cluster-Based Archive File System
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Implements a custom archive format with cluster-based file storage.
//		Supports fragmented files, dynamic updates, and defragmentation.
//
// Purpose:
//		Provides flexible save file system for game state persistence:
//		- Multiple files stored in single archive
//		- Files can be updated without rewriting entire archive
//		- Fragmentation support allows appending/updating files
//		- Defragmentation compacts archive when fragmentation excessive
//
// Architecture:
//
//		CCluster - Data Chunk/Fragment
//			Represents a contiguous block of file data within the archive
//			Contains: size, file offset, next cluster link
//			Clusters form linked list (fragmented files span multiple clusters)
//
//		CInfoFile - File Metadata
//			Represents one file within the archive
//			Contains: filename, total size, cluster chain
//			FirstCluster: Head of linked list of data chunks
//			Files can be fragmented across multiple non-contiguous clusters
//
//		CSaveBlock - Archive Container
//			Main archive file manager
//			Contains: FAT (File Allocation Table), file metadata array
//			Provides: read, write, update, defragmentation operations
//
// File Format (on disk):
//		[Header - 4 bytes] - Total data block size
//		[Data Blocks] - Variable size, contains all file data (possibly fragmented)
//		[FAT Header]
//			- Version (4 bytes)
//			- Number of files (4 bytes)
//		[FAT Entries] - One per file
//			- Filename (null-terminated string)
//			- Total file size (4 bytes)
//			- Number of clusters (4 bytes)
//			- Reserved (4 bytes)
//			- Cluster chain (8 bytes per cluster: size + offset)
//
// Cluster Chain:
//		File data can be fragmented across multiple clusters:
//		Example: 10KB file might be stored as:
//			Cluster 1: offset=100, size=4096 (first 4KB)
//			Cluster 2: offset=8000, size=4096 (next 4KB)
//			Cluster 3: offset=15000, size=1808 (remaining 1.8KB)
//
// Operations:
//		BeginRead() - Open archive for reading, load FAT into memory
//		Read() - Read file data by following cluster chain
//		GetSize() - Get total file size
//		ExistFile() - Check if file exists in archive
//		BeginSave() - Open archive for writing (append or rewrite mode)
//		Save() - Write/update file (creates new clusters or reuses existing)
//		EndSave() - Finalize archive, write updated FAT
//		Defrag() - Compact archive by removing fragmentation
//
// Fragmentation:
//		Files become fragmented when:
//		- Updating existing file with larger data (appends new clusters)
//		- Adding new files to existing archive (appends to end)
//		Defrag() resolves by:
//		- Reading all files in fragmented form
//		- Writing them back contiguously
//		- Rebuilding FAT with single cluster per file
//
// Hash Table Optimization:
//		During BeginRead(), builds hash table (CHachageString) for fast lookups
//		Maps filename -> CInfoFile pointer for O(1) file access
//		Hash table sized to next power-of-2 >= 1.33 * file count
//
// Code: Mickael Pointier/Cyril Meynier
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include "HERMES_ClusterSave.h"
#include "windows.h"
#include "HERMES_hachage.h"


#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//#############################################################################
//#############################################################################
//                             CCluster - Data Chunk/Fragment
//#############################################################################
//#############################################################################

//=============================================================================
// FUNCTION: CCluster::CCluster (Constructor)
//=============================================================================
// Description:
//		Constructs a data cluster (contiguous block within archive).
//
// Parameters:
//		_iTaille - Size of this cluster in bytes
//
// Notes:
//		- Cluster represents one fragment of a file
//		- iNext will be set to file offset when cluster is written
//		- pNext links to next cluster for fragmented files
//
//=============================================================================
CCluster::CCluster(int _iTaille)
{
	iTaille = _iTaille;
	iNext = -1;
	pNext = NULL;
}

// CCluster::~CCluster - Destructor
//		Destroys cluster node (does not free linked clusters - parent owns chain)
CCluster::~CCluster()
{
}

//#############################################################################
//#############################################################################
//                             CInfoFile - File Metadata
//#############################################################################
//#############################################################################

//=============================================================================
// FUNCTION: CInfoFile::Set
//=============================================================================
// Description:
//		Initializes file metadata with filename and size.
//
// Parameters:
//		_pcFileName - Filename to associate with this entry
//		_iTaille - Total file size in bytes (sum of all clusters)
//
// Notes:
//		- Duplicates filename string (caller can free original)
//		- Initializes first cluster with total file size
//		- Cluster chain starts with single entry (more added if fragmented)
//
//=============================================================================
void CInfoFile::Set(char * _pcFileName, int _iTaille)
{
	if (pcFileName)
	{
		free((void *)pcFileName);
		pcFileName = NULL;
	}

	pcFileName = strdup(_pcFileName);
	iTaille = _iTaille;
	iNbCluster = 0;
	FirstCluster.iTaille = _iTaille;
	FirstCluster.iNext = -1;
	FirstCluster.pNext = NULL;
}

//=============================================================================
// FUNCTION: CInfoFile::KillAll
//=============================================================================
// Description:
//		Frees all memory associated with this file entry.
//
// Algorithm:
//		1. Free filename string
//		2. Iterate through cluster chain and delete all nodes
//		3. Reset cluster head to empty state
//
// Notes:
//		- Does NOT free file data itself (just metadata)
//		- Must iterate through linked list to avoid memory leaks
//
//=============================================================================
void CInfoFile::KillAll()
{
	if (pcFileName)
	{
		free((void *)pcFileName);
		pcFileName = NULL;
	}

	CCluster * _pClusterNext = FirstCluster.pNext;

	while (_pClusterNext)
	{
		CCluster * _pClusterNextNext = _pClusterNext->pNext;
		delete _pClusterNext;
		_pClusterNext = _pClusterNextNext;
	}

	FirstCluster.iTaille = 0;
	FirstCluster.iNext = 0;
	FirstCluster.pNext = NULL;
}

//#############################################################################
//#############################################################################
//                             CSaveBlock - Archive Container
//#############################################################################
//#############################################################################

//=============================================================================
// FUNCTION: CSaveBlock::CSaveBlock (Constructor)
//=============================================================================
// Description:
//		Constructs archive container with specified filename.
//
// Parameters:
//		_pcBlockName - Path to archive file on disk
//
// Notes:
//		- Does not open file (use BeginRead/BeginSave)
//		- Initializes empty FAT (no files)
//		- Hash table created during BeginRead for fast lookups
//
//=============================================================================
CSaveBlock::CSaveBlock(char * _pcBlockName)
{
	if (_pcBlockName)
	{
		pcBlockName = strdup(_pcBlockName);
	}
	else
	{
		pcBlockName = strdup("Save Block");
	}

	hFile = NULL;
	iTailleBlock = 0;
	iNbFiles = 0;
	sInfoFile = NULL;

	bFirst = false;

	pHachage = NULL;
}

//=============================================================================
// FUNCTION: CSaveBlock::~CSaveBlock (Destructor)
//=============================================================================
// Description:
//		Destroys archive container and frees all resources.
//
// Algorithm:
//		1. Free block name string
//		2. Call KillAll on each file entry to free cluster chains
//		3. Free file info array
//		4. Close file handle if still open
//		5. Delete hash table if allocated
//
//=============================================================================
CSaveBlock::~CSaveBlock()
{
	if (pcBlockName)
	{
		free((void *)pcBlockName);
	}

	while (iNbFiles--)
	{
		sInfoFile[iNbFiles].KillAll();
	}

	free(sInfoFile);
	sInfoFile = NULL;

	if (hFile)
	{
		fclose(hFile);
		hFile = NULL;
	}

	delete pHachage;
	pHachage = NULL;
}

//=============================================================================
// FUNCTION: CSaveBlock::ResetFAT
//=============================================================================
// Description:
//		Clears File Allocation Table (FAT) and frees all file metadata.
//
// Algorithm:
//		1. Reset total data block size to 0
//		2. Free all cluster chains for each file
//		3. Free file info array
//		4. Reset file count to 0
//
// Notes:
//		- Called after EndRead/EndSave to free in-memory FAT
//		- Does NOT modify archive file on disk
//
//=============================================================================
void CSaveBlock::ResetFAT(void)
{
	iTailleBlock = 0;

	while (iNbFiles--)
	{
		sInfoFile[iNbFiles].KillAll();
	}

	free(sInfoFile);
	sInfoFile = NULL;
	iNbFiles = 0;
}

//=============================================================================
// FUNCTION: CSaveBlock::BeginRead
//=============================================================================
// Description:
//		Opens archive for reading and loads FAT into memory.
//
// Returns:
//		true - Archive opened successfully, FAT loaded
//		false - Failed to open archive file
//
// Algorithm:
//		1. Open archive file in binary read mode
//		2. Read header to get data block size
//		3. Seek to FAT (located after data blocks)
//		4. Read FAT header (version, file count)
//		5. Allocate hash table sized for file count (next power-of-2)
//		6. For each file entry:
//		   - Read filename (null-terminated string)
//		   - Read file size and cluster count
//		   - Read cluster chain (size + offset pairs)
//		7. Populate hash table for fast filename lookups
//
// File Format:
//		[0-3] Data block size (4 bytes)
//		[4 - 4+size] Data blocks
//		[FAT] Version (4 bytes)
//		[FAT+4] File count (4 bytes)
//		[FAT+8...] File entries (variable size each)
//
// Hash Table Sizing:
//		- Finds next power-of-2 >= file count
//		- If file count > 75% of that power, doubles size
//		- Ensures hash table not overloaded (good performance)
//
// Notes:
//		- FAT remains in memory until EndRead called
//		- Hash table allows O(1) file lookups by name
//		- Must call EndRead to close file and free memory
//
//=============================================================================
bool CSaveBlock::BeginRead(void)
{
	hFile = fopen((const char *)pcBlockName, (const char *)"rb");

	if (!hFile)
	{
		return false;
	}

	//READ FAT
	int _iI;

	fread((void *)&_iI, 1, 4, hFile);
	fseek(hFile, _iI + 4, SEEK_SET);

	fread((void *)&_iI, 1, 4, hFile);	//version

	fread((void *)&_iI, 1, 4, hFile);

	int iNbHache = 1;

	while (iNbHache < _iI) iNbHache <<= 1;

	int iNbHacheTroisQuart = (iNbHache * 3) / 4;

	if (_iI > iNbHacheTroisQuart) iNbHache <<= 1;

	pHachage = new CHachageString(iNbHache);

	while (_iI--)
	{
		char _pT[256], *_pTT = _pT;

		while (1)
		{
			fread((void *)_pTT, 1, 1, hFile);

			if (!*_pTT) break;

			_pTT++;
		}

		ExpandNbFiles();
		CInfoFile * _pInfoFile = &sInfoFile[iNbFiles-1];
		fread((void *)&_pInfoFile->iTaille, 1, 4, hFile);
		_pInfoFile->Set(_pT, _pInfoFile->iTaille);
		fread((void *)&_pInfoFile->iNbCluster, 1, 4, hFile);
		fseek(hFile, 4, SEEK_CUR);

		CCluster * _pCluster = &_pInfoFile->FirstCluster;
		fread((void *)&_pCluster->iTaille, 1, 4, hFile);
		fread((void *)&_pCluster->iNext, 1, 4, hFile);
		_pCluster->pNext = NULL;

		int _iJ = _pInfoFile->iNbCluster - 1;

		if (_iJ > 0)
		{
			while (_iJ--)
			{
				_pCluster->pNext = new CCluster(0);
				_pCluster = _pCluster->pNext;
				fread((void *)&_pCluster->iTaille, 1, 4, hFile);
				fread((void *)&_pCluster->iNext, 1, 4, hFile);
			}
		}
	}

	for (int i = 0;
	        i < iNbFiles;
	        ++i)
	{
		pHachage->AddString(sInfoFile[i].pcFileName, &sInfoFile[i]);
	}

	return true;
}

//=============================================================================
// FUNCTION: CSaveBlock::EndRead
//=============================================================================
// Description:
//		Closes archive after reading and frees FAT from memory.
//
// Algorithm:
//		1. Close file handle
//		2. Call ResetFAT to free all file metadata
//		3. Delete hash table
//
// Notes:
//		- Must be called after BeginRead when done reading files
//		- Frees all memory allocated during BeginRead
//
//=============================================================================
void CSaveBlock::EndRead(void)
{
	if (hFile)
	{
		fclose(hFile);
		hFile = NULL;
	}

	ResetFAT();

	delete pHachage;
	pHachage = NULL;
}

//=============================================================================
// FUNCTION: CSaveBlock::BeginSave
//=============================================================================
// Description:
//		Opens archive for writing/updating files.
//
// Parameters:
//		_bCont - Continue mode (append to existing archive vs. create new)
//		_bReWrite - Allow rewriting existing files in-place (updates clusters)
//
// Returns:
//		true - Archive opened successfully for writing
//		false - Failed to open/create archive file
//
// Algorithm:
//		If creating new archive (!_bCont or file doesn't exist):
//			1. Create new file in binary write mode
//			2. Write placeholder header (size = 0)
//			3. Set bFirst flag (indicates new archive)
//
//		If appending to existing archive:
//			1. Open existing file in binary read mode
//			2. Read data block size from header
//			3. Seek to FAT and load all file metadata
//			4. Close and reopen in write mode ("w+b")
//			5. Copy existing data blocks to new file
//			6. File pointer positioned to append new data
//
// Notes:
//		- Continue mode preserves existing files and allows updates
//		- New mode overwrites entire archive
//		- ReWrite flag controls whether Save() updates in-place or appends
//		- File remains open until EndSave called
//
//=============================================================================
bool CSaveBlock::BeginSave(bool _bCont, bool _bReWrite)
{
	bReWrite = _bReWrite;

	hFile = fopen((const char *)pcBlockName, (const char *)"rb");

	if ((!hFile) ||
	        (!_bCont))
	{
		hFile = fopen((const char *)pcBlockName, (const char *)"wb");

		if (!hFile) return false;

		int _iI = 0;

		fwrite((const void *)&_iI, 1, 4, hFile);
		bFirst = true;
	}
	else
	{
		fread((void *)&iTailleBlock, 1, 4, hFile);
		fseek(hFile, iTailleBlock + 4, SEEK_SET);

		//READ FAT
		int _iI;

		fread((void *)&_iI, 1, 4, hFile);	//version
		iVersion = _iI;

		fread((void *)&_iI, 1, 4, hFile);

		while (_iI--)
		{
			char _pT[256], *_pTT = _pT;

			while (1)
			{
				fread((void *)_pTT, 1, 1, hFile);

				if (!*_pTT) break;

				_pTT++;
			}

			ExpandNbFiles();
			CInfoFile * _pInfoFile = &sInfoFile[iNbFiles-1];
			fread((void *)&_pInfoFile->iTaille, 1, 4, hFile);
			_pInfoFile->Set(_pT, _pInfoFile->iTaille);
			fread((void *)&_pInfoFile->iNbCluster, 1, 4, hFile);
			fseek(hFile, 4, SEEK_CUR);

			CCluster * _pCluster = &_pInfoFile->FirstCluster;
			fread((void *)&_pCluster->iTaille, 1, 4, hFile);
			fread((void *)&_pCluster->iNext, 1, 4, hFile);
			_pCluster->pNext = NULL;

			int _iJ = _pInfoFile->iNbCluster - 1;

			if (_iJ > 0)
			{
				while (_iJ--)
				{
					_pCluster->pNext = new CCluster(0);
					_pCluster = _pCluster->pNext;
					fread((void *)&_pCluster->iTaille, 1, 4, hFile);
					fread((void *)&_pCluster->iNext, 1, 4, hFile);
				}
			}
		}

		fseek(hFile, 4, SEEK_SET);
		void * _pPtr = malloc(iTailleBlock);
		fread((void *)_pPtr, iTailleBlock, 1, hFile);
		fclose(hFile);
		hFile = NULL;
		hFile = fopen((const char *)pcBlockName, (const char *)"w+b");

		if (!hFile) return false;

		_iI = 0;
		fwrite((const void *)&_iI, 4, 1, hFile);
		fwrite((const void *)_pPtr, iTailleBlock, 1, hFile);

		free((void *)_pPtr);
	}

	return true;
}

//=============================================================================
// FUNCTION: CSaveBlock::EndSave
//=============================================================================
// Description:
//		Finalizes archive after writing, writes FAT, closes file.
//
// Returns:
//		true - Archive finalized successfully
//
// Algorithm:
//		If new archive (bFirst == true):
//			1. Write FAT to end of file:
//			   - Version number
//			   - File count
//			   - For each file: name, size, cluster chain
//			2. Seek to start and update data block size header
//			3. Close file
//			4. Free FAT from memory
//
//		If updating existing archive (bFirst == false):
//			1. Call Defrag() to compact and rebuild archive
//
// Notes:
//		- New archives written directly (no fragmentation)
//		- Updated archives defragmented to optimize storage
//		- FAT written at end of file (after all data blocks)
//		- Must be called after all Save() operations complete
//
//=============================================================================
bool CSaveBlock::EndSave(void)
{
	if (!bFirst)
	{
		return Defrag();
	}

	int _version = PAK_VERSION + 1;
	fwrite((const void *)&_version, 1, 4, hFile);

	fwrite((const void *)&iNbFiles, 1, 4, hFile);

	for (int _iI = 0; _iI < iNbFiles; _iI++)
	{
		fwrite((const void *)sInfoFile[_iI].pcFileName, 1, strlen((const char *)sInfoFile[_iI].pcFileName) + 1, hFile);
		fwrite((const void *)&sInfoFile[_iI].iTaille, 1, 4, hFile);
		int _iT = sInfoFile[_iI].iNbCluster;
		fwrite((const void *)&_iT, 1, 4, hFile);
		_iT *= (sizeof(CCluster) - 4) + strlen((const char *)sInfoFile[_iI].pcFileName) + 1 + 20;
		fwrite((const void *)&_iT, 1, 4, hFile);

		CCluster * _pCluster = &sInfoFile[_iI].FirstCluster;

		while (_pCluster)
		{
			fwrite((const void *)&_pCluster->iTaille, 1, 4, hFile);
			fwrite((const void *)&_pCluster->iNext, 1, 4, hFile);
			_pCluster = _pCluster->pNext;
		}
	}

	fseek(hFile, 0, SEEK_SET);
	fwrite((const void *)&iTailleBlock, 1, 4, hFile);

	fclose(hFile);
	hFile = NULL;
	ResetFAT();
	return true;
}

//=============================================================================
// FUNCTION: CSaveBlock::Defrag
//=============================================================================
// Description:
//		Defragments archive by compacting all files into contiguous storage.
//
// Returns:
//		true - Defragmentation completed successfully
//
// Algorithm:
//		1. Create temporary file (original name + "DFG" suffix)
//		2. For each file in archive:
//		   a. Allocate buffer for entire file
//		   b. Read all clusters and concatenate into buffer
//		   c. Handle version bug (old version may have wrong sizes)
//		   d. Write complete file to temp file contiguously
//		3. Write new FAT to temp file with single cluster per file
//		4. Update header with new data block size
//		5. Close both files
//		6. Delete original archive
//		7. Rename temp file to original name
//
// Fragmentation Example:
//		Before defrag:
//			File A: Cluster at offset 0 (4KB) + Cluster at offset 10000 (2KB)
//			File B: Cluster at offset 5000 (3KB)
//		After defrag:
//			File A: Single cluster at offset 0 (6KB)
//			File B: Single cluster at offset 6000 (3KB)
//
// Version Handling:
//		- Old version (PAK_VERSION) had size calculation bugs
//		- Skips first file entry if old version (index 1 instead of 0)
//		- Reallocates buffer if actual size exceeds expected size
//
// Notes:
//		- Eliminates all fragmentation (one cluster per file)
//		- Reduces archive size by removing unused gaps
//		- Critical for performance on frequently updated archives
//		- Uses temp file to avoid data loss if defrag fails
//
//=============================================================================
bool CSaveBlock::Defrag()
{
	char txt[256];
	strcpy(txt, pcBlockName);
	strcat(txt, "DFG");
	FILE * fFileTemp = fopen(txt, "wb");

	int _version = PAK_VERSION + 1;
	fwrite((const void *)&_version, 1, 4, fFileTemp);

	for (int _iI = (iVersion == PAK_VERSION) ? 1 : 0; _iI < iNbFiles; _iI++)
	{
		CCluster * pCluster = &sInfoFile[_iI].FirstCluster;
		void * pMem = malloc(sInfoFile[_iI].iTaille);
		char * pcMem = (char *)pMem;
		int iRealSize = 0;

		while (pCluster)
		{
			fseek(hFile, pCluster->iNext + 4, SEEK_SET);

			//bug size old version
			if (iVersion == PAK_VERSION)
			{
				if ((iRealSize + pCluster->iTaille) > sInfoFile[_iI].iTaille)
				{
					pMem = realloc(pMem, iRealSize + pCluster->iTaille);
					pcMem = ((char *)pMem) + iRealSize;
					sInfoFile[_iI].iTaille = iRealSize + pCluster->iTaille;
				}
			}

			iRealSize += pCluster->iTaille;

			fread(pcMem, pCluster->iTaille, 1, hFile);
			pcMem += pCluster->iTaille;
			pCluster = pCluster->pNext;
		}

		fwrite(pMem, sInfoFile[_iI].iTaille, 1, fFileTemp);
		free(pMem);
	}

	fwrite((const void *)&_version, 1, 4, fFileTemp);
 
	int iOffset = 0;
	int iNbFilesTemp = (iVersion == PAK_VERSION) ? iNbFiles - 1 : iNbFiles; //-1;
	fwrite((const void *)&iNbFilesTemp, 1, 4, fFileTemp);

	for (int _iI = (iVersion == PAK_VERSION) ? 1 : 0; _iI < iNbFiles; _iI++)
	{
		fwrite((const void *)sInfoFile[_iI].pcFileName, 1, strlen((const char *)sInfoFile[_iI].pcFileName) + 1, fFileTemp);
		fwrite((const void *)&sInfoFile[_iI].iTaille, 1, 4, fFileTemp);
		int _iT = 0;
		fwrite((const void *)&_iT, 1, 4, fFileTemp);
		fwrite((const void *)&_iT, 1, 4, fFileTemp);

		//cluster
		fwrite((const void *)&sInfoFile[_iI].iTaille, 1, 4, fFileTemp);
		fwrite((const void *)&iOffset, 1, 4, fFileTemp);

		iOffset += sInfoFile[_iI].iTaille;
	}

	fseek(fFileTemp, 0, SEEK_SET);
	fwrite((const void *)&iOffset, 1, 4, fFileTemp);

	fclose(hFile);
	hFile = NULL;
	ResetFAT();

	fclose(fFileTemp);
	DeleteFile(pcBlockName);
	MoveFile(txt, pcBlockName);

	return true;
}

//=============================================================================
// FUNCTION: CSaveBlock::ExpandNbFiles
//=============================================================================
// Description:
//		Expands file info array to accommodate one more file.
//
// Returns:
//		true - Always (no error checking on realloc)
//
// Algorithm:
//		1. Increment file count
//		2. Reallocate sInfoFile array to new size
//		3. Zero-initialize new entry
//
// Notes:
//		- Called when adding new file to archive
//		- No error handling (assumes realloc succeeds)
//
//=============================================================================
bool CSaveBlock::ExpandNbFiles()
{
	iNbFiles++;
	sInfoFile = (CInfoFile *)realloc(sInfoFile, (iNbFiles) * sizeof(CInfoFile));
	memset(&sInfoFile[iNbFiles-1], 0, sizeof(CInfoFile));
	return true;
}

//=============================================================================
// FUNCTION: CSaveBlock::Save
//=============================================================================
// Description:
//		Saves file data to archive (creates new file or updates existing).
//
// Parameters:
//		_pcFileName - Name of file to save
//		_pDatas - Pointer to file data (NULL to reserve space)
//		_iSize - Size of file data in bytes
//
// Returns:
//		true - File saved successfully
//		false - Error (invalid parameters or file I/O failure)
//
// Algorithm:
//		Search for existing file by name:
//
//		If file NOT found (new file):
//			1. Expand file array
//			2. Create new CInfoFile entry
//			3. Add single cluster at end of archive
//			4. Write data
//			5. Update total archive size
//
//		If file FOUND and bReWrite mode enabled:
//			1. Reuse existing clusters where possible
//			2. For each existing cluster:
//			   - Seek to cluster offset in archive
//			   - Write new data (up to cluster size)
//			   - Advance data pointer
//			3. If new data larger than existing clusters:
//			   - Append new cluster to chain
//			   - Write remaining data
//			4. If new data smaller than existing clusters:
//			   - Truncate cluster chain
//			   - Adjust last cluster size
//
//		If file FOUND and bReWrite mode disabled:
//			1. Find end of cluster chain
//			2. Append new cluster to chain
//			3. Write data (appends to file, doesn't replace)
//
// Cluster Reuse Example (bReWrite = true):
//		Existing file: Cluster A (4KB) + Cluster B (2KB) = 6KB total
//		New data: 5KB
//		Result:
//			- Rewrite Cluster A with 4KB of new data
//			- Rewrite Cluster B with 1KB of new data (resize to 1KB)
//			- Delete remaining clusters
//
// Notes:
//		- NULL _pDatas reserves space without writing data
//		- bReWrite flag set during BeginSave call
//		- File remains fragmented until Defrag called
//		- Case-insensitive filename comparison
//
//=============================================================================
bool CSaveBlock::Save(char * _pcFileName, void * _pDatas, int _iSize)
{
	bool _bFound = false;
	CInfoFile * _pInfoFile = sInfoFile;
	int _iI = iNbFiles;

	while (_iI--)
	{
		if (!stricmp((const char *)_pInfoFile->pcFileName, (const char *)_pcFileName))
		{
			_bFound = true;
			break;
		}

		_pInfoFile++;
	}

	if (!_bFound)
	{
		ExpandNbFiles();
		_pInfoFile = &sInfoFile[iNbFiles-1];
		_pInfoFile->Set(_pcFileName, _iSize);

		_pInfoFile->FirstCluster.iNext = iTailleBlock;
	}
	else
	{
		if (bReWrite)
		{
			if (!_pDatas) return false;

			int iNbClusters = 0;
			int iSizeWrite = 0;
			int iSize = _iSize;
			CCluster * _pClusterCurr = &_pInfoFile->FirstCluster;
			CCluster * pLastClusterCurr = NULL;

			while ((_pClusterCurr) && (iSize > 0))
			{
				pLastClusterCurr = _pClusterCurr;
				fseek(hFile, _pClusterCurr->iNext + 4, SEEK_SET);
				iSizeWrite = __min(_pClusterCurr->iTaille, iSize);
				fwrite((const void *)_pDatas, iSizeWrite, 1, hFile);
				char * _pDatasTemp = (char *)_pDatas;
				_pDatasTemp += iSizeWrite;
				_pDatas = (void *)_pDatasTemp;
				iSize -= iSizeWrite;
				iNbClusters++;
				_pClusterCurr = _pClusterCurr->pNext;
			}

			_pInfoFile->iTaille = _iSize;

			if (iSize)
			{
				if (pLastClusterCurr)
				{
					pLastClusterCurr->pNext = new CCluster(iSize);
					pLastClusterCurr->pNext->iNext = iTailleBlock;
					pLastClusterCurr->pNext->pNext = NULL;
					iTailleBlock += iSize;
					fseek(hFile, 0, SEEK_END);
					fwrite((const void *)_pDatas, iSize, 1, hFile);
					_pInfoFile->iNbCluster++;
				}
			}
			else
			{
				if (pLastClusterCurr)
				{
					if (pLastClusterCurr->iTaille > iSizeWrite)
					{
						pLastClusterCurr->iTaille = iSizeWrite;
					}

					delete pLastClusterCurr->pNext;
					pLastClusterCurr->pNext = NULL;
					_pInfoFile->iTaille = _iSize;
					_pInfoFile->iNbCluster = iNbClusters;
				}
			}

			fseek(hFile, 0, SEEK_END);

			return true;
		}
		else
		{

			CCluster * _pCluster = &_pInfoFile->FirstCluster;

			while (_pCluster->pNext)
			{
				_pCluster = _pCluster->pNext;
			}

			_pCluster->pNext = new CCluster(_iSize);
			_pCluster->pNext->iNext = iTailleBlock;

			_pInfoFile->iTaille += _iSize;
		}
	}

	_pInfoFile->iNbCluster++;
	iTailleBlock += _iSize;

	if (!hFile) return false;

	if (_pDatas) fwrite((const void *)_pDatas, 1, _iSize, hFile);

	return true;
}

//=============================================================================
// FUNCTION: CSaveBlock::Read
//=============================================================================
// Description:
//		Reads complete file from archive into memory buffer.
//
// Parameters:
//		_pcFileName - Name of file to read
//		_pPtr - Output buffer (must be pre-allocated to file size)
//
// Returns:
//		true - File read successfully
//		false - File not found in archive
//
// Algorithm:
//		1. Use hash table to find CInfoFile by filename (O(1) lookup)
//		2. Follow cluster chain from FirstCluster
//		3. For each cluster:
//		   - Seek to cluster offset (iNext + 4 bytes)
//		   - Read cluster data into buffer
//		   - Advance buffer pointer
//		   - Move to next cluster
//		4. Concatenate all clusters into contiguous buffer
//
// Example (fragmented file):
//		File "player.sav": 10KB total
//			Cluster 1: offset=100, size=4096 -> Read to _pPtr[0-4095]
//			Cluster 2: offset=8000, size=4096 -> Read to _pPtr[4096-8191]
//			Cluster 3: offset=15000, size=1808 -> Read to _pPtr[8192-9999]
//
// Notes:
//		- Caller must allocate buffer (use GetSize to determine size)
//		- Handles fragmented files transparently
//		- Hash table provides fast O(1) lookup
//		- +4 offset skips archive header when seeking
//
//=============================================================================
bool CSaveBlock::Read(char * _pcFileName, char * _pPtr)
{
	CInfoFile * _pInfoFile = (CInfoFile *)pHachage->GetPtrWithString(_pcFileName);

	if (!_pInfoFile)
	{
		return false;
	}


	CCluster * _pCluster = &_pInfoFile->FirstCluster;

	while (_pCluster)
	{
		fseek(hFile, _pCluster->iNext + 4, SEEK_SET);
		fread((void *)_pPtr, _pCluster->iTaille, 1, hFile);
		_pPtr += _pCluster->iTaille;
		_pCluster = _pCluster->pNext;
	}

	return true;
}

//=============================================================================
// FUNCTION: CSaveBlock::GetSize
//=============================================================================
// Description:
//		Gets total size of file in archive.
//
// Parameters:
//		_pcFileName - Name of file to query
//
// Returns:
//		File size in bytes (sum of all clusters)
//		-1 if file not found
//
// Algorithm:
//		1. Lookup file by name (hash table if available, else linear search)
//		2. Iterate through cluster chain
//		3. Sum size of each cluster
//		4. Return total
//
// Notes:
//		- Uses hash table during read operations (fast)
//		- Falls back to linear search during write operations
//		- Returns total file size even if fragmented across clusters
//
//=============================================================================
int CSaveBlock::GetSize(char * _pcFileName)
{
	CInfoFile * _pInfoFile = NULL;

	if (pHachage)
	{
		_pInfoFile = (CInfoFile *)pHachage->GetPtrWithString(_pcFileName);

		if (!_pInfoFile)
		{
			return -1;
		}
	}
	else
	{
		bool _bFound = false;
		CInfoFile * _pInfoFile = sInfoFile;
		int _iI = iNbFiles;

		while (_iI--)
		{
			if (!stricmp((const char *)_pInfoFile->pcFileName, (const char *)_pcFileName))
			{
				_bFound = true;
				break;
			}

			_pInfoFile++;
		}

		if (!_bFound)
		{
			return -1;
		}
	}

	int _iTaille = 0;
	CCluster * _pCluster = &_pInfoFile->FirstCluster;

	while (_pCluster)
	{
		_iTaille += _pCluster->iTaille;
		_pCluster = _pCluster->pNext;
	}

	return _iTaille;
}

//=============================================================================
// FUNCTION: CSaveBlock::ExistFile
//=============================================================================
// Description:
//		Checks if file exists in archive.
//
// Parameters:
//		_pcFileName - Name of file to check
//
// Returns:
//		true - File exists in archive
//		false - File not found
//
// Algorithm:
//		If hash table available (read mode):
//			1. Lookup filename in hash table
//			2. Return true if found, false otherwise
//
//		If hash table not available (write mode):
//			1. Linear search through sInfoFile array
//			2. Case-insensitive comparison of filenames
//			3. Return true if match found
//
// Notes:
//		- Fast O(1) lookup during read operations (hash table)
//		- Slower O(n) lookup during write operations (linear search)
//		- Case-insensitive filename matching
//
//=============================================================================
bool CSaveBlock::ExistFile(char * _pcFileName)
{
	CInfoFile * _pInfoFile = NULL;

	if (pHachage)
	{
		_pInfoFile = (CInfoFile *)pHachage->GetPtrWithString(_pcFileName);
		return _pInfoFile ? true : false;
	}
	else
	{
		_pInfoFile = sInfoFile;
		int _iI = iNbFiles;

		while (_iI--)
		{
			if (!stricmp((const char *)_pInfoFile->pcFileName, (const char *)_pcFileName))
			{
				return true;
			}

			_pInfoFile++;
		}

		return false;
	}
}
