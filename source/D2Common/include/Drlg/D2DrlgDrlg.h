#pragma once

#include "D2CommonDefinitions.h"
#include <D2Seed.h>
#include "D2DrlgDrlgVer.h"
#include <Archive.h>

#pragma pack(1)
struct TileLibraryEntry; // From D2CMP
struct TileLibraryHash; // From D2CMP
struct Game; // From D2Game
struct GameClient; // From D2Game
struct Level;
struct DrlgPresetRoom;
struct DrlgOutdoorRoom;
struct DrlgTileGrid;
struct DrlgTileData;
struct RoomTile;
struct DrlgLogicalRoomInfo;
struct Room1;
struct RoomTileList;
struct Act;
struct DrlgWarp;
struct DrlgEnvironment;
struct DrlgPresetInfo;
struct DrlgOutdoorInfo;
struct DrlgMap;
struct PresetUnit;
struct RoomCollisionGrid;
struct LvlWarpTxt;
struct LvlMazeTxt;
struct UnitAny;

enum DrlgFlags
{
	DRLGFLAG_ONCLIENT = 0x01,
	DRLGFLAG_REFRESH  = 0x10,
};

enum DrlgLimits {
	DRLG_MAX_WALL_LAYERS = 4,
	DRLG_MAX_FLOOR_LAYERS = 2,
};

enum Directions
{
	DIRECTION_INVALID = -1,
	DIRECTION_SOUTHWEST = 0,
	DIRECTION_NORTHWEST = 1,
	DIRECTION_SOUTHEAST = 2,
	DIRECTION_NORTHEAST = 3,
	DIRECTION_COUNT = 4,
};

enum AltDirections
{
	ALTDIR_WEST,
	ALTDIR_NORTH,
	ALTDIR_EAST,
	ALTDIR_SOUTH,
	ALTDIR_NORTHWEST,
	ALTDIR_NORTHEAST,
	ALTDIR_SOUTHEAST,
	ALTDIR_SOUTHWEST,
};

enum DrlgTypes
{
	DRLGTYPE_MAZE = 0x01,
	DRLGTYPE_PRESET = 0x02,
	DRLGTYPE_OUTDOOR = 0x03,
	NUM_DRLGTYPES
};

enum DrlgTileSubstitionMethod
{
	DRLGSUBST_NONE = 0,
	DRLGSUBST_FIXED = 1,
	DRLGSUBST_RANDOM = 2,
};

// Note: Lower value has priority over others
enum DrlgRoomStatus : uint8_t
{
	ROOMSTATUS_CLIENT_IN_ROOM = 0,
	ROOMSTATUS_CLIENT_IN_SIGHT = 1,
	ROOMSTATUS_CLIENT_OUT_OF_SIGHT = 2,
	ROOMSTATUS_UNTILE = 3,
	ROOMSTATUS_COUNT,
};

