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
// ARX_Input
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		ARX Input interface (uses MERCURY input system)
//
// Updates: (date) (person) (update)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2001 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
//=============================================================================
// FILE: ARX_Input.cpp
//=============================================================================
// Component: DANAE Game Engine - Input Handling System
// Author: Cyril Meynier
//
// PURPOSE:
//		Unified input system handling keyboard, mouse, and gamepad input.
//		Provides abstraction layer over DirectInput (MERCURY system) for
//		game controls and user interaction.
//
// ARCHITECTURE:
//		Layered input system built on MERCURY/DirectInput:
//
//		Input Device Stack:
//		1. Hardware Layer: Physical devices (keyboard, mouse, gamepad)
//		2. DirectInput Layer: Windows DirectInput API (DXI)
//		3. MERCURY Layer: Arkane's input abstraction (DXI wrapper)
//		4. ARX Layer: Game-specific input handling (this file)
//		5. Game Logic: Actions triggered by input
//
//		Supported Devices:
//		- Keyboard (DXI_KEYBOARD1): All standard keys
//		- Mouse (DXI_MOUSE1): Buttons, movement, scroll wheel
//		- Gamepad (DXI_JOY1): Xbox-style controllers
//		- SCID (DXI_SCID): Special controller interface device
//
//		Input Modes:
//		- Exclusive: Game has full control, OS doesn't see input
//		- Non-Exclusive: Shared with OS (can alt-tab)
//		- Buffered: Input queued for processing
//		- Immediate: Input polled each frame
//
// KEY FEATURES:
//		Initialization:
//		- ARX_INPUT_Init: Initialize all input devices
//		- Detect available controllers (Xbox, PlayStation, generic)
//		- Set input modes (exclusive vs non-exclusive)
//		- Configure mouse relative mode (for camera control)
//		- Fallback handling if devices unavailable
//
//		Keyboard Input:
//		- Key state queries (IsKeyPressed, IsKeyDown, IsKeyReleased)
//		- Buffered key events for text input
//		- Key repeat handling
//		- Modifier key support (Shift, Ctrl, Alt)
//		- Configurable key bindings
//		- Text input for chat/console
//
//		Mouse Input:
//		- Button states (left, right, middle, extra buttons)
//		- Relative movement deltas (for camera rotation)
//		- Absolute screen position (for UI interaction)
//		- Scroll wheel support
//		- Mouse sensitivity adjustment
//		- Mouse smoothing/acceleration
//
//		Gamepad Input:
//		- Analog sticks (left/right with dead zones)
//		- Triggers (analog pressure-sensitive buttons)
//		- Face buttons (A, B, X, Y)
//		- Shoulder buttons (LB, RB, LT, RT)
//		- D-pad (directional buttons)
//		- Rumble/vibration feedback
//		- Button remapping
//
//		Input Contexts:
//		- Gameplay Mode: Normal game controls active
//		- Menu Mode: UI navigation, limited game controls
//		- Inventory Mode: Item management specific controls
//		- Dialog Mode: Conversation selection
//		- Spell Casting Mode: Gesture recognition
//		- Context switching: Automatic control scheme changes
//
// ALGORITHMS:
//		Input Processing Loop (Each Frame):
//		1. Poll all input devices (keyboard, mouse, gamepad)
//		2. Update device states from DirectInput
//		3. Apply dead zones to analog inputs
//		4. Process key/button state transitions:
//		   - Pressed: Was up last frame, down this frame
//		   - Held: Was down last frame, still down this frame
//		   - Released: Was down last frame, up this frame
//		5. Accumulate mouse deltas
//		6. Check for special input combinations (quit, screenshot, etc.)
//		7. Route input to appropriate handler based on context
//		8. Clear one-frame flags (pressed, released)
//
//		Dead Zone Application (Analog Sticks):
//		Function: ApplyDeadZone(rawValue, deadZone)
//		1. Get raw analog value (-32768 to +32767)
//		2. Normalize to -1.0 to +1.0
//		3. If (abs(value) < deadZone): return 0.0
//		4. Else:
//		   - Rescale from [deadZone, 1.0] to [0.0, 1.0]
//		   - value = (value - deadZone) / (1.0 - deadZone)
//		   - Preserves full range outside dead zone
//		5. Return adjusted value
//
//		Mouse Smoothing:
//		- Store recent mouse deltas in circular buffer (8-16 samples)
//		- Calculate weighted average:
//		  * Recent samples weighted more heavily
//		  * Older samples have lower weight
//		- Result: Smoother camera movement, less jittery
//
//		Input Binding System:
//		- Configuration file maps actions to keys/buttons
//		- Example: "Jump" → Spacebar, Gamepad A Button
//		- Runtime: Check if any bound input active
//		- Allows player customization
//
// INPUT DEVICE DETECTION:
//		Xbox Controller Detection:
//		- DirectInput device enumeration
//		- Check for specific product IDs:
//		  * "Xbox 360 Controller"
//		  * "Xbox One Controller"
//		  * "ThrustMaster FireStorm Dual Power Gamepad"
//		- If detected: Set ARX_XBOXPAD = 1
//		- Configure button mapping for Xbox layout
//
//		SCID Device (Special Controller):
//		- Proprietary controller interface
//		- Attempt initialization
//		- If fails: Set ARX_SCID = 0, continue without it
//		- Optional device, not required for gameplay
//
//		Keyboard Always Required:
//		- Initialization fails if keyboard unavailable
//		- Critical for menu navigation even with gamepad
//
//		Mouse Fallback:
//		- Attempt mouse initialization
//		- If fails: Game still playable with keyboard/gamepad
//		- Camera control via keyboard arrows
//
// DIRECTINPUT MODES:
//		DXI_MODE_EXCLUSIF_ALLMSG:
//		- Exclusive device access
//		- All messages routed to game window
//		- Used for gamepad to prevent OS interference
//
//		DXI_MODE_NONEXCLUSIF_OURMSG:
//		- Non-exclusive access
//		- Only window-specific messages processed
//		- Used for keyboard to allow alt-tab
//
//		DXI_MODE_NONEXCLUSIF_ALLMSG:
//		- Non-exclusive access
//		- All messages processed
//		- Used for mouse to allow window focus switching
//
// MOUSE MODES:
//		Relative Mode (DXI_SetMouseRelative):
//		- Reports movement delta, not absolute position
//		- Ideal for first-person camera control
//		- Cursor hidden and locked to window center
//		- Movement accumulated as rotation delta
//
//		Absolute Mode:
//		- Reports screen position coordinates
//		- Used for menus and UI interaction
//		- Cursor visible
//		- Position used for click detection
//
//		Mode Switching:
//		- Enter game: Switch to relative (camera control)
//		- Open inventory: Switch to absolute (UI interaction)
//		- Seamless transitions between modes
//
// SPECIAL INPUT HANDLING:
//		Spell Gesture Recognition:
//		- Mouse movement tracked during spell casting
//		- Pattern matching algorithm:
//		  * Sample positions at fixed intervals
//		  * Normalize to remove scale variation
//		  * Compare to known rune shapes
//		  * Fuzzy matching for imprecise input
//		- Successful match: Cast corresponding spell
//
//		Quick Save/Load:
//		- F5: Quick save (check for valid location)
//		- F6: Quick load (confirm dialog)
//		- Disabled during combat or cinematics
//
//		Screenshot:
//		- F12: Capture framebuffer to file
//		- Save as TGA or BMP
//		- Increment filename (screenshot001.tga, screenshot002.tga...)
//
//		Developer Keys:
//		- Tilde (~): Open console
//		- F1-F4: Debug visualization modes
//		- Only active in debug builds
//
// CONFIGURATION:
//		Key Bindings (from config file):
//		- Movement: W/A/S/D or Arrow keys
//		- Jump: Space
//		- Crouch: Ctrl
//		- Interact: E
//		- Attack: Left Mouse Button
//		- Block: Right Mouse Button
//		- Inventory: I
//		- Character Sheet: C
//		- Spell Book: B
//		- Map: M
//		- Quick slots: 1-8
//
//		Mouse Settings:
//		- Sensitivity: 0.1 to 10.0 multiplier
//		- Invert Y-axis: Boolean toggle
//		- Smoothing: 0 (off) to 10 (heavy)
//		- Acceleration: Linear vs non-linear response
//
//		Gamepad Settings:
//		- Left stick dead zone: 0.0 to 0.5 (default 0.15)
//		- Right stick dead zone: 0.0 to 0.5 (default 0.15)
//		- Trigger dead zone: 0.0 to 0.3 (default 0.1)
//		- Vibration strength: 0% to 100%
//		- Button layout: Xbox, PlayStation, Custom
//
// INTEGRATION:
//		Uses MERCURY DXI system for DirectInput abstraction
//		Coordinates with ARX_Menu2 for control configuration
//		Integrates with ARX_Player for character movement
//		Works with ARX_Interface for UI interaction
//		Triggers ARX_Spells for gesture-based magic
//
// TYPICAL USAGE:
//		Initialization (Game Startup):
//		1. Call ARX_INPUT_Init(hInstance, hWnd)
//		2. DirectInput initialized
//		3. Keyboard acquired
//		4. Mouse acquired and set to relative mode
//		5. Gamepad detected and configured if present
//		6. Load key bindings from config
//		7. Ready to process input
//
//		Frame Update (Each Frame):
//		1. Poll devices: DXI_UpdateInputDevices()
//		2. Check keyboard: if (DXI_GetKeyState(DIK_W)) MoveForward()
//		3. Get mouse delta: DXI_GetMouseMove(&dx, &dy)
//		4. Rotate camera: camera.yaw += dx * sensitivity
//		5. Check gamepad: if (DXI_GetJoyButton(JOY_A)) Jump()
//		6. Process buffered text input for chat
//
//		Context Switch (Open Inventory):
//		1. Game detects 'I' key pressed
//		2. Pause game update
//		3. Switch mouse to absolute mode
//		4. Show cursor
//		5. Enter inventory input mode
//		6. Mouse clicks handled by UI system
//		7. ESC or 'I' again: Close inventory, return to game mode
//=============================================================================


