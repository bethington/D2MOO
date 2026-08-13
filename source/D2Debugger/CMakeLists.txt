
# The shared, version-agnostic patch core lives in the SIBLING d2-fleet repo.
# Override with -DD2FLEET_ROOT=<path> if your checkout is elsewhere; the include
# below fails loudly (with clone instructions) rather than producing a confusing
# "cannot open source file d2patch.h" later in the build.
set(D2FLEET_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../d2-fleet" CACHE PATH
    "Path to the d2-fleet checkout that owns the shared patch core")
if(NOT EXISTS "${D2FLEET_ROOT}/cmake/D2FleetPatchCore.cmake")
  message(FATAL_ERROR
    "D2Debugger needs the shared patch core from the d2-fleet repository, "
    "expected at:\n    ${D2FLEET_ROOT}\n\n"
    "Check out d2-fleet as a sibling of D2MOO, or pass -DD2FLEET_ROOT=<path>.")
endif()
include(${D2FLEET_ROOT}/cmake/D2FleetPatchCore.cmake)
d2fleet_require_patch_core()

add_library(D2Debugger SHARED
  src/D2Debugger.cpp
  src/D2Debugger.Game.cpp
  src/D2Debugger.imgui.d3d9.cpp
  src/D2Debugger.patch.common.cpp
  src/D2Debugger.LiveDispatch.cpp
  src/D2Debugger.mcp.cpp
  src/D2Debugger.oracle.cpp
  src/D2Debugger.capture.cpp
  src/D2Debugger.gtqueue.cpp
  src/D2Debugger.action.cpp
  src/D2Debugger.assetreload.cpp
  src/D2Debugger.overlay.gl.cpp
  # Virtual input now lives in the SHARED patch core (d2-fleet/patch/vinput.cpp,
  # pulled in via ${D2FLEET_PATCH_SOURCES} below) because D2FleetHook needs the
  # same measured behaviour. What stays here is the one piece that cannot be
  # shared: confirming a move landed by reading D2Client's mouse globals at
  # 1.13c RVAs, which the fleet's one-DLL-for-28-versions rule forbids.
  src/D2Debugger.vinput.moveto.cpp
  # Clean frame capture: grabs the backbuffer BEFORE ImGui composites,
  # which is the only point where an overlay-free image of the game exists.
  src/D2Debugger.vcapture.cpp
  # Present-path probe: counts + caller-module attribution for every ordinary
  # way pixels reach a window. Exists because the four documented present
  # points provably never fire on PD2.
  src/D2Debugger.probe.cpp
  # The game as a movable ImGui panel: uploads the streamed frames into a D3D9
  # texture and routes panel-local input back through the virtual-input layer.
  src/D2Debugger.gamepanel.cpp
  # Fault observer: first-chance VEH + a CHAINED unhandled filter (Fog.dll
  # installs its own and last writer wins) + a MessageBox backstop for the
  # modal "Diablo II Exception" box. Writes to disk before reporting.
  src/D2Debugger.crash.cpp
  # Audio capture: hooks DirectSound buffer writes, mixes the pre-mix sources
  # ourselves (volume + pan + loop), and plays the result through D2Debugger.
  # The audio leg of remote play. Replaces an earlier GetActiveWindow approach
  # that measurement disproved -- D2Sound imports it and never calls it.
  # Audio capture + mixer moved to the shared patch core
  # (d2-fleet/patch/audiocap.cpp): the fleet needs the same ring, and the
  # measurements behind it are about Diablo II, not about either console.
  # Remote-play audio leg: the SECOND consumer of the mixer's ring. Encodes the
  # single mixed stream as FLAC and broadcasts it over WebSocket on 127.0.0.1:8791.
  # Independent of the local playback path by construction -- it reads the ring,
  # which the producer fills without touching any audio device, so streaming
  # works on a machine that has no endpoint at all.
  src/D2Debugger.audiostream.cpp
  # The real game window's mode also moved to the shared core
  # (d2-fleet/patch/gamewindow.cpp): the fleet hides members the same way and
  # must be able to bring one back. The host-specific parts -- yielding topmost,
  # the DPI-unaware scope, the letterbox probe -- are registered from
  # D2Debugger.cpp through D2GameWindow_SetHost rather than compiled in.
  # The debugger's OWN window: picks the monitor by its PHYSICAL mode (a 2560x1440
  # panel at 125% reports 2048x1152 to an unaware caller, so the rect never
  # matches), fills it borderless, and makes only THIS thread DPI aware.
  src/D2Debugger.hostwindow.cpp
  # Magnetic 9-point snapping (corners, edge centres, dead centre) for every
  # panel, with the anchor -- not the pixel position -- remembered in imgui.ini.
  src/D2Debugger.snap.cpp
  # Dark theme with hints of the Diablo II palette, and the single place the
  # panels' semantic colours (ok / warn / divergence / no-hook) are defined.
  src/D2Debugger.theme.cpp
  # The panel's FIXED surface: the resolution the game is configured for, read
  # from ddraw.ini, plus the extension that fills it around an undersized menu
  # frame (baked art via WIC if present, else the frame's edges mirrored).
  src/D2Debugger.canvas.cpp
  src/D2Debugger.profiler.cpp
  # Live-game patch system: a uniform, listable, toggleable set of behaviour
  # changes applied to the running game (multi-instance, exception passthrough,
  # anti-anti-debug + a pre-main debugger gate, a deterministic/freezable clock,
  # focus-nopause, force-load, diagnostics capture) PLUS launch profiles that
  # redirect alternate binaries in-process and track them by content hash.
  #
  # THESE SOURCES LIVE IN THE SIBLING d2-fleet REPO, not here. They are
  # version-agnostic by construction (every patch goes through a Windows API
  # seam, never a game ordinal), which is exactly what d2-fleet needs to inject
  # into 28 D2 versions from 1.00 to PD2 -- so d2-fleet owns them and this target
  # compiles the same files. ONE copy, deliberately: duplicating the guard and
  # slot-allocation logic across two repos is the two-writers-of-one-field shape
  # that has bitten this project three times. ${D2FLEET_PATCH_SOURCES} comes from
  # d2-fleet/cmake/D2FleetPatchCore.cmake, included at the top of this file.
  ${D2FLEET_PATCH_SOURCES}
  # The ImGui panel over that system stays HERE -- it is D2Debugger's UI, and
  # d2-fleet has its own console.
  src/D2Debugger.patchpanel.cpp
  # Vendored MemoryModule (MPL-2.0) -- in-memory DLL loader for the WS-1
  # reimpl-provider hot-reload with a custom (verified-address) import resolver.
  ${CMAKE_CURRENT_LIST_DIR}/../../external/MemoryModule/MemoryModule.c
  
  # Font awesome 6
  #src/fa-solid-900.cpp
  src/IconsFontAwesome6.h
  
  include/D2Debugger.h
  ${D2FLEET_PATCH_HEADERS}
  include/D2Debugger.patch.common.h
  include/D2Debugger.theme.h
  include/D2Debugger.canvas.h
)
target_include_directories(D2Debugger PUBLIC include PRIVATE ${D2FLEET_PATCH_DIR} ${CMAKE_CURRENT_LIST_DIR}/../../external/MemoryModule
          ${CMAKE_CURRENT_LIST_DIR}/../../external/stb)