enum DrlgRoomFlags
{
	DRLGROOMFLAG_INACTIVE = 0x00000002,
	DRLGROOMFLAG_HAS_WARP_0 = 0x00000010,
	DRLGROOMFLAG_HAS_WARP_1 = 0x00000020,
	DRLGROOMFLAG_HAS_WARP_2 = 0x00000040,
	DRLGROOMFLAG_HAS_WARP_3 = 0x00000080,
	DRLGROOMFLAG_HAS_WARP_4 = 0x00000100,
	DRLGROOMFLAG_HAS_WARP_5 = 0x00000200,
	DRLGROOMFLAG_HAS_WARP_6 = 0x00000400,
	DRLGROOMFLAG_HAS_WARP_7 = 0x00000800,
	DRLGROOMFLAG_SUBSHRINE_ROW1 = 0x00001000,
	DRLGROOMFLAG_SUBSHRINE_ROW2 = 0x00002000,
	DRLGROOMFLAG_SUBSHRINE_ROW3 = 0x00004000,
	DRLGROOMFLAG_SUBSHRINE_ROW4 = 0x00008000,
	DRLGROOMFLAG_HAS_WAYPOINT = 0x00010000,				//outdoors with subtheme and subwaypoint
	DRLGROOMFLAG_HAS_WAYPOINT_SMALL = 0x00020000,		//waypoint small
	DRLGROOMFLAG_AUTOMAP_REVEAL = 0x00040000,
	DRLGROOMFLAG_NO_LOS_DRAW = 0x00080000,
	DRLGROOMFLAG_HAS_ROOM = 0x00100000,					//an active pRoom structure is attached to this RoomEx
	DRLGROOMFLAG_ROOM_FREED_SRV = 0x00200000,			//set after freeing the pRoom
	DRLGROOMFLAG_HASPORTAL = 0x00400000,				//prevents room deletion
	DRLGROOMFLAG_POPULATION_ZERO = 0x00800000,			//set for towns, also set if a ds1 has populate=0 // nospawn region
	DRLGROOMFLAG_TILELIB_LOADED = 0x01000000,
	DRLGROOMFLAG_PRESET_UNITS_ADDED = 0x02000000,		//refers to DRLGMap or DRLGFile; adds the hardcoded preset units
	DRLGROOMFLAG_PRESET_UNITS_SPAWNED = 0x04000000,		//set after RoomEx preset units have been spawned / prevents respawning them on room reactivation
	DRLGROOMFLAG_ANIMATED_FLOOR = 0x08000000,			//animated floors (river of flame, hell act5)

	DRLGROOMFLAG_HAS_WARP_MASK = DRLGROOMFLAG_HAS_WARP_0 | DRLGROOMFLAG_HAS_WARP_1 | DRLGROOMFLAG_HAS_WARP_2 | DRLGROOMFLAG_HAS_WARP_3 | DRLGROOMFLAG_HAS_WARP_4 | DRLGROOMFLAG_HAS_WARP_5 | DRLGROOMFLAG_HAS_WARP_6 | DRLGROOMFLAG_HAS_WARP_7,
	DRLGROOMFLAG_SUBSHRINE_ROWS_MASK = DRLGROOMFLAG_SUBSHRINE_ROW1 | DRLGROOMFLAG_SUBSHRINE_ROW2 | DRLGROOMFLAG_SUBSHRINE_ROW3 | DRLGROOMFLAG_SUBSHRINE_ROW4,
	DRLGROOMFLAG_HAS_WAYPOINT_MASK = DRLGROOMFLAG_HAS_WAYPOINT | DRLGROOMFLAG_HAS_WAYPOINT_SMALL,

	DRLGROOMFLAG_HAS_WARP_FIRST_BIT = 4,
	DRLGROOMFLAG_SUBSHRINE_ROWS_FIRST_BIT = 12,
	DRLGROOMFLAG_HAS_WAYPOINT_FIRST_BIT = 16,
};

enum DrlgLevelFlags
{
	DRLGLEVELFLAG_AUTOMAP_REVEAL = 0x10,
};

enum MapTileFlags
{
	MAPTILE_FLAGS_NONE = 0,
	MAPTILE_UNK_0x1 = 0x000001,
	MAPTILE_WALL_EXIT = 0x000002, // warps, door exit, 
	MAPTILE_TREES = 0x000004, // Could also be delimiting an enclosure inside another area. Probably misnamed because not only trees ?
	MAPTILE_HIDDEN = 0x000008, // used by warps & others // aka skip automap
	MAPTILE_UNK_0x10 = 0x000010,
	MAPTILE_HASPRESETUNITS = 0x000020, // used for orientations 8-9; spawn doors & towers etc
	MAPTILE_UNWALKABLE = 0x000040,
	MAPTILE_FILL_LOS = 0x000080, // all subtiles will get wall collision; 
	MAPTILE_FLOOR_LINKER_PATH = 0x000100, // the floor is near a wp or forms a path within level or to another level
	MAPTILE_UNK_0x200 = 0x000200, // Reveal hidden ?
	MAPTILE_PITCH_BLACK = 0x000400, // the lighting changed, R&B=0 
	MAPTILE_OBJECT_WALL = 0x000800, // wall tile made of crops: barrels / crates / benches / tables (material flag 0x04)
	MAPTILE_UNK_0x001000 = 0x001000,
	MAPTILE_LOS = 0x002000,
	MAPTILE_WALL_LAYER_BIT = 14,
	MAPTILE_WALL_LAYER_MASK = 0b111 << MAPTILE_WALL_LAYER_BIT, // 0x1C000: 3bits value indicating the wall layer + 1 (0 indicates no wall?)
};
// Helper function
inline bool HasMapTileLayer(uint32_t nMapTileFlags) { return (nMapTileFlags & MAPTILE_WALL_LAYER_MASK) != 0; }
inline int32_t GetMapTileLayer(uint32_t nMapTileFlags) { return int32_t((nMapTileFlags & MAPTILE_WALL_LAYER_MASK) >> MAPTILE_WALL_LAYER_BIT) - 1; }

