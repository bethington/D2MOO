#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DRLG_GetItemDataByte46
extern "C" uint8_t __stdcall DRLG_GetItemDataByte46(void* pRoom)
{
    if (pRoom == nullptr) return 0;
    if (*(int*)pRoom != 4) return (uint8_t)0xFF;
    void* pPrevRoom = *(void**)((char*)pRoom + 0x14);
    if (pPrevRoom == nullptr) return (uint8_t)0xFF;
    return *(uint8_t*)((char*)pPrevRoom + 0x46);
}
