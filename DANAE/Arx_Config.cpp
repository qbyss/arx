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
// Arx_Config.CPP - Game Configuration and Settings Management
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Configuration dialog and settings management for Arx Fatalis
//		Handles game options, graphics settings, audio, and controls
//		Provides UI for player preferences and system configuration
//
// Purpose:
//		- Display configuration dialog before game launch
//		- Manage graphics settings (resolution, detail, effects)
//		- Handle audio configuration (volume, quality)
//		- Configure input/control settings
//		- Save/load user preferences
//		- Detect hardware capabilities
//
// CMenuConfig Class:
//		- Configuration menu interface
//		- Video mode selection
//		- Graphics quality options
//		- Audio settings
//		- Control binding
//
// Configuration Options:
//		Video:
//			- Screen resolution
//			- Color depth (16/32-bit)
//			- Fullscreen/windowed mode
//			- Detail level
//			- Texture quality
//			- Bump mapping on/off
//
//		Audio:
//			- Master volume
//			- Effects volume
//			- Music volume
//			- EAX/3D audio
//
//		Gameplay:
//			- Mouse sensitivity
//			- Invert mouse
//			- Difficulty
//			- Subtitles
//			- Language selection
//
// Settings Storage:
//		- Saved to registry or config file
//		- Loaded at game startup
//		- Applied before rendering init
//		- Persistent across sessions
//
// Hardware Detection:
//		- Graphics card capabilities
//		- DirectX version
//		- Available resolutions
//		- Audio device detection
//		- Special handling for ATI 8500
//
// Integration:
//		- Launched from game launcher
//		- Can be invoked from main menu
//		- Applies settings to danaeApp
//		- Interacts with ARX_Menu2
//
// Technical Notes:
//		- Windows dialog resource (danae_resource.h)
//		- Registry/INI file persistence
//		- DirectX enumeration for modes
//		- Special case for bGATI8500 flag
//
// Copyright (c) 1999-2010 ARKANE STUDIOS SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "arx_config.h"
#include "danae_resource.h"
#include "danae.h" // pour danaeApp
#include "ARX_Menu2.h"
#include <vector>
using namespace std;

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

extern CMenuConfig * pMenuConfig;
extern bool bGATI8500;

//-----------------------------------------------------------------------------
 
 
 
 
 
 
 
 

//BOOL CALLBACK ARX_CONFIG_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

//HWND hButtonTab[8];
 
 
 
 
 

 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 


 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
