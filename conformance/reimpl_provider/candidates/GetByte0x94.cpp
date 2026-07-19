#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetByte0x94
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" uint8_t __stdcall GetByte0x94(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return *(uint8_t*)(r + 0x94);
}
