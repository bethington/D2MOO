#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetDataShortByIndex
// u16 __stdcall(pUnit, nIndex) -> *(u16*)(pSubData + 0x3e + nIndex*2), else 0.
// pSubData is the pointer at pUnit+0x14 (decompile's pUnit[5]). Same CONCAT22 upper-16
// garbage as the sibling getter -> staged ret_bits=16 so only AX is compared. dwType==4.

extern "C" unsigned int __stdcall ITEMS_GetDataShortByIndex(void* pUnit, int nIndex)
{
    if (pUnit != nullptr && *(unsigned int*)pUnit == 4) {
        void* pSubData = *(void**)((char*)pUnit + 0x14);
        if (pSubData != nullptr) {
            return *(unsigned short*)((char*)pSubData + 0x3e + nIndex * 2);
        }
    }
    return 0;
}
