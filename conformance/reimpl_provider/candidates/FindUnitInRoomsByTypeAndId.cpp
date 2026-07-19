#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: FindUnitInRoomsByTypeAndId

extern "C" void* __stdcall FindUnitInRoomsByTypeAndId(void* pItem)
{
    if (pItem == nullptr) return 0;
    if (*(int*)((char*)pItem + 0x0) != 4) return 0;
    void* pItemData = *(void**)((char*)pItem + 0x14);
    if (pItemData == nullptr) return 0;
    return (void*)((char*)pItemData + 0x4A);
}
