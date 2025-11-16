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
// ARX_Levels
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		ARX Levels Management
//
// Updates: (date) (person) (update)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
//=============================================================================
// FILE: ARX_Levels.cpp
//=============================================================================
// Component: DANAE Game Engine - Level and Area Management
// Author: Cyril Meynier
//
// PURPOSE:
//		Manages game levels, level transitions, and level subdivision system.
//		Handles mapping between logical level numbers and subdivided areas.
//
// ARCHITECTURE:
//		Level subdivision system for large continuous game world:
//
//		Physical Level Structure:
//		- Game world divided into multiple connected levels
//		- Some levels are subdivided into smaller areas for performance
//		- Each subdivision is a separate loadable area
//		- Seamless transitions between subdivisions
//
//		Level Numbering System:
//		Logical Levels (Story/Design):
//		- Level 0: Starting dungeon
//		- Level 1: Underground passages
//		- Level 2: Goblin territory
//		- Level 3: Crypts
//		- Level 4-7: Various dungeon areas
//
//		Physical Subdivisions (Technical):
//		- Level 0 → Files: level0.dlf, level8.dlf, level11.dlf, level12.dlf
//		- Level 1 → Files: level1.dlf, level13.dlf, level14.dlf
//		- Level 2 → Files: level2.dlf, level15.dlf
//		- Single level split across multiple files for memory management
//
// KEY FEATURES:
//		Level Number Mapping:
//		- ARX_LEVELS_GetRealNum: Convert physical subdivision to logical level
//		- Example: GetRealNum(13) = 1 (subdivision 13 is part of level 1)
//		- Used for save games, quest tracking, player status
//		- Ensures game logic sees unified level numbers
//
//		Level Subdivision Benefits:
//		- Memory Management: Load only active area + neighbors
//		- Performance: Smaller areas = faster rendering
//		- Streaming: Load/unload areas seamlessly
//		- Large Levels: Can exceed memory limits via subdivision
//
//		Level Loading:
//		- Load current subdivision
//		- Preload adjacent subdivisions in background
//		- Unload distant subdivisions to free memory
//		- Track which subdivisions are loaded
//
//		Level Transitions:
//		- Detect when player approaches subdivision boundary
//		- Trigger loading of next subdivision
//		- Wait for load completion
//		- Seamlessly switch active area
//		- Maintain game state across transition
//
// ALGORITHMS:
//		ARX_LEVELS_GetRealNum (Subdivision to Logical Level):
//		Function: GetRealNum(subdivisionNum) -> logicalLevel
//
//		Switch (subdivisionNum):
//		  Case 0, 8, 11, 12: Return 0 (all are level 0 subdivisions)
//		  Case 1, 13, 14: Return 1 (all are level 1 subdivisions)
//		  Case 2, 15: Return 2 (all are level 2 subdivisions)
//		  Case 3: Return 3
//		  Case 4: Return 4
//		  ...
//		  Default: Return subdivisionNum (1:1 mapping)
//
//		Example Usage:
//		Player in physical area 13 → GetRealNum(13) = 1
//		Quest check: "if playerLevel == 1" → TRUE
//		Save file: Store logical level 1, not subdivision 13
//
//		Level Loading Sequence:
//		1. Determine target subdivision number
//		2. Check if already loaded: If yes, just switch active area
//		3. If not loaded:
//		   a. Show loading screen
//		   b. Unload distant subdivisions to free memory
//		   c. Load level geometry (.DLF file)
//		   d. Load level scripts and entities
//		   e. Initialize AI pathfinding for area
//		   f. Restore dynamic state (opened doors, killed enemies)
//		4. Switch camera to new area
//		5. Hide loading screen
//		6. Resume gameplay
//
// LEVEL FILE FORMAT:
//		.DLF Files (DANAE Level Format):
//		- Binary format storing level data
//		- Contains:
//		  * Static geometry (walls, floors, ceilings)
//		  * Portal connections between rooms
//		  * Light sources and lighting data
//		  * Interactive object placement
//		  * Pathfinding navigation mesh
//		  * Zone markers and triggers
//		  * Ambient sound sources
//
//		File Structure:
//		Header: Magic number, version, level info
//		Geometry: Vertices, polygons, textures
//		Objects: Placement, rotation, scale for all interactive objects
//		Paths: Pathfinding nodes and connections
//		Lights: Position, color, radius, flags
//		Portals: Room connectivity for rendering optimization
//		Scripts: Zone scripts and trigger conditions
//
// SUBDIVISION STRATEGY:
//		Why Subdivide:
//		- Memory Limit: Year 2000 PCs had 128-256MB RAM
//		- Large level: 50MB+ geometry + textures
//		- Solution: Split into 10MB chunks
//		- Only load visible chunk + neighbors
//
//		Subdivision Boundaries:
//		- Place at natural chokepoints (doors, hallways)
//		- Avoid splitting large open areas
//		- Ensure smooth transitions (no visible pop-in)
//		- Portal system hides loading latency
//
//		Example Level 0 Subdivisions:
//		- Sub 0: Starting area (prison cells)
//		- Sub 8: East wing (torture chamber)
//		- Sub 11: West wing (guard quarters)
//		- Sub 12: Central hub (main hall)
//		- All logically part of "Level 0" dungeon
//
// INTEGRATION:
//		Works with DanaeSaveLoad for saving level state
//		Uses ARX_Paths for cross-level path management
//		Coordinates with ARX_Scene for rendering active area
//		Integrates with ARX_Spells for persistent spell effects
//		Uses HERMES pathfinding for NPC navigation
//		Coordinates with ARX_Sound for area ambient audio
//
// SAVE/LOAD CONSIDERATIONS:
//		Save Files Store:
//		- Logical level number (not subdivision)
//		- Player position within level
//		- Dynamic state (doors, items, NPCs)
//		- Quest progress
//
//		On Load:
//		- Map logical level to appropriate subdivision
//		- Load subdivision containing player position
//		- Restore dynamic state
//		- Place player at saved position
//
// PERFORMANCE:
//		Memory Usage Per Subdivision:
//		- Small subdivision: 5-10MB (simple corridor)
//		- Medium subdivision: 10-20MB (typical room)
//		- Large subdivision: 20-30MB (grand hall)
//		- Keep 2-3 subdivisions loaded simultaneously
//
//		Loading Time:
//		- Small subdivision: 0.5-1 second
//		- Medium subdivision: 1-2 seconds
//		- Large subdivision: 2-4 seconds
//		- Background loading hides most delays
//
// USE CASES:
//		1. Player Progress: GetRealNum(currentArea) to check quest requirements
//		2. Save Game: Store logical level instead of subdivision number
//		3. Load Game: Map logical level to correct subdivision
//		4. Quest System: Check "if player reached level 3"
//		5. Difficulty Scaling: Adjust encounters based on logical level
//		6. Story Tracking: Trigger events when entering new logical level
//=============================================================================
#include "ARX_LEVELS.h"
#include "DanaeSaveLoad.h"
#include "time.h"
#include "ARX_Paths.h"
#include "ARX_Scene.h"
//#include "ARX_Test.h"
#include "ARX_Spells.h"
#include "ARX_Speech.h"
#include "ARX_Sound.h"

