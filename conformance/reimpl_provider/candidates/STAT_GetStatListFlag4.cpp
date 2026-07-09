#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STAT_GetStatListFlag4
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned int __stdcall STAT_GetStatListFlag4(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return (*(unsigned int*)(r + 0x34) & 0x4u);
}
