# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

D2MOO (Diablo II Method and Ordinal Overhaul) is a re-implementation of Diablo II's game engine coupled with patching facilities for modders. It uses D2.Detours to intercept LoadLibrary calls and replace/patch original DLLs with reimplemented versions. The project stays close to the original game behavior and intentionally does not fix known game bugs.

**Target Version:** 1.10f (with support for 1.00 and 1.14d)
**Platform:** Windows 32-bit only (x86)

## Build Commands

```sh
# Clone with submodules (required for D2.Detours, doctest, imgui, squall)
git clone --recursive <repo>
# Or update submodules after clone
git submodule update --init --recursive

# Configure (VS2019, VS2022, or ninja presets available)
cmake --preset VS2022

# Build
cmake --build --preset VS2022 --config Release

# Run tests
ctest --preset VS2022 --config Release --extra-verbose

# Install
cmake --build --preset VS2022 --config Release --target install
```

**Build directory:** `out/build/VS2022/`
**Install directory:** `out/install/VS2022/`

Key CMake options:
- `D2MOO_ORDINALS_VERSION` - Version selector: "1.00", "1.10f" (default), "1.14d"
- `D2MOO_BUILD_TESTS` - Enable unit tests (doctest framework)
- `ENABLE_D2DETOURS_EMBEDDED_PATCHES` - Include patching code

## Architecture

### Core Components
- **D2Common** - Core game mechanics (items, units, collision, DRLG level generation, pathfinding). Largest module, most complete.
- **D2Game** - Game state, server logic, quests, skills, AI, unit control
- **D2.Detours** (submodule) - DLL patching/hooking infrastructure

### Supporting Libraries
- **D2CommonDefinitions** - Header-only shared definitions, constants, structs
- **Fog** - Utility library (memory, strings, math, logging, Excel file parsing)
- **Storm** - Wrapper around squall library for resource access
- **D2Debugger** - Optional ImGui-based runtime debugger (`-debug` flag or `D2_DEBUGGER=1` env var)
- **D2Lang, D2CMP, D2Hell, D2Net, D2Gfx, D2Win, D2Sound, D2MCPClient** - Other game subsystems (partial implementations)

### Patching System
Located in `D2.Detours.patches/VERSION/`:
- Each DLL has a `NAME.patch.cpp` defining which ordinals to patch
- `GetPatchAction()` returns: `FunctionReplaceOriginalByPatch`, `FunctionReplacePatchByOriginal`, `PointerReplace*`, or `Ignore`
- Patching is bidirectional and opt-in per function

## Code Conventions

### Style
- **Indentation:** Tabs, size 4
- **Struct naming:** `D2<Component><Purpose>Strc` (e.g., `D2GameStrc`, `D2UnitStrc`)
- **Function naming:** `<COMPONENT>_<Action>()` uppercase (e.g., `DUNGEON_GameToClientCoords()`)
- **Calling conventions:** `__stdcall`, `__fastcall` for Windows DLL compatibility
- **Ordinals:** Functions reference ordinals in comments like `#10892`

### Reverse-Engineered Code
Some code uses generic parameter names (`a1`, `a2`, ...) from reverse engineering tools. These are gradually cleaned up to use meaningful names.

### Module Structure
```
ModuleName/
├── include/           # Public headers
├── src/               # Implementation
├── definitions/       # .def files for ordinal exports per version
├── tests/             # Unit tests (doctest)
└── statictests/       # Compile-time struct layout checks
```

## Testing

- **Framework:** doctest (header-only)
- **Run single test:** Build the test executable and run with doctest filters
  ```sh
  D2CommonTests.exe --test-case="*TestName*"
  ```
- **Static tests:** Compile-time `static_assert` checks for struct layouts

## Key Subsystems

**D2Common:**
- `DataTbls/` - Static game data tables (items, monsters, skills)
- `Drlg/` - DRLG (procedural level generation)
- `Path/` - Pathfinding (A*, IDA*, wayfield)
- `Units/` - Entity management (Player, Monster, Item, Object)

**D2Game:**
- `GAME/` - Game state, client management
- `QUESTS/` - Quest logic organized by act (Act1-5)
- `SKILLS/` - Skill implementations
- `MONSTER/` - Monster AI and spawning
- `AI/` - Monster behavior

## Coordinate System

Game uses two coordinate systems (see `doc/Coordinates.md`):
- **Game coordinates:** Tile-based grid, 8 subtiles per floor tile, 16-bit fixed point precision
- **Client coordinates:** Dimetric (isometric) projection for rendering
- Convert with: `DUNGEON_GameToClientCoords()`, `DUNGEON_ClientToGameCoords()`

## Debugging

- Use Visual Studio with Microsoft Child Process Debugging Power Tool (original game spawns as child process)
- Symbols available only for functions patched with `FunctionReplaceOriginalByPatch`
- Runtime debugger: Start with `-debug` flag or `D2_DEBUGGER=1` environment variable
