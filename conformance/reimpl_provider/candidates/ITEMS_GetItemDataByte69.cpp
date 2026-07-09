#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataByte69

extern "C" int __stdcall ITEMS_GetItemDataByte69(void* pUnit)
{
    if (pUnit == nullptr) return 0;
    if (*(int*)pUnit != 4) return 0;
    
    int pItemData = *(int*)((char*)pUnit + 0x14);
    if (pItemData == 0) return 0;
    
    // Sentinel: pItemData + 0x5C must not be zero (i.e., pItemData != 0xffffffa4)
    if (pItemData + 0x5C == 0) return 0;
    
    // Read byte at pItemData + 0x69 = pItemData + 0x5C + 0xD
    return (int)(char)*(unsigned char*)(pItemData + 0x5C + 0xD);
}
