#pragma once

#include "D2CommonDefinitions.h"
#include "D2DrlgDrlg.h"
#include "D2DrlgDrlgGrid.h"

#pragma pack(1)


#pragma pack()

//D2Common.0x6FD75480
void __fastcall DRLGANIM_InitCache(ActMisc* pDrlg, DrlgTileData* pTileData);
//D2Common.0x6FD75560
void __fastcall DRLGANIM_TestLoadAnimatedRoomTiles(Room2* pDrlgRoom, DrlgGrid* pDrlgGrid, DrlgGrid* pTileTypeGrid, int nTileType, int nTileX, int nTileY);
//D2Common.0x6FD756B0
void __fastcall DRLGANIM_AnimateTiles(Room2* pDrlgRoom);
//D2Common.0x6FD75740
void __fastcall DRLGANIM_AllocAnimationTileGrids(Room2* pDrlgRoom, int nAnimationSpeed, DrlgGrid* pWallGrid, int nWalls, DrlgGrid* pFloorGrid, int nFloors, DrlgGrid* pShadowGrid);
//D2Common.0x6FD757B0
void __fastcall DRLGANIM_AllocAnimationTileGrid(Room2* pDrlgRoom, int nAnimationSpeed, DrlgTileData* pTiles, int nTiles, DrlgGrid* pDrlgGrid, int nUnused);
//D2Common.0x6FD75B00
void __fastcall DRLGANIM_UpdateFrameInAdjacentRooms(Room2* pDrlgRoom1, Room2* pDrlgRoom2);