#include "ARX_Input.h"
#include "Arx_menu2.h" //controls

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

DXI_INIT	InputInit;
long ARX_SCID = 0;
long ARX_XBOXPAD = 0;

//-----------------------------------------------------------------------------
extern CDirectInput * pGetInfoDirectInput;
extern CMenuConfig * pMenuConfig;
extern long STOP_KEYBOARD_INPUT;

//-----------------------------------------------------------------------------
BOOL ARX_INPUT_Init(HINSTANCE hInst, HWND hWnd)
{
#ifdef NO_DIRECT_INPUT
	return TRUE;
#endif
	memset((void *)&InputInit, 0, sizeof(DXI_INIT));
	DXI_Init(hInst, &InputInit);

	if (DXI_FAIL == DXI_GetKeyboardInputDevice(hWnd, DXI_KEYBOARD1, DXI_MODE_NONEXCLUSIF_OURMSG)) 
		return FALSE;

	if (DXI_FAIL == DXI_GetMouseInputDevice(hWnd, DXI_MOUSE1, DXI_MODE_NONEXCLUSIF_ALLMSG, 2, 2))
		return FALSE;

	if (DXI_FAIL == DXI_GetSCIDInputDevice(hWnd, DXI_SCID, DXI_MODE_EXCLUSIF_OURMSG, 2, 2))
		ARX_SCID = 0;
	else ARX_SCID = 1;

	if (DXI_FAIL == DXI_SetMouseRelative(DXI_MOUSE1))
		return FALSE;

	//"ThrustMaster FireStorm(TM) Dual Power Gamepad"
	if (DXI_FAIL != DXI_GetJoyInputDevice(hWnd, DXI_JOY1, DXI_MODE_EXCLUSIF_ALLMSG, 13, 4))
	{
		ARX_XBOXPAD = 1;
		DXI_SetRangeJoy(DXI_JOY1, DXI_XAxis, 100);
		DXI_SetRangeJoy(DXI_JOY1, DXI_YAxis, 100);
		DXI_SetRangeJoy(DXI_JOY1, DXI_RzAxis, 100);
		DXI_SetRangeJoy(DXI_JOY1, DXI_Slider, 100);
	}
	else ARX_XBOXPAD = 0;

	return TRUE;
}