struct DrlgCoords
{
	int32_t nSubtileX;				//0x00 Called game coords in original game
	int32_t nSubtileY;				//0x04
	int32_t nSubtileWidth;			//0x08 Called nSizeGameX in original game
	int32_t nSubtileHeight;			//0x0C Called nSizeGameY in original game
	int32_t nTileXPos;				//0x10
	int32_t nTileYPos;				//0x14
	int32_t nTileWidth;				//0x18
	int32_t nTileHeight;			//0x1C
};

struct Room2 // Used to be called D2RoomExStrc
{
	Level* pLevel;					//0x00
	union
	{
		struct
		{
			int32_t nTileXPos;					//0x04
			int32_t nTileYPos;					//0x08
			int32_t nTileWidth;					//0x0C
			int32_t nTileHeight;				//0x10
		};
		DrlgCoord pDrlgCoord;				//0x04
	};
	uint32_t dwFlags;							//0x14 DrlgRoomFlags
	uint32_t dwOtherFlags;						//0x18
	int32_t nType;								//0x1C
	union
	{
		DrlgPresetRoom* pMaze;			//0x20
		DrlgOutdoorRoom* pOutdoor;		//0x20
	};
	uint32_t dwDT1Mask;							//0x24 - tile caching mask, used to init below @ D2Common.0x4A380
	TileLibraryHash* pTiles[32];			//0x28
	DrlgTileGrid* pTileGrid;				//0xA8
	uint8_t fRoomStatus;						//0xAC DrlgRoomStatus
	uint8_t unk0xAD;							//0xAD
	uint16_t wRoomsInList[ROOMSTATUS_COUNT + 1];//0xAE
	Room2* pStatusNext;				//0xB8
	Room2* pStatusPrev;				//0xBC
	Room2** ppRoomsNear;				//0xC0 // names pptVisibleRooms in the original game
	int32_t nRoomsNear;							//0xC4
	RoomTile* pRoomTiles;					//0xC8
	PresetUnit* pPresetUnits;				//0xCC
	DrlgOrth* pDrlgOrth;					//0xD0
	Seed pSeed;							//0xD4
	uint32_t dwInitSeed;						//0xDC
	DrlgLogicalRoomInfo* pLogicalRoomInfo;//0xE0 aka pCoordList (in other RE sources) or pRoomCoords (Mentor's notes). This seems to be the official name.
	Room1* pRoom;					//0xE4
	Room2* pDrlgRoomNext;				//0xE8
};

typedef int32_t(__stdcall* ROOMCALLBACKFN)(Room1*, void*);

struct DrlgDelete
{
	int32_t nUnitType;							//0x00
	D2UnitGUID nUnitGUID;						//0x04
	DrlgDelete* pNext;					//0x08
};

