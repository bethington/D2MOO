#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ReadUshortPairAdvance
extern "C" uint32_t __stdcall ReadUshortPairAdvance(void* pData)
{
    if (pData == nullptr) return 0;
    *(int*)pData = *(int*)pData + 4;
    return *(uint16_t*)(*(int*)pData - 4);
}
