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
// HERMES_pack.cpp - In-Memory Directory Tree for PAK File Construction
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Implements hierarchical directory tree structure used during PAK file creation.
//		Organizes files and folders in memory before serializing to PAK archive format.
//
// Purpose:
//		Provides in-memory representation of file system hierarchy for:
//		- Building PAK archives (organizing files before packing)
//		- Directory navigation during resource loading
//		- File lookup with optional hash table acceleration
//
// Key Classes:
//
//		EVE_TFILE - File Entry
//			Represents a single file within the directory tree
//			Contains: name, size, parameters, linked list pointers
//			Forms doubly-linked list of files within a directory
//
//		EVE_REPERTOIRE - Directory/Folder Entry
//			Represents a directory node in the tree
//			Contains: subdirectories (children), files, parent link
//			Tree structure: parent/child hierarchy + sibling linked list
//			Optional hash table (CHachageString) for fast file lookup
//
// Data Structure:
//		Directory Tree Structure:
//			- Each directory has parent pointer (except root)
//			- Subdirectories form doubly-linked list (brother prev/next)
//			- Files in directory form doubly-linked list (file prev/next)
//			- Recursive tree traversal for path resolution
//
// Path Handling:
//		- Backslash separator ('\') used for Windows paths
//		- Case-insensitive path comparisons (stricmp)
//		- Recursive path parsing (e.g., "graphics\textures\wall.jpg")
//		- Path component extraction utilities
//
// Memory Management:
//		- Manual new/delete (no smart pointers - 2002 codebase)
//		- Recursive deletion of entire directory trees
//		- Careful linked list node removal
//
// Code: Mickael Pointier/Cyril Meynier
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include "HERMES_pack.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>



//#############################################################################
//#############################################################################
//                             STATIC HELPER FUNCTIONS
//#############################################################################
//#############################################################################
static EVE_U8 * GetDirName(EVE_U8 * dirplusname);
static EVE_U8 * GetFileName(EVE_U8 * dirplusname);
static EVE_U8 * GetFirstDir(EVE_U8 * dir, EVE_U32 * l);

//#############################################################################
//#############################################################################
//                             EVE_TFILE - File Entry Class
//#############################################################################
//#############################################################################

//=============================================================================
// FUNCTION: EVE_TFILE::EVE_TFILE (Constructor)
//=============================================================================
// Description:
//		Constructs a file entry node for the directory tree.
//
// Parameters:
//		dir - Directory path (unused, kept for interface compatibility)
//		n - Filename string
//
// Notes:
//		- Allocates and copies filename string
//		- Initializes all fields to zero/NULL
//		- Creates doubly-linked list node (fprev/fnext)
//		- param/param2/param3 used for file metadata (offset, size, etc.)
//
//=============================================================================
EVE_TFILE::EVE_TFILE(EVE_U8 * dir, EVE_U8 * n)
{
	this->name = NULL;
	this->taille = 0;
	this->param = this->param2 = this->param3 = 0;
	this->fprev = this->fnext = NULL;

	if (n)
	{
		this->name = new EVE_U8[strlen((const EVE_8 *)n)+1];
		strcpy((EVE_8 *)this->name, (const EVE_8 *)n);
		return;
	}
}

// EVE_TFILE::~EVE_TFILE - Destructor
//		Frees filename string and removes self from doubly-linked list
EVE_TFILE::~EVE_TFILE()
{
	if (this->name) delete[] this->name;

	if (this->fprev)
	{
		this->fprev->fnext = this->fnext;
	}

	if (this->fnext)
	{
		this->fnext->fprev = this->fprev;
	}
}

//#############################################################################
//#############################################################################
//                             EVE_REPERTOIRE - Directory/Folder Class
//#############################################################################
//#############################################################################

// EVE_REPERTOIRE::EVE_REPERTOIRE - Constructor
//		p: Parent directory (NULL for root)
//		n: Directory path (may contain nested dirs like "graphics\textures")
//		Recursively creates directory tree by parsing path components
//		Hash table disabled (pHachage = NULL) for memory efficiency
EVE_REPERTOIRE::EVE_REPERTOIRE(EVE_REPERTOIRE * p, EVE_U8 * n)
{
	pHachage = NULL; //new CHachageString(4096);
	this->param = 0;
	this->brotherprev = this->brothernext = NULL;
	this->parent = p;
	this->fils = NULL;
	this->fichiers = NULL;
	this->nbsousreps = this->nbfiles = 0;

	if (n)
	{
		EVE_U32 l;
		this->name = GetFirstDir(n, &l);

		if (this->name)
		{
			if (l != strlen((const char *)n))
			{
				this->nbsousreps = 1;
				this->fils = new EVE_REPERTOIRE(this, n + l);
			}

			return;
		}
	}

	this->name = NULL;
}

