#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsItemEthereal
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).
extern "C" uint32_t __stdcall IsItemEthereal(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    if (*(uint32_t*)(r + 0x0) != 0x4u) return 0;   // type-gate
    r = *(char**)(r + 0x14);
    if (r == nullptr) return 0;
    return (*(uint32_t*)(r + 0x18) & 0x400000u);
}
