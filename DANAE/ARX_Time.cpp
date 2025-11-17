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
// ARX_Time.CPP - Game Time Management System
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		High-precision game time and frame timing system for Arx Fatalis
//		Uses Windows QueryPerformanceCounter for microsecond-accurate timing
//		Manages game time, pause state, and frame delta calculations
//
// Purpose:
//		- Provide high-resolution game time using hardware performance counters
//		- Track elapsed game time independent of real-world time
//		- Handle pause/unpause functionality for menus and game states
//		- Calculate frame delta times for smooth animation and physics
//		- Support save/load time restoration
//
// Key Responsibilities:
//		- Initialize Windows performance counter (QueryPerformanceFrequency)
//		- Query current time in milliseconds (_ARX_TIME_GetTime)
//		- Maintain game time vs paused time separation
//		- Track total paused duration for accurate game time calculations
//		- Provide frame timing for game loop synchronization
//
// Time Variables:
//		ARXTime              - Current game time (excludes paused time)
//		ARXPausedTime        - Timestamp when pause began
//		ARXTotalPausedTime   - Cumulative paused duration
//		ARXPausedTimer       - Boolean pause state flag
//		FrameTime            - Current frame timestamp
//		LastFrameTime        - Previous frame timestamp
//
// API Functions:
//		ARX_TIME_Init()                    - Initialize timing system
//		_ARX_TIME_GetTime()                - Get raw performance counter time
//		ARX_TIME_Pause()                   - Pause game time
//		ARX_TIME_UnPause()                 - Resume game time
//		ARX_TIME_Force_Time_Restore(time)  - Set game time (for save/load)
//
// Usage Flow:
//		1. ARX_TIME_Init() - Call once at game startup
//		2. Each frame: Query time for delta calculations
//		3. On menu open: ARX_TIME_Pause()
//		4. On menu close: ARX_TIME_UnPause()
//		5. On load game: ARX_TIME_Force_Time_Restore(savedTime)
//
// Technical Notes:
//		- Uses LARGE_INTEGER for 64-bit performance counter values
//		- Frequency queried once at initialization (hardware-dependent)
//		- Time returned in milliseconds as floating-point
//		- Pause tracking ensures game time doesn't advance when paused
//		- Critical for frame-rate independent game logic
//
// Dependencies:
//		- Windows.h (QueryPerformanceCounter/Frequency)
//		- ARX_Time.h (declarations)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
#include "ARX_Time.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


extern float FrameTime, LastFrameTime;	//ARX: jycorbel (2010-07-19) - Add external vars for resetting them on ARX_TIME_Init call.

/////////////////////// GAMETIME MANAGEMENT /////////////////////////
float ARXPausedTime = 0;
float ARXTotalPausedTime = 0;
float ARXTime = 0;
bool ARXPausedTimer = 0;

//-----------------------------------------------------------------------------
LARGE_INTEGER	liFrequency;
bool			bTimerInit = false;

void _ARX_TIME_Init()
{
	if (bTimerInit)
	{
		return;
	}

	QueryPerformanceFrequency(&liFrequency);
	bTimerInit = true;

	ARX_TIME_Init();
}

float _ARX_TIME_GetTime()
{
	LARGE_INTEGER liPerfCounter;

	_ARX_TIME_Init();

	QueryPerformanceCounter(&liPerfCounter);
	return ARX_CLEAN_WARN_CAST_FLOAT((liPerfCounter.QuadPart / (double)liFrequency.QuadPart) * 1000);

}


//-----------------------------------------------------------------------------
void ARX_TIME_Init()
{
	_ARX_TIME_Init();

	float tim = _ARX_TIME_GetTime();
	ARXTotalPausedTime = tim;
	ARXTime = 0;
	ARXPausedTime = 0;
	ARXPausedTimer = 0;

//ARX_BEGIN: jycorbel (2010-07-19) - Add external vars for resetting them on ARX_TIME_Init call.
//Currently when ARX_TIME_Init the substract FrameDiff = FrameTime - LastFrameTime is negative because of resetting ARXTotalPausedTime.
//This solution reinit FrameTime & LastFrameTime to get a min frameDiff = 0 on ARX_TIME_Init.
	FrameTime = LastFrameTime = ARXTime;
//ARX_END: jycorbel (2010-07-19)
}

//-----------------------------------------------------------------------------
void ARX_TIME_Pause()
{
	if (!ARXPausedTimer)
	{
		float tim = _ARX_TIME_GetTime();
		ARXPausedTime = tim;
		ARXPausedTimer = 1;
	}
}

//-----------------------------------------------------------------------------
void ARX_TIME_UnPause()
{
	if (ARXPausedTimer)
	{
		float tim = _ARX_TIME_GetTime();
		ARXTotalPausedTime += tim - ARXPausedTime;
		ARXPausedTime = 0;
		ARXPausedTimer = 0;
	}
}

//-----------------------------------------------------------------------------
void ARX_TIME_Force_Time_Restore(float time)
{
	float tim = _ARX_TIME_GetTime();
	ARXTotalPausedTime = tim - time;
	ARXTime = time;
	ARXPausedTime = 0;
	ARXPausedTimer = 0;
}
