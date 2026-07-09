#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataShort36
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).
extern "C" unsigned short __stdcall ITEMS_GetItemDataShort36(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    if (*(unsigned int*)(r + 0x0) != 0x4u) return 0;   // type-gate
    r = *(char**)(r + 0x14);
    if (r == nullptr) return 0;
    return *(unsigned short*)(r + 0x36);
}
