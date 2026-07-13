#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: UNITS_GetUnitLevel

extern "C" uint32_t __fastcall UNITS_GetUnitLevel(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    uint32_t dwType = *(uint32_t*)pUnit;
    void* pPath = *(void**)((char*)pUnit + 0x2c);

    if (dwType == 2 || (dwType >= 4 && dwType <= 5)) {
        // Object(2) / Item(4) / Warp(5) path: 32-bit read at pPath + 0xc
        return *(uint32_t*)((char*)pPath + 0xc);
    }

    // Player(0) / Monster(1) / Missile(3) / other (>5) path:
    // pPath NULL => return 0; else zero-extended 16-bit read at pPath + 0x2
    if (pPath == nullptr) return 0;
    return (uint32_t)*(uint16_t*)((char*)pPath + 0x2);
}
