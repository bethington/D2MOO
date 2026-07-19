#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: INV_CanItemFitInStoragePage

extern "C" int __stdcall INV_CanItemFitInStoragePage(void* pUnit, uint32_t dwSlotId) {
    if (pUnit == nullptr) return 0;

    // pUnit->dwType is the first dword (offset 0); must be 4 (ITEM)
    if (*(int*)pUnit != 4) return 0;

    // Local byte storage for the two dimensions output by DATATBLS_GetItemTypeStorageDimensions.
    // The original lays these at [ESP+0x8] (width) and [ESP+0x3] (height) -- 1 byte each.
    uint8_t width = 0;
    uint8_t height = 0;

    // Resolve and call through DATATBLS_GetItemTypeStorageDimensions(pUnit, &width, &height)
    typedef void (__stdcall *DATATBLS_GetItemTypeStorageDimensions_t)(void*, uint8_t*, uint8_t*);
    DATATBLS_GetItemTypeStorageDimensions_t _f =
        (DATATBLS_GetItemTypeStorageDimensions_t)D2MOO_Resolve("DATATBLS_GetItemTypeStorageDimensions");
    if (_f == nullptr) return 0;
    _f(pUnit, &width, &height);

    uint32_t w = (uint32_t)width;
    uint32_t h = (uint32_t)height;

    // If either dimension equals the slot id, the item fits that slot.
    if (w == dwSlotId) return 1;
    if (h == dwSlotId) return 1;

    // Only slot ids 0xB (11) and 0xC (12) have the extra item-type restriction.
    if (dwSlotId != 0xb && dwSlotId != 0xc) return 0;

    // For those slots, the item type encoded in width/height must be 4 or 5.
    if (w == 4) return 1;
    if (h == 4) return 1;
    if (w == 5) return 1;
    if (h == 5) return 1;
    return 0;
}
