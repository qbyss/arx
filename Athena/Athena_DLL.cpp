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
#include <windows.h>

//////////////////////////////////////////////////////////////////////////////////////
// Athena_DLL.cpp - Athena Audio System DLL Entry Point & Stubs
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Minimal DLL entry point and stub functions for Athena audio system
//		Athena is the audio subsystem of Arx Fatalis, handling:
//		- 3D positional audio
//		- Music/ambient sound playback
//		- Sound effects mixing
//		- Audio streaming
//		- Environmental audio effects
//
// Purpose:
//		Provides DLL initialization when Athena is built as separate DLL
//		Contains stub implementations for error handling
//
// Notes:
//		ShowError() is a no-op stub - actual error handling likely in main exe
//		In final game, Athena compiled directly into executable (not as DLL)
//
// Code: Arkane Studios
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

//=============================================================================
// ShowError - Error Display Stub (No-Op)
//=============================================================================
// Description:
//		Stub implementation of error display function
//		Does nothing - actual error handling in main executable
//
// Parameters:
//		Unnamed parameters (not used in stub)
//
// Returns:
//		Always returns 0
//
// Notes:
//		Comment "//PABO" suggests this is temporary/placeholder code
//		Real error handling likely in HERMES (ShowError defined there)
//
//=============================================================================
int ShowError(char *, char *, long)
{
	return 0;		// No-op stub
}

//=============================================================================
// END OF FILE
//=============================================================================
