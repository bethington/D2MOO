// D2MOO_REIMPL_EXPORT: GetStatListOwnerType
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall GetStatListOwnerType(void* pStatList)
{
    return *(uint32_t*)((char*)pStatList + 0x4);
}
