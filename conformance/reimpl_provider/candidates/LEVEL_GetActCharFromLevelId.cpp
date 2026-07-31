#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: LEVEL_GetActCharFromLevelId

extern "C" int __fastcall LEVEL_GetActCharFromLevelId(void* pLevel)
{
    if (pLevel == nullptr) return 0;
    int nExitVariant = *(int*)((char*)pLevel + 0x1d0);
    return (nExitVariant == 2) + 0x33;
}
