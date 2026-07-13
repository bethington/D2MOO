#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STAT_GetStatListOwnerGuid
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).
extern "C" uint32_t __stdcall STAT_GetStatListOwnerGuid(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    if (*(uint32_t*)(r + 0x0) != 0x1020304u) return 0;   // type-gate
    return *(uint32_t*)(r + 0x20);
}
