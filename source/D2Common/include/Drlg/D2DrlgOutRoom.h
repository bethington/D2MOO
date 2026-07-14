#pragma once

#include "D2CommonDefinitions.h"
#include "D2DrlgDrlgGrid.h"

struct Room2;
struct DrlgLevelLinkData;

#pragma pack(1)


struct DrlgOutdoorRoom
{
	DrlgGrid pTileTypeGrid;			//0x00 aka pOrientationGrid
	DrlgGrid pWallGrid;				//0x14
	DrlgGrid pFloorGrid;				//0x28
	DrlgGrid pDirtPathGrid;			//0x3C
	DrlgVertex* pVertex;				//0x50
	int32_t dwFlags;						//0x54
	int32_t dwFlagsEx;						//0x58
	int32_t unk0x5C;						//0x5C
	int32_t unk0x60;						//0x60
	int32_t nSubType;						//0x64
	int32_t nSubTheme;						//0x68
	int32_t nSubThemePicked;				//0x6C
};

#pragma pack()


//D2Common.0x6FD83D20
void __fastcall DRLGOUTROOM_FreeDrlgOutdoorRoom(Room2* pDrlgRoom);
//D2Common.0x6FD83D90
void __fastcall DRLGOUTROOM_FreeDrlgOutdoorRoomData(Room2* pDrlgRoom);
//D2Common.0x6FD83DE0
void __fastcall DRLGOUTROOM_AllocDrlgOutdoorRoom(Room2* pDrlgRoom);
//D2Common.6FD83E20
void __fastcall DRLGOUTROOM_InitializeDrlgOutdoorRoom(Room2* pDrlgRoom);
//D2Common.0x6FD83EC0
BOOL __fastcall DRLGOUTROOM_LinkLevelsByLevelCoords(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD83F70
BOOL __fastcall DRLGOUTROOM_LinkLevelsByLevelDef(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD84010
BOOL __fastcall DRLGOUTROOM_LinkLevelsByOffsetCoords(DrlgLevelLinkData* pLevelLinkData);
