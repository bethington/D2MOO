#pragma once

#include "D2CommonDefinitions.h"
#include "D2DrlgDrlg.h"
#include "D2DrlgDrlgGrid.h"
#include <Path/Path.h>
#include <Archive.h>

struct LvlPrestTxt;

#pragma pack(1)

enum DrlgPresetRoomFlags
{
	DRLGPRESETROOMFLAG_NONE = 0,
	DRLGPRESETROOMFLAG_SINGLE_ROOM = 1 << 0,
	DRLGPRESETROOMFLAG_HAS_MAP_DS1 = 1 << 1, // needs confirmation
};

struct LevelFileList
{
	char szPath[D2_MAX_PATH];			//0x00
	long nRefCount;						//0x104
	DrlgFile* pFile;				//0x108
	LevelFileList* pNext;				//0x10C
};

struct PresetUnit
{
	int32_t nUnitType;						//0x00
	int32_t nIndex;							//0x04 see D2Common.#11278
	int32_t nMode;							//0x08
	int32_t nXpos;							//0x0C
	int32_t nYpos;							//0x10
	BOOL bSpawned;							//0x14 
	MapAI* pMapAI;					//0x18
	PresetUnit* pNext;				//0x1C
};

struct DrlgMap
{
	int32_t nLevelPrest;					//0x00
	int32_t nPickedFile;					//0x04
	LvlPrestTxt* pLvlPrestTxtRecord;		//0x08
	DrlgFile* pFile;					//0x0C
	DrlgCoord pDrlgCoord;				//0x10
	BOOL bHasInfo;							//0x20
	DrlgGrid pMapGrid;				//0x24
	PresetUnit* pPresetUnit;			//0x38
	BOOL bInited;							//0x3C
	int32_t nPops;							//0x40
	int32_t* pPopsIndex;					//0x44
	int32_t* pPopsSubIndex;					//0x48
	int32_t* pPopsOrientation;				//0x4C
	DrlgCoord* pPopsLocation;			//0x50
	DrlgMap* pNext;					//0x54
};

struct DrlgPresetRoom
{
	int32_t nLevelPrest;						//0x00
	int32_t nPickedFile;						//0x04
	DrlgMap* pMap;					//0x08
	union
	{
		struct
		{
			uint8_t nFlags;					//0x0C
			uint8_t unk0x0D[3];				//0x0D
		};
		uint32_t dwFlags;						//0x0C
	};
	DrlgGrid pWallGrid[4];			//0x10
	DrlgGrid pTileTypeGrid[4];		//0x60 aka pOrientationGrid
	DrlgGrid pFloorGrid[2];			//0xB0
	DrlgGrid pCellGrid;				//0xD8
	DrlgGrid* pMazeGrid;				//0xEC
	Coord* pTombStoneTiles;			//0xF0
	int32_t nTombStoneTiles;					//0xF4
};

#pragma pack()

//Helper function
inline bool DRLGMAZE_HasMapDS1(Room2* pDrlgRoom)
{
	return pDrlgRoom->pMaze->nFlags & DRLGPRESETROOMFLAG_HAS_MAP_DS1;
}

