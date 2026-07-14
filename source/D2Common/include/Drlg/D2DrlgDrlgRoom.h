#pragma once

#include "D2CommonDefinitions.h"
#include <Drlg/D2DrlgDrlgVer.h>

struct ActMisc;
struct Room1;
struct Room2;
struct Level;
struct DrlgOrth;
struct PresetUnit;

#pragma pack(1)


#pragma pack()

//D2Common.0x6FD771C0
Room2* __fastcall DRLGROOM_AllocRoomEx(Level* pLevel, int nType);
//D2Common.0x6FD77280
void __fastcall sub_6FD77280(Room2* pDrlgRoom, BOOL bClient, uint32_t nFlags);
//D2Common.0x6FD772B0
void __fastcall DRLGROOM_FreeRoomTiles(void* pMemPool, Room2* pDrlgRoom);
//D2Common.0x6FD772F0
void __fastcall DRLGROOM_FreeRoomEx(Room2* pDrlgRoom);
//D2Common.0x6FD774F0
void __fastcall DRLGROOM_FreeRoomData(void* pMemPool, DrlgOrth* pDrlgRoomData);
//D2Common.0x6FD77520
void __fastcall DRLGROOM_AllocDrlgOrthsForRooms(Room2* pDrlgRoom1, Room2* pDrlgRoom2, int nDirection);
//D2Common.0x6FD77600
void __fastcall DRLGROOM_AddOrth(DrlgOrth** ppDrlgOrth, Level* pLevel, int nDirection, BOOL bIsPreset);
//D2Common.0x6FD776B0
BOOL __fastcall sub_6FD776B0(DrlgOrth* pDrlgOrth1, DrlgOrth* pDrlgOrth2);
//D2Common.0x6FD77740
BOOL __fastcall DRLG_GetRectanglesManhattanDistanceAndCheckNotOverlapping(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int nMaxDistance, int* pDistanceX, int* pDistanceY);
//D2Common.0x6FD777B0
BOOL __fastcall DRLG_CheckNotOverlappingUsingManhattanDistance(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int nMaxDistance);
//D2Common.0x6FD77800
BOOL __fastcall DRLG_CheckOverlappingWithOrthogonalMargin(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int nOrthogonalDistanceMax);
//D2Common.0x6FD77890
BOOL __fastcall DRLGMAZE_CheckRoomNotOverlaping(Level* pLevel, Room2* pDrlgRoom1, Room2* pIgnoredRoom, int nMargin);
//D2Common.0x6FD77910
void __fastcall DRLGROOM_AddRoomExToLevel(Level* pLevel, Room2* pDrlgRoom);
//D2Common.0x6FD77930
BOOL __fastcall DRLGROOM_AreXYInsideCoordinates(DrlgCoord* pDrlgCoord, int nX, int nY);
//D2Common.0x6FD77980
BOOL __fastcall DRLGROOM_AreXYInsideCoordinatesOrOnBorder(DrlgCoord* pDrlgCoord, int nX, int nY);
//D2Common.0x6FD779D0
BOOL __fastcall DRLGROOM_CheckLOSDraw(Room2* pDrlgRoom);
//D2Common.0x6FD779F0
int __fastcall sub_6FD779F0(Room2* pDrlgRoom);
//D2Common.0x6FD77A00
int __fastcall DRLGROOM_CheckWaypointFlags(Room2* pDrlgRoom);
//D2Common.0x6FD77A10
int __fastcall DRLGROOM_GetLevelId(Room2* pDrlgRoom);
//D2Common.0x6FD77A20
int __fastcall DRLGROOM_GetWarpDestinationLevel(Room2* pDrlgRoom, int nSourceLevel);
//D2Common.0x6FD77AB0
int __fastcall DRLGROOM_GetLevelIdFromPopulatedRoom(Room2* pDrlgRoom);
//D2Common.0x6FD77AF0
BOOL __fastcall DRLGROOM_HasWaypoint(Room2* pDrlgRoom);
//D2Common.0x6FD77B20
const char* __fastcall DRLGROOM_GetPickedLevelPrestFilePathFromRoomEx(Room2* pDrlgRoom);
//D2Common.0x6FD77B50
int __fastcall DRLGROOM_ReorderNearRoomList(Room2* pDrlgRoom, Room1** ppRoomList);
//D2Common.0x6FD77BB0
void __fastcall sub_6FD77BB0(void* pMemPool, Room2* pDrlgRoom);
//D2Common.0x6FD77EB0
void __fastcall DRLGROOM_SortRoomListByPosition(Room2** ppRoomList, int nListSize);
//D2Common.0x6FD77F00
BOOL __fastcall sub_6FD77F00(void* pMemPool, Room2* pDrlgRoom1, uint8_t nWarpId, Room2* pDrlgRoom2, char nWarpFlag, int nDirection);
//D2Common.0x6FD780E0
PresetUnit* __fastcall DRLGROOM_AllocPresetUnit(Room2* pDrlgRoom, void* pMemPool, int nUnitType, int nIndex, int nMode, int nX, int nY);
//D2Common.0x6FD78160
PresetUnit* __fastcall DRLGROOM_GetPresetUnits(Room2* pDrlgRoom);
//D2Common.0x6FD78190
void __fastcall DRLGROOM_SetRoom(Room2* pDrlgRoom, Room1* pRoom);
//D2Common.0x6FD781A0
void __fastcall DRLGROOM_GetRGB_IntensityFromRoomEx(Room2* pDrlgRoom, uint8_t* pIntensity, uint8_t* pRed, uint8_t* pGreen, uint8_t* pBlue);
//D2Common.0x6FD781E0
int* __fastcall DRLGROOM_GetVisArrayFromLevelId(ActMisc* pDrlg, int nLevelId);
//D2Common.0x6FD78230
ActMisc* __fastcall DRLGROOM_GetDrlgFromRoomEx(Room2* pRoom);