struct Room1 // Used to be called D2RoomStrc
{
	DrlgCoords tCoords;				//0x00
	RoomTileList* pRoomTiles;		//0x20
	Room1** ppRoomList;			//0x24
	int32_t nNumRooms;						//0x28
	UnitAny* pUnitFirst;					//0x2C
	UnitAny* pUnitUpdate;				//0x30
	RoomCollisionGrid* pCollisionGrid;//0x34
	Room2* pDrlgRoom;				//0x38
	Seed pSeed;						//0x3C
	DrlgDelete* pDrlgDelete;			//0x44
	uint32_t dwFlags;						//0x48
	GameClient** ppClients;				//0x4C
	int32_t nNumClients;					//0x50
	int32_t nMaxClients;					//0x54
	int32_t nTileCount;						//0x58
	int32_t nAllies;						//0x5C // nPlayerUnits in the game
	int32_t unk0x60;						//0x60
	uint8_t nCurrentDeathIndex;				//0x64
	uint8_t pad0x65[3];						//0x65
	D2UnitGUID nLastDeadGUIDs[4];			//0x68
	Act* pAct;					//0x78
	Room1* pRoomNext;			//0x7C
};

struct RoomTile
{
	Room2* pDrlgRoom;				//0x00
	LvlWarpTxt* pLvlWarpTxtRecord;		//0x04
	BOOL bEnabled;							//0x08
	DrlgTileData* unk0x0C;			//0x0C
	DrlgTileData* unk0x10;			//0x10
	RoomTile* pNext;					//0x14
};

typedef void(__stdcall* AUTOMAPFN)(Room1*);
typedef void(__stdcall* TOWNAUTOMAPFN)(int32_t, int32_t, int32_t, int32_t);

struct RoomCoordList
{
	DrlgCoord pBox[2];				//0x00
	BOOL bNode;								//0x20
	BOOL bRoomActive;						//0x24
	int32_t nIndex;							//0x28
	RoomCoordList* pNext;				//0x2C
};

struct ActMisc
{
	Level* pLevel;				//0x00 Latest added level
	void* pMempool;							//0x04
	HD2ARCHIVE hArchive;					//0x08 Always nullptr in the game, used by DRLGPRESET_LoadDrlgFile to load DS1 binary data
	Act* pAct;					//0x0C
	uint8_t nAct;							//0x10
	uint8_t padding0x11[3];					//0x11
	Seed pSeed;						//0x14
	uint32_t dwStartSeed;					//0x1C
	uint32_t dwGameLowSeed;					//0x20
	uint32_t dwFlags;						//0x24 DrlgFlags
	Room2 tStatusRoomsLists[ROOMSTATUS_COUNT];	//0x28
	Room2* pDrlgRoom;				//0x3D8
	uint8_t nRoomsInitSinceLastUpdate;		//0x3DC
	uint8_t nRoomsInitTimeout;				//0x3DD
	uint8_t padding0x3DE[2];				//0x3DE
	int32_t nAllocatedRooms;				//0x3E0
	int32_t nFreedRooms;					//0x3E4
	Game* pGame;						//0x3E8
	uint8_t nDifficulty;					//0x3EC
	uint8_t pad0x3ED[3];					//0x3ED
	AUTOMAPFN pfAutomap;					//0x3F0
	TOWNAUTOMAPFN pfTownAutomap;			//0x3F4
	int32_t nStaffTombLevel;				//0x3F8
	int32_t nBossTombLevel;					//0x3FC
	TileLibraryHash* pTiles[32];		//0x400
	int32_t bJungleInterlink;				//0x480
	DrlgWarp* pWarp;					//0x484
};

struct DrlgTileData
{
	int32_t nWidth;							//0x00
	int32_t nHeight;						//0x04
	int32_t nPosX;							//0x08
	int32_t nPosY;							//0x0C
	int32_t unk0x10;						//0x10
	uint32_t dwFlags;						//0x14 MapTileFlags
	TileLibraryEntry* pTile;			//0x18
	int32_t nTileType;						//0x1C
	DrlgTileData* unk0x20;			//0x20
	int32_t unk0x24;						//0x24
	uint8_t nRed;							//0x28
	uint8_t nGreen;							//0x29
	uint8_t nBlue;							//0x2A
	uint8_t nIntensity;						//0x2B
	int32_t unk0x2C;						//0x2C
};

