#pragma once

#include "D2CommonDefinitions.h"
#include "D2DrlgDrlg.h"
#include "D2DrlgDrlgGrid.h"


#pragma pack(1)

enum OutDoorInfoFlags 
{
	OUTDOOR_FLAG1 = 0x00000001,
	OUTDOOR_BRIDGE = 0x00000004,
	OUTDOOR_RIVER_OTHER = 0x00000008,
	OUTDOOR_RIVER = 0x00000010,
	OUTDOOR_CLIFFS = 0x00000020,
	OUTDOOR_OUT_CAVES = 0x00000040,
	OUTDOOR_SOUTHWEST = 0x00000080,
	OUTDOOR_NORTHWEST = 0x00000100,
	OUTDOOR_SOUTHEAST = 0x00000200,
	OUTDOOR_NORTHEAST = 0x00000400,
};

struct DrlgOutdoorGrid
{
	int32_t dwFlags;						//0x00
	DrlgGrid* pSectors;				//0x04
	int32_t nWidth;							//0x08
	int32_t nHeight;						//0x0C
	BOOL bInit;								//0x10
};

union DrlgOutdoorPackedGrid2Info // TODO: rename
{
	uint32_t nPackedValue;
	struct {
		uint32_t nUnkb00 : 1;		 // Mask 0x00000001
		uint32_t bHasDirection : 1;  // Mask 0x00000002
		uint32_t nUnkb02 : 5;        // Mask 0x0000007C
		uint32_t nUnkb07 : 1;        // Mask 0x00000080 spawn preset level here ?
		uint32_t nUnkb08 : 1;        // Mask 0x00000100 related to being a blank grid cell?
		uint32_t bHasPickedFile : 1; // Mask 0x00000200
		uint32_t bLvlLink : 1;		 // Mask 0x00000400
		uint32_t nUnkb11 : 1;		 // Mask 0x00000800
		uint32_t nUnkb12 : 1;		 // Mask 0x00001000
		uint32_t nUnkb13 : 3;		 // Mask 0x0000E000
		uint32_t nPickedFile : 4;    // Mask 0x000F0000
		uint32_t nUnkb20 : 12;       // Mask 0xFFF00000
	};
};

struct DrlgOutdoorInfo
{
	uint32_t dwFlags;						//0x00 OutDoorInfoFlags
	DrlgGrid pGrid[4];				//0x04 0: LevelPrestId 1: ? 2: DrlgOutdoorPackedGrid2Info
	union
	{
		struct
		{
			int32_t nWidth;					//0x54
			int32_t nHeight;				//0x58
			int32_t nGridWidth;				//0x5C
			int32_t nGridHeight;			//0x60
		};
		DrlgCoord pCoord;
	};
	DrlgVertex* pVertex;				//0x64
	DrlgVertex* pPathStarts[6];			//0x68
	DrlgVertex pVertices[24];			//0x80
	int32_t nVertices;						//0x260 Maybe nPathStarts instead?
	DrlgOrth* pRoomData;				//0x264
};

#pragma pack()

//Helper function
inline DrlgOutdoorPackedGrid2Info DRLGOUTDOORS_GetPackedGrid2Info(DrlgOutdoorInfo* pOutdoors, int nX, int nY)
{
	uint32_t nPackedValue = DRLGGRID_GetGridEntry(&pOutdoors->pGrid[2], nX, nY);
	return DrlgOutdoorPackedGrid2Info{ nPackedValue };
}

//D2Common.0x6FD7DC20
int __fastcall DRLGOUTDOORS_GetOutLinkVisFlag(Level* pLevel, DrlgVertex* pDrlgVertex);
//D2Common.0x6FD7DD00
int __fastcall DRLGOUTDOORS_GetPresetIndexFromGridCell(Level* pLevel, int nX, int nY);
//D2Common.0x6FD7DD40
void __fastcall DRLGOUTDOORS_AlterAdjacentPresetGridCells(Level* pLevel, int nX, int nY);
//D2Common.0x6FD7DD70
void __fastcall DRLGOUTDOORS_SetBlankGridCell(Level* pLevel, int nX, int nY);
//D2Common.0x6FD7DDB0
unsigned int __fastcall DRLGOUTDOORS_TestGridCellNonLvlLink(Level* pLevel, int nX, int nY);
//D2Common.0x6FD7DDD0
BOOL __fastcall DRLGOUTDOORS_TestGridCellSpawnValid(Level* pLevel, int nX, int nY);
//D2Common.0x6FD7DDF0
BOOL __fastcall DRLGOUTDOORS_TestOutdoorLevelPreset(Level* pLevel, int nX, int nY, int nLevelPrestId, int nOffset, char nFlags);
//D2Common.0x6FD7DEF0
void __fastcall DRLGOUTDOORS_SpawnOutdoorLevelPresetEx(Level* pLevel, int nX, int nY, int nLevelPrestId, int nPickedFile, BOOL bBorder);
//D2Common.0x6FD7E0F0
BOOL __fastcall DRLGOUTDOORS_SpawnPresetFarAway(Level* pLevel, DrlgCoord* pDrlgCoord, int nLvlPrestId, int nRand, int nOffset, char nFlags);
//D2Common.0x6FD7E330
BOOL __fastcall DRLGOUTDOORS_SpawnOutdoorLevelPreset(Level* pLevel, int nLevelPrestId, int nRand, int nOffset, char nFlags);
//D2Common.0x6FD7E4D0
BOOL __fastcall DRLGOUTDOORS_SpawnRandomOutdoorDS1(Level* pLevel, int nLvlPrestId, int nRand);
//D2Common.0x6FD7E6D0
void __fastcall DRLGOUTDOORS_SpawnAct12Waypoint(Level* pLevel);
//D2Common.0x6FD7E940
void __fastcall DRLGOUTDOORS_SpawnAct12Shrines(Level* pLevel, int nShrines);
//D2Common.0x6FD7EB20
void __fastcall DRLGOUTDOORS_AddAct124SecondaryBorder(Level* pLevel, int nLvlSubId, int nLevelPrestId);
//D2Common.0x6FD7EBA0
void __fastcall DRLGOUTDOORS_AllocOutdoorInfo(Level* pLevel);
//D2Common.0x6FD7EBD0
void __fastcall DRLGOUTDOORS_GenerateLevel(Level* pLevel);
//D2Common.0x6FD7EEE0
void __fastcall DRLGOUTDOORS_FreeOutdoorInfo(Level* pLevel, BOOL bKeepRoomData);
//D2Common.0x6FD7EFE0
void __fastcall DRLG_OUTDOORS_GenerateDirtPath(Level* pLevel, Room2* pDrlgRoom);
//D2Common.0x6FD7F250
void __fastcall DRLGOUTDOORS_SpawnAct1DirtPaths(Level* pLevel);
//D2Common.0x6FD7F500
void __fastcall DRLGOUTDOORS_CalculatePathCoordinates(Level* pLevel, DrlgVertex* pVertex1, DrlgVertex* pVertex2);
//D2Common.0x6FD7F5B0
void __fastcall sub_6FD7F5B0(Level* pLevel);
//D2Common.0x6FD7F810
void __fastcall sub_6FD7F810(Level* pLevel, int nVertexId);
//D2Common.0x6FD7F9B0
void __fastcall DRLGOUTDOORS_InitAct4OutdoorLevel(Level* pLevel);
