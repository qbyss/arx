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
// Mercury_Main.cpp - DirectInput 7 Wrapper Implementation
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Complete implementation of the Mercury DirectInput wrapper system.
//		Provides simplified, high-level access to DirectInput 7 for keyboard,
//		mouse, joystick, and specialized input devices (SCID).
//
// Purpose:
//		Mercury abstracts the complexity of DirectInput 7 COM interface calls,
//		providing a clean C-style API for input device management. It handles:
//		- Device enumeration and capability detection
//		- Device acquisition and configuration
//		- Buffered and immediate input state queries
//		- Multiple simultaneous devices of each type
//		- Error recovery and device restoration
//
// Key Features:
//		- Multi-device support (multiple keyboards, mice, joysticks)
//		- Automatic device enumeration at initialization
//		- Cooperative level management (exclusive/non-exclusive, foreground/background)
//		- Axis mode configuration (relative/absolute)
//		- Deadzone and range configuration for analog axes
//		- Button and axis state queries
//		- Buffered mouse input for precise tracking
//		- Support for rare devices (SideWinder Strategic Commander)
//
// API Overview:
//		Initialization:
//			DXI_Init() - Initialize DirectInput and enumerate devices
//			DXI_Release() - Shutdown DirectInput system
//
//		Device Acquisition:
//			DXI_GetKeyboardInputDevice() - Acquire keyboard
//			DXI_GetMouseInputDevice() - Acquire mouse with capability requirements
//			DXI_GetJoyInputDevice() - Acquire joystick with capability requirements
//			DXI_GetSCIDInputDevice() - Acquire Strategic Commander
//
//		Device Management:
//			DXI_ExecuteAllDevices() - Poll all devices and update state
//			DXI_RestoreAllDevices() - Re-acquire all devices after loss
//			DXI_SleepAllDevices() - Release all devices temporarily
//			DXI_ReleaseAllDevices() - Release all acquired devices
//
//		Keyboard Input:
//			DXI_KeyPressed() - Check if specific key is pressed
//			DXI_GetKeyIDPressed() - Get ID of first pressed key
//			DXI_ClearKeys() - Clear keyboard state buffer
//
//		Mouse Input:
//			DXI_GetAxeMouseXY() - Get X/Y mouse movement
//			DXI_GetAxeMouseXYZ() - Get X/Y/Z mouse movement (with wheel)
//			DXI_MouseButtonPressed() - Check if mouse button is pressed
//			DXI_MouseButtonUnPressed() - Check if mouse button is released
//			DXI_GetIDButtonPressed() - Get ID of first pressed mouse button
//			DXI_SetMouseRelative() - Set mouse to relative (delta) mode
//			DXI_SetMouseAbsolue() - Set mouse to absolute position mode
//
//		Joystick Input:
//			DXI_GetAxeJoyXY() - Get joystick X/Y axes with directional flags
//			DXI_GetAxeJoyXYZ() - Get joystick X/Y/Z axes
//			DXI_GetAxeJoyXYZW() - Get joystick X/Y/Z/W (slider) axes
//			DXI_GetJoyButtonPressed() - Check if joystick button is pressed
//			DXI_GetIDJoyButtonPressed() - Get ID of first pressed joystick button
//			DXI_SetJoyRelative() - Set joystick to relative mode
//			DXI_SetJoyAbsolue() - Set joystick to absolute mode
//			DXI_SetRangeJoy() - Configure axis range and deadzone
//
// DirectInput 7 Notes:
//		- Uses COM interface (IDirectInput7, IDirectInputDevice7)
//		- Requires explicit Acquire() before reading input
//		- Device loss requires re-acquisition (handled automatically)
//		- Buffered input available for mouse (DIDEVICEOBJECTDATA)
//		- Immediate input for keyboard and joystick (state snapshots)
//		- Legacy API predating DirectInput 8+ (which uses different device GUIDs)
//
// Historical Notes:
//		- Code converted from C to C++ in 2010 (COM vtable calls to direct calls)
//		- Comments marked "//Old :" show original C-style COM vtable syntax
//		- Originally supported custom memory allocators (now uses malloc/free)
//		- Some features disabled (old state comparison, mouse Z cleanup)
//
// Updates: (07-23-2010) (xrichter)		File extension change from .c to .cpp
//										Comments which start with //Old : are the old way to call directx 7 functions in C
//
// Code:	Xavier RICHTER
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
/////////////////////////////////////////////////////////////////////////////////////


#include "Mercury_extern.h"


#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

//-----------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------

// Extra buffer space for mouse input events (beyond buttons+axes)
// Prevents buffer overflow when many mouse events occur in single frame
#define INPUT_STATE_ADD	(512)

/*-------------------------------------------------------------*/
// OLD MEMORY ALLOCATOR FUNCTIONS (DISABLED)
// Originally allowed custom memory allocation, now uses standard malloc/free
/*-------------------------------------------------------------*/
/*static void * DXI_malloc(int t)
{
	return malloc(t);
}
/*-------------------------------------------------------------*/
/*static void * DXI_Realloc(void *mem,int t)
{
	return realloc(mem,t);
}*/
/*-------------------------------------------------------------*/
/*static void DXI_free(void *mem)
{
	free(mem);
}*/
/*-------------------------------------------------------------*/

