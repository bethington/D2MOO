#pragma once

#include "D2CommonDefinitions.h"
#include <Drlg/D2DrlgDrlg.h>
// TODO: Reimport from .cpp

#pragma pack(1)

struct BoundingBox
{
	int32_t nLeft;								//0x00
	int32_t nBottom;							//0x04
	int32_t nRight;								//0x08
	int32_t nTop;								//0x0C
};

// Size of the unit in subtiles
enum CollisionUnitSize
{
	COLLISION_UNIT_SIZE_NONE  = 0,
	COLLISION_UNIT_SIZE_POINT = 1, // Occupies 1 subtile in width
	COLLISION_UNIT_SIZE_SMALL = 2, // Occupies 2 subtiles in width
	COLLISION_UNIT_SIZE_BIG   = 3, // Occupies 3 subtiles in width
	COLLISION_UNIT_SIZE_COUNT
};

enum CollisionPattern
{
	COLLISION_PATTERN_NONE = 0,
	COLLISION_PATTERN_SMALL_UNIT_PRESENCE = 1,
	COLLISION_PATTERN_BIG_UNIT_PRESENCE = 2,
	// Actually linked to whether a monster may be attacked?
	COLLISION_PATTERN_SMALL_PET_PRESENCE  = 3,
	COLLISION_PATTERN_BIG_PET_PRESENCE    = 4,
	COLLISION_PATTERN_SMALL_NO_PRESENCE   = 5,
};

enum CollisionMaskFlags : uint16_t
{
	COLLIDE_NONE = 0x0000,
	COLLIDE_WALL = 0x0001,					// 'black space' in arcane sanctuary, cliff walls etc. Effectively blocks player.
	COLLIDE_VISIBLE = 0x0002,				// tile based obstacles you can't shoot over
	COLLIDE_MISSILE_BARRIER = 0x0004,		// again used inconsistantly -.- Can guard against Missile / Flying ?
	COLLIDE_NOPLAYER = 0x0008,
	COLLIDE_PRESET = 0x0010,				// some floors have this set, others don't
	COLLIDE_BLANK = 0x0020,					// returned if the subtile is invalid
	COLLIDE_MISSILE = 0x0040,
	COLLIDE_PLAYER = 0x0080,
	COLLIDE_WATER = 0x00C0,
	COLLIDE_MONSTER = 0x0100,
	COLLIDE_ITEM = 0x0200,
	COLLIDE_OBJECT = 0x0400,
	COLLIDE_DOOR = 0x0800,
	COLLIDE_NO_PATH = 0x1000,				// set for units sometimes, but not always
	COLLIDE_PET = 0x2000,					// linked to whether a monster that may be attacked is present
	COLLIDE_4000 = 0x4000,
	COLLIDE_CORPSE = 0x8000,				// also used by portals, but dead monsters are mask 0x8000
	COLLIDE_ALL_MASK = 0xFFFF,
	COLLIDE_MASK_INVALID = (COLLIDE_BLANK | COLLIDE_MISSILE_BARRIER | COLLIDE_VISIBLE | COLLIDE_WALL),
		
	COLLIDE_MASK_PLAYER_PATH = COLLIDE_WALL | COLLIDE_NOPLAYER | COLLIDE_OBJECT | COLLIDE_DOOR | COLLIDE_NO_PATH,
	COLLIDE_MASK_PLAYER_FLYING = COLLIDE_DOOR | COLLIDE_MISSILE_BARRIER,
	COLLIDE_MASK_PLAYER_WW = COLLIDE_WALL | COLLIDE_OBJECT | COLLIDE_DOOR,
	COLLIDE_MASK_RADIAL_BARRIER = COLLIDE_DOOR | COLLIDE_MISSILE_BARRIER | COLLIDE_WALL,

	COLLIDE_MASK_FLYING_UNIT = COLLIDE_MISSILE_BARRIER | COLLIDE_DOOR | COLLIDE_NO_PATH,
	COLLIDE_MASK_MONSTER_THAT_CAN_OPEN_DOORS = COLLIDE_WALL | COLLIDE_OBJECT | COLLIDE_NO_PATH | COLLIDE_PET,

