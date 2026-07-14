#pragma once

#include "D2CommonDefinitions.h"

struct Room1;
struct UnitAny;

#pragma pack(1)



#pragma pack()

//D2Common.0x6FDBCF10 (#11279)
D2COMMON_DLL_DECL int __stdcall UNITROOM_AddUnitToRoomEx(UnitAny* pUnit, Room1* pRoom, int nUnused);
//D2Common.0x6FDBD100 (#10384)
D2COMMON_DLL_DECL int __stdcall UNITROOM_AddUnitToRoom(UnitAny* pUnit, Room1* pRoom);
//D2Common.0x6FDBD120 (#10385)
D2COMMON_DLL_DECL void __stdcall UNITROOM_RefreshUnit(UnitAny* pUnit);
//D2Common.0x6FDBD1B0 (#10388)
D2COMMON_DLL_DECL void __stdcall UNITROOM_SortUnitListByTargetY(Room1* pRoom);
//D2Common.0x6FDBD250 (#10390)
D2COMMON_DLL_DECL void __stdcall UNITROOM_UpdatePath(UnitAny* pUnit);
//D2Common.0x6FDBD2B0 (#10391)
D2COMMON_DLL_DECL void __stdcall UNITROOM_ClearUpdateQueue(Room1* pRoom);
//D2Common.0x6FDBD300 (#10386)
D2COMMON_DLL_DECL void __stdcall UNITROOM_RemoveUnitFromRoom(UnitAny* pUnit);
//D2Common.0x6FDBD400 (#10387)
D2COMMON_DLL_DECL void __stdcall UNITROOM_RemoveUnitFromUpdateQueue(UnitAny* pUnit);
//D2Common.0x6FDBD4C0 (#10389)
D2COMMON_DLL_DECL BOOL __stdcall UNITROOM_IsUnitInRoom(Room1* pRoom, UnitAny* pUnit);
