#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: UNIT_GetGfxInfo
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned int __stdcall UNIT_GetGfxInfo(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return *(unsigned int*)(r + 0x3c);
}