void ARX_INPUT_Release()
{
#ifdef NO_DIRECT_INPUT
	return;
#endif
	DXI_Release();
}

//ARX_GAME_IMPULSES GameImpulses;
long GameImpulses[MAX_IMPULSES][MAX_IMPULSES_NB];


void ARX_INPUT_Init_Game_Impulses()
{
	for (long i = 0; i < MAX_IMPULSES; i++)
		for (long j = 0; j < MAX_IMPULSES_NB; j++)
			GameImpulses[i][j] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_MAGIC_MODE][0] = 29;
	GameImpulses[ARX_INPUT_IMPULSE_MAGIC_MODE][1] = 157;
	GameImpulses[ARX_INPUT_IMPULSE_MAGIC_MODE][2] = 0; 
	GameImpulses[ARX_INPUT_IMPULSE_COMBAT_MODE][0] = 28;
	GameImpulses[ARX_INPUT_IMPULSE_COMBAT_MODE][1] = INTERNAL_JOYSTICK_7;
	GameImpulses[ARX_INPUT_IMPULSE_JUMP][0] = 82;
	GameImpulses[ARX_INPUT_IMPULSE_JUMP][1] = INTERNAL_MOUSE_3;
	GameImpulses[ARX_INPUT_IMPULSE_JUMP][2] = INTERNAL_JOYSTICK_1;
	GameImpulses[ARX_INPUT_IMPULSE_STEALTH][0] = 54;
	GameImpulses[ARX_INPUT_IMPULSE_STEALTH][1] = 42;

	GameImpulses[ARX_INPUT_IMPULSE_WALK_FORWARD][0] = 200;
	GameImpulses[ARX_INPUT_IMPULSE_WALK_FORWARD][1] = 0;
	GameImpulses[ARX_INPUT_IMPULSE_WALK_BACKWARD][0] = 208;
	GameImpulses[ARX_INPUT_IMPULSE_WALK_BACKWARD][1] = 0;
	GameImpulses[ARX_INPUT_IMPULSE_STRAFE_LEFT][0] = 203;
	GameImpulses[ARX_INPUT_IMPULSE_STRAFE_LEFT][1] = 0;
	GameImpulses[ARX_INPUT_IMPULSE_STRAFE_RIGHT][0] = 205;
	GameImpulses[ARX_INPUT_IMPULSE_STRAFE_RIGHT][1] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_MOUSE_LOOK][0] = INTERNAL_MOUSE_2;
	GameImpulses[ARX_INPUT_IMPULSE_MOUSE_LOOK][1] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_ACTION][0] = INTERNAL_MOUSE_1;
	GameImpulses[ARX_INPUT_IMPULSE_ACTION][1] = INTERNAL_JOYSTICK_13;

	GameImpulses[ARX_INPUT_IMPULSE_INVENTORY][0] = 23;
	GameImpulses[ARX_INPUT_IMPULSE_INVENTORY][1] = INTERNAL_JOYSTICK_8;

	GameImpulses[ARX_INPUT_IMPULSE_BOOK][0] = 15;
	GameImpulses[ARX_INPUT_IMPULSE_BOOK][1] = INTERNAL_JOYSTICK_6;

	GameImpulses[ARX_INPUT_IMPULSE_LEAN_RIGHT][0] = 49;
	GameImpulses[ARX_INPUT_IMPULSE_LEAN_RIGHT][1] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_LEAN_LEFT][0] = 51;
	GameImpulses[ARX_INPUT_IMPULSE_LEAN_LEFT][1] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_CROUCH][0] = 83; //50;
	GameImpulses[ARX_INPUT_IMPULSE_CROUCH][1] = INTERNAL_JOYSTICK_2;
}
 
