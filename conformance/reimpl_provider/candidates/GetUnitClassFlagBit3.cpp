#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetUnitClassFlagBit3
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" uint32_t __stdcall GetUnitClassFlagBit3(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return ((*(uint32_t*)(r + 0xc8) >> 0x3u) & 0x1u);
}
