// D2MOO_REIMPL_EXPORT: GetPathFlagBit3
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall GetPathFlagBit3(void* pPath)
{
    if (pPath == (void*)0x0) {
        return 0;
    }
    return *(uint32_t*)((uint8_t*)pPath + 0x34) & 8;
}