typedef void(__fastcall* ACTCALLBACKFN)(Room1*);

struct Act
{
	uint8_t nAct;							//0x00
	uint8_t pad0x01[3];						//0x01
	Room1* pRoom;				//0x04
	ActMisc* pDrlg;						//0x08
	uint32_t dwInitSeed;					//0x0C
	int32_t nTownId;						//0x10
	DrlgEnvironment* pEnvironment;	//0x14
	ACTCALLBACKFN pfnActCallBack;			//0x18
	BOOL bClient;							//0x1C
	BOOL bHasPendingRoomsUpdates;			//0x20
	BOOL bHasPendingRoomDeletions;			//0x24
	BOOL bHasPendingUnitListUpdates;		//0x28
	DrlgTileData pTileData;			//0x2C
	void* pMemPool;							//0x5C
};

struct DrlgAnimTileGrid
{
	DrlgTileData** ppMapTileData;		//0x00
	int32_t nFrames;						//0x04 In 8bits fixed point format
	int32_t nCurrentFrame;					//0x08 In 8bits fixed point format
	int32_t nAnimationSpeed;				//0x0C In 8bits fixed point format
	DrlgAnimTileGrid* pNext;			//0x10
};

struct DrlgBuild
{
	int32_t nPreset;						//0x00
	int32_t nDivisor;						//0x04
	int32_t nRand;							//0x08
	DrlgBuild* pNext;					//0x0C
};

struct DrlgFile
{
	int32_t nSubstMethod;								//0x00 DrlgTileSubstitionMethod
	void* pDS1File;										//0x04
	int32_t unk0x08;									//0x08
	int32_t nWidth;										//0x0C
	int32_t nHeight;									//0x10
	int32_t nWallLayers;								//0x14
	int32_t nFloorLayers;								//0x18
	void* pTileTypeLayer[DRLG_MAX_WALL_LAYERS];			//0x1C
	void* pWallLayer[DRLG_MAX_WALL_LAYERS];				//0x2C
	void* pFloorLayer[DRLG_MAX_FLOOR_LAYERS];			//0x3C
	void* pShadowLayer;									//0x44
	void* pSubstGroupTags;								//0x48
	int32_t nSubstGroups;								//0x4C named nClusters in original game
	struct DrlgSubstGroup* pSubstGroups;			//0x50
	PresetUnit* pPresetUnit;						//0x54
	DrlgFile* pNext;								//0x58
};

struct DrlgTileInfo
{
	int32_t nPosX;								//0x00
	int32_t nPosY;								//0x04
	int32_t nTileIndex;							//0x08
};

struct Level
{
	ActMisc* pDrlg;							//0x00
	int32_t nLevelId;							//0x04
	int32_t nLevelType;							//0x08
	int32_t nDrlgType;							//0x0C
	uint32_t dwFlags;							//0x10
	Seed pSeed;							//0x14
	uint32_t dwInitSeed;						//0x1C
	union
	{
		struct
		{
			int32_t nPosX;						//0x20
			int32_t nPosY;						//0x24
			int32_t nWidth;						//0x28
			int32_t nHeight;					//0x2C
		};
		DrlgCoord pLevelCoords;			//0x20
	};
	Room2* pFirstRoomEx;				//0x30
	int32_t nRooms;								//0x34
	union
	{
		DrlgPresetInfo* pPreset;			//0x38
		DrlgOutdoorInfo* pOutdoors;		//0x38
		LvlMazeTxt* pMaze;					//0x38

