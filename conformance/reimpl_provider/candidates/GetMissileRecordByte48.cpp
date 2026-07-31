#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetMissileRecordByte48
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter with type-gate(s).
extern "C" short __stdcall GetMissileRecordByte48(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    if (*(uint32_t*)(r + 0x0) != 0x3u) return 0;   // type-gate
    r = *(char**)(r + 0x14);
    if (r == nullptr) return 0;
    return *(short*)(r + 0xa);
}