//=============================================================================
// FUNCTION: DIEnumDevicesCallback
//=============================================================================
// Description:
//		DirectInput device enumeration callback function.
//		Called by DirectInput for each attached input device found on the system.
//		Stores device information (name, GUID, type) in DI_InputInfo[] array.
//
// Parameters:
//		lpddi - Pointer to DIDEVICEINSTANCE structure with device information
//				Contains device name, GUID, device type, and other properties
//		pvRef - User-defined reference data (unused, reserved for future use)
//
// Returns:
//		DIENUM_CONTINUE - Always continues enumeration to find all devices
//
// Algorithm:
//		1. Get next free slot in DI_InputInfo[] array
//		2. Allocate memory for device name string and copy it
//		3. Allocate memory for device GUID and copy it
//		4. Store device type (keyboard, mouse, joystick, etc.)
//		5. Increment device count (DI_NbInputInfo)
//		6. Return DIENUM_CONTINUE to enumerate next device
//
// Notes:
//		- Memory allocated here is freed in DXI_DeleteAllDevices()
//		- Maximum 128 devices supported (DI_InputInfo array size)
//		- If allocation fails, device is skipped (returns DIENUM_CONTINUE)
//		- Device GUID uniquely identifies this specific device instance
//		- Device type used to classify as keyboard/mouse/joystick/other
//
//=============================================================================
BOOL CALLBACK DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi,LPVOID pvRef)
{
	INPUT_INFO	*info;

	info=&DI_InputInfo[DI_NbInputInfo];
	memset((void*)info,0,sizeof(INPUT_INFO));

	//nom
	info->name=(char *)malloc(strlen(lpddi->tszInstanceName)+1);
	if(!info->name) return DIENUM_CONTINUE;
	strcpy((char*)info->name,(const char*)lpddi->tszInstanceName);

	//guid
	info->guid=(GUID*)malloc(sizeof(GUID));
	if(!info->guid)
	{
		free(info->name);
		return DIENUM_CONTINUE;
	}
	memcpy((void*)info->guid,(void*)&lpddi->guidInstance,sizeof(GUID));

	//type
	info->type=lpddi->dwDevType;

	DI_NbInputInfo++;
	return DIENUM_CONTINUE;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_Init
//=============================================================================
// Description:
//		Initializes the DirectInput system and enumerates all attached devices.
//		This is the first function that must be called before using Mercury.
//
// Parameters:
//		h - Application instance handle (HINSTANCE) from WinMain
//		i - Pointer to DXI_INIT structure with configuration options
//
// Returns:
//		DXI_OK (0) - Initialization successful
//		DXI_FAIL - Initialization failed (null handle, DirectInput creation failed)
//
// Algorithm:
//		1. Validate application instance handle
//		2. Copy initialization configuration to global DI_Init
//		3. Create DirectInput 7 COM interface via DirectInputCreateEx()
//		4. Enumerate all attached devices via EnumDevices() callback
//		5. Initialize device state arrays to NULL
//
// Notes:
//		- Requires valid HINSTANCE from application's WinMain
//		- Uses DIRECTINPUT_VERSION and IID_IDirectInput7 for DX7 interface
//		- DIEDFL_ATTACHEDONLY flag enumerates only currently connected devices
//		- All device arrays (keyboard, mouse, joystick) initialized to NULL
//		- DI_InputInfo[] filled during enumeration callback
//		- Must call DXI_Release() to cleanup when done
//
// Example Usage:
//		DXI_INIT init;
//		if (DXI_Init(hInstance, &init) == DXI_OK) {
//			// DirectInput ready to use
//		}
//
//=============================================================================
int DXI_Init(HINSTANCE h,DXI_INIT *i)
{
int	nb;

	if(!h) return DXI_FAIL;

	DI_DInput7=NULL;

	memcpy((void*)&DI_Init,(void*)i,sizeof(DXI_INIT));
/*	if(!DI_Init.malloc||!DI_Init.realloc||!DI_Init.free)
	{
//		DI_Init.malloc=DXI_malloc;
//		DI_Init.realloc=DXI_Realloc;
//		DI_Init.free=DXI_free;
	}
*/
	//Old : if(FAILED(DI_Hr=DirectInputCreateEx(h,DIRECTINPUT_VERSION,&IID_IDirectInput7,&DI_DInput7,NULL))) return DXI_FAIL;
	if(FAILED(DI_Hr=DirectInputCreateEx(h,DIRECTINPUT_VERSION,IID_IDirectInput7,(void**)&DI_DInput7,NULL))) return DXI_FAIL;

	DI_NbInputInfo=0;
	//Old : if(FAILED(DI_Hr=DI_DInput7->lpVtbl->EnumDevices(DI_DInput7,0,DIEnumDevicesCallback,NULL,DIEDFL_ATTACHEDONLY))) return DXI_FAIL;
	if(FAILED(DI_Hr=DI_DInput7->EnumDevices(0,DIEnumDevicesCallback,NULL,DIEDFL_ATTACHEDONLY))) return DXI_FAIL;

	nb=MAXKEYBOARD;
	while(nb--) DI_KeyBoardBuffer[nb]=NULL;
	nb=MAXMOUSE;
	while(nb--) DI_MouseState[nb]=NULL;
	nb=MAXJOY;
	while(nb--) DI_JoyState[nb]=NULL;
	nb=MAXSCID;
	while(nb--) DI_SCIDState[nb]=NULL;

	return DXI_OK;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_ReleaseDevice
//=============================================================================
// Description:
//		Releases a single DirectInput device and frees its associated state memory.
//		Called when device is no longer needed or before reconfiguration.
//
// Parameters:
//		info - Pointer to INPUT_INFO structure for device to release
//
// Returns:
//		None (void)
//
// Algorithm:
//		1. Check if device is currently active (acquired)
//		2. Unacquire the device via DirectInput Unacquire() call
//		3. Release the IDirectInputDevice7 COM interface
//		4. Free device-specific state memory based on type:
//			- Mouse: Free DIDEVICEOBJECTDATA buffer
//			- Keyboard: Free 256-byte state buffer
//			- Joystick: Free DIJOYSTATE or DIJOYSTATE2 structure
//			- SCID: Free joystick state structure
//		5. Mark device as inactive
//
// Notes:
//		- Safe to call on already-released device (checks actif flag)
//		- Memory freed depends on device type (mouse/keyboard/joystick)
//		- Joysticks use either DIJOYSTATE (2 axes) or DIJOYSTATE2 (>2 axes)
//		- SCID (Strategic Commander) treated as special joystick type
//		- Device can be re-acquired after release if needed
//
//=============================================================================
void DXI_ReleaseDevice(INPUT_INFO *info)
{
	if(!info->actif) return;

	info->actif=DEVICENOACTIF;

	//Old : if(info->inputdevice7) info->inputdevice7->lpVtbl->Unacquire(info->inputdevice7);
	if(info->inputdevice7) info->inputdevice7->Unacquire();
	RELEASE(info->inputdevice7);
	info->inputdevice7=NULL;

	switch(GET_DIDEVICE_TYPE(info->type))
	{
	case DIDEVTYPE_MOUSE:
		if(info->mousestate)
		{
			free((void*)info->mousestate);
//			free((void*)info->old_mousestate);
			info->mousestate=NULL;
//			info->old_mousestate=NULL;
		}
		break;
	case DIDEVTYPE_KEYBOARD:
		if(info->bufferstate)
		{
			free((void*)info->bufferstate);
//			free((void*)info->old_bufferstate);
			info->bufferstate=NULL;
//			info->old_bufferstate=NULL;
		}
		break;
	case DIDEVTYPE_JOYSTICK:
		if(info->datasid==DFDIJOYSTICK)
		{
			if(info->joystate)
			{
				free((void*)info->joystate);
//				free((void*)info->old_joystate);
				info->joystate=NULL;
//				info->old_joystate=NULL;
			}
		}
		else
		{
			if(info->joystate2)
			{
				free((void*)info->joystate2);
				info->joystate2=NULL;
//				free((void*)info->old_joystate2);
//				info->old_joystate2=NULL;
			}
		}
		break;
	default:
	case DIDEVTYPE_DEVICE:
		if(info->datasid==DFDIJOYSTICK)
		{
			if(info->joystate)
			{
				free((void*)info->joystate);
//				free((void*)info->old_joystate);
				info->joystate=NULL;
//				info->old_joystate=NULL;
			}
		}
		else
		{
			if(info->joystate2)
			{
				free((void*)info->joystate2);
				info->joystate2=NULL;
//				free((void*)info->old_joystate2);
//				info->old_joystate2=NULL;
			}
		}
		break;
	}
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_ReleaseAllDevices
//=============================================================================
// Description:
//		Releases all currently acquired DirectInput devices.
//		Called when application loses focus or needs to temporarily release input.
//
// Returns:
//		None (void)
//
// Algorithm:
//		Iterate through all enumerated devices in DI_InputInfo[] array
//		and call DXI_ReleaseDevice() on each one.
//
// Notes:
//		- Does not free device enumeration data (name/GUID)
//		- Devices can be re-acquired after this call
//		- Typically used when application loses focus
//		- Complementary function: DXI_RestoreAllDevices()
//
//=============================================================================
void DXI_ReleaseAllDevices(void)
{
INPUT_INFO	*info;
int			nb;

	info=DI_InputInfo;
	nb=DI_NbInputInfo;
	while(nb)
	{
		DXI_ReleaseDevice(info);
		info++;
		nb--;
	}
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_DeleteAllDevices
//=============================================================================
// Description:
//		Deletes all enumerated devices and frees all associated memory.
//		Unlike DXI_ReleaseAllDevices, this also frees device info (name/GUID).
//
// Returns:
//		None (void)
//
// Algorithm:
//		For each device in DI_InputInfo[]:
//		1. Free device GUID memory
//		2. Free device name string memory
//		3. Release device via DXI_ReleaseDevice()
//		4. Decrement device count
//
// Notes:
//		- Frees all memory allocated during device enumeration
//		- After this call, devices must be re-enumerated via DXI_Init()
//		- Does NOT release DirectInput interface (use DXI_Release() for that)
//
//=============================================================================
void DXI_DeleteAllDevices(void)
{
INPUT_INFO	*info;

	info=DI_InputInfo;
	while(DI_NbInputInfo)
	{
		free(info->guid);
		info->guid=NULL;
		free(info->name);
		info->name=NULL;
		DXI_ReleaseDevice(info);
		info++;
		DI_NbInputInfo--;
	}
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_Release
//=============================================================================
// Description:
//		Complete shutdown of DirectInput system.
//		Releases all devices and the DirectInput interface itself.
//		This is the cleanup counterpart to DXI_Init().
//
// Returns:
//		None (void)
//
// Algorithm:
//		1. Delete all devices and free memory (same as DXI_DeleteAllDevices)
//		2. Release DirectInput 7 COM interface
//		3. Set interface pointer to NULL
//
// Notes:
//		- Should be called before application exit
//		- After this call, must call DXI_Init() again to use DirectInput
//		- Frees all resources allocated by Mercury system
//		- RELEASE() macro handles COM interface Release() and ref counting
//
//=============================================================================
void DXI_Release(void)
{
INPUT_INFO	*info;

	info=DI_InputInfo;
	while(DI_NbInputInfo)
	{
		free(info->guid);
		info->guid=NULL;
		free(info->name);
		info->name=NULL;
		DXI_ReleaseDevice(info);
		info++;
		DI_NbInputInfo--;
	}

	RELEASE(DI_DInput7);
	DI_DInput7 = NULL;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: CompareGUID
//=============================================================================
// Description:
//		Compares two DirectInput GUIDs for equality.
//		Used to match device GUIDs during enumeration and capability detection.
//
// Parameters:
//		g1 - Pointer to first GUID structure
//		g2 - Pointer to second GUID structure
//
// Returns:
//		TRUE - GUIDs are identical
//		FALSE - GUIDs are different
//
// Algorithm:
//		1. Compare GUIDs as array of 32-bit integers (fast comparison)
//		2. Handle remaining bytes (GUID size not multiple of 4)
//		3. Return FALSE if any byte differs, TRUE if all match
//
// Notes:
//		- GUID is 16 bytes (4 ints + potential extra bytes)
//		- Optimized for speed using integer comparison first
//		- Used to identify device capabilities (XAxis, YAxis, Button, etc.)
//		- DirectInput uses GUIDs to identify device object types
//
//=============================================================================
BOOL CompareGUID(GUID *g1,GUID *g2)
{
int		i,j,*m1,*m2;
char	*mm1,*mm2;

	i=sizeof(GUID);
	j=i&3;
	i>>=2;

	m1=(int*)g1;
	m2=(int*)g2;
	while(i)
	{
		if(*m1++!=*m2++) return FALSE;
		i--;
	}

	mm1=(char*)m1;
	mm2=(char*)m2;
	while(j)
	{
		if(*mm1++!=*mm2++) return FALSE;
		j--;
	}

	return TRUE;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DIEnumDeviceObjectsCallback
//=============================================================================
// Description:
//		DirectInput device object enumeration callback.
//		Called for each object (button, axis, key, POV) on a device.
//		Builds capability flags for the device (which axes/buttons it has).
//
// Parameters:
//		lpddoi - Pointer to DIDEVICEOBJECTINSTANCE with object information
//		pvRef - User data (pointer to INPUT_INFO structure being configured)
//
// Returns:
//		DIENUM_CONTINUE - Always continues to enumerate all objects
//
// Algorithm:
//		Compare object GUID to known types and set corresponding capability flags:
//		- GUID_XAxis, GUID_YAxis, GUID_ZAxis - Analog axes
//		- GUID_RxAxis, GUID_RyAxis, GUID_RzAxis - Rotation axes
//		- GUID_Slider - Slider control
//		- GUID_Button - Button
//		- GUID_Key - Keyboard key
//		- GUID_POV - Point-of-View hat switch
//		- GUID_Unknown - Unknown object type
//
// Notes:
//		- Capability flags stored in INPUT_INFO->info bitmask
//		- Used to determine device capabilities before configuration
//		- Called during DXI_ChooseInputDevice() setup
//
//=============================================================================
BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,LPVOID pvRef)
{
INPUT_INFO			*info;

	info=(INPUT_INFO *)pvRef;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_XAxis)) info->info|=DXI_XAxis;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_YAxis)) info->info|=DXI_YAxis;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_ZAxis)) info->info|=DXI_ZAxis;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_RxAxis)) info->info|=DXI_RxAxis;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_RyAxis)) info->info|=DXI_RyAxis;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_RzAxis)) info->info|=DXI_RzAxis;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_Slider)) info->info|=DXI_Slider;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_Button)) info->info|=DXI_Button;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_Key)) info->info|=DXI_Key;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_POV)) info->info|=DXI_POV;
	if(CompareGUID((GUID*)&lpddoi->guidType,(GUID*)&GUID_Unknown)) info->info|=DXI_Unknown;

	return DIENUM_CONTINUE;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_GetInputInfoWithState
