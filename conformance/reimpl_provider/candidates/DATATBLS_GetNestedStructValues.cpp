// D2MOO_REIMPL_EXPORT: DATATBLS_GetNestedStructValues
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall DATATBLS_GetNestedStructValues(void* pCtx, uint32_t* pdwOutVal) {
    if ((pCtx != (void*)0x0) && (*(int*)((int)pCtx + 8) != 0)) {
        *pdwOutVal = *(uint32_t*)(*(int*)((int)pCtx + 8) + 0xC);
        return *(uint32_t*)(*(int*)((int)pCtx + 8) + 8);
    }
    *pdwOutVal = 0;
    return 0;
}
