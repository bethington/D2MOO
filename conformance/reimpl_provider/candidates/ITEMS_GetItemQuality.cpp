#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemQuality
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).  (guard-fail default = 0x2)
extern "C" unsigned int __stdcall ITEMS_GetItemQuality(void* p)
{
    if (p == nullptr) return 0x2;
    char* r = (char*)p;
    if (*(unsigned int*)(r + 0x0) != 0x4u) return 0x2;   // type-gate
    r = *(char**)(r + 0x14);
    if (r == nullptr) return 0x2;
    return *(unsigned int*)(r + 0x0);
}
