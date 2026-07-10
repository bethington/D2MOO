#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetLevelRecordBitfield06
// [abi_static] MECHANICALLY TRANSLATED from disassembly (no model): pure pointer-deref getter.
extern "C" unsigned short __stdcall DATATBLS_GetLevelRecordBitfield06(void* p)
{
    if (p == nullptr) return 0;
    char* r = (char*)p;
    return *(unsigned short*)(r + 0x6);
}
