add_subdirectory(D2CommonDefinitions)
add_subdirectory(D2Client)
# Patch carrier for the conformance lab's subject. Not a Diablo II DLL -- see
# source/SGD2FreeRes/CMakeLists.txt. Must be added here (not only via
# D2_KNOWN_DLLS) because D2.Detours.patches/CMakeLists.txt attaches patch
# sources to targets that must already exist.
add_subdirectory(SGD2FreeRes)
add_subdirectory(Storm)
add_subdirectory(Fog)
add_subdirectory(D2Hell)
add_subdirectory(D2CMP)
add_subdirectory(D2Lang)
add_subdirectory(D2Net)
add_subdirectory(D2Common)
add_subdirectory(D2Game)
add_subdirectory(D2Gfx)
add_subdirectory(D2Sound)
add_subdirectory(D2MCPClient)
add_subdirectory(D2Win)
add_subdirectory(D2Debugger)
add_subdirectory(Game)