//D2Common.0x6FD859A0 (#11222)
D2COMMON_DLL_DECL int __stdcall DRLGPRESET_CountPresetObjectsByAct(uint8_t nAct);
//D2Common.0x6FD859E0 (#11223)
D2COMMON_DLL_DECL int __stdcall DRLGPRESET_GetObjectIndexFromObjPreset(uint8_t nAct, int nUnitId);
//D2Common.0x6FD85A10
void __fastcall DRLGPRESET_ParseDS1File(DrlgFile* pDrlgFile, HD2ARCHIVE hArchive, const char* szFileName);
//D2Common.0x6FD86050
void __fastcall DRLGPRESET_LoadDrlgFile(DrlgFile** ppDrlgFile, HD2ARCHIVE hArchive, const char* szFile);
//D2Common.0x6FD86190
void __fastcall DRLGPRESET_FreeDrlgFile(DrlgFile** ppDrlgFile);
//D2Common.0x6FD86310
PresetUnit* __fastcall DRLGPRESET_CopyPresetUnit(void* pMemPool, PresetUnit* pPresetUnit, int nX, int nY);
//D2Common.0x6FD86430
void __fastcall DRLGPRESET_FreePresetUnit(void* pMemPool, PresetUnit* pPresetUnit);
//D2Common.0x6FD86480 (#10020)
D2COMMON_DLL_DECL MapAI* __fastcall DRLGPRESET_CreateCopyOfMapAI(void* pMemPool, MapAI* pMapAI);
//D2Common.0x6FD864F0 (#10021)
D2COMMON_DLL_DECL MapAI* __fastcall DRLGPRESET_ChangeMapAI(MapAI** ppMapAI1, MapAI** ppMapAI2);
//D2Common.0x6FD86500 (#10022)
D2COMMON_DLL_DECL void __fastcall DRLGPRESET_FreeMapAI(void* pMemPool, MapAI* pMapAI);
//D2Common.0x6FD86540
void __fastcall DRLGPRESET_AddPresetUnitToDrlgMap(void* pMemPool, DrlgMap* pDrlgMap, Seed* pSeed);
//D2Common.0x6FD867A0
void __fastcall DRLGPRESET_SpawnHardcodedPresetUnits(Room2* pDrlgRoom);
//D2Common.0x6FD86AC0
void __fastcall DRLGPRESET_AddPresetRiverObjects(DrlgMap* pDrlgMap, void* pMemPool, int nOffsetX, DrlgGrid* pDrlgGrid);
//D2Common.0x6FD86C80
void __fastcall DRLGPRESET_FreePresetRoomData(Room2* pDrlgRoom);
//D2Common.0x6FD86CE0
void __fastcall DRLGPRESET_FreeDrlgGrids(void* pMemPool, Room2* pDrlgRoom);
//D2Common.0x6FD86D60
void __fastcall DRLGPRESET_FreeDrlgGridsFromPresetRoom(Room2* pDrlgRoom);
//D2Common.0x6FD86D80
void __fastcall DRLGPRESET_AllocPresetRoomData(Room2* pDrlgRoom);
//D2Common.0x6FD86DC0
Room2* __fastcall DRLGPRESET_InitPresetRoomData(Level* pLevel, DrlgMap* pDrlgMap, DrlgCoord* pDrlgCoord, uint32_t dwDT1Mask, int dwRoomFlags, int dwPresetFlags, DrlgGrid* a7);
//D2Common.0x6FD86E50
void __fastcall DRLGPRESET_InitPresetRoomGrids(Room2* pDrlgRoom);
//D2Common.0x6FD870F0
void __fastcall DRLGPRESET_GetTombStoneTileCoords(Room2* pDrlgRoom, Coord** ppTombStoneTiles, int* pnTombStoneTiles);
//D2Common.0x6FD87130
void __fastcall DRLGPRESET_AddPresetRoomMapTiles(Room2* pDrlgRoom);
//D2Common.0x6FD87560
Room2* __fastcall DRLGPRESET_BuildArea(Level* pLevel, DrlgMap* pDrlgMap, int nFlags, BOOL bSingleRoom);
//D2Common.0x6FD87760
void __fastcall DRLGPRESET_BuildPresetArea(Level* pLevel, DrlgGrid* pDrlgGrid, int nFlags, DrlgMap* pDrlgMap, BOOL bSingleRoom);
//D2Common.0x6FD87E10
void __fastcall DRLGPRESET_SetPickedFileInDrlgMap(DrlgMap* pDrlgMap, int nPickedFile);
//D2Common.0x6FD87E20
DrlgMap* __fastcall DRLGPRESET_AllocDrlgMap(Level* pLevel, int nLvlPrestId, DrlgCoord* pDrlgCoord, Seed* pSeed);
//D2Common.0x6FD87F00
int __fastcall DRLGPRESET_GetSizeX(int nLvlPrestId);
//D2Common.0x6FD87F10
int __fastcall DRLGPRESET_GetSizeY(int nLvlPrestId);
//D2Common.0x6FD87F20
void __fastcall DRLGPRESET_FreeDrlgMap(void* pMemPool, DrlgMap* pDrlgMap);
//D2Common.0x6FD881A0 (#10008)
D2COMMON_DLL_DECL int __fastcall DRLGPRESET_GetLevelPrestIdFromRoomEx(Room2* pDrlgRoom);
//D2Common.0x6FD881B0 (#10009)
D2COMMON_DLL_DECL char* __fastcall DRLGPRESET_GetPickedLevelPrestFilePathFromRoomEx(Room2* pDrlgRoom);
//D2Common.0x6FD881D0
void __fastcall DRLGPRESET_UpdatePops(Room2* pDrlgRoom, int nX, int nY, BOOL bOtherRoom);
//D2Common.0x6FD88450
void __fastcall DRLGPRESET_TogglePopsVisibility(Room2* pDrlgRoom, int nPopSubIndex, DrlgCoord* pDrlgCoord, int nTick, BOOL nCellFlags);
//D2Common.0x6FD88610
void __fastcall DRLGPRESET_InitLevelData(Level* pLevel);
//D2Common.0x6FD886F0
void __fastcall DRLGPRESET_GenerateLevel(Level* pLevel);
//D2Common.0x6FD88810
void __fastcall DRLGPRESET_ResetDrlgMap(Level* pLevel, BOOL bKeepPreset);
//D2Common.0x6FD88850
int __fastcall DRLGPRESET_MapTileType(int nId);

// D2Common.0x6FDEA700
extern LevelFileList* gpLevelFilesList_6FDEA700;
