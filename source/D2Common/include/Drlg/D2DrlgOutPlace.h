#pragma once

#include "D2CommonDefinitions.h"
#include <Drlg/D2DrlgDrlg.h>


#pragma pack(1)

enum JungleIds {
	JUNGLE_MAX_ATTACH = 3,
	JUNGLE_PRESET2_ATTACH_POINT = 2
};


enum JunglePresetFlags {
	// Related to pPreset1 having a value in the direction
	JUNGLE_FLAG_LEFT_	= 0b0001, // West : deltaX < 0
	JUNGLE_FLAG_RIGHT_	= 0b0010, // East : deltaX > 0
	JUNGLE_FLAG_BOTTOM_	= 0b0100, // South: deltaY > 0
	JUNGLE_FLAG_TOP_	= 0b1000, // North: deltaY < 0
};

struct Jungle
{
	DrlgCoord pDrlgCoord;							//0x00
	int32_t field_10;									//0x10 Orientation of the jungle ?
	int32_t nBranch;									//0x14
	Jungle* pBasedOnJungle;						//0x18 Jungle we are based from
	Jungle* pJungleBranches[JUNGLE_MAX_ATTACH];	//0x1C Jungles that are based on this one
	int32_t nPresetsBlocksX;							//0x18
	int32_t nPresetsBlocksY;							//0x2C
	int32_t* pJungleDefs;								//0x30
	int32_t nJungleDefs;								//0x34
};


#pragma pack()

//D2Common.0x6FD80480
void __fastcall DRLGOUTPLACE_BuildKurast(Level* pLevel);
//D2Common.0x6FD806A0
void __fastcall DRLGOUTPLACE_InitAct3OutdoorLevel(Level* pLevel);
//D2Common.0x6FD80750
BOOL __fastcall sub_6FD80750(Level* pLevel, int nVertexId);
//D2Common.0x6FD80BE0
int __fastcall sub_6FD80BE0(int a1, int a2, int a3);
//D2Common.0x6FD80C10
int __fastcall sub_6FD80C10(int a1, int a2, int a3, int a4, int a5);
//D2Common.0x6FD80C80
void __fastcall DRLGOUTPLACE_SetBlankBorderGridCells(Level* pLevel);
//D2Common.0x6FD80DA0
void __fastcall DRLGOUTPLACE_SetOutGridLinkFlags(Level* pLevel);
//D2Common.0x6FD80E10
void __fastcall DRLGOUTPLACE_PlaceAct1245OutdoorBorders(Level* pLevel);
//D2Common.0x6FD81330
BOOL __fastcall sub_6FD81330(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD81380
BOOL __fastcall sub_6FD81380(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD81430
void __fastcall sub_6FD81430(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int a3, int a4);
//D2Common.0x6FD81530
BOOL __fastcall sub_6FD81530(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD815E0
void __fastcall sub_6FD815E0(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int a3, int a4);
//D2Common.0x6FD81720
BOOL __fastcall sub_6FD81720(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD81850
void __fastcall sub_6FD81850(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int a3, int a4);
//D2Common.0x6FD81950
BOOL __fastcall sub_6FD81950(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD81AD0
BOOL __fastcall sub_6FD81AD0(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD81B30
BOOL __fastcall sub_6FD81B30(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD81BF0
BOOL __fastcall sub_6FD81BF0(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD81CA0
BOOL __fastcall sub_6FD81CA0(DrlgLevelLinkData* pLevelLinkData);
//D2Common.0x6FD81D60
void __fastcall DRLGOUTPLACE_CreateLevelConnections(ActMisc* pDrlg, uint8_t nActNo);
//D2Common.0x6FD82050
BOOL __fastcall sub_6FD82050(DrlgLevelLinkData* pLevelLinkData, int nIteration);
//D2Common.0x6FD82130
BOOL __fastcall sub_6FD82130(DrlgLevelLinkData* pLevelLinkData, int nIteration);
//D2Common.0x6FD821E0
BOOL __fastcall DRLGOUTPLACE_LinkAct2Outdoors(DrlgLevelLinkData* pLevelLinkData, int nIteration);
//D2Common.0x6FD82240
BOOL __fastcall DRLGOUTPLACE_LinkAct2Canyon(DrlgLevelLinkData* pLevelLinkData, int nIteration);
//D2Comon.0x6FD822A0
BOOL __fastcall DRLGOUTPLACE_LinkAct4Outdoors(DrlgLevelLinkData* pLevelLinkData, int nIteration);
//D2Common.0x6FD82300
BOOL __fastcall DRLGOUTPLACE_LinkAct4ChaosSanctum(DrlgLevelLinkData* pLevelLinkData, int nIteration);
//D2Common.0x6FD82360
void __fastcall sub_6FD82360(Level* pLevel, int nIteration, int* pRand);
//D2Common.0x6FD823C0
void __fastcall sub_6FD823C0(ActMisc* pDrlg, DrlgLink* pDrlgLink, int(__fastcall* a3)(DrlgLevelLinkData*, int), void(__fastcall* a4)(Level*, int, int*));
//D2Common.0x6FD826D0
void __fastcall sub_6FD826D0(ActMisc* pDrlg, int nStartId, int nEndId);
//D2Common.0x6FD82750
void __fastcall sub_6FD82750(ActMisc* pDrlg, int nStartId, int nEndId);
//D2Common.0x6FD82820
Level* __fastcall DRLG_GenerateJungles(Level* pLevel);
//D2Common.0x6FD83970
void __fastcall sub_6FD83970(DrlgCoord* pDrlgCoord, Jungle* pJungle, int nRand, int nSizeX, int nSizeY);
//D2Common.0x6FD83A20
void __fastcall DRLGOUTPLACE_InitOutdoorRoomGrids(Room2* pDrlgRoom);
//D2Common.0x6FD83C90
void __fastcall DRLGOUTPLACE_CreateOutdoorRoomEx(Level* pLevel, int nX, int nY, int nWidth, int nHeight, int dwRoomFlags, int dwOutdoorFlags, int dwOutdoorFlagsEx, int dwDT1Mask);
