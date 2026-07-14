#pragma once

#include "D2CommonDefinitions.h"

struct DrlgOrth;

#pragma pack(1)

struct DrlgCoord
{
	int32_t nPosX;								//0x00
	int32_t nPosY;								//0x04
	int32_t nWidth;								//0x08
	int32_t nHeight;							//0x0C
};

struct DrlgVertex
{
	int32_t nPosX;							//0x00
	int32_t nPosY;							//0x04
	uint8_t nDirection;						//0x08
	uint8_t pad0x09[3];						//0x09
	int32_t dwFlags;						//0x0C
	DrlgVertex* pNext;				//0x10
};

#pragma pack()

//D2Common.0x6FD782A0
DrlgVertex* __fastcall DRLGVER_AllocVertex(void* pMemPool, uint8_t nDirection);
//D2Common.0x6FD782D0
void __fastcall DRLGVER_CreateVertices(void* pMemPool, DrlgVertex** ppVertices, DrlgCoord* pDrlgCoord, uint8_t nDirection, DrlgOrth* pDrlgRoomData);
//D2Common.0x6FD786C0
void __fastcall DRLGVER_FreeVertices(void* pMemPool, DrlgVertex** ppVertices);
//D2Common.0x6FD78730
void __fastcall DRLGVER_GetCoordDiff(DrlgVertex* pDrlgVertex, int* pDiffX, int* pDiffY);