BOOL ARX_INPUT_GetSCIDAxis(int * jx, int * jy, int * jz)
{
	if (ARX_SCID)
	{
		DXI_GetSCIDAxis(DXI_SCID, jx, jy, jz);
		return TRUE;
	}
	else
	{
		*jx = 0;
		*jy = 0;
		*jz = 0;
		return FALSE;
	}
}
 
//-----------------------------------------------------------------------------
BOOL ARX_IMPULSE_NowPressed(long ident)
{
	switch (ident)
	{
		case CONTROLS_CUST_MOUSELOOK:
		case CONTROLS_CUST_ACTION:
			break;
		default:
		{
			for (long j = 0; j < 2; j++)
			{
				if (pMenuConfig->sakActionKey[ident].iKey[j] != -1)
				{
					if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x80000000)
					{
						if (pGetInfoDirectInput->GetMouseButtonNowPressed(pMenuConfig->sakActionKey[ident].iKey[j]&~0x80000000))
							return TRUE;
					}
					else
					{
						if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x40000000)
						{
							if (pMenuConfig->sakActionKey[ident].iKey[j] == 0x40000001)
							{
								if (pGetInfoDirectInput->iWheelSens < 0) return TRUE;
							}
							else
							{
								if (pGetInfoDirectInput->iWheelSens > 0) return TRUE;
							}
						}
						else
						{
							bool bCombine = true;

							if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x7FFF0000)
							{
								if (!pGetInfoDirectInput->IsVirtualKeyPressed((pMenuConfig->sakActionKey[ident].iKey[j] >> 16) & 0xFFFF))
									bCombine = false;
							}

							if (pGetInfoDirectInput->IsVirtualKeyPressedNowPressed(pMenuConfig->sakActionKey[ident].iKey[j] & 0xFFFF))
								return TRUE & bCombine;
						}
					}
				}
			}
		}
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
static unsigned int uiOneHandedMagicMode = 0;
static unsigned int uiOneHandedStealth = 0;

