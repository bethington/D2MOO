#pragma once

#include "D2CommonDefinitions.h"
#include "D2DrlgDrlg.h"
#include "D2DrlgDrlgGrid.h"

#pragma pack(1)

struct UnkDrlgLogic
{
	Room2* pDrlgRoom;				//0x00
	DrlgGrid* field_4;				//0x04
	DrlgGrid* pTileTypeGrid;			//0x08
	DrlgGrid* pWallGrid;				//0x0C
	DrlgGrid* pFloorGrid;				//0x10
	DrlgGrid* field_14;				//0x14
	int32_t field_18;						//0x18
	int32_t nFlags;							//0x1C
};

enum DrlgLogicalRoomInfoFlags
{
	DRLGLOGIC_ROOMINFO_HAS_COORD_LIST = 0x1,
	DRLGLOGIC_ROOMINFO_HAS_GRID_CELLS = 0x2,
};

struct DrlgLogicalRoomInfo // aka D2DrlgCoordListStrc
{
	int32_t dwFlags;						//0x00 DrlgLogicalRoomInfoFlags
	int32_t nLists;							//0x04
	DrlgGrid pIndexX;					//0x08
	DrlgGrid pIndexY;					//0x1C
	RoomCoordList* pCoordList;		//0x30
};

#pragma pack()

//D2Common.0x6FD76420
void __fastcall DRLGLOGIC_FreeDrlgCoordList(Room2* pDrlgRoom);
//D2Common.0x6FD764A0
//TODO: v28, a1
void __fastcall DRLGLOGIC_InitializeDrlgCoordList(Room2* pDrlgRoom, DrlgGrid* pTileTypeGrid, DrlgGrid* pFloorGrid, DrlgGrid* pWallGrid);
//D2Common.0x6FD76830
void __fastcall DRLGLOGIC_SetTileGridFlags(UnkDrlgLogic* a1, int nX, int nY, int a4);
//D2Common.0x6FD769B0
void __fastcall sub_6FD769B0(Room2* pDrlgRoom);
//D2Common.0x6FD76A90
void __fastcall sub_6FD76A90(Room2* pDrlgRoom1, Room2* pDrlgRoom2, int nX, int nY);
//D2Common.0x6FD76B90
void __fastcall sub_6FD76B90(Room2* pDrlgRoom, int nIndex1, int nIndex2, BOOL bNode);
//D2Common.0x6FD76C20
BOOL __fastcall DRLG_CheckLayer1ButNotWallObject(DrlgTileData* pTileData);
//D2Common.0x6FD76C50
void __fastcall DRLGLOGIC_SetCoordListForTiles(Room2* pDrlgRoom);
//D2Common.0x6FD76CF0
void __fastcall DRLGLOGIC_AssignCoordListsForGrids(Room2* pDrlgRoom, DrlgLogicalRoomInfo* pDrlgCoordList, int nLists);
//D2Common.0x6FD76F90
void __fastcall DRLGLOGIC_AllocCoordLists(Room2* pDrlgRoom);
//D2Common.0x6FD77080
int __fastcall DRLGLOGIC_GetRoomCoordListIndex(Room2* pDrlgRoom, int nX, int nY);
//D2Common.0x6FD77110
RoomCoordList* __fastcall sub_6FD77110(Room2* pDrlgRoom, int nX, int nY);
//D2Common.0x6FD77190
RoomCoordList* __fastcall DRLGLOGIC_GetRoomCoordList(Room2* pDrlgRoom);
