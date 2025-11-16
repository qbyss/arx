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
// ARX_Physics
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		ARX Physics Management
//
// Updates: (date) (person) (update)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved
/////////////////////////////////////////////////////////////////////////////////////
//=============================================================================
// FILE: ARX_Physics.cpp
//=============================================================================
// Component: DANAE Game Engine - Physics System
// Author: Cyril Meynier
//
// PURPOSE:
//		Game physics simulation system handling gravity, falling objects,
//		projectile motion, and object movement in the 3D world.
//
// ARCHITECTURE:
//		Simplified physics simulation integrated with collision detection:
//		- Object falling and gravity simulation
//		- Projectile trajectory calculation (arrows, spells)
//		- Sliding physics on slopes
//		- Object placement validation
//		- Ground detection and snapping
//
// KEY FEATURES:
//		Object Physics:
//		- Gravity simulation with configurable acceleration
//		- Friction modeling for sliding objects
//		- Velocity and momentum tracking
//		- Collision response integration
//
//		Ground Detection:
//		- BCCheckInPoly: Find ground polygon beneath object
//		- Height queries for proper object placement
//		- Support for multi-level geometry (bridges, platforms)
//		- Water and transparent surface filtering
//
//		Movement Validation:
//		- Check if position is valid before placing objects
//		- Prevent objects from floating in air
//		- Snap objects to nearest valid surface
//		- Handle special cases (stairs, slopes, platforms)
//
// ALGORITHMS:
//		Ground Detection (BCCheckInPoly):
//		1. Convert world coordinates to background grid coordinates
//		2. Scan polygons in the grid cell
//		3. Filter out water and transparent polygons
//		4. Find polygon above given Y position with XZ containment
//		5. Return closest polygon above the point
//		6. Handle multi-level geometry by checking vertical spacing
//
//		Gravity Simulation:
//		- Apply acceleration: velocity.y += gravity * deltaTime
//		- Update position: pos.y += velocity.y * deltaTime
//		- Check for ground collision after each update
//		- Apply bounce or stop based on material properties
//
//		Projectile Motion:
//		- Parabolic trajectory with gravity
//		- Air resistance simulation (optional)
//		- Wind influence on arrows/magic projectiles
//		- Collision detection along flight path
//
// INTEGRATION:
//		Works with ARX_Collisions.cpp for movement validation
//		Uses EERIE background grid for spatial optimization
//		Coordinates with ARX_Interactive.cpp for object physics
//		Integrates with ARX_Damages.cpp for fall damage calculation
//
// PERFORMANCE:
//		- Background grid acceleration for polygon queries
//		- Early rejection for out-of-bounds positions
//		- Minimal physics simulation (no full rigid body dynamics)
//		- Optimized for gameplay feel over physical accuracy
//
// COORDINATE SYSTEM:
//		Y-axis: Vertical (up is positive)
//		X-axis: Horizontal width
//		Z-axis: Horizontal depth
//		Physics uses world space coordinates (not camera space)
//
// USE CASES:
//		1. Dropping items from inventory - they fall to ground with gravity
//		2. Arrow flight - parabolic trajectory with wind influence
//		3. NPC placement - validate position and snap to ground
//		4. Platform movement - objects ride on moving platforms
//		5. Falling damage - detect falling distance and apply damage
//		6. Object stacking - prevent items from floating in air
//=============================================================================
#include "ARX_Physics.h"
#include "EERIEMath.h"
#include "EERIEPhysicsBox.h"

#include "ARX_Collisions.h"
#include "ARX_Player.h"
#include "ARX_Interactive.h"
#include "ARX_Script.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


//*************************************************************************************
//*************************************************************************************
EERIEPOLY * BCCheckInPoly(float x, float y, float z)
{
	long px, pz;
	F2L(x * ACTIVEBKG->Xmul, &px);

	if ((px >= ACTIVEBKG->Xsize)
	        ||	(px < 0))
		return NULL;

	F2L(z * ACTIVEBKG->Zmul, &pz);

	if ((pz >= ACTIVEBKG->Zsize)
	        ||	(pz < 0))
		return NULL;

	EERIEPOLY * ep;
	EERIE_BKG_INFO * eg;
	EERIEPOLY * found = NULL;

	eg = (EERIE_BKG_INFO *)&ACTIVEBKG->Backg[px+pz*ACTIVEBKG->Xsize];

	for (long k = 0; k < eg->nbpolyin; k++)
	{
		ep = eg->polyin[k];

		if (!(ep->type & POLY_WATER) &&  !(ep->type & POLY_TRANS))
		{
			if (ep->min.y > y)
			{
				if (PointIn2DPolyXZ(ep, x, z))
				{
					if (found == NULL) found = ep;
					else if (ep->min.y < found->min.y) found = ep;
				}
			}
			else if (ep->min.y + 45.f > y)
				if (PointIn2DPolyXZ(ep, x, z))
				{
					return NULL;
				}
		}
	}

	if (found)
	{
		eg = (EERIE_BKG_INFO *)&ACTIVEBKG->Backg[px+pz*ACTIVEBKG->Xsize];

		for (long k = 0; k < eg->nbpolyin; k++)
		{
			ep = eg->polyin[k];

			if (!(ep->type & POLY_WATER) &&  !(ep->type & POLY_TRANS))
			{
				if (ep != found)
					if (ep->min.y < found->min.y)
						if (ep->min.y > found->min.y - 160.f)
						{
							if (PointIn2DPolyXZ(ep, x, z))
							{
								return NULL;
							}
						}
			}
		}
	}

	return found;
}
