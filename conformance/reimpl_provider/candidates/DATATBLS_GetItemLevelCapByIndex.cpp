#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemLevelCapByIndex
// scalar nIndex -> g_pExperienceTxtRecords (stride 0x20): nIndex<=0 -> base+0x1c;
// nIndex > count(base[0]) -> 0; else base + nIndex*0x20 + 0x3c. (Verified from disasm 0x6fdae840.)
extern "C" unsigned int __stdcall DATATBLS_GetItemLevelCapByIndex(int nIndex)
{
    void* _g = D2MOO_Resolve("g_pExperienceTxtRecords");
    if (_g == nullptr) return 0;
    char* base = *(char**)_g;
    if (base == nullptr) return 0;
    if (nIndex <= 0) return *(unsigned int*)(base + 0x1c);
    if (nIndex > *(int*)base) return 0;
    return *(unsigned int*)(base + nIndex * 0x20 + 0x3c);
}
