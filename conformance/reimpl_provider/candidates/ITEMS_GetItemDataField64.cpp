// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataField64
#include "../provider_runtime.h"

extern "C" unsigned int __stdcall ITEMS_GetItemDataField64(void* pUnit)
{
    if (pUnit == nullptr) return 0;
    
    unsigned int dwType = *(unsigned int*)((char*)pUnit + 0x0);
    if (dwType != 4) return 0;
    
    void* pItemData = *(void**)((char*)pUnit + 0x14);
    if (pItemData == nullptr) return 0;
    
    void* pItemListNext = *(void**)((char*)pItemData + 0x5C);
    if (pItemListNext == nullptr) return 0;
    
    return *(unsigned int*)((char*)pItemListNext + 0x8);
}
