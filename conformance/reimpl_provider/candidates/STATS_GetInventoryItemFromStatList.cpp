#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATS_GetInventoryItemFromStatList
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned int __stdcall STATS_GetInventoryItemFromStatList(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    r = *(char**)(r + 0x4);
    if (r == nullptr) return 0;
    return *(unsigned int*)(r + 0xc);
}