		//void* pLevelData;						//0x38
	};
	DrlgMap* pCurrentMap;					//0x3C
	int32_t nCoordLists;						//0x40
	DrlgTileInfo pTileInfo[32];			//0x44
	int32_t nTileInfo;							//0x1C4
	int32_t nRoom_Center_Warp_X[9];				//0x1C8
	int32_t nRoom_Center_Warp_Y[9];				//0x1EC
	int32_t nRoomCoords;						//0x210
	int32_t* pJungleDefs;						//0x214
	int32_t nJungleDefs;						//0x218
	DrlgBuild* pBuild;					//0x21C
	BOOL bActive;								//0x220
	uint32_t dwInactiveFrames;					//0x224
	int32_t* pPresetMaps;						//0x228
	Level* pNextLevel;				//0x22C
};

struct DrlgLink
{
	void* pfLinker;								//0x00
	int32_t nLevel;								//0x04
	int32_t nLevelLink;							//0x08
	int32_t nLevelLinkEx;						//0x0C
};

struct DrlgLevelLinkData
{
	Seed pSeed;							//0x00
	DrlgCoord pLevelCoord[15];			//0x08
	DrlgLink* pLink;						//0xF8
	union
	{
		int32_t nRand[4][15];					//0xFC
		int32_t nRand2[60];						//0xFC
	};
	int32_t nIteration;							//0x1EC
	int32_t nCurrentLevel;						//0x1F0
};

struct DrlgLinkerParams
{
	int32_t nLinkerLevels[3];					//0x00
	int32_t nChance[2];							//0x0C
	int32_t dwFlags;							//0x14
};


struct DrlgOrth
{
	union
	{
		Room2* pDrlgRoom;			//0x00
		Level* pLevel;			//0x00
	};
	uint8_t nDirection;						//0x04
	uint8_t unk0x05[3];						//0x05
	BOOL bPreset;							//0x08
	BOOL bInit;								//0x0C
	DrlgCoord* pBox;					//0x10
	DrlgOrth* pNext;					//0x14
};

struct DrlgPresetInfo
{
	DrlgMap* pDrlgMap;				//0x00
	int32_t nDirection;						//0x04
};

struct DrlgRGB
{
	uint32_t unk0x00[6];					//0x00
	uint8_t nRed;							//0x18
	uint8_t nGreen;							//0x19
	uint8_t nBlue;							//0x1A
};

struct RoomTileList
{
	DrlgTileData* pWallTiles;			//0x00
	int32_t nWalls;							//0x04
	DrlgTileData* pFloorTiles;		//0x08
	int32_t nFloors;						//0x0C
	DrlgTileData* pRoofTiles;			//0x10
	int32_t nRoofs;							//0x14
};

struct DrlgTileLink
{
	BOOL bFloor;							//0x00
	DrlgTileData* pMapTile;			//0x04
	DrlgTileLink* pNext;				//0x08
};

struct DrlgTileGrid
{
	DrlgTileLink* pMapLinks;			//0x00
	DrlgAnimTileGrid* pAnimTiles;		//0x04
	int32_t nWalls;							//0x08
	int32_t nFloors;						//0x0C
	int32_t nShadows;						//0x10
	RoomTileList pTiles;				//0x14
};


struct DrlgUnits
{
	uint32_t unk0x00[9];					//0x00
	BOOL bDelete;							//0x24
	BOOL bUpdate;							//0x28
	uint32_t unk0x2C[12];					//0x2C
	void* pMemPool;							//0x5C
};

struct DrlgWarp
{
	int32_t nLevel;							//0x00
	int32_t nVis[8];						//0x04
	int32_t nWarp[8];						//0x24
	DrlgWarp* pNext;					//0x44
};

#pragma pack()

