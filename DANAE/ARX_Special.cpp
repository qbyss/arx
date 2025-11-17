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
// ARX_Special.CPP - Physics Attractor System
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Physics-based attraction and repulsion system for Arx Fatalis
//		Implements force fields that pull or push objects within a radius
//		Used for magical effects, environmental hazards, and special gameplay mechanics
//
// Purpose:
//		- Create attractors that pull objects toward a point
//		- Create repulsors that push objects away
//		- Calculate physics forces on affected objects
//		- Manage multiple simultaneous attractors
//		- Support distance-based force falloff
//
// Key Responsibilities:
//		- Manage attractor/repulsor objects
//		- Calculate distance-based forces
//		- Apply forces to physics-enabled objects
//		- Handle attractor creation/removal
//		- Prevent self-attraction
//
// Attractor System:
//		ARX_SPECIAL_ATTRACTORS_Add()       - Create new attractor
//		ARX_SPECIAL_ATTRACTORS_Remove()    - Remove existing attractor
//		ARX_SPECIAL_ATTRACTORS_Reset()     - Clear all attractors
//		ARX_SPECIAL_ATTRACTORS_Exist()     - Check if attractor exists
//		ARX_SPECIAL_ATTRACTORS_ComputeForIO() - Calculate force on object
//
// Data Structure:
//		ARX_SPECIAL_ATTRACTOR:
//			ionum  - Interactive object number (-1 = unused)
//			power  - Attraction strength (positive=pull, negative=push)
//			radius - Maximum effect distance
//
// Force Calculation:
//		1. Check distance between object and attractor
//		2. Skip if outside radius or too close (touching)
//		3. Calculate ratio_dist = 1.0 - (distance / max_radius)
//		4. Normalize direction vector from object to attractor
//		5. Apply force = direction * power * ratio_dist * 0.01
//
// Use Cases:
//		- Magic spells (telekinesis, vortex, black hole)
//		- Environmental hazards (whirlpools, tornadoes)
//		- Puzzle mechanics (magnetic objects)
//		- Boss attacks (vacuum/push effects)
//		- Anti-gravity zones
//
// Force Types:
//		Attraction (power > 0):
//			- Pulls objects toward attractor
//			- Useful for vacuum effects
//			- Gather loose items
//
//		Repulsion (power < 0):
//			- Pushes objects away from attractor
//			- Useful for explosions
//			- Protective barriers
//			- Shockwave effects
//
// Limitations:
//		- Maximum 16 simultaneous attractors (MAX_ATTRACTORS)
//		- Single force per object per frame
//		- No attractor chaining/cascading
//		- Objects must have physics enabled
//		- Objects must be in treatment zone
//		- No self-attraction (object ignored if too close)
//
// Integration:
//		- Called from physics update loop
//		- Force added to object's velocity
//		- Works with collision system
//		- Respects object physics flags (IO_NO_COLLISIONS)
//		- Only affects objects in scene (SHOW_FLAG_IN_SCENE)
//
// Performance:
//		- O(MAX_ATTRACTORS) per object check
//		- Distance checks optimized
//		- Early rejection for invalid attractors
//		- No force applied if outside radius
//
// Script Integration:
//		- Attractors typically created via script commands
//		- Power can be adjusted dynamically
//		- Radius can be modified at runtime
//		- Objects can become attractors temporarily
//
// Technical Notes:
//		- Power scaled by 0.01 for reasonable values
//		- Distance falloff is linear (1.0 to 0.0)
//		- Force direction normalized before scaling
//		- Minimum separation prevents singularities
//		- Only one force vector returned (last active attractor wins)
//
// Dependencies:
//		- ARX_Interactive.h (interactive object system)
//		- EERIEMath.h (vector math, distance calculations)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
#include "ARX_Special.h"
#include "ARX_Interactive.h"
#include "EERIEMath.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


typedef struct
{
	long	ionum;  // -1 == not defined
	float	power;
	float	radius;
} ARX_SPECIAL_ATTRACTOR;
#define MAX_ATTRACTORS 16

ARX_SPECIAL_ATTRACTOR attractors[MAX_ATTRACTORS];

void ARX_SPECIAL_ATTRACTORS_Reset()
{
	for (long i = 0; i < MAX_ATTRACTORS; i++)
	{
		attractors[i].ionum = -1;
	}
}

void ARX_SPECIAL_ATTRACTORS_Remove(long ionum)
{
	for (long i = 0; i < MAX_ATTRACTORS; i++)
	{
		if (attractors[i].ionum == ionum)
			attractors[i].ionum = -1;
	}
}

long ARX_SPECIAL_ATTRACTORS_Exist(long ionum)
{
	for (long i = 0; i < MAX_ATTRACTORS; i++)
	{
		if (attractors[i].ionum == ionum)
			return i;
	}

	return -1;
}

bool ARX_SPECIAL_ATTRACTORS_Add(long ionum, float power, float radius)
{
	if (power == 0.f) ARX_SPECIAL_ATTRACTORS_Remove(ionum);

	long tst;

	if ((tst = ARX_SPECIAL_ATTRACTORS_Exist(ionum)) != -1)
	{
		attractors[tst].power = power;
		attractors[tst].radius = radius;
		return FALSE;
	}

	for (long i = 0; i < MAX_ATTRACTORS; i++)
	{
		if (attractors[i].ionum == -1)
		{
			attractors[i].ionum = ionum;
			attractors[i].power = power;
			attractors[i].radius = radius;
			return TRUE;
		}
	}

	return FALSE;
}

void ARX_SPECIAL_ATTRACTORS_ComputeForIO(INTERACTIVE_OBJ * ioo, EERIE_3D * force)
{
	force->x = 0;
	force->y = 0;
	force->z = 0;

	for (long i = 0; i < MAX_ATTRACTORS; i++)
	{
		if (attractors[i].ionum != -1)
		{
			if (ValidIONum(attractors[i].ionum))
			{
				INTERACTIVE_OBJ * io = inter.iobj[attractors[i].ionum];

				if ((io->show == SHOW_FLAG_IN_SCENE)
				        && !(io->ioflags & IO_NO_COLLISIONS)
				        && (io->GameFlags & GFLAG_ISINTREATZONE))
				{
					float power = attractors[i].power;
					EERIE_3D pos;
					pos.x = ioo->pos.x;
					pos.y = ioo->pos.y;
					pos.z = ioo->pos.z;
					float dist = EEDistance3D(&pos, &io->pos);

					if ((dist > ioo->physics.cyl.radius + io->physics.cyl.radius + 10.f)
					        || (power < 0.f))
					{
						float max_radius = attractors[i].radius; 

						if (dist < max_radius)
						{
							float ratio_dist = 1.f - (dist / max_radius);
							EERIE_3D vect;
							vect.x = io->pos.x - pos.x;
							vect.y = io->pos.y - pos.y;
							vect.z = io->pos.z - pos.z;
							Vector_Normalize(&vect);
							power *= ratio_dist * 0.01f;
							force->x = vect.x * power;
							force->y = vect.y * power;
							force->z = vect.z * power;
						}
					}
				}
			}
		}
	}
}