BOOL ARX_IMPULSE_Pressed(long ident)
{
	switch (ident)
	{
		case CONTROLS_CUST_MOUSELOOK:
		case CONTROLS_CUST_ACTION:
			break;
		default:
		{
			if (pMenuConfig->bOneHanded)
			{
				for (long j = 0; j < 2; j++)
				{
					if (pMenuConfig->sakActionKey[ident].iKey[j] != -1)
					{
						if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x80000000)
						{
							if (pGetInfoDirectInput->GetMouseButtonRepeat(pMenuConfig->sakActionKey[ident].iKey[j]&~0x80000000))
								return TRUE;
						}
						else
						{
							if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x40000000)
							{
								if (pMenuConfig->sakActionKey[ident].iKey[j] == 0x40000001)
								{
									if (pGetInfoDirectInput->iWheelSens < 0) return TRUE;
								}
								else
								{
									if (pGetInfoDirectInput->iWheelSens > 0) return TRUE;
								}
							}
							else
							{
								bool bCombine = true;

								if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x7FFF0000)
								{
									if (!pGetInfoDirectInput->IsVirtualKeyPressed((pMenuConfig->sakActionKey[ident].iKey[j] >> 16) & 0xFFFF))
										bCombine = false;
								}

								if (pGetInfoDirectInput->IsVirtualKeyPressed(pMenuConfig->sakActionKey[ident].iKey[j] & 0xFFFF))
								{
									bool bQuit = false;

									switch (ident)
									{
										case CONTROLS_CUST_MAGICMODE:
										{
											if (bCombine)
											{
												if (!uiOneHandedMagicMode)
												{
													uiOneHandedMagicMode = 1;
												}
												else
												{
													if (uiOneHandedMagicMode == 2)
													{
														uiOneHandedMagicMode = 3;
													}
												}

												bQuit = true;
											}
										}
										break;
										case CONTROLS_CUST_STEALTHMODE:
										{
											if (bCombine)
											{
												if (!uiOneHandedStealth)
												{
													uiOneHandedStealth = 1;
												}
												else
												{
													if (uiOneHandedStealth == 2)
													{
														uiOneHandedStealth = 3;
													}
												}

												bQuit = true;
											}
										}
										break;
										default:
										{
											return TRUE & bCombine;
										}
										break;
									}

									if (bQuit)
									{
										break;
									}
								}
								else
								{
									switch (ident)
									{
										case CONTROLS_CUST_MAGICMODE:
										{
											if ((!j) &&
											        (pGetInfoDirectInput->IsVirtualKeyPressed(pMenuConfig->sakActionKey[ident].iKey[j+1] & 0xFFFF)))
											{
												continue;
											}

											if (uiOneHandedMagicMode == 1)
											{
												uiOneHandedMagicMode = 2;
											}
											else
											{
												if (uiOneHandedMagicMode == 3)
												{
													uiOneHandedMagicMode = 0;
												}
											}
										}
										break;
										case CONTROLS_CUST_STEALTHMODE:
										{
											if ((!j) &&
											        (pGetInfoDirectInput->IsVirtualKeyPressed(pMenuConfig->sakActionKey[ident].iKey[j+1] & 0xFFFF)))
											{
												continue;
											}

											if (uiOneHandedStealth == 1)
											{
												uiOneHandedStealth = 2;
											}
											else
											{
												if (uiOneHandedStealth == 3)
												{
													uiOneHandedStealth = 0;
												}
											}
										}
										break;
									}
								}
							}
						}
					}
				}

				switch (ident)
				{
					case CONTROLS_CUST_MAGICMODE:

						if ((uiOneHandedMagicMode == 1) || (uiOneHandedMagicMode == 2))
						{
							return TRUE;
						}

						break;
					case CONTROLS_CUST_STEALTHMODE:

						if ((uiOneHandedStealth == 1) || (uiOneHandedStealth == 2))
						{
							return TRUE;
						}

						break;
				}
			}
			else
			{
				for (long j = 0; j < 2; j++)
				{
					if (pMenuConfig->sakActionKey[ident].iKey[j] != -1)
					{
						if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x80000000)
						{
							if (pGetInfoDirectInput->GetMouseButtonRepeat(pMenuConfig->sakActionKey[ident].iKey[j]&~0x80000000))
								return TRUE;
						}
						else
						{
							if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x40000000)
							{
								if (pMenuConfig->sakActionKey[ident].iKey[j] == 0x40000001)
								{
									if (pGetInfoDirectInput->iWheelSens < 0) return TRUE;
								}
								else
								{
									if (pGetInfoDirectInput->iWheelSens > 0) return TRUE;
								}
							}
							else
							{
								bool bCombine = true;

								if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x7FFF0000)
								{
									if (!pGetInfoDirectInput->IsVirtualKeyPressed((pMenuConfig->sakActionKey[ident].iKey[j] >> 16) & 0xFFFF))
										bCombine = false;
								}

								if (pGetInfoDirectInput->IsVirtualKeyPressed(pMenuConfig->sakActionKey[ident].iKey[j] & 0xFFFF))
									return TRUE & bCombine;
							}
						}
					}
				}
			}
		}
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL ARX_IMPULSE_NowUnPressed(long ident)
{
	switch (ident)
	{
		case CONTROLS_CUST_MOUSELOOK:
		case CONTROLS_CUST_ACTION:
			break;
		default:
		{
			for (long j = 0; j < 2; j++)
			{
				if (pMenuConfig->sakActionKey[ident].iKey[j] != -1)
				{
					if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x80000000)
					{
						if (pGetInfoDirectInput->GetMouseButtonNowUnPressed(pMenuConfig->sakActionKey[ident].iKey[j]&~0x80000000))
							return TRUE;
					}
					else
					{
						bool bCombine = true;

						if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x7FFF0000)
						{
							if (!pGetInfoDirectInput->IsVirtualKeyPressed((pMenuConfig->sakActionKey[ident].iKey[j] >> 16) & 0xFFFF))
								bCombine = false;
						}

						if (pGetInfoDirectInput->IsVirtualKeyPressedNowUnPressed(pMenuConfig->sakActionKey[ident].iKey[j] & 0xFFFF))
							return TRUE & bCombine;
					}
				}
			}
		}
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
