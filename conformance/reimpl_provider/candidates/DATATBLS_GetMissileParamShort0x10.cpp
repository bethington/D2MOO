// D2MOO_REIMPL_EXPORT: DATATBLS_GetMissileParamShort0x10
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall DATATBLS_GetMissileParamShort0x10(void* p) {
    return *(const uint16_t*)((const uint8_t*)p + 0x10);
}
