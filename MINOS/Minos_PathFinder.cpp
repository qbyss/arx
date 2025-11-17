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
/*******************************************************************************
 * MINOS_PathFinder.cpp
 *
 * PURPOSE:
 *   AI pathfinding system for Arx Fatalis using the A* algorithm. This system
 *   enables NPCs and enemies to navigate through the game world using a
 *   predefined anchor point network.
 *
 * FEATURES:
 *   - A* pathfinding with configurable heuristic
 *   - Stealth pathfinding (avoids lit areas)
 *   - Flee behavior (move away from danger)
 *   - Wander behavior (random exploration)
 *   - LookFor behavior (search around a target position)
 *   - Cylinder-based collision checking (radius and height)
 *   - Light awareness for stealth gameplay
 *
 * ALGORITHM:
 *   Uses A* (A-star) algorithm:
 *   - Open list: Nodes to be examined
 *   - Closed list: Already examined nodes
 *   - Cost function: f(n) = heuristic * g(n) + (1-heuristic) * h(n)
 *     where g(n) = actual cost from start, h(n) = estimated cost to goal
 *
 * DATA STRUCTURES:
 *   - Anchor points: Pre-placed navigation waypoints in the game world
 *   - Each anchor has: position, links to neighbors, height, radius, flags
 *   - MINOSNode: A* algorithm node with cost values and parent pointer
 *
 ******************************************************************************/

#include "Minos_PathFinder.h"
#include <Float.h>    // FLT_MAX constant
#include <time.h>     // time() for random seed

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>   // Debug memory allocation tracking

//-----------------------------------------------------------------------------
// CONSTANTS AND TUNING PARAMETERS
//-----------------------------------------------------------------------------

// Minimum radius for wander/look operations - below this, no movement occurs
const Float MIN_RADIUS(110.F);

// Stealth cost multiplier for lit areas - higher value makes AI avoid light more
Float fac3(300.0F);

// Cost multiplier for flee behavior when still within danger range
Float fac5(130.0F);

//-----------------------------------------------------------------------------
// LINEAR CONGRUENTIAL PSEUDO-RANDOM NUMBER GENERATOR
// Used for deterministic random behavior in pathfinding
//-----------------------------------------------------------------------------
static const unsigned long SEED = 43;           // Initial seed value
static const unsigned long MODULO = 2147483647; // Modulus (2^31 - 1, a prime number)
static const unsigned long FACTOR = 16807;      // Multiplier
static const unsigned long SHIFT = 91;          // Additive constant

static unsigned long __current(SEED);  // Current random state

// Generate next pseudo-random number using linear congruential generator
static unsigned long Random()
{
	return __current = (__current * FACTOR + SHIFT) % MODULO;
}

// Initialize random seed with current time
ULong InitSeed()
{
	__current = (ULong)time(NULL);
	return Random();
}

// Generate random float in range [-1.0, 1.0]
// Note: rnd() is defined elsewhere and returns [0.0, 1.0]
#define frnd() (1.0F - 2 * rnd())

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Constructor and Destructor                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

/*******************************************************************************
 * CONSTRUCTOR: PathFinder
 *
 * PURPOSE:
 *   Initialize the pathfinding system with map data and light information.
 *
 * PARAMETERS:
 *   map_size     - Number of anchor points in the navigation mesh
 *   map_data     - Array of anchor point data (positions, links, properties)
 *   slight_count - Number of static lights in the level
 *   slight_list  - Array of pointers to static light objects
 *   dlight_count - Number of dynamic lights in the level
 *   dlight_list  - Array of pointers to dynamic light objects
 *
 * NOTES:
 *   - Initializes heuristic to default (balanced A* behavior)
 *   - Stores references to level geometry (anchors) and lighting
 *   - Sets default cylinder size for collision checking
 *   - Initializes random seed for wandering behavior
 ******************************************************************************/
