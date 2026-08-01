#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetGlobalConfigByte2
// NEEDS GLOBAL: g_abGlobalConfig
extern "C" void __stdcall SetGlobalConfigByte2(uint8_t byValue)
{
    char* base = (char*)D2MOO_Resolve("g_abGlobalConfig");
    if (!base) return; // resolver missing / unknown global -> no-op (void fn)
    *(char*)(base + 0) = (char)byValue;
}
