#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_IsUnitDualWieldClass
// u16 __stdcall(pUnit, nWeaponIndex) -> *(u16*)(pItemData + 0x38 + nWeaponIndex*2), else 0.
// The original's CONCAT22 packs the sub-data pointer's upper 16 bits into EAX above the
// real AX result -- garbage the caller ignores. The shadow dispatcher is staged ret_bits=16
// so only AX is compared; the reimpl returns just the u16 (zero-extended). Gated dwType==4.

extern "C" unsigned int __stdcall ITEMS_IsUnitDualWieldClass(void* pUnit, int nWeaponIndex)
{
    if (pUnit != nullptr && *(unsigned int*)pUnit == 4) {
        void* pItemData = *(void**)((char*)pUnit + 0x14);
        if (pItemData != nullptr) {
            return *(unsigned short*)((char*)pItemData + 0x38 + nWeaponIndex * 2);
        }
    }
    return 0;
}