PathFinder::PathFinder(const ULong & map_size, _ANCHOR_DATA * map_data,
                       const ULong & slight_count, EERIE_LIGHT ** slight_list,
                       const ULong & dlight_count, EERIE_LIGHT ** dlight_list) :
	heuristic(MINOS_DEFAULT_HEURISTIC),    // Balanced A* heuristic (0.5 typical)
	map_s(map_size), map_d(map_data),      // Navigation mesh data
	slight_c(slight_count), slight_l(slight_list),  // Static lights
	dlight_c(dlight_count), dlight_l(dlight_list),  // Dynamic lights
	height(MINOS_DEFAULT_RADIUS), radius(MINOS_DEFAULT_HEIGHT)  // Collision cylinder
{
	InitSeed();  // Initialize random number generator
}

/*******************************************************************************
 * DESTRUCTOR: ~PathFinder
 *
 * PURPOSE:
 *   Clean up pathfinding data structures.
 *
 * NOTES:
 *   Frees all nodes in open and closed lists.
 ******************************************************************************/
PathFinder::~PathFinder()
{
	Clean();  // Free open and closed lists
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Configuration Methods                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

/*******************************************************************************
 * METHOD: SetHeuristic
 *
 * PURPOSE:
 *   Configure the A* heuristic balance between actual and estimated cost.
 *
 * PARAMETERS:
 *   _heuristic - Balance factor (0.0 to 1.0)
 *                0.0 = Pure greedy (fast, may not find optimal path)
 *                0.5 = Balanced (default, good tradeoff)
 *                1.0 = Pure Dijkstra (slow, always optimal path)
 *
 * NOTES:
 *   Values are clamped to valid range [MINOS_HEURISTIC_MIN, 0.5]
 ******************************************************************************/
void PathFinder::SetHeuristic(const Float & _heuristic)
{
	// Clamp heuristic to valid range
	heuristic = _heuristic >= MINOS_HEURISTIC_MAX ? 0.5F :
	            _heuristic < 0.0F ? MINOS_HEURISTIC_MIN : _heuristic;
}

/*******************************************************************************
 * METHOD: SetCylinder
 *
 * PURPOSE:
 *   Set the collision cylinder dimensions for the pathfinding agent.
 *
 * PARAMETERS:
 *   _radius - Horizontal radius of the agent (width)
 *   _height - Vertical height of the agent
 *
 * NOTES:
 *   Used to check if the agent can fit through passages. Anchors with
 *   insufficient clearance (radius or height) are rejected during pathfinding.
 ******************************************************************************/
void PathFinder::SetCylinder(const Float & _radius, const Float & _height)
{
	radius = _radius;
	height = _height;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Core Pathfinding Methods                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

/*******************************************************************************
 * METHOD: Move
 *
 * PURPOSE:
 *   Find a path from one anchor point to another using A* algorithm.
 *
 * PARAMETERS:
 *   flags  - Pathfinding flags (MINOS_STEALTH = avoid lit areas)
 *   f      - From anchor index (start position)
 *   t      - To anchor index (goal position)
 *   rstep  - [OUT] Number of steps in the path
 *   rlist  - [OUT] Array of anchor indices forming the path
 *
 * RETURNS:
 *   UTRUE if path found, UFALSE if no path exists or error occurred
 *
 * ALGORITHM:
 *   Standard A* implementation:
 *   1. Add start node to open list
 *   2. While open list not empty:
 *      a. Get lowest cost node from open list
 *      b. If it's the goal, build path and return success
 *      c. For each neighbor:
 *         - Calculate costs (g_cost + h_cost)
 *         - Add to open list if passable and not in closed list
 *      d. Add current node to closed list
 *   3. If open list empty, no path exists
 *
 * NOTES:
 *   - Checks for blocked anchors, insufficient clearance (height/radius)
 *   - In stealth mode, adds cost for lit areas (AI avoids light)
 *   - Caller must free the returned path array (rlist)
 ******************************************************************************/
UBool PathFinder::Move(const ULong & flags, const ULong & f, const ULong & t, SLong * rstep, UWord ** rlist)
{

	MINOSNode * node, *child;
	long _from, _to;

	// Initialize/clear open and close lists from any previous pathfinding
	Clean();

	// Validate output parameters
	if (!rlist || !rstep)	return UFALSE;

	// Trivial case: already at destination
	if (f == t)
	{
		*rlist = (UWord *)malloc(sizeof(UWord));
		** rlist = (UWord)t;
		*rstep = 1;
		return UTRUE;
	}

	_from = f, _to = t;

	// Create start node with no parent
	if (!(node = CreateNode(_from, NULL)))
	{
		Clean();
		*rstep = 0;
		return UFALSE;
	}

	// Initialize costs for start node
	node->g_cost = 0;  // Actual cost from start is zero
	node->f_cost = Distance(map_d[_from].pos, map_d[_to].pos);  // Heuristic: straight-line distance to goal

	// In stealth mode, add cost penalty for lit areas
	if (flags & MINOS_STEALTH) AddEnlightmentCost(node);

	// Add start node to open list
	if (open.Append(node))
	{
		free(node);
		Clean();
		*rstep = 0;
		return UFALSE;
	}

	// A* main loop - process nodes until goal found or open list empty
	while (node = GetBestNode())  // Get and remove lowest cost node from open list
	{
		// Check if we've reached the goal
		if (node->data == _to)
		{
			// Add goal node to closed list
			if (close.Append(node))
			{
				free(node);
				Clean();
				*rstep = 0;
				return UFALSE;
			}

			// Build the path by tracing parent pointers from goal to start
			if (BuildPath(rlist, rstep))
			{
				Clean();
				*rstep = 0;
				return UFALSE;
			}

			Clean();
			return UTRUE;  // Success! Path found
		}

		// Expand current node - examine all neighbors
		long _pipo(node->data);  // Current anchor index

		// Iterate through all linked (neighboring) anchors
		for (SWord i(0); i < map_d[_pipo].nblinked; i++)
		{
			// Create child node for this neighbor
			child = CreateNode(map_d[_pipo].linked[i], node);

			if (!child)
			{
				free(node);
				Clean();
				*rstep = 0;
				return UFALSE;
			}

			// Check if this anchor is passable for our agent
			if ((map_d[child->data].flags & ANCHOR_FLAG_BLOCKED) ||         // Anchor blocked
			        map_d[child->data].height > height ||                    // Ceiling too low
			        map_d[child->data].radius < radius)                      // Passage too narrow
				free(child);  // Reject this child
			else
			{
				// Calculate actual cost to reach this child (parent cost + edge cost)
				child->g_cost = node->g_cost + Distance(map_d[child->data].pos, map_d[node->data].pos);

				// Add stealth cost penalty if in lit areas
				if (flags & MINOS_STEALTH) AddEnlightmentCost(child);

				// Check if this child should be added (not in closed list, better than existing)
				if (Check(child))
				{
					// Add to open list for later examination
					if (open.Append(child))
					{
						free(node);
						free(child);
						*rstep = 0;
						return UFALSE;
					}

					// Calculate total cost: f(n) = heuristic * g(n) + (1-heuristic) * h(n)
					// g(n) = actual cost from start
					// h(n) = estimated cost to goal (straight-line distance)
					child->f_cost = heuristic * child->g_cost +
					                (1.0F - heuristic) * Distance(map_d[child->data].pos, map_d[_to].pos);
				}
				else free(child);  // Child already examined or worse path
			}
		}

		// Current node fully examined - add to closed list
		if (close.Append(node))
		{
			free(node);
			Clean();
			*rstep = 0;
			return UFALSE;
		}
	}

	// Open list exhausted without finding goal - no path exists
	Clean();
	*rstep = 0;
	return UFALSE;
}

/*******************************************************************************
 * METHOD: Flee
 *
 * PURPOSE:
 *   Find a path away from a dangerous position to a safe distance.
 *   Uses A* with inverted goal: find anchor >= safe_dist from danger point.
 *
 * PARAMETERS:
 *   flags     - Pathfinding flags (MINOS_STEALTH supported)
 *   f         - Starting anchor index
 *   danger    - 3D position to flee from (threat location)
 *   safe_dist - Minimum safe distance from danger
 *   rstep     - [OUT] Number of steps in escape path
 *   rlist     - [OUT] Array of anchor indices forming escape path
 *
 * RETURNS:
 *   UTRUE if escape path found, UFALSE otherwise
 *
 * NOTES:
 *   - If already at safe distance, returns single-step path (stay put)
 *   - Cost function penalizes nodes still within danger radius
 *   - Useful for NPC flee behavior from explosions, player, etc.
 ******************************************************************************/
UBool PathFinder::Flee(const ULong & flags, const ULong & f, const EERIE_3D & danger, const Float & safe_dist, SLong * rstep, UWord ** rlist)
{
	MINOSNode * node, *child;
	long _from;

	// Initialize open and close lists
	Clean();

	// Validate output parameters
	if (!rlist || !rstep)
		return UFALSE;

	// Already at safe distance - no movement needed
	if (Distance(map_d[f].pos, danger) >= safe_dist)
	{
		*rlist = (UWord *)malloc(sizeof(UWord));
		** rlist = (UWord)f;
		*rstep = 1;
		return UTRUE;
	}

	_from = f;

	// Create start node
	if (!(node = CreateNode(_from, NULL)))
	{
		Clean();
		*rstep = 0;
		return UFALSE;
	}

	node->g_cost = 0;

	// Add stealth cost if needed
	if (flags & MINOS_STEALTH)
		AddEnlightmentCost(node);

	// Initial cost based on how far we are from safe distance
	node->f_cost = safe_dist - Distance(map_d[_from].pos, danger);

	if (node->f_cost < 0.0F)
		node->f_cost = 0.0F;

	node->f_cost += node->g_cost;

	// Add to open list
	if (open.Append(node))
	{
		free(node);
		Clean();
		*rstep = 0;
		return UFALSE;
	}

	// A* main loop - find first anchor at safe distance
	while (node = GetBestNode())
	{
		// Success condition: found anchor at safe distance from danger
		if (Distance(map_d[node->data].pos, danger) >= safe_dist)
		{
			if (close.Append(node))
			{
				free(node);
				*rstep = 0;
				return UFALSE;
			}

			//BuildPath(rlist, rstep);
			if (BuildPath(rlist, rstep))
			{
				Clean(); 
				*rstep = 0;
				return UFALSE;
			}

			Clean(); 
			return UTRUE;
		}

		//Otherwise, generate child from current node
		long _pipo = node->data;

		for (SWord i(0); i < map_d[_pipo].nblinked; i++)
		{
			child = CreateNode(map_d[_pipo].linked[i], node);

			if (!child)
			{
				free(node);
				Clean(); 
				*rstep = 0;
				return UFALSE;
			}

			// Check if anchor is passable (not blocked, sufficient clearance)
			if ((map_d[child->data].flags & ANCHOR_FLAG_BLOCKED) || map_d[child->data].height > height || map_d[child->data].radius < radius)
				free(child);
			else
			{
				child->g_cost = node->g_cost + Distance(map_d[child->data].pos, map_d[node->data].pos);

				if (flags & MINOS_STEALTH)
					AddEnlightmentCost(child);

				if (Check(child))
				{
					Float dist;

					if (open.Append(child))
					{
						free(node);
						free(child);
						*rstep = 0;
						return UFALSE;
					}

					// Calculate cost: base cost + penalty if still within danger zone
					child->f_cost = child->g_cost;
					dist = Distance(map_d[child->data].pos, danger);

					if ((dist = safe_dist - dist) > 0.0F)
						child->f_cost += fac5 * dist;  // Penalty for being too close to danger
				}
				else free(child);
			}
		}

		// Add to closed list
		if (close.Append(node))
		{
			free(node);
			Clean();
			*rstep = 0;
			return UFALSE;
		}
	}

	Clean();
	*rstep = 0;

	// No escape path found
	return UFALSE;
}

/*******************************************************************************
 * METHOD: WanderAround
 *
 * PURPOSE:
 *   Generate a random circular patrol path centered on starting position.
 *
 * PARAMETERS:
 *   flags  - Pathfinding flags
 *   f      - Starting anchor index
 *   rad    - Radius of wander area
 *   rstep  - [OUT] Number of steps in wander path
 *   rlist  - [OUT] Array of anchors forming circular path
 *
 * RETURNS:
 *   UTRUE if wander path generated, UFALSE otherwise
 *
 * ALGORITHM:
 *   1. Generate 5-10 random waypoints within radius
 *   2. Find paths between consecutive waypoints
 *   3. Close loop by returning to start
 *   4. Concatenate all sub-paths into one circular patrol route
 *
 * NOTES:
 *   Used for idle NPC patrol behavior. Creates variety through randomness.
 ******************************************************************************/
UBool PathFinder::WanderAround(const ULong & flags, const ULong & f, const Float & rad, SLong * rstep, UWord ** rlist)
{
	
	Void * ptr;
	ULong step_c, last, next;
	SLong temp_c(0), path_c(0);
	UWord * temp_d = NULL, *path_d = NULL;

	Clean();

	// Validate parameters
	if (!rlist || !rstep) return UFALSE;

	// Starting anchor must have connections
	if (!map_d[f].nblinked)
	{
		*rstep = 0;
		return UFALSE;
	}

	// Radius too small - just stay in place
	if (rad <= MIN_RADIUS)
	{
		*rlist = (UWord *)malloc(sizeof(UWord));
		** rlist = (UWord)f;
		*rstep = 1;
		return UTRUE;
	}

	last = f;

	// Generate 5-10 random waypoints
	step_c = Random() % 5 + 5;

	// Generate each random waypoint
	for (ULong i(0); i < step_c; i++)
	{
		// Random walk distance (in anchor steps, scaled by radius)
		ULong nb = ULong(rad * rnd() * DIV50);
		long _current = f;

		// Random walk: take nb random steps through anchor network
		while (nb)
		{
			if ((map_d[_current].nblinked))
			{
				long notfinished = 4;  // Try up to 4 times to find valid next anchor

				while (notfinished--)
				{
					// Pick random linked anchor
					ULong r = ULong(rnd() * (Float)map_d[_current].nblinked);

					if (r >= (ULong)map_d[_current].nblinked)
						r = ULong(map_d[_current].nblinked - 1);

					// Check if anchor is passable
					if ((!(map_d[map_d[_current].linked[r]].flags & ANCHOR_FLAG_BLOCKED))
					        &&	(map_d[map_d[_current].linked[r]].nblinked)
					        &&	(map_d[map_d[_current].linked[r]].height <= height)
					        &&	(map_d[map_d[_current].linked[r]].radius >= radius))
					{
						_current = map_d[_current].linked[r];
						notfinished = 0;
					}
				}
			}

			nb--;
		}

		if (_current < 0) continue;

		next = nb = _current;

		// Find path from last waypoint to this waypoint
		if (Move(flags, last, next, &temp_c, &temp_d) && temp_c)
		{
			// Append this sub-path to the complete wander path
			if (!(ptr = realloc(path_d, sizeof(UWord) * (path_c + temp_c))))
			{
				free(temp_d);
				free(path_d);
				Clean();
				*rstep = 0;
				return UFALSE;
			}

			path_d = (UWord *)ptr;
			memcpy(&path_d[path_c], temp_d, sizeof(UWord) * temp_c);
			path_c += temp_c;

			// Free the temporary sub-path
			free(temp_d), temp_d = NULL, temp_c = 0;
		}
		else i--;  // Path failed, retry this waypoint

		last = next;
	}

	// Close the loop: return to starting position
	if (!path_c || !Move(flags, last, f, &temp_c, &temp_d))
	{
		*rstep = 0;
		return UFALSE;
	}

	if (!(ptr = realloc(path_d, sizeof(UWord) * (path_c + temp_c))))
	{
		free(temp_d);
		free(path_d);
		Clean();
		*rstep = 0;
		return UFALSE;
	}

	// Append final segment to complete the loop
	path_d = (UWord *)ptr;
	memcpy(&path_d[path_c], temp_d, sizeof(UWord) * temp_c);
	path_c += temp_c;

	free(temp_d);

	*rlist = path_d;
	*rstep = path_c;
	Clean();
	return UTRUE;
}

/*******************************************************************************
 * METHOD: GetNearestNode
 * PURPOSE: Find the closest anchor point to a given 3D position.
 * PARAMS:  pos - 3D world position
 * RETURNS: Index of nearest anchor with valid connections
 * NOTES:   Only considers anchors that have links (nblinked > 0)
 ******************************************************************************/
ULong PathFinder::GetNearestNode(const EERIE_3D & pos) const
{
	ULong best(0);
	Float dist, b_dist(FLT_MAX);

	// Linear search through all anchors (could be optimized with spatial partitioning)
	for (ULong i(0); i < map_s; i++)
	{
		dist = Distance(map_d[i].pos, pos);

		if (dist < b_dist && map_d[i].nblinked) best = i, b_dist = dist;
	}

	return best;
}

/*******************************************************************************
 * METHOD: LookFor
 * PURPOSE: Generate path that searches around a target position randomly.
 * PARAMS:  flags  - Pathfinding flags
 *          f      - Starting anchor
 *          pos    - Target position to search around
 *          radius - Search radius
 *          rstep/rlist - Output path
 * RETURNS: UTRUE if search path created
 * NOTES:   Similar to WanderAround but centered on a specific position.
 *          Used for NPC "search for object/player" behavior.
 ******************************************************************************/
UBool PathFinder::LookFor(const ULong & flags, const ULong & f, const EERIE_3D & pos, const Float & radius, SLong * rstep, UWord ** rlist)
{
	Void * ptr;
	ULong step_c, to, last, next;
	SLong temp_c(0), path_c(0);
	UWord * temp_d = NULL, *path_d = NULL;

	Clean(); 
	//Check if params are valid
	if (!rlist || !rstep)
	{
		Clean();
		*rstep = 0;
		return UFALSE;
	}

	if (radius <= MIN_RADIUS)
	{
		*rlist = (UWord *)malloc(sizeof(UWord));
		** rlist = (UWord)f;
		*rstep = 1;
		Clean();
		return UTRUE;
	}

	to = GetNearestNode(pos);

	last = f;

	step_c = Random() % 5 + 5;

	for (ULong i(0); i < step_c; i++)
	{
		EERIE_3D pos;

		pos.x = map_d[to].pos.x + radius * frnd();
		pos.y = map_d[to].pos.y + radius * frnd();
		pos.z = map_d[to].pos.z + radius * frnd();
		next = GetNearestNode(pos);

		if (Move(flags, last, next, &temp_c, &temp_d) && temp_c)
		{
			if ((path_c + temp_c - 1) <= 0)
			{
				if (temp_d) 
				{
					free(temp_d);
					temp_d = NULL;
				}

				if (path_d)
				{
					free(path_d);
					path_d = NULL;
				}

				Clean(); 
				*rstep = 0;
				return UFALSE;
			}

			if (!(ptr = realloc(path_d, sizeof(UWord) * (path_c + temp_c - 1))))
			{
				if (temp_d) 
				{
					free(temp_d);
					temp_d = NULL;
				}

				Clean(); 
				*rstep = 0;
				return UFALSE;
			}

			//Add temp path to wander around path
			path_d = (UWord *)ptr;
			memcpy(&path_d[path_c], temp_d, sizeof(UWord) *(temp_c - 1));
			path_c += temp_c - 1;

			//Free temp path
			free(temp_d), temp_d = NULL, temp_c = 0;
		}
		else i--;

		last = next;
	}

	//Close wander around path (return to start position)
	if (!path_c)
	{
		Clean(); // Cyril
		*rstep = 0;
		return UFALSE;
	}

	*rlist = path_d;
	*rstep = path_c;
	Clean(); // Cyril
	return UTRUE;
}

/*******************************************************************************
 * METHOD: Clean
 * PURPOSE: Free all nodes in open and closed lists. Called after pathfinding
 *          completes or fails to prevent memory leaks.
 ******************************************************************************/
Void PathFinder::Clean()
{
	ULong i;

	// Free all nodes in closed list
	for (i = 0; i < close.Count(); i++) free(close[i]);
	close.Free();

	// Free all nodes in open list
	for (i = 0; i < open.Count(); i++) free(open[i]);
	open.Free();
}

/*******************************************************************************
 * METHOD: GetBestNode
 * PURPOSE: Extract the node with lowest f_cost from the open list (A* core operation).
 * RETURNS: Node with lowest cost, or NULL if open list is empty.
 * NOTES:   Linear search - could be optimized with priority queue/heap.
 ******************************************************************************/
MINOSNode * PathFinder::GetBestNode()
{
	MINOSNode * node;
	ULong best(0);
	Float cost(FLT_MAX);

	if (!open.Count()) return NULL;  // No more nodes to explore

	// Find lowest cost node in open list
	for (ULong i(0); i < open.Count(); i++)
		if (open[i]->f_cost < cost) cost = open[i]->f_cost, best = i;

	// Remove and return it
	node = open[best];
	open.Remove(best);

	return node;
}

/*******************************************************************************
 * METHOD: Check
 * PURPOSE: Determine if a node should be added to the open list.
 * PARAMS:  node - Node to check
 * RETURNS: UTRUE if should be added, UFALSE if already processed or worse path
 * LOGIC:   - If in closed list: reject (already processed)
 *          - If in open list with better cost: reject (worse path)
 *          - If in open list with worse cost: remove old, accept new (better path)
 ******************************************************************************/
UBool PathFinder::Check(MINOSNode * node)
{
	ULong i;

	// Already in closed list - already fully explored this anchor
	for (i = 0; i < close.Count(); i++)
		if (close[i]->data == node->data) return UFALSE;

	// Check if a better path to this anchor already exists in open list
	for (i = 0; i < open.Count(); i++)
		if (open[i]->data == node->data)
		{
			if (open[i]->g_cost < node->g_cost) return UFALSE;  // Existing path is better

			free(open[i]), open.Remove(i);  // New path is better, remove old
		}

	return UTRUE;  // Node should be added to open list
}

/*******************************************************************************
 * METHOD: BuildPath
 * PURPOSE: Construct final path array by tracing parent pointers from goal to start.
 * PARAMS:  rlist - [OUT] Allocated array of anchor indices
 *          rstep - [OUT] Number of steps in path
 * RETURNS: STRUE on success, SFALSE on memory allocation failure
 * NOTES:   Path is reversed (goal->start), so we reverse it when copying to output.
 ******************************************************************************/
SBool PathFinder::BuildPath(UWord ** rlist, SLong * rstep)
{
	Void * ptr;
	MINOSNode * next;
	UWord path_c(0);
	UWord * path_d = NULL;

	// Start from goal node (last node added to closed list)
	next = close[close.Count() - 1];

	// Trace back through parent pointers to build path (in reverse)
	while (next)
	{
		if (!(ptr = realloc(path_d, (path_c + 1) << 1))) return SFALSE;

		path_d = (UWord *)ptr;
		path_d[path_c++] = (UWord)next->data;  // Add anchor index
		next = next->parent;  // Move to parent
	}

	// Allocate output array
	if (!rlist || !(*rlist = (UWord *)malloc(sizeof(UWord) * path_c)))
	{
		free(path_d);
		return SFALSE;
	}

	// Reverse path (convert goal->start to start->goal)
	for (ULong i(0); i < path_c; i++)(*rlist)[i] = path_d[path_c - i - 1];

	free(path_d);

	if (rstep) *rstep = path_c;

	return STRUE;
}

/*******************************************************************************
 * METHOD: CreateNode
 * PURPOSE: Allocate and initialize a new A* node.
 * PARAMS:  data   - Anchor index this node represents
 *          parent - Parent node in search tree (NULL for start node)
 * RETURNS: Pointer to new node, or NULL on allocation failure
 * NOTES:   Caller must set g_cost and f_cost after creation.
 ******************************************************************************/
MINOSNode * PathFinder::CreateNode(long data, MINOSNode * parent)
{
	MINOSNode * node;

	node = (MINOSNode *)malloc(sizeof(MINOSNode));

	if (!node) return NULL;

	node->data = data;
	node->parent = parent;

	return node;
}

/*******************************************************************************
 * METHOD: AddEnlightmentCost
 * PURPOSE: Add stealth penalty cost based on light intensity at this anchor.
 *          Makes AI prefer dark areas when in stealth mode.
 * PARAMS:  node - Node to add light cost to (modifies g_cost)
 * LOGIC:   For each static light:
 *          - If anchor is within light's falloff range
 *          - Add cost based on: intensity * brightness * distance_falloff
 *          - Closer to light = higher cost
 * NOTES:   Only checks static lights (slight_l), not dynamic lights.
 *          Cost multiplier fac3 = 300.0 (tunable parameter).
 ******************************************************************************/
Void PathFinder::AddEnlightmentCost(MINOSNode * node)
{
	// Check each static light in the level
	for (ULong i(0); i < slight_c; i++)
	{
		if (!slight_l[i] || !slight_l[i]->exist || !slight_l[i]->status) continue;

		Float dist = Distance(slight_l[i]->pos, map_d[node->data].pos);

		// If anchor is within light's range
		if (slight_l[i]->fallend >= dist)
		{
			Float l_cost(fac3);  // Base stealth cost multiplier

			// Scale by light intensity and brightness (average of RGB)
			l_cost *= slight_l[i]->intensity * (slight_l[i]->rgb.r + slight_l[i]->rgb.g + slight_l[i]->rgb.b) * DIV3;

			// Apply distance falloff
			if (slight_l[i]->fallstart >= dist)
				node->g_cost += l_cost;  // Full cost within fallstart radius
			else
				node->g_cost += l_cost * ((dist - slight_l[i]->fallstart) / (slight_l[i]->fallend - slight_l[i]->fallstart));  // Linear falloff
		}
	}
}

/*******************************************************************************
 * METHOD: Distance (inline)
 * PURPOSE: Calculate Euclidean distance between two 3D points.
 * PARAMS:  from, to - 3D positions
 * RETURNS: Distance as Float
 * NOTES:   Uses EEsqrt() which is likely a fast sqrt implementation.
 ******************************************************************************/
inline Float PathFinder::Distance(const EERIE_3D & from, const EERIE_3D & to) const
{
	Float x, y, z;

	x = from.x - to.x;
	y = from.y - to.y;
	z = from.z - to.z;

	return Float(EEsqrt(x * x + y * y + z * z));
}

//-----------------------------------------------------------------------------
// End of Minos_PathFinder.cpp
//-----------------------------------------------------------------------------
