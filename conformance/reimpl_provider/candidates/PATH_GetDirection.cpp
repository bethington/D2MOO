// D2MOO_REIMPL_EXPORT: PATH_GetDirection
#include "../provider_runtime.h"

// CONCAT31 byte-getter: the decompile returns bField65 in AL with the upper 3
// bytes left as pointer-derived garbage (CONCAT31 artifact). Only the low byte is
// meaningful (direction 0-63); the oracle masks ret to the low 8 bits (ret:u8), so
// return JUST the byte. (The batch draft oscillated u8<->u32 and the provider timed
// out before converging on this; pinned here.)
extern "C" unsigned int __stdcall PATH_GetDirection(void* pPathData)
{
    if (pPathData == nullptr) return 0;
    return *(unsigned char*)((char*)pPathData + 0x65);
}