D2MOO_configure_exports(D2Debugger IGNORE_MISSING_DEF)

target_link_libraries(D2Debugger
  PRIVATE
    D2Common
    D2Game
    imgui::imgui d3d9
    # OpenGL present-hook overlay (D2Debugger.overlay.gl.cpp): Microsoft Detours
    # to hook wglSwapBuffers/SwapBuffers, opengl32 for the GL present entry point.
    Detours::Detours
    opengl32
    # WIC, for loading baked extension art (D2Debugger.canvas.cpp). Part of
    # Windows since Vista -- chosen over vendoring stb_image for one call.
    windowscodecs
    # Lossless encoder for the remote audio stream (D2Debugger.audiostream.cpp).
    # Static, so nothing extra has to be shipped next to the game.
    FLAC
    # SHA-256 for the profile system's content-hash version tracking
    # (D2Debugger.patchcore.cpp). Part of Windows since Vista.
    bcrypt
)
# These are required because somehow loading a .dll that loads DirectX during LoadLibrary crashes.
# So we just delay its loading until first use.
if(MSVC)
    target_link_options(D2Debugger PRIVATE "/DELAYLOAD:d3d9.dll" "Delayimp.lib")
endif()

# WS-1 hot-reloadable reimpl provider (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md):
# a standalone DLL exporting reimpl equivalents by name. Fast to rebuild alone so
# a new equivalent can be hot-loaded into a running PD2 without restarting.
#
# WS-6a generalized drop-in: the baseline coord_provider.cpp plus any drafted
# candidate in reimpl_provider/candidates/*.cpp are compiled in, and the module
# .def is AUTO-GENERATED from every `// D2MOO_REIMPL_EXPORT: <name>` marker across
# those sources -- so a drafted equivalent (a NEW .cpp file) drops in with no
# CMake/.def edit: CONFIGURE_DEPENDS re-globs and regenerates the .def the next
# time CMake configures.
#
# CAVEAT (found the hard way, 2026-07-07): CONFIGURE_DEPENDS only detects the
# glob's FILE LIST changing (a file added/removed) -- it does NOT detect a
# CONTENT edit to an ALREADY-globbed file (e.g. adding a new
# `D2MOO_REIMPL_EXPORT:` marker inside an existing candidates/*.cpp). A plain
# `cmake --build` after such an edit compiles the new function into the object
# file but the .def (and thus the exported symbol) is stale, so
# MemoryGetProcAddress can't find it -- "reimpl not found" even though the code
# is right there. Force a reconfigure (`cmake -S . -B <build-dir>`) after adding
# exports to an EXISTING file; only brand-new files pick up automatically.
set(REIMPL_DIR ${CMAKE_CURRENT_LIST_DIR}/../../conformance/reimpl_provider)
file(GLOB REIMPL_CANDIDATES CONFIGURE_DEPENDS ${REIMPL_DIR}/candidates/*.cpp)
set(REIMPL_SOURCES ${REIMPL_DIR}/coord_provider.cpp ${REIMPL_DIR}/provider_runtime.cpp ${REIMPL_CANDIDATES})

set(REIMPL_EXPORTS "")
foreach(_src ${REIMPL_SOURCES})
  file(READ ${_src} _txt)
  string(REGEX MATCHALL "D2MOO_REIMPL_EXPORT:[ \t]*[A-Za-z_][A-Za-z0-9_]*" _hits "${_txt}")
  foreach(_h ${_hits})
    string(REGEX REPLACE "D2MOO_REIMPL_EXPORT:[ \t]*" "" _name "${_h}")
    list(APPEND REIMPL_EXPORTS "${_name}")
  endforeach()
endforeach()
list(REMOVE_DUPLICATES REIMPL_EXPORTS)
set(REIMPL_GEN_DEF ${CMAKE_CURRENT_BINARY_DIR}/D2MOO_ReimplProvider.gen.def)
set(_defbody "LIBRARY D2MOO_ReimplProvider\nEXPORTS\n")
foreach(_e ${REIMPL_EXPORTS})
  string(APPEND _defbody "    ${_e}\n")
endforeach()
file(WRITE ${REIMPL_GEN_DEF} "${_defbody}")
message(STATUS "[D2MOO_ReimplProvider] exports: ${REIMPL_EXPORTS}")

add_library(D2MOO_ReimplProvider SHARED ${REIMPL_SOURCES} ${REIMPL_GEN_DEF})
set_target_properties(D2MOO_ReimplProvider PROPERTIES OUTPUT_NAME "D2MOO_ReimplProvider")

# Patch-system unit tests. D2Debugger is built with a raw add_library (not
# D2MOO_prepare_targets), so the automatic "add tests/ if it exists" convention
# does not apply here -- wire it explicitly. The test compiles only the pure
# config .cpp, so it does not drag in this DLL's D2Common/imgui/d3d9 deps.
if(D2MOO_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  add_subdirectory(tests)
endif()
