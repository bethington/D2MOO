#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: MISSILE_GetMissileDataStatus
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).
extern "C" uint32_t __stdcall MISSILE_GetMissileDataStatus(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    if (*(uint32_t*)(r + 0x0) != 0x3u) return 0;   // type-gate
    r = *(char**)(r + 0x14);
    if (r == nullptr) return 0;
    return *(uint32_t*)(r + 0x28);
}
