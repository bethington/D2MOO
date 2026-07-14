#pragma once

#include "D2CommonDefinitions.h"

struct ActMisc;
struct Level;
struct LvlWarpTxt;
struct Room1;
struct Room2;
struct UnitAny;

#pragma pack(1)


#pragma pack()

//D2Common.0x6FD78780
Room1* __fastcall DRLGWARP_GetDestinationRoom(Room2* pDrlgRoom, int nSourceLevel, int* pDestinationLevel, LvlWarpTxt** ppLvlWarpTxtRecord);
//D2Common.0x6FD787F0
void __fastcall DRLGWARP_ToggleRoomTilesEnableFlag(Room2* pDrlgRoom, BOOL bEnabled);
//D2Common.0x6FD78810
void __fastcall DRLGWARP_UpdateWarpRoomSelect(Room2* pDrlgRoom, int nLevelId);
//D2Common.0x6FD78870
void __fastcall DRLGWARP_UpdateWarpRoomDeselect(Room2* pDrlgRoom, int nLevelId);
//D2Common.0x6FD788D0
Room1* __fastcall sub_6FD788D0(ActMisc* pDrlg, int nLevelId, int nTileIndex, int* pX, int* pY);
//D2Common.0x6FD78C10
Room2* __fastcall DRLGWARP_GetWaypointRoomExFromLevel(Level* pLevel, int* pX, int* pY);
//D2Common.0x6FD78CC0
int* __fastcall DRLGWARP_GetWarpIdArrayFromLevelId(ActMisc* pDrlg, int nLevelId);
//D2Common.0x6FD78D10
int __fastcall DRLGWARP_GetWarpDestinationFromArray(Level* pLevel, uint8_t nArrayId);
//D2Common.0x6FD78D80
LvlWarpTxt* __fastcall DRLGWARP_GetLvlWarpTxtRecordFromWarpIdAndDirection(Level* pLevel, uint8_t nWarpId, char szDirection);
//D2Common.0x6FD78DF0
LvlWarpTxt* __fastcall DRLGWARP_GetLvlWarpTxtRecordFromUnit(Room2* pDrlgRoom, UnitAny* pUnit);
