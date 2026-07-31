#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ROOM_CheckTileAccessBoundary

extern "C" int __cdecl ROOM_ProcessEncodedTileBuffer(uint8_t* pTileData, uint32_t* pAccessData);

extern "C" int __cdecl ROOM_CheckTileAccessBoundary(
    uint8_t* pTileData,
    uint32_t* pTileAccessData,
    uint32_t* pRoomCtx,
    int nCol,
    int nRow)
{
    uint32_t dwField18 = *(uint32_t*)((char*)pTileAccessData + 0x18);

    if (dwField18 != 0) {
        int nField78 = *(int*)((char*)pRoomCtx + 0x78);
        int nField7c = *(int*)((char*)pRoomCtx + 0x7c);
        uint32_t dwTileIndex = (uint32_t)(nField78 * nRow + 1 + nCol);

        if ((dwTileIndex % dwField18 == 0) &&
            (dwTileIndex < (uint32_t)(nField7c * nField78))) {

            *(uint32_t*)((char*)pTileAccessData + 0x24) = 0;
            *(int*)((char*)pTileAccessData + 0x28) = 0;
            *(uint32_t*)((char*)pTileAccessData + 0x2c) = 0;
            *(int*)((char*)pTileAccessData + 0x30) = 0;

            uint8_t bTileFlags = pTileData[0x30];
            pTileData[0xc] = 0;
            pTileData[0xd] = 0;
            pTileData[0xe] = 0;
            pTileData[0xf] = 0;

            if (bTileFlags != 0) {
                pTileData[0x30] = 0;
                return 0;
            }

            int nResult = ROOM_ProcessEncodedTileBuffer(pTileData, (uint32_t*)&pTileAccessData);
            if (nResult != 0) {
                return nResult;
            }
        }
    }
    return 0;
}
