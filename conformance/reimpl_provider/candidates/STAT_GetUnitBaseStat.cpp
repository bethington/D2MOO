// D2MOO_REIMPL_EXPORT: STAT_GetUnitBaseStat
#include "../provider_runtime.h"

extern "C" void __cdecl _exit(int _Code);

extern "C" uint32_t __stdcall STAT_GetUnitBaseStat(void* pData)
{
    if (pData == (void*)0x0)
    {
        _exit(-1);
    }
    return *(uint32_t*)((uintptr_t)pData + 0x48);
}
