#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: UNITS_GetUnitStat84
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" uint32_t __stdcall UNITS_GetUnitStat84(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return *(uint32_t*)(r + 0x84);
}
