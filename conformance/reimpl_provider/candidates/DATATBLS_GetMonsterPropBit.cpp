#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetMonsterPropBit

extern "C" int __stdcall DATATBLS_GetMonsterPropBit(void* pUnit) {
    if (pUnit == nullptr) return 0;

    // pUnit->dwType == 1  (offset 0x0)
    if (*(int*)((char*)pUnit + 0x0) != 1) return 0;

    // pUnit->dwTxtFileNo  (offset 0x4)
    int dwTxtFileNo = *(int*)((char*)pUnit + 0x4);
    if (dwTxtFileNo < 0) return 0;

    // g_pDataTables -- pointer variable, deref once to get base struct
    void* pDT_addr = D2MOO_Resolve("g_pDataTables");
    char* pDT = (char*)*(void**)pDT_addr;

    // bounds check: count at offset 0xa80
    if (dwTxtFileNo >= *(int*)(pDT + 0xa80)) return 0;

    // entries base pointer at offset 0xa78
    char* pEntries = *(char**)(pDT + 0xa78);
    if (pEntries == nullptr) return 0;

    // record = pEntries + dwTxtFileNo * 0x1A8; read byte at +0xD
    uint8_t byteAtD = *(uint8_t*)(pEntries + dwTxtFileNo * 0x1A8 + 0xd);

    // g_dat_6fdd90b0 -- bitmask array pointer variable
    char* pBM = (char*)*(void**)D2MOO_Resolve("g_dat_6fdd90b0");
    uint8_t mask = *(uint8_t*)(pBM + 0x14);

    // NEG AL / SBB EAX,EAX / NEG EAX -> normalized 0/1
    return (byteAtD & mask) != 0 ? 1 : 0;
}
