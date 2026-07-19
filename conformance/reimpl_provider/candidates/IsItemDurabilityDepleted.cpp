#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: IsItemDurabilityDepleted

extern "C" int __stdcall IsItemDurabilityDepleted(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    // dwType at offset 0 must be 4 (Item)
    if (*(int*)((char*)pUnit + 0x0) != 4) return 0;

    // dwTxtFileNo at offset 4 -- class id for record lookup
    uint32_t classId = *(uint32_t*)((char*)pUnit + 0x4);

    // GetItemDataRecord(classId)
    typedef void* (__stdcall *GetItemDataRecord_t)(uint32_t);
    GetItemDataRecord_t _fGetItemDataRecord =
        (GetItemDataRecord_t)D2MOO_Resolve("GetItemDataRecord");
    if (_fGetItemDataRecord == nullptr) return 0;
    void* pRecord = _fGetItemDataRecord(classId);
    if (pRecord == nullptr) return 0;

    // record+0x113 byte must be 0 (not indestructible)
    if (*(char*)((char*)pRecord + 0x113) != 0) return 0;
    // record+0x112 byte must be nonzero (has-durability flag)
    if (*(char*)((char*)pRecord + 0x112) == 0) return 0;

    // GetMaxDurabilityStat(pUnit)
    typedef int (__stdcall *GetMaxDurabilityStat_t)(void*);
    GetMaxDurabilityStat_t _fGetMaxDurabilityStat =
        (GetMaxDurabilityStat_t)D2MOO_Resolve("GetMaxDurabilityStat");
    if (_fGetMaxDurabilityStat == nullptr) return 0;
    int maxDur = _fGetMaxDurabilityStat(pUnit);
    if (maxDur == 0) return 0;

    // GetUnitBaseStat(pUnit, 0x98, 0)  -- third arg pushed first (0x0 = base layer)
    typedef int (__stdcall *GetUnitBaseStat_t)(void*, int, int);
    GetUnitBaseStat_t _fGetUnitBaseStat =
        (GetUnitBaseStat_t)D2MOO_Resolve("GetUnitBaseStat");
    if (_fGetUnitBaseStat == nullptr) return 0;
    int curDur = _fGetUnitBaseStat(pUnit, 0x98, 0);

    // SETLE: return 1 if curDur <= 0, else 0
    return (curDur <= 0) ? 1 : 0;
}
