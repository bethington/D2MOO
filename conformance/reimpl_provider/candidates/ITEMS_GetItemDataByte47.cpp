// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataByte47
#include "../provider_runtime.h"

// Shadow for ITEMS_GetItemDataByte47 (D2Common 0x6fd73740)
// Returns D2UnitStrc.pItemData->nItemCell (offset 0x47) when unit is type 4
// and pItemData is non-null; otherwise OR-style 0xFF sentinel.
// stdcall, RET 0x4, return in EAX (32-bit meaningful width).
extern "C" uint32_t __stdcall ITEMS_GetItemDataByte47(void* pUnit)
{
    uint32_t eax = (uint32_t)pUnit;
    if (eax != 0u && *(const uint32_t*)eax == 4u) {
        eax = *(const uint32_t*)(eax + 0x14u);   // pItemData (ItemData_F720*)
        if (eax != 0u) {
            // Success: AL = byte at +0x47; upper 24 bits preserved (pItemData)
            return (eax & 0xFFFFFF00u) | (uint32_t)(*(const uint8_t*)(eax + 0x47u));
        }
    }
    // Failure: OR AL, 0xFF (only low byte replaced; upper bits preserved)
    return (eax & 0xFFFFFF00u) | 0xFFu;
}
