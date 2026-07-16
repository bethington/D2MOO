// D2MOO_REIMPL_EXPORT: CheckWeaponModeSwapActive
#include "../provider_runtime.h"

// ECX -> pUnit (UnitAny*)
// ESI -> callee-saved; not used in the algorithm (PUSH/POP only)
extern "C" int __cdecl CheckWeaponModeSwapActive(int pUnit, int pUnit_esi)
{
    if (pUnit != 0) {
        int pItemData = *(int*)((char*)pUnit + 0x14);
        if (pItemData != 0) {
            int pStatList   = *(int*)((char*)pUnit + 0x1B4);
            uint8_t swapIndex = *(uint8_t*)((char*)pStatList + 0x450);
            int slotValue   = *(int*)((char*)pItemData + (int)swapIndex * 4 + 0x4);
            return (slotValue == 1) ? 1 : 0;
        }
    }
    // Abort path (original calls CleanupAndAbort / _exit).
    // Shadow comparison never reaches here in normal play; return 0 as a no-op.
    return 0;
}
