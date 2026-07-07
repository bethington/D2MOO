
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
  src/D2Debugger.overlay.gl.cpp
  src/D2Debugger.profiler.cpp
  # Vendored MemoryModule (MPL-2.0) -- in-memory DLL loader for the WS-1
  # reimpl-provider hot-reload with a custom (verified-address) import resolver.
  ${CMAKE_CURRENT_LIST_DIR}/../../external/MemoryModule/MemoryModule.c
  
  # Font awesome 6
  #src/fa-solid-900.cpp
  src/IconsFontAwesome6.h
  
  include/D2Debugger.h
  include/D2Debugger.patch.common.h
)
target_include_directories(D2Debugger PUBLIC include PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../external/MemoryModule)
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
# those sources -- so a drafted equivalent drops in with no CMake/.def edit.
# CONFIGURE_DEPENDS re-globs (and thus regenerates the .def) when files change.
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
