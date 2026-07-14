#pragma once

#include "D2CommonDefinitions.h"

struct ActMisc;
struct Room1;
struct Room2;
struct Level;

#pragma pack(1)

struct MazeLevelId
{
	int32_t nLevelPrestId1;					//0x00
	int32_t nLevelPrestId2;					//0x04
	int32_t nPickedFile;					//0x08
	int32_t nDirection;						//0x0C
};

#pragma pack()

//D2Common.0x6FD78E50
Room2* __fastcall sub_6FD78E50(Level* pLevel);
//D2Common.0x6FD78F70
void __fastcall DRLGMAZE_PickRoomPreset(Room2* pDrlgRoom, BOOL bResetFlag);
//D2Common.0x6FD79240
Room2* __fastcall sub_6FD79240(Level* pLevel);
//D2Common.0x6FD79360
Room2* __fastcall sub_6FD79360(Level* pLevel);
//D2Common.0x6FD79480
void __fastcall DRLGMAZE_InitLevelData(Level* pLevel);
//D2Common.0x6FD794A0
void __fastcall DRLGMAZE_GenerateLevel(Level* pLevel);
//D2Common.0x6FD79E10
void __fastcall DRLGMAZE_SetPickedFileAndPresetId(Room2* pDrlgRoom, int nLevelPrest, int nPickedFile, BOOL bResetFlag);
//D2Commo.0x6FD79E40
Room2* __fastcall DRLGMAZE_ReplaceRoomPreset(Level* pLevel, int nLevelPrestId1, int nLevelPrestId2, int nPickedFile, BOOL bResetFlag);
//D2Common.0x6FD79EA0
Room2* __fastcall DRLGMAZE_AddAdjacentMazeRoom(Room2* pDrlgRoom, int nDirection, int bMergeRooms);
//D2Common.0x6FD7A110
void __fastcall DRLGMAZE_InitBasicMazeLayout(Level* pLevel, int nRoomsPerDirection);
//D2Common.0x6FD7A340
BOOL __fastcall DRLGMAZE_LinkMazeRooms(Room2* pDrlgRoom1, Room2* pDrlgRoom2, int nDirection);
//D2Common.0x6FD7A450
void __fastcall DRLGMAZE_MergeMazeRooms(Room2* pDrlgRoom);
//D2Common.0x6FD7A570
Room2* __fastcall DRLGMAZE_GetRandomRoomExFromLevel(Level* pLevel);
//D2Common.0x6FD7A5D0
void __fastcall DRLGMAZE_BuildBasicMaze(Level* pLevel);
//D2Common.0x6FD7A830
void __fastcall DRLGMAZE_PlaceAct5LavaPresets(Level* pLevel);
//D2Common.0x6FD7A9B0
void __fastcall DRLGMAZE_FillBlankMazeSpaces(Level* pLevel, int nLevelPrest, Room2* pIgnoreRoomEx);
//D2Common.0x6FD7AAC0
void __fastcall DRLGMAZE_PlaceAct2TombPrev_Act5BaalPrev(Level* pLevel);
//D2Common.0x6FD7ABC0
void __fastcall DRLGMAZE_PlaceArcaneSanctuary(Level* pLevel);
//D2Common.0x6FD7AFD0
Room2* __fastcall DRLGMAZE_PlaceAdjacentPresetRoom(Room2* pParentRoomEx, int nDirection, int bMergeRooms);
//D2Common.0x6FD7B230
void __fastcall DRLGMAZE_ScanReplaceSpecialPreset(Level* pLevel, const MazeLevelId* pMazeInit, int* pRand);
//D2Common.0x6FD7B330
void __fastcall DRLGMAZE_ScanReplaceSpecialAct2SewersPresets(Level* pLevel);
//D2Common.0x6FD7B660
void __fastcall DRLGMAZE_AddSpecialPreset(Level* pLevel, int nDirection, int nLvlPrestId, int nFile);
//D2Common.0x6FD7B710
BOOL __fastcall DRLGMAZE_CheckIfMayPlaceAdjacentPresetRoom(Room2* pDrlgRoom, int nDirection);
//D2Common.0x6FD7B8B0
void __fastcall DRLGMAZE_PlaceAct2TombStuff(Level* pLevel);
//D2Common.0x6FD7BC40
Room2* __fastcall DRLGMAZE_InitRoomFixedPreset(Room2* pDrlgRoom, int nDirection, int nLvlPrestId, int nFile, int bUseInitPreset);
//D2Common.0x6FD7BCD0
void __fastcall DRLGMAZE_PlaceAct2LairStuff(Level* pLevel);
//D2Common.0x6FD7BE60
void __fastcall DRLGMAZE_PlaceAct3DungeonStuff(Level* pLevel);
//D2Common.0x6FD7C000
void __fastcall DRLGMAZE_PlaceAct3SewerStuff(Level* pLevel);
//D2Common.0x6FD7C1A0
void __fastcall DRLGMAZE_PlaceAct3MephistoStuff(Level* pLevel);
//D2Common.0x6FD7C380
void __fastcall DRLGMAZE_PlaceAct5TempleStuff(Level* pLevel);
//D2Common.0x6FD7C500
void __fastcall DRLGMAZE_PlaceAct5BaalStuff(Level* pLevel);
//D2Common.0x6FD7C660
void __fastcall DRLGMAZE_PlaceAct1Barracks(Level* pLevel);
//D2Common.0x6FD7CA20
void __fastcall DRLGMAZE_SetRoomSize(Room2* pDrlgRoom);
//D2Common.0x6FD7CA40
void __fastcall DRLGMAZE_PlaceAct4Lava(Level* pLevel);
//D2Common.0x6FD7CCB0
void __fastcall DRLGMAZE_PlaceAct5IceStuff(Level* pLevel);
//D2Common.0x6FD7CEA0
void __fastcall DRLGMAZE_RollAct_1_2_3_BasicPresets(Level* pLevel);
//D2Common.0x6FD7D130
void __fastcall DRLGMAZE_RollBasicPresets(Level* pLevel);
//D2Common.0x6FD7D3D0
void __fastcall DRLGMAZE_ResetMazeRecord(Level* pLevel, BOOL bKeepMazeRecord);
