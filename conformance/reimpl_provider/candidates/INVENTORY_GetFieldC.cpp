#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: INVENTORY_GetFieldC
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).
extern "C" unsigned int __stdcall INVENTORY_GetFieldC(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    if (*(unsigned int*)(r + 0x0) != 0x1020304u) return 0;   // type-gate
    return *(unsigned int*)(r + 0xc);
}