// EVE_REPERTOIRE::~EVE_REPERTOIRE - Destructor
//		Recursively deletes entire directory tree (files and subdirectories)
//		Frees hash table if allocated
//		Deletes all files in this directory
//		Recursively deletes all subdirectories
EVE_REPERTOIRE::~EVE_REPERTOIRE()
{
	if (this->name)
	{
		delete[] this->name;
	}

	if (pHachage)
	{
		delete pHachage;
		pHachage = NULL;
	}

	EVE_TFILE * f = fichiers;

	while (nbfiles--)
	{
		EVE_TFILE * fnext = f->fnext;
		delete f;
		f = fnext;
	}

	fichiers = NULL;

	EVE_REPERTOIRE * r = this->fils;
	EVE_U32 nb = this->nbsousreps;

	while (nb--)
	{
		EVE_REPERTOIRE * rnext = r->brothernext;
		delete r;
		r = rnext;
	}
}

// EVE_REPERTOIRE::AddSousRepertoire - Add subdirectory to tree
//		sname: Directory path (can be nested like "graphics\textures")
//		Recursively creates directory hierarchy
//		Reuses existing directories if name matches (case-insensitive)
void EVE_REPERTOIRE::AddSousRepertoire(EVE_U8 * sname)
{
	EVE_U32			nbs = this->nbsousreps, l;
	EVE_REPERTOIRE	* rf = this->fils;

	EVE_U8 * fdir = GetFirstDir(sname, &l);

	while (nbs--)
	{
		if (!stricmp((const EVE_8 *)fdir, (const EVE_8 *)rf->name))
		{
			delete[] fdir;
			rf->AddSousRepertoire(sname + l);
			return;
		}

		rf = rf->brothernext;
	}

	delete[] fdir;
	this->nbsousreps++;
	rf = new EVE_REPERTOIRE(this, sname);
	rf->brotherprev = NULL;
	rf->brothernext = this->fils;

	if (this->fils) this->fils->brotherprev = rf;

	this->fils = rf;
}

// EVE_REPERTOIRE::DelSousRepertoire - Remove subdirectory from tree
//		sname: Directory path to delete
//		Returns: TRUE if deleted, FALSE if not found
//		Recursively navigates path, deletes target directory
//		Updates sibling linked list pointers
BOOL EVE_REPERTOIRE::DelSousRepertoire(EVE_U8 * sname)
{
	EVE_U32			nbs = this->nbsousreps, l;
	EVE_REPERTOIRE	* rf = this->fils;

	EVE_U8 * fdir = GetFirstDir(sname, &l);

	while (nbs--)
	{
		if (!stricmp((const EVE_8 *)fdir, (const EVE_8 *)rf->name))
		{
			delete[] fdir;
			BOOL ok;

			if (l == strlen((const EVE_8 *)sname))
			{
				ok = TRUE;
			}
			else
			{
				ok = rf->DelSousRepertoire(sname + l);
				return ok;
			}

			if (ok)
			{
				if (rf == this->fils)
				{
					this->fils = rf->brothernext;

					if (this->fils) this->fils->brotherprev = NULL;
				}
				else
				{
					rf->brotherprev->brothernext = rf->brothernext;

					if (rf->brothernext)
					{
						rf->brothernext->brotherprev = rf->brotherprev;
					}
				}

				this->nbsousreps--;
				delete rf;
			}

			return ok;
		}

		rf = rf->brothernext;
	}

	delete[] fdir;
	return FALSE;
}

// EVE_REPERTOIRE::GetSousRepertoire - Find subdirectory by path
//		sname: Directory path to find
//		Returns: Pointer to directory if found, NULL otherwise
//		Recursively navigates directory tree following path components
//		Case-insensitive name matching
EVE_REPERTOIRE * EVE_REPERTOIRE::GetSousRepertoire(EVE_U8 * sname)
{
	EVE_U32			nbs = this->nbsousreps, l;
	EVE_REPERTOIRE	* rf = this->fils;

	EVE_U8 * fdir = GetFirstDir(sname, &l);

	while (nbs--)
	{
		if (!stricmp((const EVE_8 *)fdir, (const EVE_8 *)rf->name))
		{
			delete[] fdir;

			if (l == strlen((const EVE_8 *)sname))
			{
				return rf;
			}
			else
			{
				return rf->GetSousRepertoire(sname + l);
			}
		}

		rf = rf->brothernext;
	}

	delete[] fdir;
	return NULL;
}

