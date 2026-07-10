#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetInventoryRecord788Field
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned int __stdcall DATATBLS_GetInventoryRecord788Field(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    r = *(char**)(r + 0x48);
    if (r == nullptr) return 0;
    return *(unsigned int*)(r + 0x94);
}
