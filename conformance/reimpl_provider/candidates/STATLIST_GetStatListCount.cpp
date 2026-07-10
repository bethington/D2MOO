#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATLIST_GetStatListCount
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" short __stdcall STATLIST_GetStatListCount(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    r = *(char**)(r + 0x5c);
    if (r == nullptr) return 0;
    return *(short*)(r + 0x4c);
}
