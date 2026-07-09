#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetDrlgRoom

extern "C" void* __stdcall PATH_GetDrlgRoom(void* pUnit) {
    if (pUnit == nullptr) {
        return nullptr;
    }
    return (void*)*(unsigned int*)((char*)pUnit + 0x58);
}
