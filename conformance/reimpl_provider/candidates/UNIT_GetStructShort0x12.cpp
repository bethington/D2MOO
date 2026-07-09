#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: UNIT_GetStructShort0x12
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned short __stdcall UNIT_GetStructShort0x12(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return *(unsigned short*)(r + 0x12);
}
