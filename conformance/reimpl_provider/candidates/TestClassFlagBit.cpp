#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: TestClassFlagBit

extern "C" uint32_t __stdcall TestClassFlagBit(void* p, uint32_t dwBitIndex) {
    if ((int)dwBitIndex < 0) return 0;

    void* _dtv = D2MOO_Resolve("g_pDataTables");
    if (_dtv == nullptr) return 0;
    char* dt = *(char**)_dtv;
    if ((int)dwBitIndex >= *(int*)(dt + 0xc4)) return 0;

    void* _mv = D2MOO_Resolve("g_dat_6fdd90b0");
    if (_mv == nullptr) return 0;
    uint32_t* masks = *(uint32_t**)_mv;

    uint32_t* table;
    if (p == nullptr || *(int*)p != 1) {
        table = *(uint32_t**)(dt + 0x100);
    } else {
        table = *(uint32_t**)(dt + 0x104);
    }

    return table[dwBitIndex >> 5] & masks[dwBitIndex & 0x1F];
}
