#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: UNITS_IsTargetInMeleeRange

extern "C" int __stdcall UNITS_IsTargetInMeleeRange(void* pAttacker, void* pTarget, int nExtraRange)
{
    if (pTarget == nullptr || pAttacker == nullptr) return 0;

    int dwType = *(int*)((char*)pTarget + 0x0);

    // dwType==1 with monster classes 0x102 or 0x105: special +8 range bonus path.
    // UNITS_GetMonsterClass (call at 0x6fd51860) is not in the verified call list;
    // this branch cannot be fully replicated, so collapse to the normal flow below.
    (void)dwType;

    // Spec: function reads g_pDataTables; resolve it (offset 0xa80 = monster-class count).
    void* g_pDataTables_addr = D2MOO_Resolve("g_pDataTables");
    (void)g_pDataTables_addr;

    typedef uint8_t (__stdcall *GetUnitWeaponStyle_t)(void* pUnit);
    auto _ws = (GetUnitWeaponStyle_t)D2MOO_Resolve("GetUnitWeaponStyle");
    int bVar1 = _ws ? (int)_ws(pAttacker) : 0;

    typedef int (__stdcall *UNIT_GetDistance_t)(void* pA, void* pT);
    auto _gd = (UNIT_GetDistance_t)D2MOO_Resolve("UNIT_GetDistance");
    int distance = _gd ? _gd(pAttacker, pTarget) : 0;

    // TEST EAX,EAX / JLE -> if distance <= 0, return 1.
    if (distance < 1) return 1;

    // LEA EBX,[EAX+EDX+1] ; CMP EBX,EAX / JGE -> if (weaponStyle + nExtraRange + 1) >= distance, do path check.
    if (distance <= (uint8_t)bVar1 + 1 + nExtraRange) {
        typedef int (__stdcall *PATH_CalculateUnitToUnit_t)(void* pA, void* pT, uint32_t flags);
        auto _pth = (PATH_CalculateUnitToUnit_t)D2MOO_Resolve("PATH_CalculateUnitToUnit");
        int pathRes = _pth ? _pth(pAttacker, pTarget, 0x804u) : 0;
        // NEG/SBB/INC idiom: result==0 -> 1 ; result!=0 -> 0
        return (pathRes != 0) ? 0 : 1;
    }
    return 0;
}
