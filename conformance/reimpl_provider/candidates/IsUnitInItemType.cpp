#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: IsUnitInItemType

extern "C" uint32_t __stdcall IsUnitInItemType(void* pUnit, int nItemType)
{
    if (pUnit == nullptr) return 0;

    void* _g_dt = D2MOO_Resolve("g_pDataTables");
    if (_g_dt == nullptr) return 0;
    char* dtBase = *(char**)_g_dt;

    int nRecordCount = *(int*)(dtBase + 0xbfc);
    if (nItemType < 0 || nItemType >= nRecordCount) return 0;

    uint32_t dwTxtFileNo = *(uint32_t*)((char*)pUnit + 0x4);

    typedef int (__stdcall *GetItemTypeFromClassId_t)(uint32_t);
    GetItemTypeFromClassId_t _getType =
        (GetItemTypeFromClassId_t)D2MOO_Resolve("GetItemTypeFromClassId");
    if (_getType == nullptr) return 0;
    int nPrimaryTypeId = _getType(dwTxtFileNo);

    void* _g_bm = D2MOO_Resolve("g_dat_6fdd90b0");
    if (_g_bm == nullptr) return 0;
    uint32_t* pBitmask = *(uint32_t**)_g_bm;

    int bitIdx = nItemType & 0x1F;
    int dwordIdx = nItemType >> 5;

    uint32_t rowCount = *(uint32_t*)(dtBase + 0xc00);
    uint32_t* pRows    = *(uint32_t**)(dtBase + 0xc04);

    uint32_t mask = pBitmask[bitIdx];
    uint32_t rowVal =
        pRows[rowCount * (uint32_t)nPrimaryTypeId + dwordIdx];
    if (mask & rowVal) return 1;

    typedef void* (__stdcall *GetItemDataRecord_t)(uint32_t);
    GetItemDataRecord_t _getRec =
        (GetItemDataRecord_t)D2MOO_Resolve("GetItemDataRecord");
    if (_getRec == nullptr) return 0;
    void* pRec = _getRec(dwTxtFileNo);
    if (pRec == nullptr) return 0;

    short sVar1 = *(short*)((char*)pRec + 0x120);
    if (sVar1 <= 0) return 0;
    if ((int)sVar1 >= nRecordCount) return 0;

    uint32_t rowVal2 =
        pRows[rowCount * (uint32_t)(int)sVar1 + dwordIdx];
    return rowVal2 & mask;
}