//D2Common.0x6FD74120 (#10014)
D2COMMON_DLL_DECL ActMisc* __fastcall DRLG_AllocDrlg(Act* pAct, uint8_t nActNo, HD2ARCHIVE hArchive, uint32_t nInitSeed, int nLevelId, uint32_t nFlags, Game* pGame, uint8_t nDifficulty, AUTOMAPFN pfAutoMap, TOWNAUTOMAPFN pfTownAutoMap);
//D2Common.0x6FD743B0 (#10012)
D2COMMON_DLL_DECL void __fastcall DRLG_FreeDrlg(ActMisc* pDrlg);
//D2Common.0x6FD74440
void __fastcall DRLG_FreeLevel(void* pMemPool, Level* pLevel, BOOL bAlloc);
//D2Common.0x6FD745C0
void __fastcall sub_6FD745C0(Room2* pDrlgRoom1, Room2* pDrlgRoom2);
//D2Common.0x6FD74700
void __fastcall DRLG_UpdateAndFreeInactiveRooms(ActMisc* pDrlg);
//D2Common.0x6FD748D0 (#10013)
D2COMMON_DLL_DECL Level* __fastcall DRLG_AllocLevel(ActMisc* pDrlg, int nLevelId);
//D2Common.0x6FD749A0 (#10005)
D2COMMON_DLL_DECL Level* __stdcall DRLG_GetLevel(ActMisc* pDrlg, int nLevelId);
//D2Common.0x6FD749D0
int __fastcall DRLG_GetHoradricStaffTombLevelId(ActMisc* pDrlg);
//D2Common.0x6FD749E0
int __fastcall DRLG_GetDirectionFromCoordinates(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2);
//D2Common.0x6FD74A40
void __fastcall DRLG_CreateRoomForRoomEx(ActMisc* pDrlg, Room2* pDrlgRoom);
//D2Common.0x6FD74B30
int* __fastcall DRLG_GetRoomCenterX_RoomWarpXFromRoom(Room2* pDrlgRoom);
//D2Common.0x6FD74B40
void __fastcall DRLG_ComputeLevelWarpInfo(Level* pLevel);
//D2Common.0x6FD74C10 (#10006)
D2COMMON_DLL_DECL void __stdcall DRLG_InitLevel(Level* pLevel);
//D2Common.0x6FD74D50
int __fastcall DRLG_GetNumberOfPopulatedRoomsInLevel(ActMisc* pDrlg, int nLevelId);
//D2Common.0x6FD74D90
void __fastcall DRLG_GetMinAndMaxCoordinatesFromLevel(Level* pLevel, int* pTileMinX, int* pTileMinY, int* pTileMaxX, int* pTileMaxY);
//D2Common.0x6FD74E10
void __fastcall DRLG_UpdateRoomExCoordinates(Level* pLevel);
//D2Common.0x6FD74EF0
Room2* __fastcall DRLG_GetRoomExFromLevelAndCoordinates(Level* pLevel, int nX, int nY);
//D2Common.0x6FD74F70
Room2* __fastcall DRLG_GetRoomExFromCoordinates(int nX, int nY, ActMisc* pDrlg, Room2* pDrlgRoom, Level* pLevel);
//D2Common.0x6FD751C0
BOOL __fastcall DRLG_IsTownLevel(int nLevelId);
//D2Common.0x6FD75260 (#10000)
D2COMMON_DLL_DECL int __stdcall DRLG_GetLevelTypeFromLevelId(int nLevelId);
//D2Common.0x6FD75270
void __fastcall DRLG_SetLevelPositionAndSize(ActMisc* pDrlg, Level* pLevel);
//D2Common.0x6FD75300 (#10001)
D2COMMON_DLL_DECL uint8_t __stdcall DRLG_GetActNoFromLevelId(int nLevelId);
//D2Common.0x6FD75330 (#10004)
D2COMMON_DLL_DECL int __stdcall DRLG_GetSaveMonstersFromLevelId(int nLevelId);
//D2Common.0x6FD75350 (#10002)
D2COMMON_DLL_DECL int __stdcall DRLG_GetLOSDrawFromLevelId(int nLevelId);
//D2Common.0x6FD75370
DrlgWarp* __fastcall DRLG_GetDrlgWarpFromLevelId(ActMisc* pDrlg, int nLevelId);
//D2Common.0x6FD753F0
void __fastcall DRLG_SetWarpId(DrlgWarp* pDrlgWarp, int nVis, int nWarp, int nId);
//D2Common.0x6FD75450
int __fastcall DRLG_IsOnClient(ActMisc* pDrlg);
