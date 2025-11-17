# Arx Fatalis - Source Code Documentation

## Table of Contents
- [Overview](#overview)
- [Quick Start](#quick-start)
- [Architecture](#architecture)
- [Core Systems](#core-systems)
- [Directory Structure](#directory-structure)
- [Building the Project](#building-the-project)
- [Key Concepts](#key-concepts)
- [Code Navigation Guide](#code-navigation-guide)
- [Further Reading](#further-reading)

---

## Overview

**Arx Fatalis** is a first-person action role-playing game developed by Arkane Studios and released in 2002. The source code was released under GPL in 2011, providing a complete example of a commercial game engine from the early 2000s era.

### Quick Facts
- **Developer:** Arkane Studios
- **Release:** November 2002
- **Engine:** Custom DirectX 7 engine
- **Language:** C++ with some C-style code
- **License:** GPL (Open Source since 2011)
- **Lines of Code:** ~150,000+
- **Files:** 251 source files (116 .cpp + 135 .h)

### Technology Stack
- **Graphics:** DirectX 7 (Direct3D)
- **Audio:** Custom Athena library with DirectSound + EAX
- **Platform:** Windows (Win32)
- **Build System:** Visual C++ 6.0 projects (.dsp files)

---

## Quick Start

### For Developers Wanting to Understand the Code

**Recommended Reading Order:**

1. **Start Here:** Read this README and `CODEBASE_SUMMARY.md`
2. **Data Structures:** Study `Include/EERIETypes.h` - core game data structures
3. **Game Loop:** Look at DANAE's main game loop and update cycle
4. **Rendering:** Examine EERIE rendering pipeline
5. **Pathfinding:** Study MINOS pathfinding (fully documented example)
6. **Physics:** Explore collision detection and physics systems

### For Players Wanting to Modify the Game

**Common Modifications:**
- **Spell Effects:** See `DANAE/ARX_SpellFX_Lvl*.cpp` (10 spell level files)
- **Player Stats:** Modify `DANAE/ARX_Player.cpp`
- **NPC Behavior:** Edit `DANAE/ARX_NPC.cpp`
- **Item Properties:** Adjust script files (not in this source tree)

---

## Architecture

### High-Level Design

Arx Fatalis uses a **modular, component-based architecture** with clear separation between subsystems:

```
┌─────────────────────────────────────────────────────────────┐
│                      DANAE (Game Engine)                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Player     │  │     NPC      │  │    Script    │      │
│  │  Management  │  │      AI      │  │  Interpreter │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │    Spells    │  │  Particles   │  │   Physics    │      │
│  │  (10 levels) │  │    System    │  │  Collision   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   EERIE (Rendering Engine)                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  3D Objects  │  │   Lighting   │  │   Textures   │      │
│  │  Animation   │  │   Dynamic    │  │  Management  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Polygon    │  │    Camera    │  │     LOD      │      │
│  │  Rendering   │  │    System    │  │   (Meshes)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Athena     │    │    HERMES    │    │    MINOS     │
│ Audio Engine │    │   Resource   │    │ Pathfinding  │
│ (3D Sound)   │    │  Management  │    │   (A* AI)    │
└──────────────┘    └──────────────┘    └──────────────┘
```

### Design Patterns Used

1. **Singleton Pattern** - Debug logging (ArxDebug), managers
2. **Component-Based Entities** - Game objects have modular data components
3. **Data-Oriented Design** - Large structs with grouped data for cache efficiency
4. **Manager Pattern** - Centralized resource management (textures, particles, lights)
5. **Object Pooling** - Reuse of particles, effects, and temporary objects

---

## Core Systems

### 1. DANAE - Game Engine (65 files)

**Location:** `/DANAE/`

**Purpose:** Main game logic, player systems, NPC AI, and gameplay mechanics.

**Key Components:**

#### Player System
- **Files:** `ARX_Player.cpp`
- **Functionality:** Character movement, camera control, inventory management, stats
- **Key Structures:** Player health, mana, stats, equipment

#### NPC AI System
- **Files:** `ARX_NPC.cpp`
- **Functionality:** Enemy behavior, combat AI, dialogue
- **Integration:** Uses MINOS for pathfinding, scripts for behavior trees

#### Spell System (10 Levels)
- **Files:** `ARX_SpellFX_Lvl01.cpp` through `ARX_SpellFX_Lvl10.cpp`
- **Functionality:** Magic system with rune-based spell casting
- **Features:**
  - Level 1: Basic spells (Magic Missile, Heal)
  - Level 10: Advanced spells (Mass Paralysis, Control)
  - Particle effects for each spell
  - Mana cost and cooldown management

#### Particle System
- **Files:** `ARX_Particles.cpp`
- **Functionality:** Visual effects for spells, weather, ambient effects
- **Performance:** Object pooling for efficiency

#### Physics & Collision
- **Files:** `ARX_Physics.cpp`, `ARX_Collisions.cpp`
- **Functionality:** Collision detection, damage calculation, object interaction
- **Features:** Cylinder-based collision for characters, mesh collision for objects

#### Script System
- **Files:** `ARX_Script.cpp`
- **Functionality:** Custom scripting language for interactive objects and quests
- **Usage:** Event-driven execution, state management

#### Interface System
- **Files:** `ARX_Interface.cpp`, `ARX_Menu.cpp`
- **Functionality:** HUD, inventory screen, menus, dialogue boxes

---

### 2. EERIE - 3D Rendering Engine (20 files)

**Location:** `/EERIE/`

**Purpose:** Low-level graphics rendering, animation, and visual systems.

**Key Components:**

#### 3D Object Management
- **Files:** `EERIEObject.cpp`, `EERIEPoly.cpp` (142KB - largest file)
- **Functionality:** 3D mesh loading, manipulation, rendering
- **Format:** Custom .TEO format for 3D models
- **Features:** Progressive mesh LOD, bone structure for animation

#### Animation System
- **Files:** `EERIEAnim.cpp` (112KB)
- **Functionality:** Skeletal animation with keyframe interpolation
- **Features:**
  - Bone hierarchy
  - Animation blending
  - Interpolation between keyframes
  - Multiple animations per object

#### Texture System
- **Files:** `EERIETexture.cpp` (145KB)
- **Functionality:** Texture loading, caching, management
- **Formats:** JPEG (via jpeglib), custom formats
- **Features:**
  - Texture pooling and reuse
  - Mipmapping
  - Compression support

#### Lighting System
- **Files:** `EERIELight.cpp`
- **Functionality:** Dynamic and static lighting calculations
- **Features:**
  - Point lights with falloff
  - Colored lighting (RGB)
  - Light intensity control
  - Static lightmaps for performance

#### Rendering Pipeline
- **Files:** `EERIEDraw.cpp`
- **Functionality:** DirectX 7 polygon rendering
- **Process:**
  1. Visibility determination (portal culling)
  2. LOD selection based on distance
  3. Texture binding
  4. Polygon submission to DirectX
  5. Post-processing effects

#### Portal Rendering
- **Purpose:** Optimize rendering of large indoor environments
- **How it Works:**
  - Scene divided into portal-connected rooms
  - Only render visible rooms through portals
  - Significant performance gain for complex levels

#### Cloth Simulation
- **Files:** `EERIECloth.cpp`
- **Functionality:** Spring-mass system for realistic cloth
- **Usage:** Character clothing, banners, curtains

---

### 3. Athena - Audio Engine (16 files)

**Location:** `/Athena/`

**Purpose:** Custom 3D audio system for game sounds and music.

**Key Features:**
- **3D Positional Audio:** Sound sources positioned in game space
- **Streaming:** Large audio files (music) streamed from disk
- **Formats:** WAV, ASF (Advanced Streaming Format)
- **Codec:** ADPCM compression for smaller file sizes
- **EAX Support:** Hardware-accelerated environmental audio effects
- **Ambiance System:** Layered ambient sounds for atmosphere

**Key Files:**
- `AthenaSample.cpp` - Audio sample playback
- `AthenaStream.cpp` - Streaming audio for music
- `AthenaAmbiance.cpp` - Environmental sound management
- `AthenaMixer.cpp` - Audio mixing and output

---

### 4. HERMES - Resource Management (10 files)

**Location:** `/HERMES/`

**Purpose:** File packaging, compression, and resource loading.

**Key Features:**
- **PAK Files:** Archive format for game resources
- **Compression:** Reduce file sizes for distribution
- **Hashing:** Fast resource lookup by name
- **Clustering:** Group related resources for efficient loading

**Key Files:**
- `HERMES_PAK.cpp` - PAK archive format implementation
- `HERMES_File.cpp` - File I/O abstraction
- `HERMES_Compression.cpp` - Compression algorithms

---

### 5. MINOS - Pathfinding (1 file)

**Location:** `/MINOS/Minos_PathFinder.cpp`

**Purpose:** AI pathfinding for NPC navigation.

**Algorithm:** A* (A-star) pathfinding

**Key Features:**
- **Anchor-Based Navigation:** Pre-placed waypoint network
- **Configurable Heuristic:** Balance between speed and optimality
- **Stealth Mode:** AI avoids lit areas when sneaking
- **Behaviors:**
  - **Move:** Find path from A to B
  - **Flee:** Move away from danger to safe distance
  - **Wander:** Random patrol in area
  - **LookFor:** Search around target position

**Status:** ✅ **Fully documented** - Study this file as an example of well-commented code

---

## Directory Structure

```
/arx/
│
├── DANAE/                    # Main game engine (65 .cpp files)
│   ├── ARX_Player.cpp        # Player character management
│   ├── ARX_NPC.cpp           # NPC AI and behavior
│   ├── ARX_SpellFX_Lvl*.cpp  # Spell effects (10 levels)
│   ├── ARX_Particles.cpp     # Particle system
│   ├── ARX_Physics.cpp       # Physics and collision
│   ├── ARX_Script.cpp        # Script interpreter
│   ├── ARX_Interface.cpp     # User interface
│   └── ...                   # 58 more game logic files
│
├── EERIE/                    # 3D rendering engine (20 .cpp files)
│   ├── EERIEPoly.cpp         # Polygon rendering (142KB)
│   ├── EERIETexture.cpp      # Texture system (145KB)
│   ├── EERIEAnim.cpp         # Animation system (112KB)
│   ├── EERIELight.cpp        # Lighting system
│   ├── EERIEObject.cpp       # 3D object management
│   ├── EERIEDraw.cpp         # DirectX rendering
│   └── ...                   # 14 more rendering files
│
├── Athena/                   # Audio engine (16 .cpp files)
│   ├── AthenaSample.cpp      # Audio sample playback
│   ├── AthenaStream.cpp      # Streaming audio
│   ├── AthenaAmbiance.cpp    # Environmental audio
│   └── ...                   # 13 more audio files
│
├── HERMES/                   # Resource management (10 .cpp files)
│   ├── HERMES_PAK.cpp        # PAK file format
│   ├── HERMES_File.cpp       # File I/O
│   └── ...                   # 8 more resource files
│
├── MINOS/                    # Pathfinding (1 .cpp file)
│   └── Minos_PathFinder.cpp  # A* pathfinding (FULLY DOCUMENTED)
│
├── Mercury/                  # Utility module (2 .cpp files)
│   └── ...                   # Additional functionality
│
├── ArxCommon/                # Common utilities (1 .cpp file)
│   └── ARX_Common.cpp        # Debug logging (FULLY DOCUMENTED)
│
├── Include/                  # Header files (135 .h files)
│   ├── EERIETypes.h          # Core data structures (IMPORTANT!)
│   ├── ARX_*.h               # Game engine headers
│   ├── EERIE*.h              # Rendering engine headers
│   ├── Athena_Types.h        # Audio system headers
│   └── OtherLibs/            # Third-party library headers
│       ├── jpeglib.h         # JPEG support
│       ├── eax.h             # EAX audio
│       └── ...
│
├── lib/                      # Pre-compiled libraries
│   └── otherlibs/
│       ├── Jpeglib.lib       # JPEG image support
│       ├── zlib.lib          # Compression
│       ├── eax.lib           # 3D audio
│       └── ...
│
├── CODEBASE_SUMMARY.md       # Detailed architecture document
└── README.md                 # This file
```

---

## Building the Project

### Requirements

- **Visual C++ 6.0** (or compatible compiler)
- **DirectX 7 SDK**
- **Windows Platform SDK**

### Build Configurations

- **Debug:** Full debug symbols, no optimizations
- **Release:** Optimized for speed, minimal debug info

### Dependencies

All required libraries are included in `/lib/otherlibs/`:
- JPEG library (jpeglib.lib)
- zlib compression (zlib.lib)
- EAX audio (eax.lib, eaxguid.lib)
- DirectShow (amstrmid.lib)

---

## Key Concepts

### 1. Anchor-Based Navigation

**What:** Pre-placed waypoint network for NPC pathfinding

**How it Works:**
- Level designers place "anchor points" throughout the game world
- Each anchor has: position, links to neighbors, clearance (height/radius)
- NPCs use A* pathfinding to navigate between anchors
- See: `/MINOS/Minos_PathFinder.cpp` (fully documented)

**Data Structure:**
```cpp
typedef struct _ANCHOR_DATA {
    EERIE_3D pos;           // 3D position
    short nblinked;         // Number of connected anchors
    short *linked;          // Array of neighbor indices
    float radius;           // Clearance radius
    float height;           // Clearance height
    long flags;             // Status flags (blocked, etc.)
} ANCHOR_DATA;
```

### 2. Interactive Objects

**What:** Game objects with scripts and behaviors

**Structure:**
```cpp
typedef struct INTERACTIVE_OBJ {
    EERIE_3DOBJ *obj;       // Visual 3D model
    char *script;           // Script code
    ANIM_HANDLE *anims;     // Animations
    INVENTORY_DATA *inv;    // Inventory (if container)
    EERIE_3D pos;           // World position
    float speed_modif;      // Movement speed
    long ioflags;           // Object type flags
    // ... many more fields
} INTERACTIVE_OBJ;
```

### 3. Rune-Based Magic System

**How it Works:**
- Player draws rune sequences with mouse
- Sequences map to spells (e.g., AAM + MEGA + STREGUM = Mass Lightning)
- Each spell has: mana cost, duration, particle effects
- Spell levels 1-10 implemented in separate files

### 4. Progressive Mesh LOD

**Purpose:** Optimize rendering performance

**How it Works:**
- Each 3D model has multiple detail levels
- Engine selects LOD based on distance to camera
- Smooth transitions prevent popping artifacts

### 5. Portal-Based Rendering

**Purpose:** Efficiently render large indoor environments

**How it Works:**
- Level divided into "rooms" connected by "portals"
- Camera view frustum tested against portals
- Only render rooms visible through portal chain
- Massive performance gain for complex architecture

---

## Code Navigation Guide

### Finding Specific Functionality

#### Player Actions
- **Movement:** `DANAE/ARX_Player.cpp` → `ARX_PLAYER_ManageMovement()`
- **Combat:** `DANAE/ARX_Player.cpp` → `ARX_PLAYER_Combat()`
- **Inventory:** `DANAE/ARX_Inventory.cpp` → inventory functions

#### NPC Behavior
- **AI Update:** `DANAE/ARX_NPC.cpp` → `ARX_NPC_Manage_Behaviour()`
- **Pathfinding:** `MINOS/Minos_PathFinder.cpp` → `PathFinder::Move()`
- **Combat AI:** `DANAE/ARX_NPC.cpp` → combat behavior states

#### Rendering
- **Main Draw:** `EERIE/EERIEDraw.cpp` → `EERIE_DRAW_*` functions
- **3D Objects:** `EERIE/EERIEObject.cpp` → object loading/rendering
- **Textures:** `EERIE/EERIETexture.cpp` → texture management
- **Lighting:** `EERIE/EERIELight.cpp` → light calculations

#### Spells
- **Level 1 Spells:** `DANAE/ARX_SpellFX_Lvl01.cpp`
- **Level 10 Spells:** `DANAE/ARX_SpellFX_Lvl10.cpp`
- **Spell System:** `DANAE/ARX_Spells.cpp` → spell management

#### Audio
- **Play Sound:** `Athena/AthenaSample.cpp` → sample playback
- **3D Audio:** `Athena/` → positional audio functions
- **Music:** `Athena/AthenaStream.cpp` → streaming playback

### Important Data Structures

**Location:** `Include/EERIETypes.h`

Key structures to understand:
- `EERIE_3DOBJ` - 3D mesh object
- `INTERACTIVE_OBJ` - Game object with script
- `EERIE_CAMERA` - Camera system
- `EERIE_LIGHT` - Light source
- `EERIE_3D` - 3D vector (x, y, z)

---

## Game Loop Overview

```cpp
// Simplified game loop structure
while (game_running) {
    // 1. Input Processing
    ARX_INPUT_Update();

    // 2. Game Logic Update
    ARX_PLAYER_Frame_Check();      // Update player
    ARX_NPC_Manage_Behaviour();    // Update NPCs
    ARX_SCRIPT_Timer_Check();      // Run scripts
    ARX_PARTICLES_Update();        // Update particles
    ARX_PHYSICS_Apply();           // Apply physics

    // 3. Audio Update
    ARX_SOUND_Update3DSource();    // Update 3D audio

    // 4. Rendering
    EERIE_DRAW_Scene();            // Draw 3D world
    ARX_INTERFACE_Draw();          // Draw UI

    // 5. Present
    Flip();                        // Show rendered frame
}
```

---

## Performance Considerations

### Optimization Techniques Used

1. **Object Pooling**
   - Particles reused instead of allocated/freed each frame
   - Reduces memory allocation overhead

2. **Spatial Partitioning**
   - Portal-based culling for indoor areas
   - Grid-based acceleration for collision detection

3. **LOD System**
   - Multiple mesh detail levels
   - Automatic selection based on distance

4. **Texture Caching**
   - Textures loaded once and cached
   - Reference counting for memory management

5. **Data-Oriented Layout**
   - Structures organized for cache efficiency
   - Hot data grouped together

---

## Debugging and Logging

### Debug Console

**File:** `ArxCommon/ARX_Common.cpp` (fully documented)

**Usage:**
```cpp
ArxDebug::GetInstance()->Log(eLog, "Message: %s", text);
ArxDebug::GetInstance()->Log(eLogWarning, "Warning: %d", value);
ArxDebug::GetInstance()->Log(eLogError, "Error: %s", error);
```

**Features:**
- Color-coded output (normal, warning, error)
- Timestamped messages
- File logging to `../Log/Log__<timestamp>.txt`
- Hierarchical log tags for organization

**Hierarchical Logging:**
```cpp
ArxDebug::GetInstance()->OpenTag("Initialization");
    Log(eLog, "Loading textures...");
    Log(eLog, "Loading sounds...");
ArxDebug::GetInstance()->CloseTag();
```

---

## Common Patterns and Idioms

### Error Handling
```cpp
// Typical error checking pattern
if (!resource) {
    Log(eLogError, "Failed to load resource");
    return FAILURE;
}
```

### Resource Management
```cpp
// Allocation
texture = LoadTexture("player.jpg");

// Usage
if (texture) {
    RenderWithTexture(texture);
}

// Cleanup handled by manager
```

### Singleton Access
```cpp
// Debug system
ArxDebug::GetInstance()->Log(eLog, "Message");

// Cleanup on shutdown
ArxDebug::CleanInstance();
```

---

## Known Limitations

### DirectX 7 Era Constraints

1. **Fixed Function Pipeline**
   - No programmable shaders
   - Limited per-pixel effects
   - Lighting computed per-vertex

2. **Memory Constraints**
   - Designed for 128-256MB RAM systems
   - Aggressive resource pooling required

3. **Single-Threaded**
   - No multi-threading
   - All processing on main thread

---

## Further Reading

### Documentation Files
- **CODEBASE_SUMMARY.md** - Detailed architecture overview
- **ArxCommon/ARX_Common.cpp** - Debugging system (fully commented)
- **MINOS/Minos_PathFinder.cpp** - Pathfinding (fully commented)

### External Resources
- [DirectX 7 Documentation](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/bb219837(v=vs.85))
- [A* Pathfinding](https://www.redblobgames.com/pathfinding/a-star/)
- [Game Engine Architecture](https://www.gameenginebook.com/)

### Recommended Study Order

**For Game Programmers:**
1. Study DANAE game loop and update cycle
2. Examine spell system implementation
3. Explore NPC AI and pathfinding
4. Review physics and collision systems

**For Graphics Programmers:**
1. Study EERIE rendering pipeline
2. Examine lighting system
3. Review texture management
4. Explore animation system

**For Audio Programmers:**
1. Study Athena audio architecture
2. Examine 3D audio positioning
3. Review streaming audio system
4. Explore ambiance management

---

## Contributing

This is a study of historical game engine code. The original Arx Fatalis source was released under GPL.

For the open-source continuation of this engine, see:
- **Arx Libertatis** - Modern cross-platform port (https://arx-libertatis.org/)

---

## License

**GNU General Public License (GPL)**

Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This source code is provided for educational and historical purposes.

---

## Acknowledgments

- **Arkane Studios** - Original development
- **ZeniMax Media** - GPL source release
- **Arx Libertatis Team** - Continued open-source development

---

## Glossary

- **Anchor:** Waypoint in navigation mesh for pathfinding
- **DANAE:** Main game engine module name
- **EERIE:** 3D rendering engine module name
- **Interactive Object:** Game object with script and behavior
- **LOD:** Level of Detail - mesh complexity based on distance
- **Portal:** Connection between rooms for rendering optimization
- **Rune:** Symbol used in spell casting system
- **TEO:** 3D object file format (.teo extension)

---

**Last Updated:** 2025
**For:** Arx Fatalis GPL Source Code Study

For detailed architectural information, see `CODEBASE_SUMMARY.md`
