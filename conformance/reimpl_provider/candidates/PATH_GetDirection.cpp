// D2MOO_REIMPL_EXPORT: PATH_GetDirection
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall PATH_GetDirection(uint32_t pPathData)
{
    uint8_t bField65 = *(uint8_t *)(pPathData + 0x65u);
    return (pPathData & 0xFFFFFF00u) | (uint32_t)bField65;
}
