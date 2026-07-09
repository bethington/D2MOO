#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STAT_IsUnitStatListFlag8Set
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned int __stdcall STAT_IsUnitStatListFlag8Set(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    r = *(char**)(r + 0x5c);
    if (r == nullptr) return 0;
    return ((*(unsigned int*)(r + 0x10) >> 0x8u) & 0x1u);
}