//=============================================================================
// Description:
//		Finds the INPUT_INFO structure for a device given its state pointer.
//		Used to map from state arrays (DI_MouseState, DI_KeyBoardBuffer) back
//		to the full device information structure.
//
// Parameters:
//		state - Pointer to device state (from DI_MouseState/KeyBoardBuffer/JoyState)
//		type - Device type (DIDEVTYPE_MOUSE, DIDEVTYPE_KEYBOARD, DIDEVTYPE_JOYSTICK)
//
// Returns:
//		Pointer to INPUT_INFO structure if found
//		NULL if not found
//
// Algorithm:
//		Search through DI_InputInfo[] array for matching device type and state pointer
//
// Notes:
//		- For mouse/joystick: compares INPUT_INFO pointer directly
//		- For keyboard: compares bufferstate pointer
//		- Used internally to find device info when only state pointer is available
//
//=============================================================================
static INPUT_INFO * DXI_GetInputInfoWithState(void *state,int type)
{
int			nbdev;
INPUT_INFO	*info;

	info=DI_InputInfo;
	nbdev=DI_NbInputInfo;
	while(nbdev)
	{
		if(GET_DIDEVICE_TYPE(info->type)==type)
		{
			switch(type)	
			{
			case DIDEVTYPE_MOUSE:
//				if(info->mousestate==state) return info;
				if(state==info) return info;
				break;
			case DIDEVTYPE_KEYBOARD:
				if(info->bufferstate==state) return info;
				break;
			case DIDEVTYPE_JOYSTICK:
				if(state==info) return info;
				break;
			default:
			case DIDEVTYPE_DEVICE:
				break;
			}
		}
		info++;
		nbdev--;
	}

	return NULL;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_RestoreAllDevices
//=============================================================================
// Description:
//		Re-acquires all active DirectInput devices.
//		Called after device loss (e.g., application regains focus).
//
// Returns:
//		None (void)
//
// Algorithm:
//		For each active device in DI_InputInfo[], call Acquire() to regain control
//
// Notes:
//		- Only attempts to acquire devices marked as actif (previously acquired)
//		- Automatically called by DXI_ExecuteAllDevices() on device loss
//		- Counterpart to DXI_SleepAllDevices()
//		- Does not fail if Acquire() fails (device may not be ready yet)
//
//=============================================================================
void DXI_RestoreAllDevices(void)
{
int			nbdev;
INPUT_INFO	*info;

	info=DI_InputInfo;
	nbdev=DI_NbInputInfo;
	while(nbdev)
	{
		if(info->actif)
		{
			//Old : info->inputdevice7->lpVtbl->Acquire(info->inputdevice7);
			info->inputdevice7->Acquire();
		}
		info++;
		nbdev--;
	}
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_SleepAllDevices
//=============================================================================
// Description:
//		Unacquires all active DirectInput devices without releasing them.
//		Called when application loses focus but wants to keep devices configured.
//
// Returns:
//		None (void)
//
// Algorithm:
//		For each active device in DI_InputInfo[], call Unacquire() to release control
//
// Notes:
//		- Devices remain configured but cannot be read
//		- Allows other applications to use input devices
//		- Use DXI_RestoreAllDevices() to re-acquire devices
//		- Does not free device memory or configuration
//
//=============================================================================
void DXI_SleepAllDevices(void)
{
int			nbdev;
INPUT_INFO	*info;

	info=DI_InputInfo;
	nbdev=DI_NbInputInfo;
	while(nbdev)
	{
		if(info->actif)
		{
			//Old : info->inputdevice7->lpVtbl->Unacquire(info->inputdevice7);
			info->inputdevice7->Unacquire();
		}
		info++;
		nbdev--;
	}
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_GetKeyboardInputDevice
//=============================================================================
// Description:
//		Acquires a keyboard device for input.
//		Finds first available keyboard and configures it for use.
//
// Parameters:
//		hwnd - Window handle for cooperative level
//		id - Keyboard slot ID (0 to MAXKEYBOARD-1)
//		mode - Cooperative level mode (DXI_MODE_EXCLUSIF_ALLMSG, etc.)
//
// Returns:
//		DXI_OK - Keyboard successfully acquired
//		DXI_FAIL - No keyboard available or acquisition failed
//
// Algorithm:
//		1. Validate keyboard ID is within range
//		2. If keyboard already in this slot, release it first
//		3. Search for first available keyboard device
//		4. Call DXI_ChooseInputDevice() to configure and acquire it
//
// Notes:
//		- Only one keyboard per slot
//		- Automatically releases previous keyboard in slot
//		- Keyboard data accessible via DI_KeyBoardBuffer[id]
//		- Uses 256-byte buffer for all keyboard keys
//
//=============================================================================
int DXI_GetKeyboardInputDevice(HWND hwnd,int id,int mode)
{
int			nbdev,num=0;
INPUT_INFO	*info;

	if(id>=MAXKEYBOARD) 
		return DXI_FAIL;
	if(DI_KeyBoardBuffer[id])
	{
		info=DXI_GetInputInfoWithState(DI_KeyBoardBuffer[id],DIDEVTYPE_KEYBOARD);
		if(info) DXI_ReleaseDevice(info);
		DI_KeyBoardBuffer[id]=NULL;
	}

	info=DI_InputInfo;
	nbdev=DI_NbInputInfo;
	while(nbdev)
	{
		if((GET_DIDEVICE_TYPE(info->type)==DIDEVTYPE_KEYBOARD)&&(!info->actif))
		{
			if(DXI_ChooseInputDevice(hwnd,id,num,mode)==DXI_OK) 
				return DXI_OK;
		}
		num++;
		info++;
		nbdev--;
	}

	return DXI_FAIL;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_GetMouseInputDevice
//=============================================================================
// Description:
//		Acquires a mouse device for input with minimum capability requirements.
//		Finds first available mouse meeting button and axis requirements.
//
// Parameters:
//		hwnd - Window handle for cooperative level
//		id - Mouse slot ID (0 to MAXMOUSE-1)
//		mode - Cooperative level mode (DXI_MODE_EXCLUSIF_ALLMSG, etc.)
//		minbutton - Minimum number of buttons required
//		minaxe - Minimum number of axes required (X/Y = 2, X/Y/Z with wheel = 3)
//
// Returns:
//		DXI_OK - Mouse with required capabilities successfully acquired
//		DXI_FAIL - No suitable mouse available or acquisition failed
//
// Algorithm:
//		1. Validate mouse ID is within range
//		2. If mouse already in this slot, release it first
//		3. Search for first available mouse device
//		4. Call DXI_ChooseInputDevice() to configure it
//		5. Check if device meets minimum button/axis requirements
//		6. If not suitable, release and continue searching
//
// Notes:
//		- Only one mouse per slot
//		- Automatically releases previous mouse in slot
//		- Mouse data accessible via DI_MouseState[id]
//		- Uses buffered input (DIDEVICEOBJECTDATA array)
//		- Supports mice with >2 axes (wheel) via DFDIMOUSE2 format
//
//=============================================================================
int DXI_GetMouseInputDevice(HWND hwnd,int id,int mode,int minbutton,int minaxe)
{
int			nbdev,num=0;
INPUT_INFO	*info;

	if(id>=MAXMOUSE) return DXI_FAIL;
	if(DI_MouseState[id])
	{
		info=DXI_GetInputInfoWithState(DI_MouseState[id],DIDEVTYPE_MOUSE);
		if(info) DXI_ReleaseDevice(info);
		DI_MouseState[id]=NULL;
	}

	info=DI_InputInfo;
	nbdev=DI_NbInputInfo;
	while(nbdev)
	{
		if((GET_DIDEVICE_TYPE(info->type)==DIDEVTYPE_MOUSE)&&(!info->actif))
		{
			if(DXI_ChooseInputDevice(hwnd,id,num,mode)==DXI_OK)
			{
				if((info->nbbuttons>=minbutton)&&(info->nbaxes>=minaxe)) return DXI_OK;
				else
				{
					DXI_ReleaseDevice(info);
				}
			}
		}
		num++;
		info++;
		nbdev--;
	}

	return DXI_FAIL;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_GetJoyInputDevice
//=============================================================================
// Description:
//		Acquires a joystick device for input with minimum capability requirements.
//		Finds first available joystick meeting button and axis requirements.
//
// Parameters:
//		hwnd - Window handle for cooperative level
//		id - Joystick slot ID (0 to MAXJOY-1)
//		mode - Cooperative level mode (DXI_MODE_EXCLUSIF_ALLMSG, etc.)
//		minbutton - Minimum number of buttons required
//		minaxe - Minimum number of axes required
//
// Returns:
//		DXI_OK - Joystick with required capabilities successfully acquired
//		DXI_FAIL - No suitable joystick available or acquisition failed
//
// Algorithm:
//		1. Validate joystick ID is within range
//		2. If joystick already in this slot, release it first
//		3. Search for first available joystick device
//		4. Call DXI_ChooseInputDevice() to configure it
//		5. Check if device meets minimum button/axis requirements
//		6. If not suitable, release and continue searching
//
// Notes:
//		- Supports DIJOYSTATE (≤2 axes) and DIJOYSTATE2 (>2 axes) formats
//		- Joystick data accessible via DI_JoyState[id]
//		- Typical joystick has 2-4 axes and 4-32 buttons
//		- Use DXI_SetRangeJoy() to configure axis ranges and deadzones
//
//=============================================================================
int DXI_GetJoyInputDevice(HWND hwnd,int id,int mode,int minbutton,int minaxe)
{
int			nbdev,num=0;
INPUT_INFO	*info;

	if(id>=MAXJOY) return DXI_FAIL;
	if(DI_JoyState[id])
	{
		info=DXI_GetInputInfoWithState(DI_JoyState[id],DIDEVTYPE_JOYSTICK);
		if(info) DXI_ReleaseDevice(info);
		DI_JoyState[id]=NULL;
	}

	info=DI_InputInfo;
	nbdev=DI_NbInputInfo;
	while(nbdev)
	{
		if((GET_DIDEVICE_TYPE(info->type)==DIDEVTYPE_JOYSTICK)&&(!info->actif))
		{
			if(DXI_ChooseInputDevice(hwnd,id,num,mode)==DXI_OK)
			{
				if((info->nbbuttons>=minbutton)&&(info->nbaxes>=minaxe)) return DXI_OK;
				else
				{
					DXI_ReleaseDevice(info);
				}
			}
		}

		num++;
		info++;
		nbdev--;
	}

	return DXI_FAIL;
}

//=============================================================================
// FUNCTION: DXI_GetSCIDInputDevice
//=============================================================================
// Description:
//		Acquires Microsoft SideWinder Strategic Commander device.
//		This is a specialized game controller with programmable buttons.
//
// Parameters:
//		hwnd - Window handle for cooperative level
//		id - SCID slot ID (0 to MAXSCID-1)
//		mode - Cooperative level mode
//		minbutton - Minimum number of buttons required
//		minaxe - Minimum number of axes required
//
// Returns:
//		DXI_OK - Strategic Commander successfully acquired
//		DXI_FAIL - Device not found or acquisition failed
//
// Notes:
//		- SCID = Microsoft SideWinder Strategic Commander (rare device)
//		- Device type is DIDEVTYPE_DEVICE, not DIDEVTYPE_JOYSTICK
//		- Specific device name check: "Microsoft SideWinder Strategic Commander"
//		- Treated like joystick for data format purposes
//		- Data accessible via DI_SCIDState[id]
//
//=============================================================================
int DXI_GetSCIDInputDevice(HWND hwnd,int id,int mode,int minbutton,int minaxe)
{
int			nbdev,num=0;
INPUT_INFO	*info;

/*	if(id>=MAXJOY) return DXI_FAIL;
	if(DI_JoyState[id])
	{
		info=DXI_GetInputInfoWithState(DI_JoyState[id],DIDEVTYPE_JOYSTICK);
		if(info) DXI_ReleaseDevice(info);
		DI_JoyState[id]=NULL;
	}
*/  // A checker....
	info=DI_InputInfo;
	nbdev=DI_NbInputInfo;
	while(nbdev)
	{
		if((GET_DIDEVICE_TYPE(info->type)==DIDEVTYPE_DEVICE )&&(!info->actif))
		{
			if (!strcmp("Microsoft SideWinder Strategic Commander",info->name))
			if(DXI_ChooseInputDevice(hwnd,id,num,mode)==DXI_OK)
			{
				if((info->nbbuttons>=minbutton)&&(info->nbaxes>=minaxe)) return DXI_OK;
				else
				{
					DXI_ReleaseDevice(info);
				}
			}
		}
		
		
		num++;
		info++;
		nbdev--;
	}

	return DXI_FAIL;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_ChooseInputDevice
//=============================================================================
// Description:
//		Configures and acquires a specific DirectInput device.
//		This is the core function that creates, configures, and acquires devices.
//
// Parameters:
//		hwnd - Window handle for cooperative level
//		id - Device slot ID for this device type (keyboard/mouse/joystick)
//		num - Index into DI_InputInfo[] array for device to configure
//		mode - Cooperative level mode:
//				DXI_MODE_EXCLUSIF_ALLMSG - Exclusive, background
//				DXI_MODE_EXCLUSIF_OURMSG - Exclusive, foreground
//				DXI_MODE_NONEXCLUSIF_ALLMSG - Non-exclusive, background
//				DXI_MODE_NONEXCLUSIF_OURMSG - Non-exclusive, foreground
//
// Returns:
//		DXI_OK - Device successfully configured and acquired
//		DXI_FAIL - Configuration or acquisition failed
//
// Algorithm:
//		1. Validate device number is within enumeration range
//		2. Release device if already configured
//		3. Create DirectInputDevice7 interface via CreateDeviceEx()
//		4. Get device capabilities (button count, axis count)
//		5. Set cooperative level (exclusive/non-exclusive, foreground/background)
//		6. Enumerate device objects to build capability flags
//		7. Configure device based on type:
//			MOUSE:
//				- Set buffered input mode (128 events)
//				- Allocate DIDEVICEOBJECTDATA buffer
//				- Choose DFDIMOUSE or DFDIMOUSE2 format (based on button count)
//				- Store pointer in DI_MouseState[id]
//			KEYBOARD:
//				- Allocate 256-byte state buffer
//				- Use DFDIKEYBOARD format
//				- Store pointer in DI_KeyBoardBuffer[id]
//			JOYSTICK:
//				- Allocate DIJOYSTATE or DIJOYSTATE2 buffer (based on axis count)
//				- Use DFDIJOYSTICK or DFDIJOYSTICK2 format
//				- Store pointer in DI_JoyState[id]
//			SCID (Strategic Commander):
//				- Same as joystick, but device name must match "Microsoft SideWinder Strategic Commander"
//				- Store pointer in DI_SCIDState[id]
//		8. Set data format via SetDataFormat()
//		9. Acquire device via Acquire()
//		10. Mark device as active
//
// Notes:
//		- Creates COM interface for device interaction
//		- Memory allocated here freed in DXI_ReleaseDevice()
//		- Buffered input for mouse (event queue), immediate for keyboard/joystick
//		- Acquire() may fail initially but device still marked active (re-acquire later)
//		- Commented code shows old C-style COM vtable calling convention
//		- SCID devices require exact name match (rare gaming peripheral)
//
//=============================================================================
int DXI_ChooseInputDevice( HWND hwnd, int id, int num, int mode )
{
DIDEVCAPS		devcaps;
INPUT_INFO*		info;
int				flag;
DIDATAFORMAT*	dformat;

	if( num >= DI_NbInputInfo ) return DXI_FAIL;
	info = &DI_InputInfo[num];

	DXI_ReleaseDevice( info );
	//Old : if( FAILED( DI_Hr = DI_DInput7->lpVtbl->CreateDeviceEx( DI_DInput7, info->guid, &IID_IDirectInputDevice7, &info->inputdevice7, NULL ) ) ) return DXI_FAIL;
	if( FAILED( DI_Hr = DI_DInput7->CreateDeviceEx(*(info->guid), IID_IDirectInputDevice7, (void**)&info->inputdevice7, NULL ) ) ) return DXI_FAIL;

	INITSTRUCT( devcaps );
	//Old : if( FAILED( DI_Hr = info->inputdevice7->lpVtbl->GetCapabilities( info->inputdevice7, &devcaps ) ) ) return DXI_FAIL;
	if( FAILED( DI_Hr = info->inputdevice7->GetCapabilities(&devcaps ) ) ) return DXI_FAIL;

	info->nbbuttons	=	devcaps.dwButtons;
	info->nbaxes	=	devcaps.dwAxes;

	switch( mode )
	{
		case DXI_MODE_EXCLUSIF_ALLMSG:
			flag = DISCL_EXCLUSIVE | DISCL_BACKGROUND;
			break;
		case DXI_MODE_EXCLUSIF_OURMSG:
			flag = DISCL_EXCLUSIVE | DISCL_FOREGROUND;
			break;
		case DXI_MODE_NONEXCLUSIF_ALLMSG:
			flag = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
			break;
		case DXI_MODE_NONEXCLUSIF_OURMSG:
			flag = DISCL_NONEXCLUSIVE | DISCL_FOREGROUND;
			break;
		//ARX_BEGIN: jycorbel (2010-06-30) - clean warning on not-initialized variable
		// flag should be always set unless 'mode' doesn't match any case which could resume on a fatal error
		default:
			ARX_CHECK_NO_ENTRY();
			flag = 0; //clean warning
		//ARX_END: jycorbel (2010-06-30)
	}

	//Old : 	if( FAILED( DI_Hr = info->inputdevice7->lpVtbl->SetCooperativeLevel( info->inputdevice7, hwnd, flag ) ) ) return DXI_FAIL;
	if( FAILED( DI_Hr = info->inputdevice7->SetCooperativeLevel(hwnd, flag ) ) ) return DXI_FAIL;
	//Old : 	if( FAILED( DI_Hr = info->inputdevice7->lpVtbl->EnumObjects( info->inputdevice7, DIEnumDeviceObjectsCallback, (void*)info, DIDFT_ALL ) ) ) return DXI_FAIL;
	if( FAILED( DI_Hr = info->inputdevice7->EnumObjects(DIEnumDeviceObjectsCallback, (void*)info, DIDFT_ALL ) ) ) return DXI_FAIL;

	switch( GET_DIDEVICE_TYPE( info->type ) )
	{
	case DIDEVTYPE_MOUSE:
		{
			DIPROPDWORD dipdw={
				// the header
				{
					sizeof(DIPROPDWORD),        // diph.dwSize
					sizeof(DIPROPHEADER),       // diph.dwHeaderSize
					0,                          // diph.dwObj
					DIPH_DEVICE,                // diph.dwHow
				},
		        // the data
				128,              // dwData
			};

			info->mousestate=(DIDEVICEOBJECTDATA*)malloc(sizeof(DIDEVICEOBJECTDATA)*(info->nbbuttons+info->nbaxes+INPUT_STATE_ADD));
			memset(info->mousestate,0,(sizeof(DIDEVICEOBJECTDATA)*(info->nbbuttons+info->nbaxes+INPUT_STATE_ADD)));
//			info->old_mousestate=(DIDEVICEOBJECTDATA*)malloc(sizeof(DIDEVICEOBJECTDATA)*(info->nbbuttons+info->nbaxes));
//			memset(info->old_mousestate,0,(sizeof(DIDEVICEOBJECTDATA)*(info->nbbuttons+info->nbaxes)));
//			DI_MouseState[id]=info->mousestate;
			DI_MouseState[id]=info;
			if(info->nbbuttons>4)
			{
				info->datasid=DFDIMOUSE2;
				dformat=(DIDATAFORMAT*)&c_dfDIMouse2;
			}
			else
			{
				info->datasid=DFDIMOUSE;
				dformat=(DIDATAFORMAT*)&c_dfDIMouse;
			}
			//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->SetProperty(info->inputdevice7,DIPROP_BUFFERSIZE,&dipdw.diph))) return DXI_FAIL;
			if(FAILED(DI_Hr=info->inputdevice7->SetProperty(DIPROP_BUFFERSIZE,&dipdw.diph))) return DXI_FAIL;
		}
		break;
	case DIDEVTYPE_KEYBOARD:
		info->datasid=DFDIKEYBOARD;
		info->bufferstate=(char*)malloc(256);
		memset(info->bufferstate,0,256);
//		info->old_bufferstate=(char*)malloc(256);
//		memset(info->old_bufferstate,0,256);
		DI_KeyBoardBuffer[id]=info;//->bufferstate;
		//DI_OldKeyBoardBuffer[id]=info->old_bufferstate;
		dformat=(DIDATAFORMAT*)&c_dfDIKeyboard;
		break;
	case DIDEVTYPE_JOYSTICK:
		DI_JoyState[id]=info;
		if(info->nbaxes>2)
		{
			info->joystate2=(DIJOYSTATE2*)malloc(sizeof(DIJOYSTATE2));
			memset(info->joystate2,0,sizeof(DIJOYSTATE2));
//			info->old_joystate2=(DIJOYSTATE2*)malloc(sizeof(DIJOYSTATE2));
//			memset(info->old_joystate2,0,sizeof(DIJOYSTATE2));
			info->datasid=DFDIJOYSTICK2;
			dformat=(DIDATAFORMAT*)&c_dfDIJoystick2;
		}
		else
		{
			info->joystate=(DIJOYSTATE*)malloc(sizeof(DIJOYSTATE));
			memset(info->joystate,0,sizeof(DIJOYSTATE));
//			info->old_joystate=(DIJOYSTATE*)malloc(sizeof(DIJOYSTATE));
//			memset(info->old_joystate,0,sizeof(DIJOYSTATE));
			info->datasid=DFDIJOYSTICK;
			dformat=(DIDATAFORMAT*)&c_dfDIJoystick;
		}
		break;
	default:
	case DIDEVTYPE_DEVICE:
		if (!strcmp("Microsoft SideWinder Strategic Commander",info->name))
		{
			DI_SCIDState[id]=info;
			if(info->nbaxes>2)
			{
				info->joystate2=(DIJOYSTATE2*)malloc(sizeof(DIJOYSTATE2));
				memset(info->joystate2,0,sizeof(DIJOYSTATE2));
//				info->old_joystate2=(DIJOYSTATE2*)malloc(sizeof(DIJOYSTATE2));
//				memset(info->old_joystate2,0,sizeof(DIJOYSTATE2));
				info->datasid=DFDIJOYSTICK2;
				dformat=(DIDATAFORMAT*)&c_dfDIJoystick2;
			}
			else
			{
				info->joystate=(DIJOYSTATE*)malloc(sizeof(DIJOYSTATE));
				memset(info->joystate,0,sizeof(DIJOYSTATE));
//				info->old_joystate=(DIJOYSTATE*)malloc(sizeof(DIJOYSTATE));
//				memset(info->old_joystate,0,sizeof(DIJOYSTATE));
				info->datasid=DFDIJOYSTICK;
				dformat=(DIDATAFORMAT*)&c_dfDIJoystick;
			}
		
			/*
			DIPROPDWORD dipdw={
				// the header
				{
					sizeof(DIPROPDWORD),        // diph.dwSize
					sizeof(DIPROPHEADER),       // diph.dwHeaderSize
					0,                          // diph.dwObj
					DIPH_DEVICE,                // diph.dwHow
				},
		        // the data
				16,              // dwData
			};

			info->SCIDstate=(DIDEVICEOBJECTDATA*)malloc(sizeof(DIDEVICEOBJECTDATA)*(info->nbbuttons+info->nbaxes));
			info->old_SCIDstate=(DIDEVICEOBJECTDATA*)malloc(sizeof(DIDEVICEOBJECTDATA)*(info->nbbuttons+info->nbaxes));
//			DI_MouseState[id]=info->mousestate;
			DI_SCIDState[id]=info;
			if(info->nbbuttons>4)
			{
				info->datasid=DFDIMOUSE2;
				dformat=(DIDATAFORMAT*)&c_dfDIMouse2;
			}
			else
			{
				info->datasid=DFDIMOUSE;
				dformat=(DIDATAFORMAT*)&c_dfDIMouse;
			}
			
			if(FAILED(DI_Hr=info->inputdevice7->SetProperty(info->inputdevice7,DIPROP_BUFFERSIZE,&dipdw.diph))) return DXI_FAIL;
*/
		}
		else dformat=NULL;
		break;
	}
	if(!dformat) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->SetDataFormat(info->inputdevice7,dformat))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->SetDataFormat(dformat))) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Acquire(info->inputdevice7))) 
	if(FAILED(DI_Hr=info->inputdevice7->Acquire())) 
	{}
	//	info->actif=DEVICENOACTIF;
	//	return DXI_FAIL;
	//else 
	info->actif=DEVICEACTIF;
	return DXI_OK;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_GetInfoDevice
//=============================================================================
// Description:
//		Retrieves information about an enumerated device.
//		Returns a DXI_INPUT_INFO structure with device name, type, and capabilities.
//
// Parameters:
//		num - Index into DI_InputInfo[] array (0 to DI_NbInputInfo-1)
//
// Returns:
//		Pointer to allocated DXI_INPUT_INFO structure (caller must free)
//		NULL if index out of range or allocation failed
//
// Notes:
//		- Allocates new DXI_INPUT_INFO structure (use DXI_freeInfoDevice to free)
//		- Copies device name string (separate allocation)
//		- Returns -1 for button/axis counts if device not yet configured
//		- Used to query available devices before acquisition
//
//=============================================================================
DXI_INPUT_INFO * DXI_GetInfoDevice(int num)
{
DXI_INPUT_INFO	*dinf;
INPUT_INFO		*info;

	if(num>=DI_NbInputInfo) return NULL;
	dinf=(DXI_INPUT_INFO*)malloc(sizeof(DXI_INPUT_INFO));
	if(!dinf) return NULL;

	info=&DI_InputInfo[num];
	dinf->name=(char*)malloc(strlen(info->name)+1);
	if(!dinf)
	{
		free((void*)dinf);
		return NULL;
	}

	strcpy(dinf->name,info->name);
	dinf->type=info->type;
	dinf->numlist=num;
	if(info->inputdevice7)
	{
		dinf->nbbuttons=info->nbbuttons;
		dinf->nbaxes=info->nbaxes;
		dinf->info=info->info;
	}
	else
	{
		dinf->nbbuttons=-1;
		dinf->nbaxes=-1;
		dinf->info=-1;
	}

	return dinf;
}

//=============================================================================
// FUNCTION: DXI_CleanAxeMouseZ
//=============================================================================
// Description:
//		[DISABLED] Originally intended to clean/normalize mouse Z-axis (wheel) data.
//		Function immediately returns FALSE without performing any action.
//
// Parameters:
//		id - Mouse device ID
//
// Returns:
//		FALSE - Always (function disabled)
//
// Notes:
//		- Function body commented out/disabled
//		- Was intended to process relative mouse wheel movement
//		- No longer used in current implementation
//
//=============================================================================
BOOL DXI_CleanAxeMouseZ(int id)
{
DIDEVICEOBJECTDATA	*od;
DIDEVICEOBJECTDATA	*od2;
int					nb,flg=0;

return FALSE;

	nb=DI_MouseState[id]->nbele;
	if(!nb) return FALSE;
	od=DI_MouseState[id]->mousestate;
//	od2=DI_MouseState[id]->old_mousestate;
	while(nb)
	{
		switch(od->dwOfs)
		{
		case DIMOFS_X:
			flg++;
			break;
		case DIMOFS_Y:
			flg++;
			break;
		case DIMOFS_Z:
			od->dwData-=od2->dwData;
			flg++;
			break;
		default:
			break;
		}
		od++;
		od2++;
		nb--;
	}
	return (flg>0);
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_freeInfoDevice
//=============================================================================
// Description:
//		Frees a DXI_INPUT_INFO structure allocated by DXI_GetInfoDevice().
//
// Parameters:
//		dinf - Pointer to DXI_INPUT_INFO structure to free
//
// Returns:
//		None (void)
//
// Notes:
//		- Frees device name string and structure itself
//		- Safe to call with NULL pointer
//
//=============================================================================
void DXI_freeInfoDevice(DXI_INPUT_INFO *dinf)
{
	if(!dinf) return;
	if(dinf->name) free((void*)dinf->name);
	free((void*)dinf);
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_ExecuteAllDevices
//=============================================================================
// Description:
//		Polls all active DirectInput devices and updates their state.
//		This is the main input polling function, called every frame.
//
// Parameters:
//		_bKeept - If TRUE, peek at mouse data without removing from buffer
//				  If FALSE, consume mouse data from buffer
//
// Returns:
//		TRUE - All devices polled successfully
//		FALSE - One or more devices failed (but state still updated)
//
// Algorithm:
//		For each active device in DI_InputInfo[]:
//
//		MOUSE:
//			1. Call GetDeviceData() to retrieve buffered input events
//			2. Store events in mousestate buffer (DIDEVICEOBJECTDATA array)
//			3. Update nbele with number of events retrieved
//			4. If fails, retry up to 3 times (device may have been lost)
//			5. Call DXI_CleanAxeMouseZ() (currently disabled)
//
//		KEYBOARD:
//			1. Call GetDeviceState() to get immediate 256-byte key state
//			2. Each byte represents one key (0x80 = pressed, 0x00 = released)
//			3. If fails, call DXI_RestoreAllDevices() and retry
//			4. If still fails, zero the keyboard buffer
//
//		JOYSTICK:
//			1. Call Poll() to update joystick state
//			2. Call GetDeviceState() with DIJOYSTATE or DIJOYSTATE2
//			3. Structure contains axis values and button states
//			4. If fails, return FALSE but continue polling other devices
//
//		SCID (Strategic Commander):
//			1. Same as joystick (Poll + GetDeviceState)
//
// Notes:
//		- Should be called once per frame to update input state
//		- Automatically handles device loss and restoration
//		- Mouse uses buffered input (event queue), keyboard/joystick use immediate
//		- DIGDD_PEEK flag allows reading mouse data without consuming it
//		- Device failure doesn't stop polling other devices
//		- Retry logic handles temporary device loss (task switch, sleep mode)
//		- Contains extensive commented-out error handling code
//
//=============================================================================
BOOL DXI_ExecuteAllDevices(BOOL _bKeept)
{
int			nb,nbele;
DWORD		dwNbele;//ARX: xrichter (2010-06-30) - treat warnings C4057 for 'LPDWORD' differs in indirection to slightly different base types from 'int *'
INPUT_INFO	*info;
BOOL		flg=TRUE;
void * temp;
	
//DIDEVICEOBJECTDATA	* od;
//DIDEVICEOBJECTDATA	* odd;
				

	info=DI_InputInfo;
	nb=DI_NbInputInfo;
	
	while(nb)
	{
		if(info->actif)
		{
			// union!!!
			temp=info->mousestate;
//			info->mousestate=info->old_mousestate;
//			info->old_mousestate=temp;
			switch(GET_DIDEVICE_TYPE(info->type))
			{

			//ARX_BEGIN: xrichter (2010-06-30) - treat warnings C4057 for 'LPDWORD' differs in indirection to slightly different base types from 'int *'
			case DIDEVTYPE_MOUSE:
				nbele=info->nbbuttons+info->nbaxes+INPUT_STATE_ADD;
				dwNbele=(DWORD)nbele; 
				//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceData(info->inputdevice7,sizeof(DIDEVICEOBJECTDATA),info->mousestate,&dwNbele,(_bKeept)?DIGDD_PEEK:0 ))) 
				if(FAILED(DI_Hr=info->inputdevice7->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),info->mousestate,&dwNbele,(_bKeept)?DIGDD_PEEK:0 ))) 
				{
					nbele=info->nbbuttons+info->nbaxes+INPUT_STATE_ADD;
					dwNbele=(DWORD)nbele; 
					//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceData(info->inputdevice7,sizeof(DIDEVICEOBJECTDATA),info->mousestate,&dwNbele,(_bKeept)?DIGDD_PEEK:0 ))) 
					if(FAILED(DI_Hr=info->inputdevice7->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),info->mousestate,&dwNbele,(_bKeept)?DIGDD_PEEK:0 ))) 
					{
						//Old : info->inputdevice7->lpVtbl->GetDeviceData(info->inputdevice7,sizeof(DIDEVICEOBJECTDATA),info->mousestate,&dwNbele,(_bKeept)?DIGDD_PEEK:0 );
						info->inputdevice7->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),info->mousestate,&dwNbele,(_bKeept)?DIGDD_PEEK:0 );
			//ARX_END: xrichter (2010-06-30)	

//					DXI_RestoreAllDevices();
				//	info->actif=DEVICENOACTIF;
					/*
					if (DIERR_INPUTLOST == DI_Hr) 
					{
				//		DXI_RestoreAllDevices();
					}
					if (DIERR_NOTACQUIRED  == DI_Hr)
					{
				//		DXI_RestoreAllDevices();
						//info->inputdevice7->Acquire
					}
				/*	if (DIERR_INVALIDPARAM  == DI_Hr) MessageBox(NULL,"DIERR_INVALIDPARAM ","",0);
					if (DIERR_NOTACQUIRED  == DI_Hr) MessageBox(NULL,"DIERR_NOTACQUIRED ","",0);
					if (DIERR_NOTINITIALIZED  == DI_Hr) MessageBox(NULL,"DIERR_NOTINITIALIZED ","",0);
					if (E_PENDING  == DI_Hr) MessageBox(NULL,"E_PENDING ","",0);
*/
					flg=FALSE;
					}
				}
				DXI_CleanAxeMouseZ(DXI_MOUSE1); //////////////////////////
			//	od=info->mousestate;
			//	odd=info->old_mousestate;
			//	od++;od++;
			//	odd++;odd++;
			//	od->dwData-=odd->dwData;
				
				nbele=(int)dwNbele; //ARX: xrichter (2010-06-30) - treat warnings C4057 for 'LPDWORD' differs in indirection to slightly different base types from 'int *'
				info->nbele=nbele;
				break;
			case DIDEVTYPE_KEYBOARD: 
				
				//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceState(info->inputdevice7,256,(void*)info->bufferstate))) 
				if(FAILED(DI_Hr=info->inputdevice7->GetDeviceState(256,(void*)info->bufferstate))) 
				{
					DXI_RestoreAllDevices(); 
					
					//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceState(info->inputdevice7,256,(void*)info->bufferstate))) 
					if(FAILED(DI_Hr=info->inputdevice7->GetDeviceState(256,(void*)info->bufferstate))) 
					{
//					DXI_RestoreAllDevices();

					/*
					if (DIERR_INPUTLOST == DI_Hr) 
					{
						//DXI_RestoreAllDevices(); 
						//Old : info->inputdevice7->lpVtbl->Acquire(info->inputdevice7);
						info->inputdevice7->Acquire(info->inputdevice7);
					}
					if (DIERR_NOTACQUIRED  == DI_Hr)
					{
						//DXI_RestoreAllDevices(); 
						//Old : info->inputdevice7->lpVtbl->Acquire(info->inputdevice7);
						info->inputdevice7->Acquire(info->inputdevice7);
					}
					if (DIERR_NOTINITIALIZED  == DI_Hr) 
					{	//Old : info->inputdevice7->lpVtbl->Acquire(info->inputdevice7);
						info->inputdevice7->Acquire(info->inputdevice7);
						//MessageBox(NULL,"DIERR_NOTINITIALIZED ","",0);
					}
					/*
					if (DIERR_INVALIDPARAM  == DI_Hr) MessageBox(NULL,"DIERR_INVALIDPARAM ","",0);
					if (DIERR_NOTACQUIRED  == DI_Hr) MessageBox(NULL,"DIERR_NOTACQUIRED ","",0);
					if (DIERR_NOTINITIALIZED  == DI_Hr) MessageBox(NULL,"DIERR_NOTINITIALIZED ","",0);
					if (E_PENDING  == DI_Hr) MessageBox(NULL,"E_PENDING ","",0);
					*/		
					//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceState(info->inputdevice7,256,(void*)info->bufferstate))) 
//					if(FAILED(DI_Hr=info->inputdevice7->GetDeviceState(256,(void*)info->bufferstate))) 
						memset(info->bufferstate,0,256); //seb 27/03/2002
						flg=FALSE;					
					}
					
				}
				break;
			case DIDEVTYPE_JOYSTICK: 
				//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Poll(info->inputdevice7))) flg=FALSE;
				if(FAILED(DI_Hr=info->inputdevice7->Poll())) flg=FALSE;

				if(info->datasid==DFDIJOYSTICK2)
				{	
					//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceState(info->inputdevice7,sizeof(DIJOYSTATE2),(void*)info->joystate2))) 
					if(FAILED(DI_Hr=info->inputdevice7->GetDeviceState(sizeof(DIJOYSTATE2),(void*)info->joystate2))) 
						flg=FALSE;
				}
				else
				{	
					//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceState(info->inputdevice7,sizeof(DIJOYSTATE),(void*)info->joystate))) 
					if(FAILED(DI_Hr=info->inputdevice7->GetDeviceState(sizeof(DIJOYSTATE),(void*)info->joystate))) 
						flg=FALSE;						
				}
				break;
			default:
			case DIDEVTYPE_DEVICE: 
				//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Poll(info->inputdevice7))) 
				if(FAILED(DI_Hr=info->inputdevice7->Poll())) 
					flg=FALSE;
					
				if(info->datasid==DFDIJOYSTICK2)
				{	
					//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceState(info->inputdevice7,sizeof(DIJOYSTATE2),(void*)info->joystate2))) 
					if(FAILED(DI_Hr=info->inputdevice7->GetDeviceState(sizeof(DIJOYSTATE2),(void*)info->joystate2))) 
						flg=FALSE;
				/*	else
					{
						long togo=sizeof(DIJOYSTATE2);
						long ii=0;
						char * dat1=(char *)info->joystate2;
						char * dat2=(char *)info->old_joystate2;
						while (ii<togo)
						{							
							if (dat1[ii]!=dat2[ii]) 
								dat1[ii]=dat1[ii];
							ii++;
						}
					}*/

				}
				else
				{	
					//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceState(info->inputdevice7,sizeof(DIJOYSTATE),(void*)info->SCIDstate))) 
					if(FAILED(DI_Hr=info->inputdevice7->GetDeviceState(sizeof(DIJOYSTATE),(void*)info->SCIDstate))) 
						flg=FALSE;							
				}
				break;
				/*
				nbele=info->nbbuttons+info->nbaxes; 
				// Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->GetDeviceData(info->inputdevice7,sizeof(DIDEVICEOBJECTDATA),info->SCIDstate,&nbele,0))) 
				if(FAILED(DI_Hr=info->inputdevice7->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),info->SCIDstate,&nbele,0))) 
					flg=FALSE;			
				info->nbele=nbele;
				break;*/
			}
		}
		info++;
		nb--;
	}

	return flg;
}

