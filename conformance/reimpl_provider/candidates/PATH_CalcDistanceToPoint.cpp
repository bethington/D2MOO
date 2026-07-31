#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: PATH_CalcDistanceToPoint

extern "C" int __stdcall PATH_CalcDistanceToPoint(void* pUnit, int nTargetX, int nTargetY) {
    if (pUnit == nullptr) return 0;

    uint32_t dwUnitType = *(uint32_t*)pUnit;
    void* pPathNode;
    uint32_t dwUnitPosX;
    uint32_t dwUnitPosY;
    int dwDx;
    int dwDeltaY;

    if ((dwUnitType == 2u) || ((3 < (int)dwUnitType && ((int)dwUnitType < 6)))) {
        pPathNode = *(void**)((char*)pUnit + 0x2c);
        dwUnitPosX = *(uint32_t*)((char*)pPathNode + 0xc);
    } else {
        pPathNode = *(void**)((char*)pUnit + 0x2c);
        if (pPathNode == nullptr) {
            dwUnitPosX = 0;
        } else {
            dwUnitPosX = (uint32_t)*(uint16_t*)((char*)pPathNode + 0x2);
        }
    }

    dwDx = nTargetX - (int)dwUnitPosX;
    if (dwDx < 0) {
        dwDx = -dwDx;
    }

    if ((dwUnitType == 2u) || ((3 < (int)dwUnitType && ((int)dwUnitType < 6)))) {
        dwUnitPosY = *(uint32_t*)((char*)pPathNode + 0x10);
    } else if (pPathNode == nullptr) {
        dwUnitPosY = 0;
    } else {
        dwUnitPosY = (uint32_t)*(uint16_t*)((char*)pPathNode + 0x6);
    }

    dwDeltaY = nTargetY - (int)dwUnitPosY;
    if (dwDeltaY < 0) {
        dwDeltaY = -dwDeltaY;
    }

    if (dwDx < 0) {
        dwDx = 0;
    }
    if (dwDeltaY < 0) {
        dwDeltaY = 0;
    }

    if (dwDeltaY < dwDx) {
        return (dwDeltaY + dwDx * 2) / 2;
    }
    return (dwDx + dwDeltaY * 2) / 2;
}
