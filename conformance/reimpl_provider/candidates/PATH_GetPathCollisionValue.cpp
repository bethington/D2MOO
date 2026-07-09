#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetPathCollisionValue

extern "C" unsigned int __stdcall PATH_GetPathCollisionValue(void* pPath) {
    if (pPath == nullptr) {
        return 0xFFFF;
    }
    return *(unsigned int*)((char*)pPath + 0x4C);
}
