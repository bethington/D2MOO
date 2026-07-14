# D2MOO - Diablo II Method and Ordinal Overhaul

![Cow](doc/assets/img/ECWLHTH.gif)

This project is a re-implementation of the Diablo2 game coupled with patching facilities for modders.
The aim is to provide the modding community with an easy tool to understand and patch the game.
To ensure that we stay as close as possible to the original game, we do not provide fixes for bugs (yet).

The game assets, main executable code and anything related to hacking or piracy is NOT endorsed by the authors, and as such we will not provide code related to such subjects.
The original game assets and binaries is required to use this project.

### The project, in six questions

* **Who** is this for? Diablo II modders and mod-team developers — in particular
  the [Project Diablo 2](https://www.projectdiablo2.com/) (PD2) team and anyone
  building on top of PD2 — plus anyone reverse-engineering the base game's
  `D2Common`/`D2Game`/etc. DLLs.
* **What** is it? A clean-room, source-level re-implementation of Diablo II's
  game engine, paired with [D2.Detours](https://github.com/Lectem/D2.Detours.git),
  a per-function/per-ordinal patching facility that lets a reimplemented function
  transparently replace (or be replaced by) the original one at runtime.
* **When** does this matter? Whenever a mod needs to change engine behavior that
  isn't exposed by the game's data tables (`.txt`/`.bin`) alone — i.e. anything
  that requires touching actual game logic.
* **Where** does it run? As a set of Windows 32-bit (x86) DLLs, loaded in place
  of (or alongside) the originals via `D2.Detours`' `LoadLibrary` interception,
  inside a real Diablo II / PD2 process.
* **Why** does it exist? To give the modding community a single, well-understood,
  reverse-engineered reference implementation instead of everyone re-deriving the
  same engine internals independently — and, concretely, to make it possible to
  extend and patch **Project Diablo 2's 1.13c** engine, which has no public
  source of its own.
* **How** is correctness verified? Reimplemented functions are checked against
  the real binaries function-by-function, using [Ghidra](https://ghidra-sre.org/)
  driven over an MCP (Model Context Protocol) server to disassemble/decompile
  and even call the original functions live in a running PD2 process, capturing
  golden input/output vectors. D2MOO's version is only flipped to replace the
  original once it is proven bit-exact — see the [conformance](./conformance/)
  harness for the current burn-down status.

## How to build

### Dependencies

For the patching we rely on the [D2.Detours](https://github.com/Lectem/D2.Detours.git) project, which is included as a git submodule. (use the `git submodule update --init --recursive` command, or clone this project with `git clone --recursive`)
You will also need to install the [CMake](https://cmake.org) build system and Visual C++ (or any C++ compiler that can generate .DLLs on Windows) which are freely available.

### Build the project

The recommended way is to use CMake presets.
You can generate a VS solution (located in `out/build/VS20XX`) and build using the command-line:

```sh
# Configure the CMake project
cmake --preset VS2019
# Build the release config (Optional. You may do it from Visual Studio itself)
cmake --build --preset VS2019 --config Release
# Install (Optional. Most people will want to debug instead of install)
cmake --build --preset VS2019 --config Release --target install
```

Or using the CMake-gui:
![CMake GUI](doc/assets/img/CMake-GUI.png)

Where of course you can replace `VS2019` by `VS2022` if you are using Visual Studio 2022.

Read more in the [Advanced build and run](./doc/AdvancedBuildAndRun.md) documentation.

## Usage

If you are using a default Diablo2 install and generated `.sln` through *CMake*, you are good to go, simply build and run with `F5`!

Otherwise have a look at the [Advanced build and run](./doc/AdvancedBuildAndRun.md) and [Debugging](./doc/Debugging.md) documentation.

## Documentation

An embryo of documentation is available in the repository's [doc](./doc/) folder.

## D2MOO Debugger (Experimental!)

Start the game with the `-debug` argument.
For example: `D2.DetoursLauncher -- -debug`.  
Alternatively, you may set the environment variable `D2_DEBUGGER=1`.

![D2Moo Debugger](./doc/assets/img/D2MooDebugger.png)

## Versions

D2MOO's engine logic was originally derived from the **1.10f** version of the game,
but the project now targets **Project Diablo 2 (built on 1.13c) exclusively** --
multi-version support (1.00/1.10f/1.14d) has been removed.

The focus of the project is **Project Diablo 2's 1.13c**: proving
D2MOO's reimplementation bit-exact against a live PD2 process (via the Ghidra MCP
oracle described above) and patching per-ordinal `1.10f -> 1.13c` divergences as
they're found, so D2MOO can act as a real engine base for PD2 modding. See
`D2.Detours.patches/1.13c/` for the 1.13c patch set and [conformance/](./conformance/)
for the ongoing verification work.

## FAQ

### Why is the code so ugly and with names such as `a1`, `a2`, ... ?

The code was originally extracted using a reverse engineering tool, and slowly cleaned to use more understandable names.

### Can I build D2Common.dll and replace it directly with the one from the game ?

Yes this is now possible! However it is not guaranteed to be bugfree, so you may want to patch the functions you use instead.
We are in the (slow) process of checking each ordinal (exported functions) and patching them one by one. See [D2Common.patch.cpp](D2.Detours.patches/1.13c/D2Common.patch.cpp) for the 1.13c (PD2) status of each ordinal, which is verified via the Ghidra MCP conformance harness described above.

This is **NOT** the case yet for other .dlls.

### Can I replace the game .dll files directly with the ones from D2MOO ?

NO !
As mentioned in the previous question, it only works with D2Common.dll (and is not bugfree).

### Why are some DLLs missing ?

Reversing the game is very time consuming. Since `D2Common.dll` and `D2Game.dll` contain most of the game logic, this is where the focus has been set. Any contribution and help is welcome !

### How can I write my own mod using this ?

The documentation for that is not written yet, please contact us directly on the Phrozen Keep [forums](https://www.d2mods.info) / [Discord server](https://discord.gg/NvfftHY).
More importantly, we need your [feedback](https://github.com/ThePhrozenKeep/D2MOO/issues/20) to determine a roadmap.

### Why yet another project for D2 modding and code editing ?

We felt that the current projects are not good enough, and more importantly did not cover enough parts of the game.
Having a centralized code that one can launch and use as reference will make it easier, we hope, for the modding community.

### I can not set breakpoints in Visual Studio

Please have a look at [Microsoft Child Process Debugging Power Tool](https://marketplace.visualstudio.com/items?itemName=vsdbgplat.MicrosoftChildProcessDebuggingPowerTool).

### I have other questions !

Please feel free to open an issue or visit the Phrozen Keep [forums](https://www.d2mods.info) / [Discord server](https://discord.gg/NvfftHY) !


## Credits

This could not have been done without the amazing help and work of the Phrozen Keep community!
Non-exhaustive list of members who helped putting this together (alphabetical order):

 * @Araksson (aka @Conqueror)
 * @dzik
 * @FearedBliss
 * @Firehawk
 * @Harvest
 * @Kieran
 * @Kingpin
 * @Lectem
 * @lolet
 * @Mentor
 * @MirDrualga (aka @IAmTrial)
 * @misiek1294
 * @Mnw1995
 * @Myhrginoc
 * @Necrolis
 * @Nefarius
 * @Nizari
 * @Ogodei
 * Paul Siramy
 * @raler (that sparked the idea for the current name of the project)
 * @SVR
 * @Szumigajowy
 * @whist
 * ...

If you think you should be on this list, reach us on the forum, discord, or open a pull request!

## Legal


The source code in this repository is intended for non-commercial use only. However it uses a permissive license (MIT) so that any modder may use this. Credits to the team are appreciated, and derivative work must respect the license.

Battle.net(R) - Copyright (C) 1996 Blizzard Entertainment, Inc. All rights reserved. Battle.net and Blizzard Entertainment are trademarks or registered trademarks of Blizzard Entertainment, Inc. in the U.S. and/or other countries.

Diablo(R) - Copyright (C) 1996 Blizzard Entertainment, Inc. All rights reserved. Diablo and Blizzard Entertainment are trademarks or registered trademarks of Blizzard Entertainment, Inc. in the U.S. and/or other countries.

D2MOO and any of its' maintainers are in no way associated with or endorsed by Blizzard Entertainment(R).

