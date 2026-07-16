// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataField32
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall ITEMS_GetItemDataField32(void* p) {
    if (p == NULL) {
        return 0;
    }
    if (*(uint32_t*)p != 0x4) {
        return 0;
    }
    void* r = *(void**)((uintptr_t)p + 0x14);
    if (r == NULL) {
        return 0;
    }
    return *(uint16_t*)((uintptr_t)r + 0x32);
}