/*-------------------------------------------------------------*/
//=============================================================================
// KEYBOARD INPUT QUERY FUNCTIONS
//=============================================================================

//=============================================================================
// FUNCTION: DXI_KeyPressed
//=============================================================================
// Description:
//		Checks if a specific keyboard key is currently pressed.
//
// Parameters:
//		id - Keyboard device ID (DXI_KEYBOARD1, etc.)
//		dikkey - DirectInput key code (DIK_ESCAPE, DIK_A, etc.)
//
// Returns:
//		TRUE - Key is currently pressed
//		FALSE - Key is not pressed
//
// Notes:
//		- Uses DirectInput key codes (DIK_*), not virtual key codes
//		- Reads from 256-byte keyboard state buffer
//		- High bit (0x80) indicates key down
//
//=============================================================================
BOOL DXI_KeyPressed(int id,int dikkey)
{
	if(DI_KeyBoardBuffer[id]->bufferstate[dikkey]&0x80) return TRUE;
	return FALSE;
}

//=============================================================================
// FUNCTION: DXI_OldKeyPressed
//=============================================================================
// Description:
//		[DISABLED] Originally checked previous frame key state for edge detection.
//		Always returns FALSE.
//
// Notes:
//		- Function disabled, old state tracking removed
//		- Would have been used to detect key press/release events
//
//=============================================================================
BOOL DXI_OldKeyPressed(int id,int dikkey)
{
	//if(DI_InputInfo->old_bufferstate[id*dikkey]&0x80) return TRUE;
//	if(DI_KeyBoardBuffer[id]->old_bufferstate[dikkey]&0x80) return TRUE;
	return FALSE;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_GetKeyIDPressed
//=============================================================================
// Description:
//		Finds the first pressed key on the keyboard.
//		Used for key binding and "press any key" prompts.
//
// Parameters:
//		id - Keyboard device ID
//
// Returns:
//		0-255 - DirectInput key code of first pressed key found
//		-1 - No keys are pressed
//
// Notes:
//		- Scans all 256 keyboard state bytes
//		- Returns first key found (scan order 0-255)
//		- Useful for key remapping interfaces
//
//=============================================================================
int DXI_GetKeyIDPressed(int id)
{
int		nb;
char	*buf;

	buf=DI_KeyBoardBuffer[id]->bufferstate;
	nb=256;
	while(nb)
	{
		if((*buf)&0x80) 
			return 256-nb;
		buf++;
		nb--;
	}
	return -1;
}

//=============================================================================
// FUNCTION: DXI_ClearKeys
//=============================================================================
// Description:
//		Clears the keyboard state buffer, marking all keys as unpressed.
//
// Parameters:
//		id - Keyboard device ID
//
// Returns:
//		None (void)
//
// Notes:
//		- Zeros entire 256-byte keyboard buffer
//		- Used to reset keyboard state after focus changes
//		- Does not affect physical keyboard, only internal state
//
//=============================================================================
void DXI_ClearKeys(int id)
{
	memset(DI_KeyBoardBuffer[id],0,256);
}

/*-------------------------------------------------------------*/
//=============================================================================
// MOUSE INPUT QUERY FUNCTIONS
//=============================================================================

//=============================================================================
// FUNCTION: DXI_GetAxeMouseXY
//=============================================================================
// Description:
//		Retrieves mouse X and Y axis movement from buffered input events.
//
// Parameters:
//		id - Mouse device ID
//		mx - Output pointer for X-axis movement (relative pixels)
//		my - Output pointer for Y-axis movement (relative pixels)
//
// Returns:
//		TRUE - Movement data retrieved successfully
//		FALSE - No movement events in buffer
//
// Algorithm:
//		Iterate through mouse event buffer (DIDEVICEOBJECTDATA array)
//		and extract the last X and Y movement values.
//
// Notes:
//		- Uses buffered input (event queue from DXI_ExecuteAllDevices)
//		- Returns relative movement (delta) not absolute position
//		- Only processes events since last DXI_ExecuteAllDevices call
//		- Multiple movements in one frame are processed separately
//
//=============================================================================
BOOL DXI_GetAxeMouseXY(int id,int *mx,int *my)
{
DIDEVICEOBJECTDATA	*od;
int					nb,flg=0;

	nb=DI_MouseState[id]->nbele;
	if(!nb) return FALSE;
	od=DI_MouseState[id]->mousestate;
	while(nb)
	{
		switch(od->dwOfs)
		{
		case DIMOFS_X:
			*mx=od->dwData;
			flg++;
			break;
		case DIMOFS_Y:
			*my=od->dwData;
			flg++;
			break;
		default:
			break;
		}
		od++;
		nb--;
	}
	return(flg>0);
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_GetAxeMouseXYZ
//=============================================================================
// Description:
//		Retrieves mouse X, Y, and Z (wheel) axis movement from buffered events.
//
// Parameters:
//		id - Mouse device ID
//		mx - Output pointer for X-axis movement (relative pixels)
//		my - Output pointer for Y-axis movement (relative pixels)
//		mz - Output pointer for Z-axis movement (wheel clicks, typically +/-120 per click)
//
// Returns:
//		TRUE - Movement data retrieved successfully
//		FALSE - No movement events in buffer
//
// Algorithm:
//		Iterate through mouse event buffer and accumulate all X, Y, and Z movements.
//		Multiple movements of same axis are summed together.
//
// Notes:
//		- Accumulates all movements in buffer (unlike GetAxeMouseXY)
//		- Z-axis is mouse wheel (positive = scroll up, negative = scroll down)
//		- Output parameters initialized to 0 before accumulation
//		- Returns TRUE if any axis had movement
//
//=============================================================================
BOOL DXI_GetAxeMouseXYZ(int id,int *mx,int *my,int *mz)
{
DIDEVICEOBJECTDATA	*od;
int					nb,flg=0;

	*mx=*my=*mz=0;

	nb=DI_MouseState[id]->nbele;
	if(!nb) return FALSE;
	od=DI_MouseState[id]->mousestate;
	while(nb)
	{
		switch(od->dwOfs)
		{
		case DIMOFS_X:
			*mx+=od->dwData;
			flg++;
			break;
		case DIMOFS_Y:
			*my+=od->dwData;
			flg++;
			break;
		case DIMOFS_Z:
			*mz+=od->dwData;
			flg++;
			break;
		default:
			break;
		}
		od++;
		nb--;
	}
	return (flg>0);
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_MouseButtonImage
//=============================================================================
// Description:
//		[DEBUG] Logs mouse button state to file for debugging purposes.
//		Writes '1' for pressed, '0' for released to c:\temp\dinput.txt.
//
// Notes:
//		- Debug function, not for production use
//		- Only handles BUTTON0 and BUTTON1 (left/right click)
//		- Creates/appends to c:\temp\dinput.txt
//
//=============================================================================
BOOL DXI_MouseButtonImage(int id,int numb)
{
DIDEVICEOBJECTDATA	*od;
int					state,nb;
static FILE *fTemp=NULL;

	if(!fTemp)
	{
		fTemp=fopen("c:\\temp\\dinput.txt","wb");
	}

	nb=DI_MouseState[id]->nbele;
	if(!nb) return FALSE;
	od=DI_MouseState[id]->mousestate;
	while(nb)
	{
		switch(numb)
		{
		case DXI_BUTTON0:
			if(od->dwOfs==DIMOFS_BUTTON0)
			{
				state=od->dwData;
				if(state&0x80)
				{
					fprintf(fTemp,"1");
				}
				else
				{
					fprintf(fTemp,"0");
				}
			}
			break;
		case DXI_BUTTON1:
			if(od->dwOfs==DIMOFS_BUTTON1)
			{
				state=od->dwData;
				if(state&0x80)
				{
					fprintf(fTemp,"1");
				}
				else
				{
					fprintf(fTemp,"0");
				}
			}
			break;
		case DXI_BUTTON2:
			break;
		case DXI_BUTTON3:
			break;
		case DXI_BUTTON4:
			break;
		case DXI_BUTTON5:
			break;
		case DXI_BUTTON6:
			break;
		case DXI_BUTTON7:
			break;
		default:
			return FALSE;
		}

		od++;
		nb--;
	}

	fprintf(fTemp,"\r\n---------\r\n");
	return TRUE;
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_MouseButtonCountClick
//=============================================================================
// Description:
//		Counts number of mouse button press and release events in buffer.
//
// Parameters:
//		id - Mouse device ID
//		numb - Button ID (DXI_BUTTON0-7)
//		_iNumClick - Output pointer for number of press events
//		_iNumUnClick - Output pointer for number of release events
//
// Returns:
//		None (void)
//
// Notes:
//		- Useful for detecting double-clicks or rapid clicking
//		- Counts all press/release events since last poll
//
//=============================================================================
void DXI_MouseButtonCountClick(int id,int numb,int *_iNumClick,int *_iNumUnClick)
{
DIDEVICEOBJECTDATA	*od;
int					state,nb;

	*_iNumClick=0;
	*_iNumUnClick=0;

	nb=DI_MouseState[id]->nbele;
	if(!nb) return;
	od=DI_MouseState[id]->mousestate;
	while(nb)
	{
		switch(numb)
		{
		case DXI_BUTTON0:
			if(od->dwOfs==DIMOFS_BUTTON0)
			{
				state=od->dwData;
				if(state&0x80)
				{
					*_iNumClick+=1;
				}
				else
				{
					*_iNumUnClick+=1;
				}
			}
			break;
		case DXI_BUTTON1:
			if(od->dwOfs==DIMOFS_BUTTON1)
			{
				state=od->dwData;
				if(state&0x80)
				{
					*_iNumClick+=1;
				}
				else
				{
					*_iNumUnClick+=1;
				}
			}
			break;
		case DXI_BUTTON2:
			if(od->dwOfs==DIMOFS_BUTTON2)
			{
				state=od->dwData;
				if(state&0x80)
				{
					*_iNumClick+=1;
				}
				else
				{
					*_iNumUnClick+=1;
				}
			}
			break;
		case DXI_BUTTON3:
			if(od->dwOfs==DIMOFS_BUTTON3)
			{
				state=od->dwData;
				if(state&0x80)
				{
					*_iNumClick+=1;
				}
				else
				{
					*_iNumUnClick+=1;
				}
			}
			break;
		case DXI_BUTTON4:
			if(od->dwOfs==DIMOFS_BUTTON4)
			{
				state=od->dwData;
				if(state&0x80)
				{
					*_iNumClick+=1;
				}
				else
				{
					*_iNumUnClick+=1;
				}
			}
			break;
		case DXI_BUTTON5:
			if(od->dwOfs==DIMOFS_BUTTON5)
			{
				state=od->dwData;
				if(state&0x80)
				{
					*_iNumClick+=1;
				}
				else
				{
					*_iNumUnClick+=1;
				}
			}
			break;
		case DXI_BUTTON6:
			if(od->dwOfs==DIMOFS_BUTTON6)
			{
				state=od->dwData;
				if(state&0x80)
				{
					*_iNumClick+=1;
				}
				else
				{
					*_iNumUnClick+=1;
				}
			}
			break;
		case DXI_BUTTON7:
			if(od->dwOfs==DIMOFS_BUTTON7)
			{
				state=od->dwData;
				if(state&0x80)
				{
					*_iNumClick+=1;
				}
				else
				{
					*_iNumUnClick+=1;
				}
			}
			break;
		default:
			break;
		}

		od++;
		nb--;
	}
}

/*-------------------------------------------------------------*/
//=============================================================================
// FUNCTION: DXI_MouseButtonPressed
//=============================================================================
// Description:
//		Checks if a mouse button is currently pressed and calculates time between presses.
//
// Parameters:
//		id - Mouse device ID
//		numb - Button ID (DXI_BUTTON0-7)
//		_iDeltaTime - Output pointer for time between first and last press (milliseconds)
//
// Returns:
//		TRUE - Button is pressed
//		FALSE - Button is not pressed
//
// Notes:
//		- Checks button state from buffered input events
//		- DeltaTime useful for detecting double-clicks
//		- Uses DirectInput timestamps (dwTimeStamp field)
//		- High bit (0x80) in dwData indicates button down
//
//=============================================================================
BOOL DXI_MouseButtonPressed(int id,int numb,int *_iDeltaTime)
{
DIDEVICEOBJECTDATA	*od;
int					state,iTime1,iTime2,nb;
BOOL				bResult;

	nb=DI_MouseState[id]->nbele;
	if(!nb) return FALSE;
	od=DI_MouseState[id]->mousestate;
	iTime1=iTime2=0;
	while(nb)
	{
		bResult=FALSE;
		switch(numb)
		{
		case DXI_BUTTON0:
			if(od->dwOfs==DIMOFS_BUTTON0)
			{
				state=od->dwData;
				if(state&0x80)
				{
					bResult=TRUE;
				}
			}
			break;
		case DXI_BUTTON1:
			if(od->dwOfs==DIMOFS_BUTTON1)
			{
				state=od->dwData;
				if(state&0x80)
				{
					bResult=TRUE;
				}
			}
			break;
		case DXI_BUTTON2:
			if(od->dwOfs==DIMOFS_BUTTON2)
			{
				state=od->dwData;
				if(state&0x80)
				{
					bResult=TRUE;
				}
			}
			break;
		case DXI_BUTTON3:
			if(od->dwOfs==DIMOFS_BUTTON3)
			{
				state=od->dwData;
				if(state&0x80)
				{
					bResult=TRUE;
				}
			}
			break;
		case DXI_BUTTON4:
			if(od->dwOfs==DIMOFS_BUTTON4)
			{
				state=od->dwData;
				if(state&0x80)
				{
					bResult=TRUE;
				}
			}
			break;
		case DXI_BUTTON5:
			if(od->dwOfs==DIMOFS_BUTTON5)
			{
				state=od->dwData;
				if(state&0x80)
				{
					bResult=TRUE;
				}
			}
			break;
		case DXI_BUTTON6:
			if(od->dwOfs==DIMOFS_BUTTON6)
			{
				state=od->dwData;
				if(state&0x80)
				{
					bResult=TRUE;
				}
			}
			break;
		case DXI_BUTTON7:
			if(od->dwOfs==DIMOFS_BUTTON7)
			{
				state=od->dwData;
				if(state&0x80)
				{
					bResult=TRUE;
				}
			}
			break;
		default:
			return FALSE;
		}

		if(bResult)
		{
			if(!iTime1)
			{
				iTime1=od->dwTimeStamp;
			}
			else
			{
				iTime2=od->dwTimeStamp;
			}
		}

		od++;
		nb--;
	}

	if(!iTime2)
	{
		*_iDeltaTime=0;
	}
	else
	{
		*_iDeltaTime=iTime2-iTime1;
	}

	return (iTime1)?TRUE:FALSE;
}

/*-------------------------------------------------------------*/
// DXI_MouseButtonUnPressed - Checks if mouse button was released
//		id: Mouse ID, numb: Button ID (DXI_BUTTON0-7)
//		Returns: TRUE if button released, FALSE if still pressed or no events
BOOL DXI_MouseButtonUnPressed(int id,int numb)
{
DIDEVICEOBJECTDATA	*od;
int					state,nb;

	nb=DI_MouseState[id]->nbele;
	if(!nb) return FALSE;
	od=DI_MouseState[id]->mousestate;
	state=0x80;
	while(nb)
	{
//		state=0x80;
		switch(numb)
		{
		case DXI_BUTTON0:
			if(od->dwOfs==DIMOFS_BUTTON0) state=od->dwData;
			break;
		case DXI_BUTTON1:
			if(od->dwOfs==DIMOFS_BUTTON1) state=od->dwData;
			break;
		case DXI_BUTTON2:
			if(od->dwOfs==DIMOFS_BUTTON2) state=od->dwData;
			break;
		case DXI_BUTTON3:
			if(od->dwOfs==DIMOFS_BUTTON3) state=od->dwData;
			break;
		case DXI_BUTTON4:
			if(od->dwOfs==DIMOFS_BUTTON4) state=od->dwData;
			break;
		case DXI_BUTTON5:
			if(od->dwOfs==DIMOFS_BUTTON5) state=od->dwData;
			break;
		case DXI_BUTTON6:
			if(od->dwOfs==DIMOFS_BUTTON6) state=od->dwData;
			break;
		case DXI_BUTTON7:
			if(od->dwOfs==DIMOFS_BUTTON7) state=od->dwData;
			break;
		default:
			return FALSE;
		}
//		if(!(state&0x80)) return TRUE;
		od++;
		nb--;
	}
	if(!(state&0x80)) return TRUE;
	return FALSE;
}

// DXI_OldMouseButtonPressed - [DISABLED] Check old mouse button state
//		Returns: Always FALSE (function disabled)
BOOL DXI_OldMouseButtonPressed(int id,int numb)
{
DIDEVICEOBJECTDATA	*od;
int					state,nb;

return FALSE;
	nb=DI_MouseState[id]->nbele;
	if(!nb) return FALSE;
//	od=DI_MouseState[id]->old_mousestate;
	while(nb)
	{
		state=0;
		switch(numb)
		{
		case DXI_BUTTON0:
			if(od->dwOfs==DIMOFS_BUTTON0) state=od->dwData;
			break;
		case DXI_BUTTON1:
			if(od->dwOfs==DIMOFS_BUTTON1) state=od->dwData;
			break;
		case DXI_BUTTON2:
			if(od->dwOfs==DIMOFS_BUTTON2) state=od->dwData;
			break;
		case DXI_BUTTON3:
			if(od->dwOfs==DIMOFS_BUTTON3) state=od->dwData;
			break;
		case DXI_BUTTON4:
			if(od->dwOfs==DIMOFS_BUTTON4) state=od->dwData;
			break;
		case DXI_BUTTON5:
			if(od->dwOfs==DIMOFS_BUTTON5) state=od->dwData;
			break;
		case DXI_BUTTON6:
			if(od->dwOfs==DIMOFS_BUTTON6) state=od->dwData;
			break;
		case DXI_BUTTON7:
			if(od->dwOfs==DIMOFS_BUTTON7) state=od->dwData;
			break;
		default:
			return FALSE;
		}
		if(state&0x80) return TRUE;
		od++;
		nb--;
	}
	return FALSE;
}

/*-------------------------------------------------------------*/
//=============================================================================
// SCID (STRATEGIC COMMANDER) INPUT QUERY FUNCTIONS
//=============================================================================

// DXI_GetSCIDAxis - Get SideWinder Strategic Commander joystick axes
//		id: SCID ID, jx/jy/jz: Output pointers for X/Y/Z axes
//		Returns: Directional flags (DXI_JOYLEFT|RIGHT|UP|DOWN|NONE)
int DXI_GetSCIDAxis(int id,int *jx,int *jy,int *jz)
{
INPUT_INFO	*io;
int			dir;

	dir=DXI_JOYNONE;

	io=DI_SCIDState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		{
			DIJOYSTATE2	*js;
			js=io->joystate2;
			*jx=js->lX;
			*jy=js->lY;
			*jz=js->lRz;//>lZ;
			if(js->lX>0)
			{
				dir|=DXI_JOYRIGHT;
			}
			else
			{
				if(js->lX<0)
				{
					dir|=DXI_JOYLEFT;
				}
			}

			if(js->lY>0)
			{
				dir|=DXI_JOYDOWN;
			}
			else
			{
				if(js->lY<0)
				{
					dir|=DXI_JOYUP;
				}
			}
		}
	}
	else
	{
		{
			DIJOYSTATE	*js;
			js=io->joystate;
			*jx=js->lX;
			*jy=js->lY;
			*jz=js->lZ;
			if(js->lX>0)
			{
				dir|=DXI_JOYRIGHT;
			}
			else
			{
				if(js->lX<0)
				{
					dir|=DXI_JOYLEFT;
				}
			}

			if(js->lY>0)
			{
				dir|=DXI_JOYDOWN;
			}
			else
			{
				if(js->lY<0)
				{
					dir|=DXI_JOYUP;
				}
			}
		}
	}

	return dir;
}

// DXI_IsSCIDButtonPressed - Check if SCID button is pressed
//		id: SCID ID, numb: Button number (0-127)
//		Returns: TRUE if pressed, FALSE otherwise
BOOL DXI_IsSCIDButtonPressed(int id,int numb)
{
INPUT_INFO	*io;

	io=DI_SCIDState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		{
			DIJOYSTATE2	*js;
			js=io->joystate2;
			if(js->rgbButtons[numb]&0x80) return TRUE;
		}
	}
	else
	{
		{
			DIJOYSTATE	*js;
			js=io->joystate;
			if(js->rgbButtons[numb]&0x80) return TRUE;
		}
	}
	return FALSE;
}

// DXI_GetSCIDButtonPressed - Get first pressed SCID button
//		id: SCID ID
//		Returns: Button number (0-127), or -1 if none pressed
int DXI_GetSCIDButtonPressed(int id)
{
	INPUT_INFO	*io;
	int			nb;

	io=DI_SCIDState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		{
			DIJOYSTATE2	*js;
			js=io->joystate2;
			nb=128;
			while(nb--)
			{
				if (nb==9) 
					nb=9;
				if(js->rgbButtons[nb]&0x80) return nb;
			}
		}
	}
	else
	{
		{
			DIJOYSTATE	*js;
			js=io->joystate;
			nb=32;
			while(nb--)
			{
				if(js->rgbButtons[nb]&0x80) return nb;
			}
		}
	}
	return -1;
}
/*
DIDEVICEOBJECTDATA	*od;
int					nb;

	nb=DI_SCIDState[id]->nbele;
	if(!nb) return -1;
	od=DI_SCIDState[id]->SCIDstate;
	while(nb)
	{
		switch(od->dwOfs)
		{
		case DIMOFS_BUTTON0:
			if(od->dwData&0x80) return DXI_BUTTON0;
			break;
		case DIMOFS_BUTTON1:
			if(od->dwData&0x80) return DXI_BUTTON1;
			break;
		case DIMOFS_BUTTON2:
			if(od->dwData&0x80) return DXI_BUTTON2;
			break;
		case DIMOFS_BUTTON3:
			if(od->dwData&0x80) return DXI_BUTTON3;
			break;
		case DIMOFS_BUTTON4:
			if(od->dwData&0x80) return DXI_BUTTON4;
			break;
		case DIMOFS_BUTTON5:
			if(od->dwData&0x80) return DXI_BUTTON5;
			break;
		case DIMOFS_BUTTON6:
			if(od->dwData&0x80) return DXI_BUTTON6;
			break;
		case DIMOFS_BUTTON7:
			if(od->dwData&0x80) return DXI_BUTTON7;
			break;
		case DIMOFS_BUTTON8:
			if(od->dwData&0x80) return DXI_BUTTON8;
			break;
		case DIMOFS_BUTTON9:
			if(od->dwData&0x80) return DXI_BUTTON9;
			break;
		case DIMOFS_BUTTON10:
			if(od->dwData&0x80) return DXI_BUTTON10;
			break;
		case DIMOFS_BUTTON11:
			if(od->dwData&0x80) return DXI_BUTTON11;
			break;
		case DIMOFS_BUTTON12:
			if(od->dwData&0x80) return DXI_BUTTON12;
			break;
		case DIMOFS_BUTTON13:
			if(od->dwData&0x80) return DXI_BUTTON13;
			break;
		case DIMOFS_BUTTON14:
			if(od->dwData&0x80) return DXI_BUTTON14;
			break;
		case DIMOFS_BUTTON15:
			if(od->dwData&0x80) return DXI_BUTTON15;
			break; 
		default:
			break;
		}
		od++;
		nb--;
	}

	return -1;
}
*/

/*-------------------------------------------------------------*/
//=============================================================================
// MOUSE BUTTON AND CONFIGURATION FUNCTIONS
//=============================================================================

// DXI_GetIDButtonPressed - Get first pressed mouse button
//		id: Mouse ID
//		Returns: Button ID (DXI_BUTTON0-7), or -1 if none pressed
int DXI_GetIDButtonPressed(int id)
{
DIDEVICEOBJECTDATA	*od;
int					nb;

	nb=DI_MouseState[id]->nbele;
	if(!nb) return -1;
	od=DI_MouseState[id]->mousestate;
	while(nb)
	{
		switch(od->dwOfs)
		{
		case DIMOFS_BUTTON0:
			if(od->dwData&0x80) return DXI_BUTTON0;
			break;
		case DIMOFS_BUTTON1:
			if(od->dwData&0x80) return DXI_BUTTON1;
			break;
		case DIMOFS_BUTTON2:
			if(od->dwData&0x80) return DXI_BUTTON2;
			break;
		case DIMOFS_BUTTON3:
			if(od->dwData&0x80) return DXI_BUTTON3;
			break;
		case DIMOFS_BUTTON4:
			if(od->dwData&0x80) return DXI_BUTTON4;
			break;
		case DIMOFS_BUTTON5:
			if(od->dwData&0x80) return DXI_BUTTON5;
			break;
		case DIMOFS_BUTTON6:
			if(od->dwData&0x80) return DXI_BUTTON6;
			break;
		case DIMOFS_BUTTON7:
			if(od->dwData&0x80) return DXI_BUTTON7;
			break;
		default:
			break;
		}
		od++;
		nb--;
	}

	return -1;
}

/*-------------------------------------------------------------*/
// DXI_SetMouseRelative - Set mouse to relative (delta) mode
//		id: Mouse ID
//		Returns: DXI_OK on success, DXI_FAIL on failure
//		Note: Relative mode reports movement deltas, not absolute position
int DXI_SetMouseRelative(int id)
{
INPUT_INFO		*info;
DIPROPDWORD		dipdw={
				{
					sizeof(DIPROPDWORD),        // diph.dwSize
					sizeof(DIPROPHEADER),       // diph.dwHeaderSize
					0,			                // diph.dwObj
					DIPH_DEVICE,	            // diph.dwHow
				},
				DIPROPAXISMODE_REL,				// dwData
				};

	info=DXI_GetInputInfoWithState((void*)DI_MouseState[id],DIDEVTYPE_MOUSE);
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Unacquire(info->inputdevice7))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->Unacquire())) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->SetProperty(info->inputdevice7,DIPROP_AXISMODE,&dipdw.diph))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->SetProperty(DIPROP_AXISMODE,&dipdw.diph))) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Acquire(info->inputdevice7))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->Acquire())) return DXI_FAIL;
	return DXI_OK;
}

