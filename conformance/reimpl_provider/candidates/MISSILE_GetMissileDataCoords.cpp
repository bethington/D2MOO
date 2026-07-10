#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: MISSILE_GetMissileDataCoords
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).
extern "C" unsigned int __stdcall MISSILE_GetMissileDataCoords(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    if (*(unsigned int*)(r + 0x0) != 0x3u) return 0;   // type-gate
    r = *(char**)(r + 0x14);
    if (r == nullptr) return 0;
    return *(unsigned int*)(r + 0x2c);
}
