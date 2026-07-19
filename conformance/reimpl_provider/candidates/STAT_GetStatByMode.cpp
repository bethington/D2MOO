#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: STAT_GetStatByMode

extern "C" int __fastcall STAT_GetStatByMode(int nStatIndex, int nMode, int nUnused, void* ppUnit) {
    if (ppUnit == nullptr) return 0;
    void* pUnit = *(void**)ppUnit;
    if (pUnit == nullptr) return 0;
    if (nStatIndex < 0) return 0;

    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* base = (char*)*(void**)_g;
    if (base == nullptr) return 0;

    int nMonStatsCount = *(int*)(base + 0xbd4);
    if (nMonStatsCount <= nStatIndex) return 0;

    if (nStatIndex == 0x13) {
        typedef int (__stdcall *STAT_CalcCombatBonus_t)(void*);
        STAT_CalcCombatBonus_t _f = (STAT_CalcCombatBonus_t)D2MOO_Resolve("STAT_CalcCombatBonus");
        if (_f == nullptr) return 0;
        return _f(pUnit);
    }

    if (nMode != 1) {
        if (nMode != 2) {
            typedef int (__stdcall *GetUnitBaseStat_t)(void*, int, int);
            GetUnitBaseStat_t _f = (GetUnitBaseStat_t)D2MOO_Resolve("GetUnitBaseStat");
            if (_f == nullptr) return 0;
            return _f(pUnit, nStatIndex, 0);
        }
        typedef int (__stdcall *DATATBLS_GetItemStatCostValue_t)(void*, int, int);
        DATATBLS_GetItemStatCostValue_t _f = (DATATBLS_GetItemStatCostValue_t)D2MOO_Resolve("DATATBLS_GetItemStatCostValue");
        if (_f == nullptr) return 0;
        return _f(pUnit, nStatIndex, 0);
    }
    typedef int (__stdcall *DATATBLS_GetCursorItemStatCost_t)(void*, int, int);
    DATATBLS_GetCursorItemStatCost_t _f = (DATATBLS_GetCursorItemStatCost_t)D2MOO_Resolve("DATATBLS_GetCursorItemStatCost");
    if (_f == nullptr) return 0;
    return _f(pUnit, nStatIndex, 0);
}
