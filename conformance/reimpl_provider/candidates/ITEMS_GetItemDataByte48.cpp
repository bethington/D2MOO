#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataByte48
extern "C" unsigned int __stdcall ITEMS_GetItemDataByte48(void* pUnit)
{
    if (pUnit == nullptr) return 1;
    if (*(int*)((char*)pUnit + 0x0) != 4) return 1;
    void* pPlayerData = *(void**)((char*)pUnit + 0x14);
    if (pPlayerData == nullptr) return 1;
    unsigned char result = *(unsigned char*)((char*)pPlayerData + 0x48);
    return result;
}
