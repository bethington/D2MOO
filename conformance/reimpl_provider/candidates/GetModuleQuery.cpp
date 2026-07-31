#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetModuleQuery
extern "C" uint32_t __stdcall GetModuleQuery(void* hModule, uint32_t dwMode)
{
    uint32_t dwResult = 0;
    switch (dwMode) {
        case 0:
        case 2:
        case 3:
            dwResult = 1;
            break;
        case 1:
            // ROOM_InitializeTileSystem(hModule) -- not defined in provider
            // return extraout_EAX;  // decompiler phantom: return value of ROOM_InitializeTileSystem
            return 0;
    }
    return dwResult;
}
