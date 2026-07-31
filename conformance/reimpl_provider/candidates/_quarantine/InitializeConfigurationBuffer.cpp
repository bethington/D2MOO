// Reimpl of Storm.dll!InitializeConfigurationBuffer (0x6fc0d990)
// Allocates and initializes a static 48-byte configuration buffer with
// four DWORD values and a 32-byte null-terminated string.
// Storm.dll SXLIB internal utility -- reads/writes a static pointer
// variable (g_pConfigurationBuffer) which is lazily allocated on first
// successful call.  All four DWORD slots + the string slot are then
// populated on every successful call.

#include "../provider_runtime.h"

// Storm.dll helpers expected to be exposed by the provider runtime.
//   MEMORY_AllocateMemoryFromArena -- arena allocator used for the buffer
//   SStrCopy                       -- bounded string copy (incl. NUL)
extern "C" void* __stdcall MEMORY_AllocateMemoryFromArena(uint32_t size, const char* srcFile, int line, int flags);
extern "C" void __stdcall SStrCopy(uint8_t* dst, const uint8_t* src, int maxLen);

// D2MOO_REIMPL_EXPORT: InitializeConfigurationBuffer
extern "C" uint32_t __stdcall InitializeConfigurationBuffer(
    uint32_t dwConfigValue1,
    uint32_t dwConfigValue2,
    uint32_t dwConfigValue3,
    uint32_t dwConfigValue4,
    char* szConfigString)
{
    // Resolve the static buffer pointer variable BY NAME.
    // Ghidra names it `_g_pConfigurationBuffer` -- per the rules we drop
    // the leading underscore.
    // NEEDS GLOBAL: g_pConfigurationBuffer
    void** ppGlobal = (void**)D2MOO_Resolve("g_pConfigurationBuffer");
    if (!ppGlobal)
        return 0;  // resolver missing -> obvious mismatch sentinel

    // STEP 1: the symbol is a POINTER VARIABLE (g_p...), so deref ONCE --
    // `base` is now the address of the 0x30-byte configuration buffer.
    char* base = (char*)*ppGlobal;

    // Validation (matches decompile 1:1; short-circuit AND order):
    //   dwConfigValue1 != 0
    //   AND dwConfigValue2 != 0
    //   AND szConfigString != NULL
    //   AND *szConfigString != '\0'
    if ((((dwConfigValue1 != 0) && (dwConfigValue2 != 0)) && (szConfigString != (char*)0x0)) &&
        (*szConfigString != '\0')) {
        // Lazy-allocate the buffer the first time only.
        if (base == (char*)0x0) {
            base = (char*)MEMORY_AllocateMemoryFromArena(
                0x30, "..\\3rdParty\\STORM\\SOURCE\\SDLG.CPP", 0x7de, 0);
            *ppGlobal = base;  // mirror original's store-back side effect
        }
        // Literal offset translation of the four DWORD stores:
        *(uint32_t*)base          = dwConfigValue1;   // +0x00
        *(uint32_t*)(base + 4)    = dwConfigValue2;   // +0x04
        *(uint32_t*)(base + 8)    = dwConfigValue3;   // +0x08
        *(uint32_t*)(base + 0x0c) = dwConfigValue4;   // +0x0c
        // 32-byte bounded copy of the configuration string into +0x10.
        SStrCopy((uint8_t*)(base + 0x10),
                 (uint8_t*)szConfigString,
                 0x20);
        return 1;
    }
    return 0;
}
