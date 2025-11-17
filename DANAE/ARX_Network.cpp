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
// ARX_Network.CPP - Network System Stub
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Placeholder for multiplayer networking functionality in Arx Fatalis
//		This file is largely empty - networking was planned but not implemented
//		Arx Fatalis shipped as a single-player only game
//
// Purpose:
//		- Reserved for potential multiplayer features
//		- Network communication infrastructure (unused)
//		- Player synchronization across network (not implemented)
//		- Game state replication (not implemented)
//
// Current State:
//		- No active networking code
//		- Only includes HERMES networking headers
//		- No functions or classes defined
//		- Stub/placeholder implementation
//
// Historical Context:
//		- Arx Fatalis was originally envisioned with multiplayer
//		- Development focused on single-player experience
//		- Multiplayer features were cut from final release
//		- This file remains as vestige of that plan
//
// HERMESNet Integration:
//		- HERMESNet.h provides low-level network API
//		- Supports TCP/IP communication
//		- Session management
//		- Player discovery
//		- None of these features utilized in final game
//
// Potential Use Cases (Unimplemented):
//		- Co-op dungeon crawling
//		- Player vs Player combat
//		- Shared world exploration
//		- Chat/messaging system
//		- Leaderboards/statistics
//
// Why Networking Was Cut:
//		- Single-player story focus
//		- Development time constraints
//		- Technical complexity
//		- First-person immersive sim design better suited to single-player
//		- Balancing/design challenges for multiplayer
//
// Technical Notes:
//		- File exists but provides no functionality
//		- Linked against HERMESNet library
//		- No network sockets created
//		- No packet handling code
//		- May have been used in early prototypes
//
// Dependencies:
//		- ARX_Player.h (player data structures)
//		- ARX_Network.h (network system declarations)
//		- HERMESMain.h (HERMES framework)
//		- HERMESNet.h (networking library)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include "ARX_Player.h"
#include "ARX_Network.h"

#include "HERMESMain.h"
#include "HERMESNet.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
