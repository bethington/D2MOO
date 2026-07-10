#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: UNITS_GetField08
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned char __stdcall UNITS_GetField08(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return *(unsigned char*)(r + 0x8);
}
