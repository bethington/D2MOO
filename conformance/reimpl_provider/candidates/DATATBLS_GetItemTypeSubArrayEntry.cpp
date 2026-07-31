#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeSubArrayEntry
extern "C" void* __stdcall DATATBLS_GetItemTypeSubArrayEntry(void* pUnit) {
    if (pUnit == nullptr) return 0;
    return *(void**)((char*)pUnit + 0xc);
}
