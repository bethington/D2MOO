#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATLIST_GetStateIndex
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.  (guard-fail default = 0x6)
extern "C" unsigned int __stdcall STATLIST_GetStateIndex(void* p)
{
    if (p == nullptr) return 0x6;
    char* r = (char*)p;
    return *(unsigned int*)(r + 0x8);
}