/*-------------------------------------------------------------*/
// DXI_SetMouseAbsolue - Set mouse to absolute position mode
//		id: Mouse ID
//		Returns: DXI_OK on success, DXI_FAIL on failure
//		Note: Absolute mode reports screen position coordinates
int DXI_SetMouseAbsolue(int id)
{
INPUT_INFO		*info;
DIPROPDWORD		dipdw={
				{
					sizeof(DIPROPDWORD),        // diph.dwSize
					sizeof(DIPROPHEADER),       // diph.dwHeaderSize
					0,			                // diph.dwObj
					DIPH_DEVICE,	            // diph.dwHow
				},
				DIPROPAXISMODE_ABS,				// dwData
				};

	info=DXI_GetInputInfoWithState((void*)DI_MouseState[id],DIDEVTYPE_MOUSE);
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Unacquire(info->inputdevice7))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->Unacquire())) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->SetProperty(info->inputdevice7,DIPROP_AXISMODE,&dipdw.diph))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->SetProperty(DIPROP_AXISMODE,&dipdw.diph))) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Acquire(info->inputdevice7))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->Acquire())) return DXI_FAIL;
	return DXI_OK;
}

/*-------------------------------------------------------------*/
//=============================================================================
// JOYSTICK INPUT QUERY FUNCTIONS
//=============================================================================

