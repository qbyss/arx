# Arx Fatalis - Codebase Summary

## Overview

Arx Fatalis is a first-person action role-playing game developed by Arkane Studios and released in 2002. The source code was released under GPL in 2011. This codebase represents a complete game engine built on DirectX 7, featuring advanced rendering, physics, AI, and audio systems for its era.

**Technology Stack:**
- Language: C++ (with some C-style code)
- Graphics API: DirectX 7 (Direct3D)
- Audio: Custom Athena library with DirectSound + EAX
- Platform: Windows (Win32)
- Total Files: 251 source files (116 .cpp + 135 .h)
- Codebase Size: ~7.8 MB

---

## Directory Structure

```
/arx/
├── ArxCommon/          - Common utilities and debugging infrastructure
├── Athena/             - Audio library (16 .cpp files)
├── DANAE/              - Main game engine (65 .cpp files)
├── DANAE_Debugger/     - Script debugging tools
├── EERIE/              - 3D rendering engine (20 .cpp files)
├── HERMES/             - File packaging system (10 .cpp files)
├── Mercury/            - Additional module (2 .cpp files)
├── MINOS/              - Pathfinding (1 .cpp file)
├── Include/            - Header files (135 .h files)
│   └── OtherLibs/      - Third-party library headers
└── lib/                - Static libraries
    └── otherlibs/      - JPEG, EAX, zlib, etc.
```

---

## Component Architecture

### 1. **DANAE** - Core Game Engine (65 files)

**Location:** `/arx/DANAE/`

**Purpose:** Main game logic, player systems, NPC AI, and game state management.

**Key Subsystems:**
- **Player Management:** Player controls, camera, movement, interaction
- **NPC AI:** Enemy behavior, pathfinding integration, combat AI
- **Spell System:** 10 spell levels (ARX_SpellFX_Lvl01 through Lvl10)
- **Particle Systems:** Visual effects for magic and environmental effects
- **Interface:** Menus, inventory, HUD, dialogue systems
- **Physics:** Collision detection, damage calculation, object interaction
- **Script Interpreter:** Scripting language for interactive objects and quests
- **Save/Load:** Game state serialization
- **Level Management:** Scene loading, transitions, and management

**Key Files:**
- `ARX_SpellFX_Lvl*.cpp` - Ten spell effect implementation files (one per spell level)
- `ARX_Particles.cpp` - Particle system for visual effects
- `ARX_Player.cpp` - Player character management
- `ARX_NPC.cpp` - Non-player character AI and behavior
- `ARX_Interface.cpp` - User interface and menus
- `ARX_Script.cpp` - Script interpreter for interactive objects
- `ARX_Physics.cpp` - Physics and collision systems
- `ARX_Scene.cpp` - Scene and level management

---

### 2. **EERIE** - 3D Rendering Engine (20 files)

**Location:** `/arx/EERIE/`

**Purpose:** Low-level 3D graphics rendering, animation, and visual systems.

**Key Subsystems:**
- **Object Management:** 3D mesh loading, manipulation, and rendering
- **Animation System:** Skeletal animation with bones and keyframes
- **Texture System:** Texture loading, management, and caching
- **Lighting:** Dynamic and static lighting calculations
- **Rendering:** DirectX 7 polygon rendering and draw calls
- **Collision:** Collision detection and sphere-based collision systems
- **Effects:** Progressive mesh LOD, portal rendering, cloth simulation

**Key Files:**
- `EERIEPoly.cpp` (142KB) - Polygon rendering and management (largest file)
- `EERIETexture.cpp` (145KB) - Texture loading and management
- `EERIEAnim.cpp` (112KB) - Skeletal animation system
- `EERIELight.cpp` - Dynamic lighting system
- `EERIEObject.cpp` - 3D object structure and manipulation
- `EERIEDraw.cpp` - DirectX rendering and draw calls
- `EERIEMesh.cpp` - 3D mesh data structures
- `EERIEPathfinding.cpp` - Pathfinding grid integration
- `EERIECloth.cpp` - Cloth simulation using spring-mass system

---

### 3. **Athena** - Audio Library (16 files)

**Location:** `/arx/Athena/`

**Purpose:** Custom audio engine for 3D positional sound, music, and environmental audio.

