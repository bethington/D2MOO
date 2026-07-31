#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetUnitItemTableFieldEC
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).
extern "C" short __stdcall ITEMS_GetUnitItemTableFieldEC(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    if (*(uint32_t*)(r + 0x0) != 0x3u) return 0;   // type-gate
    return *(short*)(r + 0x4c);
}
