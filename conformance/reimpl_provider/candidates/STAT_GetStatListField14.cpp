#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STAT_GetStatListField14
extern "C" void* __stdcall STAT_GetStatListField14(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0;
    }
    return *(void**)((char*)pUnit + 0x14);
}