**Key Features:**
- 3D positional audio (sound sources in game space)
- Sample playback and streaming
- Environmental audio effects (reverb, echo)
- ADPCM codec support
- WAV and ASF format support
- EAX (Environmental Audio Extensions) for hardware acceleration
- Audio mixing and hierarchical mixer system
- Ambiance management for atmospheric sound

**Key Files:**
- `AthenaSample.cpp` - Audio sample management
- `AthenaStream.cpp` - Streaming audio for music and long sounds
- `AthenaAmbiance.cpp` - Environmental ambiance system
- `AthenaMixer.cpp` - Audio mixing and output
- `AthenaCodecADPCM.cpp` - ADPCM compression codec
- `AthenaFileWAV.cpp` - WAV file format support
- `AthenaFileASF.cpp` - ASF file format support

---

### 4. **HERMES** - Resource Management (10 files)

**Location:** `/arx/HERMES/`

**Purpose:** File packaging, compression, and resource loading system.

**Key Features:**
- PAK file system for game resource archives
- File compression and decompression
- Resource hashing for fast lookup
- Cluster-based resource organization
- DDE (Dynamic Data Exchange) support for inter-process communication

**Key Files:**
- `HERMES_PAK.cpp` - PAK archive file format
- `HERMES_File.cpp` - File I/O abstraction
- `HERMES_Compression.cpp` - Compression algorithms
- `HERMES_Hash.cpp` - Resource hashing system
- `HERMES_Cluster.cpp` - Resource clustering

---

### 5. **MINOS** - Pathfinding (1 file)

**Location:** `/arx/MINOS/`

**Purpose:** AI pathfinding algorithms for NPC navigation.

**Key Features:**
- Grid-based pathfinding
- A* or similar pathfinding algorithm
- Integration with EERIE collision system
- Path smoothing and optimization

**Key Files:**
- `MINOS_Pathfind.cpp` - Core pathfinding implementation

---

### 6. **Mercury** - Utility Module (2 files)

**Location:** `/arx/Mercury/`

**Purpose:** Additional utility functionality (specific purpose requires deeper analysis).

---

### 7. **ArxCommon** - Common Utilities (1 file)

**Location:** `/arx/ArxCommon/`

**Purpose:** Shared debugging, logging, and utility infrastructure used across all modules.

**Key Features:**
- Assertion macros for debugging
- Type-casting helpers
- Logging infrastructure
- Cross-module utilities

**Key Files:**
- `ARX_Common.cpp` - Common utility functions and debugging tools

---

## Core Data Structures

### EERIE_3DOBJ
**Location:** `Include/EERIETypes.h`

Main 3D object structure containing:
- Vertex data
- Polygon/face data
- Texture coordinates
- Skeletal bone structure
- Animation data
- Bounding volumes

### INTERACTIVE_OBJ
**Location:** `Include/EERIETypes.h`

Interactive game object structure containing:
- 3D object reference
- Script data
- AI behavior
- Inventory data
- Physical properties
- Game state flags

### EERIE_CAMERA
**Location:** `Include/EERIETypes.h`

Camera system structure containing:
- Position and orientation
- Projection matrices
- View frustum
- Clipping planes

---

## System Flow

### Game Initialization
1. **Window Creation** - Initialize Win32 window and DirectX
2. **Resource Loading** - Load PAK files via HERMES
3. **Audio Initialization** - Start Athena audio system
4. **Rendering Setup** - Initialize EERIE rendering engine
5. **Game State** - Load or create new game via DANAE

### Main Game Loop
1. **Input Processing** - Handle keyboard/mouse via DANAE
2. **AI Update** - Update NPC behavior and pathfinding
3. **Physics Update** - Calculate collisions and physics
4. **Script Execution** - Run object scripts
5. **Rendering** - Draw scene via EERIE
6. **Audio Update** - Update 3D audio positions via Athena
7. **Particle Update** - Update visual effects

### Rendering Pipeline
1. **Visibility Determination** - Portal-based culling
2. **Lighting Calculation** - Dynamic light calculations
3. **LOD Selection** - Choose appropriate mesh detail level
4. **Texture Binding** - Bind textures for objects
5. **Draw Calls** - Submit polygons to DirectX
6. **Post-Processing** - Apply visual effects
7. **UI Rendering** - Draw interface elements

