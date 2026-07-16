// D2MOO_REIMPL_EXPORT: UNIT_GetStructShort0x12
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall UNIT_GetStructShort0x12(void* p)
{
    return *(uint16_t*)((uint8_t*)p + 0x12);
}