// EVE_REPERTOIRE::AddFileToSousRepertoire - Add file to directory
//		sname: Directory path (NULL for this directory)
//		name: Filename
//		Returns: Pointer to new file entry, NULL if already exists or dir not found
//		Checks for duplicate filenames (case-insensitive)
//		Adds file to hash table if enabled
//		Inserts file at head of linked list
EVE_TFILE * EVE_REPERTOIRE::AddFileToSousRepertoire(EVE_U8 * sname, EVE_U8 * name)
{
	EVE_REPERTOIRE * r;

	if (!sname)
	{
		r = this;
	}
	else
	{
		r = this->GetSousRepertoire(sname);

		if (!r)
		{
			return NULL;
		}
	}

	EVE_TFILE * f = r->fichiers;
	EVE_U32 nb = r->nbfiles;

	while (nb--)
	{
		if (!stricmp((const EVE_8 *)f->name, (const EVE_8 *)name)) return NULL;

		f = f->fnext;
	}

	f = new EVE_TFILE(sname, name);

	if (!f) return NULL;

	f->fprev = NULL;
	f->fnext = r->fichiers;

	if (f->fnext) f->fnext->fprev = f;

	r->fichiers = f;
	r->nbfiles++;

	//on l'insert dans la table de hachage
	if (r->pHachage)
	{
		r->pHachage->AddString((char *)name, (void *)f);
	}

	return f;
}

// EVE_REPERTOIRE::ConstructFullNameRepertoire - Build full directory path
//		t: Output buffer for path string
//		Recursively walks parent chain to build complete path
//		Example: "graphics\textures\stone\"
void EVE_REPERTOIRE::ConstructFullNameRepertoire(char * t)
{
	if (parent)
	{
		parent->ConstructFullNameRepertoire(t);
	}

	if (name)
	{
		strcat((char *)t, (const char *)name);
	}
}

// Kill - Recursive directory tree deletion
//		r: Root directory to delete
//		Deletes all files and subdirectories recursively
//		Alternative to destructor for external cleanup
void Kill(EVE_REPERTOIRE * r)
{
	EVE_TFILE * f = r->fichiers;

	while (r->nbfiles--)
	{
		EVE_TFILE * fnext = f->fnext;
		delete f;
		f = fnext;
	}

	r->fichiers = NULL;

	EVE_REPERTOIRE * brep = r->fils;
	int nb = r->nbsousreps;

	while (nb--)
	{
		EVE_REPERTOIRE * brepnext = brep->brothernext;
		Kill(brep);
		brep = brepnext;
	}

	r->fils = NULL;
	delete r;
}

// GetFirstDir - Extract first directory component from path
//		dir: Path string (e.g., "graphics\textures\wall.jpg")
//		l: Output length of extracted component (including backslash)
//		Returns: Allocated string with first component (e.g., "graphics\")
//		Used for recursive path parsing
static EVE_U8 * GetFirstDir(EVE_U8 * dir, EVE_U32 * l)
{
	EVE_U8	* dirc = dir;

	*l = 1;

	while ((*dirc) &&
	        (*dirc != '\\'))
	{
		*l += 1;
		dirc++;
	}

	EVE_U8 * fdir = new EVE_U8[*l+1];

	if (!fdir) return NULL;

	strncpy((EVE_8 *)fdir, (const EVE_8 *)dir, *l);
	fdir[*l] = 0;

	return fdir;
}

// EVEF_GetDirName - Extract directory path from full file path
//		dirplusname: Full path (e.g., "graphics\textures\wall.jpg")
//		Returns: Allocated directory path (e.g., "graphics\textures")
//		Scans backwards from end to find last backslash
//		Returns NULL if no directory component (file in root)
EVE_U8 * EVEF_GetDirName(EVE_U8 * dirplusname)
{
	EVE_U32 l = strlen((const EVE_8 *)dirplusname);

	EVE_U8 * dir = new EVE_U8[l+1], *dirc = dir;

	if (!dir) return NULL;

	strcpy((EVE_8 *)dir, (const EVE_8 *)dirplusname);

	dirc += l;

	while ((l--) &&
	        (*dirc != '\\'))
	{
		dirc--;
	}

	if (l)
	{
		*dirc = 0;
	}
	else
	{
		delete[] dir;
		return NULL;
	}

	EVE_U8 * dirf = new EVE_U8[strlen((const EVE_8 *)dir)+1];

	if (!dirf)
	{
		delete[] dir;
		return NULL;
	}

	strcpy((EVE_8 *)dirf, (const EVE_8 *)dir);

	delete[] dir;
	return dirf;
}

// EVEF_GetFileName - Extract filename from full file path
//		dirplusname: Full path (e.g., "graphics\textures\wall.jpg")
//		Returns: Allocated filename string (e.g., "wall.jpg")
//		Scans backwards from end to find last backslash
//		Returns entire string if no directory component
EVE_U8 * EVEF_GetFileName(EVE_U8 * dirplusname)
{
	EVE_S32 l = strlen((const EVE_8 *)dirplusname);
	dirplusname += l;

	while ((l--) &&
	        (*dirplusname != '\\'))
	{
		dirplusname--;
	}

	if (l >= 0) dirplusname++;

	EVE_U8 * fname = new EVE_U8[strlen((const EVE_8 *)dirplusname)+1];

	if (!fname) return NULL;

	strcpy((EVE_8 *)fname, (const EVE_8 *)dirplusname);

	return fname;
}
