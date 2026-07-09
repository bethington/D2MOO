#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataByte45

extern "C" unsigned char __stdcall ITEMS_GetItemDataByte45(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0xFF;
    }
    if (*(unsigned int*)pUnit != 4) {
        return 0xFF;
    }
    void* pPlayerData = *(void**)((char*)pUnit + 0x14);
    if (pPlayerData == nullptr) {
        return 0xFF;
    }
    return *(unsigned char*)((char*)pPlayerData + 0x45);
}