// DXI_GetAxeJoyXY - Get joystick X/Y axes with directional flags
//		id: Joystick ID, jx/jy: Output pointers for X/Y values
//		Returns: Directional flags (DXI_JOYLEFT|RIGHT|UP|DOWN|NONE)
int DXI_GetAxeJoyXY(int id,int *jx,int *jy)
{
INPUT_INFO	*io;
int			dir;

	dir=DXI_JOYNONE;

	io=DI_JoyState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		{
			DIJOYSTATE2	*js;
			js=io->joystate2;
			*jx=js->lX;
			*jy=js->lY;
			if(js->lX>0)
			{
				dir|=DXI_JOYRIGHT;
			}
			else
			{
				if(js->lX<0)
				{
					dir|=DXI_JOYLEFT;
				}
			}

			if(js->lY>0)
			{
				dir|=DXI_JOYDOWN;
			}
			else
			{
				if(js->lY<0)
				{
					dir|=DXI_JOYUP;
				}
			}
		}
	}
	else
	{
		{
			DIJOYSTATE	*js;
			js=io->joystate;
			*jx=js->lX;
			*jy=js->lY;
			if(js->lX>0)
			{
				dir|=DXI_JOYRIGHT;
			}
			else
			{
				if(js->lX<0)
				{
					dir|=DXI_JOYLEFT;
				}
			}

			if(js->lY>0)
			{
				dir|=DXI_JOYDOWN;
			}
			else
			{
				if(js->lY<0)
				{
					dir|=DXI_JOYUP;
				}
			}
		}
	}

	return dir;
}

