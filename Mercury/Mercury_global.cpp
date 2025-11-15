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
// Mercury_global.cpp - DirectInput 7 Global Variable Definitions
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Global variable definitions for the Mercury DirectInput wrapper system.
//		This file declares all global state needed for DirectInput device management,
//		including the DirectInput interface, device arrays, and input buffers.
//
// Purpose:
//		Mercury is a lightweight abstraction layer around DirectInput 7 that provides
//		simplified input handling for keyboard, mouse, and joystick devices. This file
//		contains the shared global state used across the entire input system.
//
// Key Components:
//		- DirectInput 7 interface pointer (DI_DInput7)
//		- Device enumeration arrays (DI_InputInfo)
//		- Per-device-type state arrays (keyboard, mouse, joystick, SCID)
//		- Initialization configuration (DI_Init)
//		- Error handling (DI_Hr)
//
// Design Notes:
//		- Supports multiple devices per type (MAXKEYBOARD, MAXMOUSE, MAXJOY, MAXSCID)
//		- Uses INPUT_INFO structure to track device capabilities and state
//		- DirectInput 7 API (COM interface) - predates DirectInput 8+
//		- SCID = Microsoft SideWinder Strategic Commander (specialized device)
//
// Updates: (07-23-2010) (xrichter) (File extension change from .c to .cpp)
//
// Code:	Xavier RICHTER
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
/////////////////////////////////////////////////////////////////////////////////////


#include "Mercury_extern.h"

/*-----------------------------------------------------*/
// GLOBAL STATE VARIABLES FOR DIRECTINPUT SYSTEM
/*-----------------------------------------------------*/

// Error code storage for DirectInput operations
// All DI function calls store their HRESULT here for debugging/error handling
HRESULT			DI_Hr;

// Initialization configuration provided by the application
// Contains memory allocation functions and other setup parameters
DXI_INIT		DI_Init;

// Number of input devices enumerated during initialization
// Incremented for each device found (keyboard, mouse, joystick, etc.)
int				DI_NbInputInfo;

// Master array of all enumerated input devices (max 128 devices)
// Each entry contains device name, GUID, type, capabilities, and state
// Filled during DXI_Init() via device enumeration callback
INPUT_INFO		DI_InputInfo[128];

// Main DirectInput 7 COM interface pointer
// Created via DirectInputCreateEx() during initialization
// Used to enumerate devices and create device instances
IDirectInput7	*DI_DInput7;

/*-----------------------------------------------------*/
// PER-DEVICE-TYPE STATE ARRAYS
// These arrays point to entries in DI_InputInfo[] array
// Indexed by device ID (e.g., DXI_KEYBOARD1, DXI_MOUSE1)
/*-----------------------------------------------------*/

// Keyboard device array (supports up to MAXKEYBOARD keyboards)
// Points to INPUT_INFO->bufferstate (256-byte keyboard state buffer)
INPUT_INFO				*DI_KeyBoardBuffer[MAXKEYBOARD];

// Mouse device array (supports up to MAXMOUSE mice)
// Points to INPUT_INFO entries containing DIDEVICEOBJECTDATA arrays
// Commented-out old approach used direct DIDEVICEOBJECTDATA pointer
//DIDEVICEOBJECTDATA	*DI_MouseState[MAXMOUSE];
INPUT_INFO			*DI_MouseState[MAXMOUSE];

// Joystick device array (supports up to MAXJOY joysticks)
// Points to INPUT_INFO entries containing DIJOYSTATE/DIJOYSTATE2 data
INPUT_INFO			*DI_JoyState[MAXJOY];

// SCID device array (supports up to MAXSCID Strategic Commanders)
// SCID = Microsoft SideWinder Strategic Commander (rare gaming device)
// Treated as special joystick-like device with unique button layout
INPUT_INFO			*DI_SCIDState[MAXSCID];
/*------------------------------------- ----------------*/