	COLLIDE_MASK_MONSTER_MISSILE = COLLIDE_MONSTER | COLLIDE_WALL,
	COLLIDE_MASK_MONSTER_PATH = COLLIDE_MASK_MONSTER_THAT_CAN_OPEN_DOORS | COLLIDE_DOOR,
	COLLIDE_MASK_DOOR_BLOCK_VIS = COLLIDE_DOOR | COLLIDE_MISSILE_BARRIER | COLLIDE_VISIBLE,
	
	COLLIDE_MASK_BLOCKS_DOOR = COLLIDE_PLAYER | COLLIDE_MONSTER | COLLIDE_CORPSE,
	COLLIDE_MASK_SPAWN = COLLIDE_WALL | COLLIDE_ITEM | COLLIDE_OBJECT | COLLIDE_DOOR | COLLIDE_NO_PATH | COLLIDE_PET,
	COLLIDE_MASK_PLACEMENT = COLLIDE_MASK_SPAWN | COLLIDE_PRESET | COLLIDE_MONSTER,

};

struct RoomCollisionGrid
{
	DrlgCoords pRoomCoords;			//0x00
	uint16_t* pCollisionMask;					//0x20
};
#pragma pack()

//D2Common.0x6FD41000
void __fastcall D2Common_COLLISION_FirstFn_6FD41000(Room1* pRoom, DrlgTileData* pTileData, TileLibraryEntry* pTileLibraryEntry);
//D2Common.0x6FD411F0
void __fastcall sub_6FD411F0(RoomCollisionGrid* pCollisionGrid, TileLibraryEntry* pTileLibraryEntry, int nX, int nY);
//D2Common.0x6FD412B0 (#10018)
D2COMMON_DLL_DECL int __stdcall D2COMMON_10018_Return0();
//D2Common.0x6FD412C0
void __fastcall COLLISION_AllocRoomCollisionGrid(void* pMemPool, Room1* pRoom);
//D2Common.0x6FD413E0
void __fastcall sub_6FD413E0(RoomCollisionGrid* pCollisionGrid, RoomCollisionGrid* pAdjacentCollisionGrid, DrlgTileData* pTiles, int nTiles, BOOL bRemoveOldFlags);
//D2Common.0x6FD41610
void __fastcall COLLISION_FreeRoomCollisionGrid(void* pMemPool, Room1* pRoom);
//D2Common.0x6FD41650 (#10118)
D2COMMON_DLL_DECL uint16_t __stdcall COLLISION_CheckMask(Room1* pRoom, int nX, int nY, uint16_t nMask);
//D2Common.0x6FD41720 (#10127)
D2COMMON_DLL_DECL void __stdcall COLLISION_SetMask(Room1* pRoom, int nX, int nY, uint16_t nMask);
//D2Common.0x6FD417F0 (#10123)
D2COMMON_DLL_DECL void __stdcall COLLISION_ResetMask(Room1* pRoom, int nX, int nY, uint16_t nMask);
//D2Common.0x6FD418C0 (#10120)
D2COMMON_DLL_DECL uint16_t __stdcall COLLISION_CheckMaskWithSizeXY(Room1* pRoom, int nX, int nY, unsigned int nSizeX, unsigned int nSizeY, uint16_t nMask);
//D2Common.0x6FD41B40
uint16_t __fastcall COLLISION_CheckCollisionMaskForBoundingBox(RoomCollisionGrid* pCollisionGrid, BoundingBox* pBoundingBox, uint16_t nMask);
//D2Common.0x6FD41BE0
int __fastcall COLLISION_AdaptBoundingBoxToGrid(Room1* pRoom, BoundingBox* pBoundingBox, BoundingBox* pBoundingBoxes);
//D2Common.0x6FD41CA0
uint16_t __fastcall COLLISION_CheckCollisionMaskForBoundingBoxRecursively(Room1* pRoom, BoundingBox* pBoundingBox, uint16_t nMask);
//D2Common.0x6FD41DE0 (#10121)
D2COMMON_DLL_DECL uint16_t __stdcall COLLISION_CheckMaskWithPattern(Room1* pRoom, int nX, int nY, int nCollisionPattern, uint16_t nMask);
//D2Common.0x6FD42000
uint16_t __fastcall COLLISION_CheckCollisionMaskWithAdjacentCells(Room1* pRoom, int nX, int nY, uint16_t nMask);
//D2Common.0x6FD42670
uint16_t __fastcall COLLISION_CheckCollisionMask(Room1* pRoom, int nX, int nY, uint16_t nMask);
//D2Common.0x6FD42740 (#10122)
D2COMMON_DLL_DECL int __stdcall COLLISION_CheckAnyCollisionWithPattern(Room1* pRoom, int nX, int nY, int nCollisionPattern, uint16_t nMask);
//D2Common.0x6FD428D0
// Faster than COLLISION_CheckCollisionMaskForBoundingBoxRecursively since can stop as soon as a collision is found.
BOOL __fastcall COLLISION_CheckAnyCollisionForBoundingBoxRecursively(Room1* pRoom, BoundingBox* pBoundingBox, uint16_t nMask);
//D2Common.0x6FD42A30
BOOL __fastcall COLLISION_CheckAnyCollisionWithAdjacentCells(Room1* pRoom, int nX, int nY, uint16_t nMask);
//D2Common.0x6FD43080 (#10119)
D2COMMON_DLL_DECL uint16_t __stdcall COLLISION_CheckMaskWithSize(Room1* pRoom, int nX, int nY, int nUnitSize, uint16_t nMask);
//D2Common.0x6FD432A0 (#10128)
D2COMMON_DLL_DECL void __stdcall COLLISION_SetMaskWithSize(Room1* pRoom, int nX, int nY, int nUnitSize, uint16_t nMask);
//D2Common.0x6FD434B0
void __fastcall COLLISION_SetCollisionMask(Room1* pRoom, int nX, int nY, uint16_t nMask);
//D2Common.0x6FD43580
void __fastcall COLLISION_SetCollisionMaskForBoundingBoxRecursively(Room1* pRoom, BoundingBox* pBoundingBox, uint16_t nMask);
//D2Common.0x6FD436F0 (#10130)
D2COMMON_DLL_DECL void __stdcall COLLISION_SetMaskWithPattern(Room1* pRoom, int nX, int nY, int nCollisionPattern, uint16_t nMask);
//D2Common.0x6FD439D0 (#10124)
D2COMMON_DLL_DECL void __stdcall COLLISION_ResetMaskWithSize(Room1* pRoom, int nX, int nY, int nUnitSize, uint16_t nMask);
//D2Common.0x6FD43C10
void __fastcall COLLISION_ResetCollisionMask(Room1* pRoom, int nX, int nY, uint16_t nMask);
//D2Common.0x6FD43CE0
void __fastcall COLLISION_ResetCollisionMaskForBoundingBoxRecursively(Room1* pRoom, BoundingBox* pBoundingBox, uint16_t nMask);
//D2Common.0x6FD43E60 (#10126)
D2COMMON_DLL_DECL void __stdcall COLLISION_ResetMaskWithPattern(Room1* pRoom, int nX, int nY, int nCollisionPattern, uint16_t nMask);
//D2Common.0x6FD44140 (#10125)
D2COMMON_DLL_DECL void __stdcall COLLISION_ResetMaskWithSizeXY(Room1* pRoom, int nX, int nY, unsigned int nSizeX, unsigned int nSizeY, uint16_t nMask);
//D2Common.0x6FD44370
void __fastcall COLLISION_ResetCollisionMaskForBoundingBox(RoomCollisionGrid* pCollisionGrid, BoundingBox* pBoundingBox, uint16_t nMask);
//D2Common.0x6FD443E0 (#10129)
D2COMMON_DLL_DECL void __stdcall COLLISION_SetMaskWithSizeXY(Room1* pRoom, int nX, int nY, unsigned int nSizeX, unsigned int nSizeY, uint16_t nMask);
//D2Common.0x6FD44600
void __fastcall COLLISION_SetCollisionMaskForBoundingBox(RoomCollisionGrid* pCollisionGrid, BoundingBox* pBoundingBox, uint16_t nMask);
//D2Common.0x6FD44660 (#10131)
D2COMMON_DLL_DECL uint16_t __fastcall COLLISION_TryMoveUnitCollisionMask(Room1* pRoom, int nX1, int nY1, int nX2, int nY2, int nUnitSize, uint16_t nCollisionMask, uint16_t nMoveConditionMask);
//D2Common.0x6FD44910
void __fastcall COLLISION_CreateBoundingBox(BoundingBox* pBoundingBox, int nCenterX, int nCenterY, unsigned int nSizeX, unsigned int nSizeY);
//D2Common.0x6FD44950 (#10132)
D2COMMON_DLL_DECL uint16_t __fastcall COLLISION_TryTeleportUnitCollisionMask(Room1* pRoom, int nX1, int nY1, int nX2, int nY2, int nCollisionPattern, uint16_t nFootprintCollisionMask, uint16_t nMoveConditionMask);
//D2Common.0x6FD44BB0
uint16_t __fastcall COLLISION_ForceTeleportUnitCollisionMaskAndGetCollision(Room1* pRoom1, int nX1, int nY1, Room1* pRoom2, int nX2, int nY2, int nUnitSize, uint16_t nFootprintCollisionMask, uint16_t nMoveConditionMask);
//D2Common.0x6FD44E00
uint16_t __fastcall COLLISION_TeleportUnitCollisionMask(Room1* pRoom1, int nX1, int nY1, Room1* pRoom2, int nX2, int nY2, int nUnitSize, uint16_t nMask);
//D2Common.0x6FD44FF0
int __fastcall COLLISION_TrySetUnitCollisionMask(Room1* pRoom1, int nX1, int nY1, Room1* pRoom2, int nX2, int nY2, int nCollisionPattern, uint16_t nFootprintCollisionMask, uint16_t nMoveConditionMask);
//D2Common.0x6FD451D0 (#10133)
D2COMMON_DLL_DECL void __fastcall COLLISION_SetUnitCollisionMask(Room1* pRoom1, int nX1, int nY1, Room1* pRoom2, int nX2, int nY2, int nCollisionPattern, uint16_t nCollisionMask);
//D2Common.0x6FD45210 (#11263)
//Returns true if a collision with mask was found. pEndCoord will be set to the collision location.
D2COMMON_DLL_DECL BOOL __stdcall COLLISION_RayTrace(Room1* pRoom, Coord* pBeginCoord, Coord* pEndCoord, uint16_t nCollisionMask);
//D2Common.0x6FD459D0 (#10135)
D2COMMON_DLL_DECL Room1* __stdcall COLLISION_GetFreeCoordinatesWithMaxDistance(Room1* pRoom, Coord* pSpawnPoint, int nUnitSize, unsigned int nMask, BOOL bAllowNeighborRooms, int nMaxDistance);
//D2Common.0x6FD45A00
Room1* __fastcall COLLISION_GetFreeCoordinatesImpl(Room1* pRoom, Coord* ptSpawnPoint, Coord* pFieldCoord, int nUnitSize, unsigned int nMask, unsigned int nFieldMask, BOOL bAllowNeighborRooms, int nMaxDistance, int nPosIncrementValue);
//D2Common.0x6FD46280 (#10134)
D2COMMON_DLL_DECL Room1* __stdcall COLLISION_GetFreeCoordinates(Room1* pRoom, Coord* pSpawnPoint, int nUnitSize, unsigned int nMask, BOOL bAllowNeighborRooms);
//D2Common.0x6FD462B0 (#10137)
D2COMMON_DLL_DECL Room1* __stdcall COLLISION_GetFreeCoordinatesEx(Room1* pRoom, Coord* pSpawnPoint, int nUnitSize, unsigned int nMask, int nPosIncrementValue);
//D2Common.0x6FD462E0 (#10138)
D2COMMON_DLL_DECL Room1* __stdcall COLLISION_GetFreeCoordinatesWithField(Room1* pRoom, Coord* pSpawnPoint, Coord* pFieldCoord, int nUnitSize, unsigned int nMask, unsigned int nFieldMask, BOOL bAllowNeighborRooms);
//D2Common.0x6FD46310 (#10136)
D2COMMON_DLL_DECL void __fastcall D2Common_10136(Room1* pRoom, Coord* pCoord, int a3, uint16_t nMask, Room1** ppRoom);
//D2Common.0x6FD46620
Room1* __fastcall COLLISION_GetRoomBySubTileCoordinates(Room1* pRoom, int nX, int nY);