/*-------------------------------------------------------------*/
// DXI_GetAxeJoyXYZ - Get joystick X/Y/Z axes with directional flags
//		id: Joystick ID, jx/jy/jz: Output pointers for X/Y/Z values
//		Returns: Directional flags for X/Y axes
int DXI_GetAxeJoyXYZ(int id,int *jx,int *jy,int *jz)
{
INPUT_INFO	*io;
int			dir;

	dir=DXI_JOYNONE;

	io=DI_JoyState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		{
			DIJOYSTATE2	*js;
			js=io->joystate2;
			*jx=js->lX;
			*jy=js->lY;
			*jz=js->lZ;
			if(js->lX>0)
			{
				dir|=DXI_JOYRIGHT;
			}
			else
			{
				if(js->lX<0)
				{
					dir|=DXI_JOYLEFT;
				}
			}

			if(js->lY>0)
			{
				dir|=DXI_JOYDOWN;
			}
			else
			{
				if(js->lY<0)
				{
					dir|=DXI_JOYUP;
				}
			}
		}
	}
	else
	{
		{
			DIJOYSTATE	*js;
			js=io->joystate;
			*jx=js->lX;
			*jy=js->lY;
			*jz=js->lZ;
			if(js->lX>0)
			{
				dir|=DXI_JOYRIGHT;
			}
			else
			{
				if(js->lX<0)
				{
					dir|=DXI_JOYLEFT;
				}
			}

			if(js->lY>0)
			{
				dir|=DXI_JOYDOWN;
			}
			else
			{
				if(js->lY<0)
				{
					dir|=DXI_JOYUP;
				}
			}
		}
	}

	return dir;
}

