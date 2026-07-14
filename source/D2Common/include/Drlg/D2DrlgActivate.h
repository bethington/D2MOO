#pragma once

#include "D2CommonDefinitions.h"

struct Room1;
struct Room2;
struct ActMisc;

#pragma pack(1)


#pragma pack()

//D2Common.0x6FD733D0
void __fastcall DRLGACTIVATE_RoomExSetStatus_ClientInRoom(Room2* pDrlgRoom);
//D2Common.0x6FD73450
void __fastcall DRLGACTIVATE_RoomExSetStatus_ClientInSight(Room2* pDrlgRoom);
//D2Common.0x6FD73550
void __fastcall DRLGACTIVATE_RoomExSetStatus_ClientOutOfSight(Room2* pDrlgRoom);
//D2Common.0x6FD736F0
void __fastcall DRLGACTIVATE_RoomExSetStatus_Untile(Room2* pDrlgRoom);
//D2Common.0x6FD73790
void __fastcall DRLGACTIVATE_RoomExIdentifyRealStatus(Room2* pDrlgRoom);
//D2Common.0x6FD73880
void __fastcall DRLGACTIVATE_RoomExStatusUnset_Untile(Room2* pDrlgRoom);
//D2Common.0x6FD739A0
void __fastcall DRLGACTIVATE_SetClientIsInSight(ActMisc* pDrlg, int nLevelId, int nX, int nY, Room2* pDrlgRoomHint);
//D2Common.0x6FD73A30
void __fastcall DRLGACTIVATE_RoomExPropagateSetStatus(void* pMemPool, Room2* pDrlgRoom, uint8_t nStatus);
//D2Common.0x6FD73B40
void __fastcall DRLGACTIVATE_UnsetClientIsInSight(ActMisc* pDrlg, int nLevelId, int nX, int nY, Room2* pDrlgRoomHint);
//D2Common.0x6FD73BE0
void __fastcall DRLGACTIVATE_RoomExPropagateUnsetStatus(Room2* pDrlgRoom, uint8_t nStatus);
//D2Common.0x6FD73C40
void __fastcall DRLGACTIVATE_ChangeClientRoom(Room2* pPreviousRoom, Room2* pNewRoom);
//D2Common.0x6FD73CF0
void __fastcall DRLGACTIVATE_InitializeRoomEx(Room2* pDrlgRoom);
//D2Common.0x6FD73D80
Room1* __fastcall DRLGACTIVATE_StreamRoomAtCoords(ActMisc* pDrlg, int nX, int nY);
//D2Common.0x6FD73E30
void __fastcall DRLGACTIVATE_InitializeRoomExStatusLists(ActMisc* pDrlg);
//D2Common.0x6FD73E60
Room1* __fastcall DRLGACTIVATE_GetARoomInClientSight(ActMisc* pDrlg);
//D2Common.0x6FD73E90
Room1* __fastcall DRLGACTIVATE_GetARoomInSightButWithoutClient(ActMisc* pDrlg, Room2* pDrlgRoom);
//D2Common.0x6FD73EF0 (#10015)
void __fastcall DRLGACTIVATE_GetRoomsAllocationStats(int* pOutStatsClientAllocatedRooms, int* pOutStatsClientFreedRooms, int* pOutStatsAllocatedRooms, int* pOutStatsFreedRooms);
//D2Common.0x6FD73F20 (#10003)
void __stdcall DRLGACTIVATE_Update(ActMisc* pDrlg);
//D2Common.0x6FD74060
BOOL __fastcall DRLGACTIVATE_TestRoomCanUnTile(Room2* pDrlgRoom);
//D2Common.0x6FD740F0
void __fastcall DRLGACTIVATE_ToggleHasPortalFlag(Room2* pDrlgRoom, BOOL bReset);
//D2Common.0x6FD74110
uint8_t __fastcall DRLGACTIVATE_GetRoomStatusFlags(Room2* pDrlgRoom);
