#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeDataByte49
extern "C" uint8_t __stdcall DATATBLS_GetItemTypeDataByte49(void* pItem) {
    if (pItem == nullptr) return 0;

    int dwType = *(int*)((char*)pItem + 0x00);
    if (dwType != 4) return 0;

    void* pTypeData = *(void**)((char*)pItem + 0x14);
    if (pTypeData == nullptr) return 0;

    return *(uint8_t*)((char*)pTypeData + 0x49);
}
