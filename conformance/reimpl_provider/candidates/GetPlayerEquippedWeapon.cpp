#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: GetPlayerEquippedWeapon
extern "C" void* __stdcall GetPlayerEquippedWeapon(void* pUnit) {
    if (pUnit == (void*)0) return (void*)0;
    if (*(uint32_t*)((char*)pUnit + 0x0) != 1u) return (void*)0;
    void* pInventory = *(void**)((char*)pUnit + 0x60);
    if (pInventory == (void*)0) return (void*)0;
    void* pUnitOut = (void*)0;
    uint32_t dwUnknown04 = 0;
    uint32_t dwUnknown08 = 0;
    typedef int (__stdcall *INV_FindUsableWeaponByUnit_t)(void*, void**, uint32_t*, uint32_t*);
    INV_FindUsableWeaponByUnit_t _f = (INV_FindUsableWeaponByUnit_t)D2MOO_Resolve("INV_FindUsableWeaponByUnit");
    if (_f == (INV_FindUsableWeaponByUnit_t)0) return (void*)0;
    int iVar1 = _f(pInventory, &pUnitOut, &dwUnknown04, &dwUnknown08);
    return (void*)((-(uint32_t)(iVar1 != 0)) & (uint32_t)pUnitOut);
}