#include "HERMESMain.h"
#include "EERIEPathfinder.h"
#include "EERIECollisionSpheres.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

extern long LOAD_N_DONT_ERASE;
extern long DONT_LOAD_SCENE;
extern long FASTmse;
extern float InventoryDir;

//-----------------------------------------------------------------------------
//Used to know the true level of a "subdivided" level
long ARX_LEVELS_GetRealNum(long num)
{
	switch (num)
	{
		case 0:
		case 8:
		case 11:
		case 12:
			return 0;
			break;
		case 1:
		case 13:
		case 14:
			return 1;
			break;
		case 2:
		case 15:
			return 2;
			break;
		case 3:
		case 16:
		case 17:
			return 3;
			break;
		case 4:
		case 18:
		case 19:
			return 4;
			break;
		case 5:
		case 21:
			return 5;
			break;
		case 6:
		case 22:
			return 6;
			break;
		case 7:
		case 23:
			return 7;
			break;
	}

	if (num < 0) return -1;

	return num;
}

long GetLevelNumByName(char * name)
{
	if (name)
	{
		char temp[256];
		strcpy(temp, name);
		MakeUpcase(temp);

		if (IsIn(temp, "LEVEL10")) return 10;

		if (IsIn(temp, "LEVEL11")) return 11;

		if (IsIn(temp, "LEVEL12")) return 12;

		if (IsIn(temp, "LEVEL13")) return 13;

		if (IsIn(temp, "LEVEL14")) return 14;

		if (IsIn(temp, "LEVEL15")) return 15;

		if (IsIn(temp, "LEVEL16")) return 16;

		if (IsIn(temp, "LEVEL17")) return 17;

		if (IsIn(temp, "LEVEL18")) return 18;

		if (IsIn(temp, "LEVEL19")) return 19;

		if (IsIn(temp, "LEVEL20")) return 20;

		if (IsIn(temp, "LEVEL21")) return 21;

		if (IsIn(temp, "LEVEL22")) return 22;

		if (IsIn(temp, "LEVEL23")) return 23;

		if (IsIn(temp, "LEVEL24")) return 24;

		if (IsIn(temp, "LEVEL25")) return 25;

		if (IsIn(temp, "LEVEL26")) return 26;

		if (IsIn(temp, "LEVEL27")) return 27;

		if (IsIn(temp, "LEVELDEMO2")) return 29;

		if (IsIn(temp, "LEVELDEMO3")) return 30;

		if (IsIn(temp, "LEVELDEMO4")) return 31;

		if (IsIn(temp, "LEVELDEMO")) return 28;

		if (IsIn(temp, "LEVEL0")) return 0;

		if (IsIn(temp, "LEVEL1")) return 1;

		if (IsIn(temp, "LEVEL2")) return 2;

		if (IsIn(temp, "LEVEL3")) return 3;

		if (IsIn(temp, "LEVEL4")) return 4;

		if (IsIn(temp, "LEVEL5")) return 5;

		if (IsIn(temp, "LEVEL6")) return 6;

		if (IsIn(temp, "LEVEL7")) return 7;

		if (IsIn(temp, "LEVEL8")) return 8;

		if (IsIn(temp, "LEVEL9")) return 9;
	}

	return -1;
}
void GetLevelNameByNum(long num, char * name)
{
	if (name)
	{
		strcpy(name, "NONE");

		if (num == 0) strcpy(name, "0");

		if (num == 1) strcpy(name, "1");

		if (num == 2) strcpy(name, "2");

		if (num == 3) strcpy(name, "3");

		if (num == 4) strcpy(name, "4");

		if (num == 5) strcpy(name, "5");

		if (num == 6) strcpy(name, "6");

		if (num == 7) strcpy(name, "7");

		if (num == 8) strcpy(name, "8");

		if (num == 9) strcpy(name, "9");

		if (num == 10) strcpy(name, "10");

		if (num == 11) strcpy(name, "11");

		if (num == 12) strcpy(name, "12");

		if (num == 13) strcpy(name, "13");

		if (num == 14) strcpy(name, "14");

		if (num == 15) strcpy(name, "15");

		if (num == 16) strcpy(name, "16");

		if (num == 17) strcpy(name, "17");

		if (num == 18) strcpy(name, "18");

		if (num == 19) strcpy(name, "19");

		if (num == 20) strcpy(name, "20");

		if (num == 21) strcpy(name, "21");

		if (num == 22) strcpy(name, "22");

		if (num == 23) strcpy(name, "23");

		if (num == 24) strcpy(name, "24");

		if (num == 25) strcpy(name, "25");

		if (num == 26) strcpy(name, "26");

		if (num == 27) strcpy(name, "27");

		if (num == 28) strcpy(name, "DEMO");

		if (num == 29) strcpy(name, "DEMO2");

		if (num == 30) strcpy(name, "DEMO3");

		if (num == 31) strcpy(name, "DEMO4");
	}
}