/*-------------------------------------------------------------*/
// DXI_GetAxeJoyXYZW - Get joystick X/Y/Z/W (slider) axes
//		id: Joystick ID, jx/jy/jz/jw: Output pointers for all 4 axes
//		Returns: 1 (always succeeds)
//		Note: Z is rotation (Rz), W is slider control
int DXI_GetAxeJoyXYZW(int id,int *jx,int *jy,int *jz,int * jw)
{
INPUT_INFO	*io;
//int			dir;

//	dir=DXI_JOYNONE;

	io=DI_JoyState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		DIJOYSTATE2	*js;
		js=io->joystate2;
		*jx=js->lX;
		*jy=js->lY;
		*jz=js->lRz;
		*jw=js->rglSlider[0];			
	}
	else
	{
		DIJOYSTATE	*js;
		js=io->joystate;
		*jx=js->lX;
		*jy=js->lY;
		*jz=js->lZ;		
		*jw=0;//js->rglSlider;			
	}

	return 1;
}

/*-------------------------------------------------------------*/
//=============================================================================
// JOYSTICK CONFIGURATION FUNCTIONS
//=============================================================================

// DXI_SetJoyRelative - Set joystick to relative (delta) mode
//		id: Joystick ID
//		Returns: DXI_OK on success, DXI_FAIL on failure
int DXI_SetJoyRelative(int id)
{
INPUT_INFO		*info;
DIPROPDWORD		dipdw={
				{
					sizeof(DIPROPDWORD),        // diph.dwSize
					sizeof(DIPROPHEADER),       // diph.dwHeaderSize
					0,			                // diph.dwObj
					DIPH_DEVICE,	            // diph.dwHow
				},
				DIPROPAXISMODE_REL,				// dwData
				};

	info=DXI_GetInputInfoWithState((void*)DI_JoyState[id],DIDEVTYPE_JOYSTICK);
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Unacquire(info->inputdevice7))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->Unacquire())) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->SetProperty(info->inputdevice7,DIPROP_AXISMODE,&dipdw.diph))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->SetProperty(DIPROP_AXISMODE,&dipdw.diph))) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Acquire(info->inputdevice7))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->Acquire())) return DXI_FAIL;
	return DXI_OK;
}

/*-------------------------------------------------------------*/
// DXI_SetJoyAbsolue - Set joystick to absolute position mode
//		id: Joystick ID
//		Returns: DXI_OK on success, DXI_FAIL on failure
int DXI_SetJoyAbsolue(int id)
{
INPUT_INFO		*info;
DIPROPDWORD		dipdw={
				{
					sizeof(DIPROPDWORD),        // diph.dwSize
					sizeof(DIPROPHEADER),       // diph.dwHeaderSize
					0,			                // diph.dwObj
					DIPH_DEVICE,	            // diph.dwHow
				},
				DIPROPAXISMODE_ABS,				// dwData
				};

	info=DXI_GetInputInfoWithState((void*)DI_JoyState[id],DIDEVTYPE_JOYSTICK);
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Unacquire(info->inputdevice7))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->Unacquire())) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->SetProperty(info->inputdevice7,DIPROP_AXISMODE,&dipdw.diph))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->SetProperty(DIPROP_AXISMODE,&dipdw.diph))) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->Acquire(info->inputdevice7))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->Acquire())) return DXI_FAIL;
	return DXI_OK;
}

/*-------------------------------------------------------------*/
// DXI_SetRangeJoy - Configure joystick axis range and deadzone
//		id: Joystick ID
//		axe: Axis to configure (DXI_XAxis, DXI_YAxis, DXI_ZAxis, DXI_RzAxis, DXI_Slider)
//		range: Axis value range (-range to +range)
//		Returns: DXI_OK on success, DXI_FAIL on failure
//		Note: Also sets 50% deadzone (5000 out of 10000)
int DXI_SetRangeJoy(int id,int axe,int range)
{
INPUT_INFO		*info;
DIPROPRANGE		diprg; 
DIPROPDWORD		dipdw={
				{
					sizeof(DIPROPDWORD),        // diph.dwSize
					sizeof(DIPROPHEADER),       // diph.dwHeaderSize
					0,			                // diph.dwObj
					DIPH_BYOFFSET,	            // diph.dwHow
				},
				0,								// dwData
				};

	if(!range) return DXI_FAIL;
	
	info=DXI_GetInputInfoWithState((void*)DI_JoyState[id],DIDEVTYPE_JOYSTICK);

	diprg.diph.dwSize=sizeof(diprg); 
	diprg.diph.dwHeaderSize=sizeof(diprg.diph); 
	switch(axe)
	{
	case DXI_XAxis:
		diprg.diph.dwObj=DIJOFS_X; 
		dipdw.diph.dwObj=DIJOFS_X;
		break;
	case DXI_YAxis:
		diprg.diph.dwObj=DIJOFS_Y; 
		dipdw.diph.dwObj=DIJOFS_Y;
		break;
	case DXI_ZAxis:
		diprg.diph.dwObj=DIJOFS_Z; 
		dipdw.diph.dwObj=DIJOFS_Z;
		break;
	case DXI_RzAxis:
		diprg.diph.dwObj=DIJOFS_RZ; 
		dipdw.diph.dwObj=DIJOFS_RZ;
		break;
	case DXI_Slider:
		diprg.diph.dwObj=DIJOFS_SLIDER(0); 
		dipdw.diph.dwObj=DIJOFS_SLIDER(0);
		break;
	default:
		return DXI_FAIL;
	}
	diprg.diph.dwHow=DIPH_BYOFFSET; 
	diprg.lMin=-range; 
	diprg.lMax=range; 
	dipdw.dwData=5000;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->SetProperty(info->inputdevice7,DIPROP_RANGE,&diprg.diph))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->SetProperty(DIPROP_RANGE,&diprg.diph))) return DXI_FAIL;
	//Old : if(FAILED(DI_Hr=info->inputdevice7->lpVtbl->SetProperty(info->inputdevice7,DIPROP_DEADZONE,&dipdw.diph))) return DXI_FAIL;
	if(FAILED(DI_Hr=info->inputdevice7->SetProperty(DIPROP_DEADZONE,&dipdw.diph))) return DXI_FAIL;

	return DXI_OK;
}

/*-------------------------------------------------------------*/
//=============================================================================
// JOYSTICK BUTTON QUERY FUNCTIONS
//=============================================================================

// DXI_GetJoyButtonPressed - Check if joystick button is pressed
//		id: Joystick ID, numb: Button number (0-127)
//		Returns: TRUE if pressed, FALSE otherwise
BOOL DXI_GetJoyButtonPressed(int id,int numb)
{
INPUT_INFO	*io;

	io=DI_JoyState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		{
			DIJOYSTATE2	*js;
			js=io->joystate2;
			if(js->rgbButtons[numb]&0x80) return TRUE;
		}
	}
	else
	{
		{
			DIJOYSTATE	*js;
			js=io->joystate;
			if(js->rgbButtons[numb]&0x80) return TRUE;
		}
	}
	return FALSE;
}

// DXI_OldGetJoyButtonPressed - [DISABLED] Check old joystick button state
//		Returns: Always FALSE (function disabled)
BOOL DXI_OldGetJoyButtonPressed(int id,int numb)
{
INPUT_INFO	*io;

return FALSE;
	io=DI_JoyState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		{
			DIJOYSTATE2	*js;
//			js=io->old_joystate2;
			if(js->rgbButtons[numb]&0x80) return TRUE;
		}
	}
	else
	{
		{
			DIJOYSTATE	*js;
//			js=io->old_joystate;
			if(js->rgbButtons[numb]&0x80) return TRUE;
		}
	}
	return FALSE;
}

/*-------------------------------------------------------------*/
// DXI_GetIDJoyButtonPressed - Get first pressed joystick button
//		id: Joystick ID
//		Returns: Button number (0-127), or -1 if none pressed
int DXI_GetIDJoyButtonPressed(int id)
{
INPUT_INFO	*io;
int			nb;

	io=DI_JoyState[id];
	if(io->datasid==DFDIJOYSTICK2)
	{
		{
			DIJOYSTATE2	*js;
			js=io->joystate2;
			nb=128;
			while(nb--)
			{
				if(js->rgbButtons[nb]&0x80) return nb;
			}
		}
	}
	else
	{
		{
			DIJOYSTATE	*js;
			js=io->joystate;
			nb=32;
			while(nb--)
			{
				if(js->rgbButtons[nb]&0x80) return nb;
			}
		}
	}
	return -1;
}
/*-------------------------------------------------------------*/

