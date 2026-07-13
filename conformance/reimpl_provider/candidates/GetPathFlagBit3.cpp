#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetPathFlagBit3
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" uint32_t __stdcall GetPathFlagBit3(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return (*(uint32_t*)(r + 0x34) & 0x8u);
}
