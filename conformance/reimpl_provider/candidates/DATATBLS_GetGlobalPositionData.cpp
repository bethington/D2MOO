// D2MOO_REIMPL_EXPORT: DATATBLS_GetGlobalPositionData
#include "../provider_runtime.h"
#include <stdlib.h>

extern "C" uint32_t __stdcall DATATBLS_GetGlobalPositionData(void* pStruct)
{
    if (pStruct == (void*)0)
    {
        // Inlined abort sequence (matches original CleanupAndAbort + _exit(-1)).
        _exit(-1);
    }
    return *(uint32_t*)((int)pStruct + 0x18);
}
