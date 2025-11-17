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
// HERMESnet.cpp - Windows Registry Configuration Storage
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Simple registry read/write utilities for storing HERMES configuration.
//		Provides abstraction layer over Windows Registry API.
//
// Purpose:
//		Originally intended for storing network/multiplayer configuration:
//		- Server addresses
//		- Player preferences
//		- Game settings
//		- Resource pack locations
//
// Functions:
//		WriteRegKey() - Write string value to registry
//		WriteRegKeyValue() - Write DWORD value to registry
//		ReadRegKey() - Read string value from registry (with default)
//		ReadRegKeyValue() - Read DWORD value from registry
//
// Notes:
//		- Wraps Windows RegSetValueEx/RegQueryValueEx APIs
//		- Minimal error handling (returns S_OK even on failure for ReadReg functions)
//		- Uses TCHAR for potential Unicode support
//		- Registry keys must be opened by caller (HKEY parameter)
//
// Code: Mickael Pointier/Cyril Meynier
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "HERMESnet.h"
#include "HERMESMain.h"
#include "ResourceHERMESnet.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


//=============================================================================
// FUNCTION: WriteRegKey
//=============================================================================
// Description:
//		Writes a string value to the Windows registry.
//
// Parameters:
//		hKey - Open registry key handle (HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, etc.)
//		strName - Value name to write
//		strValue - String data to store
//
// Returns:
//		S_OK - Value written successfully
//		E_FAIL - Registry write failed
//
// Notes:
//		- Stores as REG_SZ (null-terminated string)
//		- Includes null terminator in stored data
//		- Caller must open/close registry key
//
//=============================================================================
HRESULT WriteRegKey( HKEY hKey, TCHAR* strName, TCHAR* strValue )
{
	LONG bResult;

	bResult = RegSetValueEx( hKey, strName, 0, REG_SZ, 
							 (LPBYTE) strValue, strlen(strValue) + 1 );
	if ( bResult != ERROR_SUCCESS )
		return E_FAIL;

    return S_OK;
}

//=============================================================================
// FUNCTION: WriteRegKeyValue
//=============================================================================
// Description:
//		Writes a DWORD (32-bit integer) value to the Windows registry.
//
// Parameters:
//		hKey - Open registry key handle
//		strName - Value name to write
//		val - DWORD value to store
//
// Returns:
//		S_OK - Value written successfully
//		E_FAIL - Registry write failed
//
// Notes:
//		- Stores as REG_DWORD (4-byte integer)
//		- Used for numeric settings (port numbers, flags, counts, etc.)
//
//=============================================================================
HRESULT WriteRegKeyValue( HKEY hKey, TCHAR* strName, DWORD val )
{
	LONG bResult;

	bResult = RegSetValueEx( hKey, strName, 0, REG_DWORD, 
							 (LPBYTE) &val, 4 );
	if ( bResult != ERROR_SUCCESS )
		return E_FAIL;

    return S_OK;
}

//=============================================================================
// FUNCTION: ReadRegKey
//=============================================================================
// Description:
//		Reads a string value from the Windows registry with default fallback.
//
// Parameters:
//		hKey - Open registry key handle
//		strName - Value name to read
//		strValue - Output buffer for string data
//		dwLength - Size of output buffer in bytes
//		strDefault - Default value if registry read fails
//
// Returns:
//		S_OK - Always (even on failure, uses default)
//
// Notes:
//		- Reads REG_SZ (null-terminated string) type
//		- Fills buffer with strDefault if value doesn't exist or read fails
//		- Always returns S_OK (minimal error handling)
//
//=============================================================================
HRESULT ReadRegKey( HKEY hKey, TCHAR* strName, TCHAR* strValue,
                    DWORD dwLength, TCHAR* strDefault )
{
	DWORD dwType;
	LONG bResult;

	bResult = RegQueryValueEx( hKey, strName, 0, &dwType, 
							 (LPBYTE) strValue, &dwLength );
	if ( bResult != ERROR_SUCCESS )
		strcpy( strValue, strDefault );

    return S_OK;
}

//=============================================================================
// FUNCTION: ReadRegKeyValue
//=============================================================================
// Description:
//		Reads a DWORD value from the Windows registry.
//
// Parameters:
//		hKey - Open registry key handle
//		strName - Value name to read
//		val - Output pointer for DWORD value
//		defaultt - Default value (currently unused - bug)
//
// Returns:
//		S_OK - Always (even on failure)
//
// Notes:
//		- Reads REG_DWORD (4-byte integer) type
//		- BUG: defaultt parameter unused, val not set on failure
//		- Always returns S_OK regardless of success
//
//=============================================================================
HRESULT ReadRegKeyValue( HKEY hKey, TCHAR* strName, long * val, long defaultt )
{
	DWORD dwType;
	unsigned long dwLength=4;
	LONG bResult;

	bResult = RegQueryValueEx( hKey, strName, 0, &dwType, 
							 (LPBYTE) val, &dwLength );
    return S_OK;
}