---

## Key Design Patterns

### Data-Oriented Design
- Large structs (EERIE_3DOBJ, INTERACTIVE_OBJ) with data grouped by type
- Array-based storage for performance
- Cache-friendly memory layout

### Component-Based Entities
- Objects have various data components (visual, physics, AI, script)
- Modular attachment of functionality
- Flexible object composition

### Manager Classes
- Texture manager for resource pooling
- Particle manager for effect pooling
- Lighting manager for light pooling
- Centralized resource management

### Resource Pooling
- Reuse of textures, sounds, particles
- Memory efficiency
- Reduced allocation overhead

---

## Notable Technical Features

### Progressive Mesh LOD System
- Automatic level-of-detail based on distance
- Smooth transitions between LOD levels
- Performance optimization for complex scenes

### Portal-Based Rendering
- Scene divided into portal-connected rooms
- Visibility culling for performance
- Efficient rendering of large indoor environments

### Cloth Simulation
- Spring-mass system for realistic cloth
- Integration with collision system
- Used for character clothing and banners

### Skeletal Animation
- Bone-based character animation
- Keyframe interpolation
- Blending between animations

### Advanced Particle Effects
- Spell effects with complex particle behaviors
- Environmental effects (rain, fog, fire)
- Performance-optimized particle pooling

### Scripting System
- Custom scripting language for interactive objects
- Event-driven script execution
- Quest and dialogue scripting

---

## Third-Party Dependencies

**Location:** `/arx/lib/otherlibs/`

- **JPEG Library** (jpeglib.lib) - Texture loading
- **zlib** (zlib.lib) - Compression
- **IMPLODE** (IMPLODE.LIB) - Additional compression
- **EAX** (eax.lib, eaxguid.lib) - 3D audio hardware acceleration
- **DirectShow** (amstrmid.lib) - Video playback

---

## Build Configuration

The project uses Visual C++ 6.0 project files (.dsp) with the following typical configuration:
- **Platform:** Win32
- **Configurations:** Debug, Release
- **Runtime:** Multithreaded DLL
- **Optimizations:** Speed optimizations in Release
- **DirectX SDK:** DirectX 7 SDK required

---

## Historical Context

**Development Period:** ~1999-2002
**Developer:** Arkane Studios
**Publisher:** JoWooD Productions (PC), Dreamcatcher Interactive (NA)
**Release:** November 2002
**Open Source Release:** 2011 (GPL license)

This engine represents state-of-the-art game technology for the early 2000s, with advanced features like:
- Physics-based magic system with rune combinations
- Immersive sim design with interactive objects
- Advanced lighting and shadows for DirectX 7
- Complex AI behaviors and pathfinding
- Rich environmental audio

---

## Code Statistics

- **Total Files:** 251
- **C++ Implementation Files:** 116
- **Header Files:** 135
- **Estimated Lines of Code:** ~150,000+ LOC
- **Primary Language:** C++
- **API Version:** DirectX 7

---

## Getting Started with the Code

### Recommended Reading Order for Understanding the Engine:

1. **Start with ArxCommon** - Understand basic utilities
2. **Study EERIE basics** - Learn 3D object structure (EERIETypes.h)
3. **Examine HERMES** - Understand resource loading
4. **Explore Athena** - Learn audio system
5. **Study DANAE** - Understand game logic
6. **Deep dive EERIE** - Master rendering system

### Key Entry Points:

- **WinMain** - Application entry point (in DANAE)
- **Game Loop** - Main update loop (in DANAE)
- **Rendering** - EERIE_DRAW_* functions
- **Player Input** - ARX_Player.cpp input handlers
- **Script Execution** - ARX_Script.cpp interpreter

---

## File Naming Conventions

- **ARX_*** - DANAE game engine files
- **EERIE*** - EERIE rendering engine files
- **Athena*** - Athena audio system files
- **HERMES_*** - HERMES resource system files
- **MINOS_*** - MINOS pathfinding files

---

## License

GPL (GNU General Public License) - Open source since 2011

---

*This summary was generated to help developers understand and navigate the Arx Fatalis codebase. For detailed implementation comments, see individual source files.*
