// D2MOO_REIMPL_EXPORT: DATATBLS_GetIndexedShort0x9A
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall DATATBLS_GetIndexedShort0x9A(void* pStruct) {
    uint8_t* p = (uint8_t*)pStruct;
    uint32_t count = *(uint32_t*)(p + 0x28);
    if (count != 0) {
        return *(uint16_t*)(p + 0x9A + count * 4u);
    }
    return 0;
}
