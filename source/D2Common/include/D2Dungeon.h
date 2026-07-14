#pragma once

#include "D2CommonDefinitions.h"
#include <Drlg/D2DrlgDrlg.h>

#pragma pack(1)


#pragma pack()

//D2Common.0x6FD8B8A0 (#10038)
D2COMMON_DLL_DECL Act* __stdcall DUNGEON_AllocAct(uint8_t nActNo, uint32_t nInitSeed, BOOL bClient, Game* pGame, uint8_t nDifficulty, void* pMemPool, int nTownLevelId, AUTOMAPFN pfAutoMap, TOWNAUTOMAPFN pfTownAutoMap);
//D2Common.0x6FD8B950 (#10039)
D2COMMON_DLL_DECL void __stdcall DUNGEON_FreeAct(Act* pAct);
//D2Common.0x6FD8B9D0
void* __fastcall DUNGEON_GetMemPoolFromAct(Act* pAct);
//D2Common.0x6FD8B9E0 (#10026)
D2COMMON_DLL_DECL void __stdcall DUNGEON_ToggleRoomTilesEnableFlag(Room1* pRoom, BOOL bEnabled);
//D2Common.0x6FD8BA20 (#10027)
D2COMMON_DLL_DECL UnitAny* __stdcall DUNGEON_GetWarpTileFromRoomAndSourceLevelId(Room1* pRoom, int nSourceLevel, LvlWarpTxt** ppLvlWarpTxtRecord);
//D2Common.0x6FD8BAB0 (#10028)
D2COMMON_DLL_DECL LvlWarpTxt* __stdcall DUNGEON_GetLvlWarpTxtRecordFromRoomAndUnit(Room1* pRoom, UnitAny* pUnit);
//D2Common.0x6FD8BAF0 (#10030)
D2COMMON_DLL_DECL DrlgTileData* __stdcall DUNGEON_GetFloorTilesFromRoom(Room1* pRoom, int* pFloorCount);
//D2Common.0x6FD8BB20 (#10031)
D2COMMON_DLL_DECL DrlgTileData* __stdcall DUNGEON_GetWallTilesFromRoom(Room1* pRoom, int* pWallCount);
//D2Common.0x6FD8BB60 (#10032)
D2COMMON_DLL_DECL DrlgTileData* __stdcall DUNGEON_GetRoofTilesFromRoom(Room1* pRoom, int* pRoofCount);
//D2Common.0x6FD8BBA0 (#10033)
D2COMMON_DLL_DECL DrlgTileData* __stdcall DUNGEON_GetTileDataFromAct(Act* pAct);
//D2Common.0x6FD8BBB0 (#10034)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GetRoomCoordinates(Room1* pRoom, DrlgCoords* pCoords);
//D2Common.0x6FD8BC10 (#10035)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GetAdjacentRoomsListFromRoom(Room1* pRoom, Room1*** pppRoomList, int* pNumRooms);
//D2Common.0x6FD8BC50
Room1* __fastcall DUNGEON_AllocRoom(Act* pAct, Room2* pDrlgRoom, DrlgCoords* pDrlgCoords, RoomTileList* pRoomTiles, int nLowSeed, uint32_t dwFlags);
//D2Common.0x6FD8BD90 (#10040)
D2COMMON_DLL_DECL BOOL __stdcall DUNGEON_DoRoomsTouchOrOverlap(Room1* ptFirst, Room1* ptSecond);
//D2Common.0x6FD8BE30 (#10043)
D2COMMON_DLL_DECL BOOL __stdcall DUNGEON_AreTileCoordinatesInsideRoom(Room1* pRoom, int nX, int nY);
//D2Common.0x6FD8BE90 (#10048)
D2COMMON_DLL_DECL int __stdcall DUNGEON_CheckRoomsOverlapping_BROKEN(Room1* pPrimary, Room1* pSecondary);
//D2Commmon.0x6FD8BF00 (#10051)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_FindRoomByTileCoordinates(Act* pAct, int nX, int nY);
//D2Common.0x6FD8BF50 (#10050)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_GetAdjacentRoomByTileCoordinates(Room1* pRoom, int nX, int nY);
//D2Common.0x6FD8BFF0 (#10049)
D2COMMON_DLL_DECL void __stdcall DUNGEON_CallRoomCallback(Room1* pRoom, ROOMCALLBACKFN pfnRoomCallback, void* pArgs);
//D2Common.0x6FD8C080 (#10052)
D2COMMON_DLL_DECL void __stdcall D2Common_10052(Room1* pRoom, RECT* pRect);
//D2Common.0x6FD8C170 (#10053)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GetSubtileRect(Room1* pRoom, RECT* pRect);
//D2Common.0x6FD8C210 (#10054)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GetRGB_IntensityFromRoom(Room1* pRoom, uint8_t* pIntensity, uint8_t* pRed, uint8_t* pGreen, uint8_t* pBlue);
//D2Common.0x6FD8C240 (#10041)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_FindRoomBySubtileCoordinates(Act* pAct, int nX, int nY);
//D2Common.0x6FD8C290
BOOL __fastcall DUNGEON_AreSubtileCoordinatesInsideRoom(DrlgCoords* pDrlgCoords, int nX, int nY);
//D2Common.0x6FD8C2F0 (#10046)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_FindActSpawnLocation(Act* pAct, int nLevelId, int nTileIndex, int* pX, int* pY);
//D2Common.0x6FD8C340 (#10045)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_FindActSpawnLocationEx(Act* pAct, int nLevelId, int nTileIndex, int* pX, int* pY, int nUnitSize);
//D2Common.0x6FD8C4A0 (#10029)
D2COMMON_DLL_DECL UnitAny* __stdcall DUNGEON_GetFirstUnitInRoom(Room1* pRoom);
//D2Common.0x6FD8C4E0 (#10100)
D2COMMON_DLL_DECL void __stdcall DUNGEON_IncreaseAlliedCountOfRoom(Room1* pRoom);
//D2Comon.0x6FD8C4F0 (#10036)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetAlliedCountFromRoom(Room1* pRoom);
//D2Common.0x6FD8C510 (#10101)
D2COMMON_DLL_DECL void __stdcall DUNGEON_DecreaseAlliedCountOfRoom(Room1* pRoom);
//D2Common.0x6FD8C550
UnitAny** __fastcall DUNGEON_GetUnitListFromRoom(Room1* pRoom);
//D2Common.0x6FD8C580
UnitAny** __fastcall DUNGEON_GetUnitUpdateListFromRoom(Room1* pRoom, BOOL bUpdate);
//D2Common.0x6FD8C5C0 (#10055)
D2COMMON_DLL_DECL PresetUnit* __stdcall DUNGEON_GetPresetUnitsFromRoom(Room1* pRoom);
//D2Common.0x6FD8C600
RoomCollisionGrid* __fastcall DUNGEON_GetCollisionGridFromRoom(Room1* pRoom);
//D2Common.0x6FD8C630
void __fastcall DUNGEON_SetCollisionGridInRoom(Room1* pRoom, RoomCollisionGrid* pCollisionGrid);
//D2Common.0x6FD8C660 (#10063)
D2COMMON_DLL_DECL void __stdcall DUNGEON_SetClientIsInSight(Act* pAct, int nLevelId, int nX, int nY, Room1* pRoom);
//D2Common.0x6FD8C6B0 (#10064)
D2COMMON_DLL_DECL void __stdcall DUNGEON_UnsetClientIsInSight(Act* pAct, int nLevelId, int nX, int nY, Room1* pRoom);
//D2Common.0x6FD8C700 (#10062)
D2COMMON_DLL_DECL void __stdcall DUNGEON_ChangeClientRoom(Room1* pRoom1, Room1* pRoom2);
//D2Common.0x6FD8C730 (#10065)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_StreamRoomAtCoords(Act* pAct, int nX, int nY);
//D2Common.0x6FD8C770 (#10056)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_GetRoomFromAct(Act* pAct);
//D2Common.0x6FD8C7A0 (#10057)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetLevelIdFromRoom(Room1* pRoom);
//D2Common.0x6FD8C7C0 (#10058)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetWarpDestinationLevel(Room1* pRoom, int nSourceLevel);
//D2Common.0x6FD8C7E0 (#10059)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetLevelIdFromPopulatedRoom(Room1* pRoom);
//D2Common.0x6FD8C800 (#10060)
D2COMMON_DLL_DECL BOOL __stdcall DUNGEON_HasWaypoint(Room1* pRoom);
//D2Common.0x6FD8C840 (#10061)
D2COMMON_DLL_DECL const char* __stdcall DUNGEON_GetPickedLevelPrestFilePathFromRoom(Room1* pRoom);
//D2Common.0x6FD8C860 (#10066)
D2COMMON_DLL_DECL void __stdcall DUNGEON_AllocDrlgDelete(Room1* pRoom, int nUnitType, D2UnitGUID nUnitGuid);
//D2Common.0x6FD8C8B0 (#10067)
D2COMMON_DLL_DECL void __stdcall DUNGEON_FreeDrlgDelete(Room1* pRoom);
//D2Common.0x6FD8C910 (#10068)
D2COMMON_DLL_DECL DrlgDelete* __stdcall DUNGEON_GetDrlgDeleteFromRoom(Room1* pRoom);
//D2Common.0x6FD8C940 (#10069)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_GetARoomInClientSight(Act* pAct);
//D2Common.0x6FD8C980 (#10070)
D2COMMON_DLL_DECL Room1* __stdcall DUNGEON_GetARoomInSightButWithoutClient(Act* pAct, Room1* pRoom);
//D2Common.0x6FD8C9E0 (#10071)
D2COMMON_DLL_DECL BOOL __stdcall DUNGEON_TestRoomCanUnTile(Act* pAct, Room1* pRoom);
//D2Common.0x6FD8CA60 (#10072)
D2COMMON_DLL_DECL bool __stdcall DUNGEON_GetRoomStatusFlags(Room1* pRoom);
//D2Common.0x6FD8CA80 (#10073)
D2COMMON_DLL_DECL BOOL __stdcall D2Common_10073(Room1* pRoom);
//D2Common.0x6FD8CAE0 (#10074)
D2COMMON_DLL_DECL BOOL __stdcall D2Common_10074(Room1* pRoom);
//D2Common.0x6FD8CB10 (#10075)
D2COMMON_DLL_DECL void __stdcall D2Common_10075(Room1* pRoom, BOOL bSet);
//D2Common.0x6FD8CB60 (#10079)
D2COMMON_DLL_DECL void __stdcall DUNGEON_AddClientToRoom(Room1* pRoom, GameClient* pClient);
//D2Common.0x6FD8CC50
void __fastcall DUNGEON_UpdateClientListOfRoom(Room1* pRoom);
//D2Common.0x6FD8CD10 (#10080)
D2COMMON_DLL_DECL void __stdcall DUNGEON_RemoveClientFromRoom(Room1* pRoom, GameClient* pClient);
//D2Common.0x6FD8CDF0 (#10081)
D2COMMON_DLL_DECL int __stdcall D2Common_10081_GetTileCountFromRoom(Room1* pRoom);
//D2Common.0x6FD8CE40
void __fastcall DUNGEON_FreeRoom(void* pMemPool, Room1* pRoom);
//D2Common.0x6FD8CF10 (#10076)
D2COMMON_DLL_DECL void __stdcall DUNGEON_RemoveRoomFromAct(Act* pAct, Room1* pRoom);
//D2Common.0x6FD8D000 (#10077)
D2COMMON_DLL_DECL void __stdcall D2Common_10077(Room1* pRoom1, Room1* pRoom2);
//D2Common.0x6FD8D030 (#10078)
D2COMMON_DLL_DECL void __stdcall DUNGEON_UpdateAndFreeInactiveRooms(Act* pAct);
//D2Common.0x6FD8D040 (#10044)
D2COMMON_DLL_DECL int __stdcall DUNGEON_CheckLOSDraw(Room1* pRoom);
//D2Common.0x6FD8D060
DrlgEnvironment* __fastcall DUNGEON_GetEnvironmentFromAct(Act* pAct);
//D2Common.0x6FD8D090 (#10088)
D2COMMON_DLL_DECL ActMisc* __stdcall DUNGEON_GetDrlgFromAct(Act* pAct);
//D2Common.0x6FD912D0 (#10089)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetInitSeedFromAct(Act* pAct);
//D2Common.0x6FD8D0C0 (#10007)
D2COMMON_DLL_DECL Room2* __fastcall DUNGEON_GetRoomExFromRoom(Room1* pRoom);
//D2Common.0x6FD8D0D0 (#10086)
D2COMMON_DLL_DECL BOOL __stdcall DUNGEON_IsTownLevelId(int nLevelId);
//D2Common.0x6FD8D0E0 (#10082)
D2COMMON_DLL_DECL BOOL __stdcall DUNGEON_IsRoomInTown(Room1* pRoom);
//D2Common.0x6FD8D100 (#10083)
D2COMMON_DLL_DECL int __stdcall D2COMMON_10083_Return0(Room1* pRoom);
//D2Common.0x6FD8D130 (#10084)
D2COMMON_DLL_DECL int __stdcall D2Common_10084(Room1* pRoom);
//D2Common.0x6FD8D140 (#10085)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetTownLevelIdFromActNo(uint8_t nAct);
//D2Common.0x6FD8D180 (#10087)
D2COMMON_DLL_DECL int __stdcall D2Common_10087(Room1* pRoom);
//D2Common.0x6FD8D1C0 (#10090)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetNumberOfPopulatedRoomsInLevel(Act* pAct, int nLevelId);
//D2Common.0x6FD8D1E0 (#10025)
D2COMMON_DLL_DECL int* __stdcall DUNGEON_GetWarpCoordinatesFromRoom(Room1* pRoom);
//D2Common.0x6FD8D220 (#10091)
D2COMMON_DLL_DECL void __stdcall DUNGEON_UpdateWarpRoomSelect(Room1* pRoom, int nLevelId);
//D2Common.0x6FD8D260 (#10092)
D2COMMON_DLL_DECL void __stdcall DUNGEON_UpdateWarpRoomDeselect(Room1* pRoom, int nLevelId);
//D2Common.0x6FD8D2A0 (#10093)
D2COMMON_DLL_DECL void __stdcall DUNGEON_UpdatePops(Room1* pRoom, int nX, int nY, BOOL bOtherRoom);
//D2Common.0x6FD8D2E0 (#10094)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GetTombStoneTileCoords(Room1* pRoom, Coord** ppTombStoneTiles, int* pnTombStoneTiles);
//D2Common.0x6FD8D300 (#10095)
D2COMMON_DLL_DECL int __stdcall D2Common_10095(Room1* pRoom, int nX, int nY);
//D2Common.0x6FD8D3A0 (#10096)
D2COMMON_DLL_DECL RoomCoordList* __stdcall D2Common_10096(Room1* pRoom, int nX, int nY);
//D2Common.0x6FD8D3C0 (#10097)
D2COMMON_DLL_DECL RoomCoordList* __stdcall DUNGEON_GetRoomCoordList(Room1* pRoom);
//D2Common.0x6FD8D3D0 (#10098)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GetPortalLevelArrayFromPortalFlags(void* pMemPool, int nFlags, int** ppLevels, int* pnLevels);
//D2Common.0x6FD8D4B0 (#10099)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetPortalFlagFromLevelId(int nPortalLevelId);
//D2Common.0x6FD8D4F0 (#10037)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetTownLevelIdFromAct(Act* pAct);
//D2Common.0x6FD8D520 (#10047)
D2COMMON_DLL_DECL int __stdcall DUNGEON_GetHoradricStaffTombLevelId(Act* pAct);
//D2Common.0x6FD8D540 (#10102)
D2COMMON_DLL_DECL void __stdcall DUNGEON_ToggleHasPortalFlag(Room1* pRoom, BOOL bReset);
//D2Common.0x6FD8D560 (#10104)
D2COMMON_DLL_DECL void __stdcall DUNGEON_AnimateTiles(Room1* pRoom);
//D2Common.0x6FD8D580 (#10105)
D2COMMON_DLL_DECL void __stdcall DUNGEON_InitRoomTileAnimation(Act* pAct, Room1* pRoom1, Room1* pRoom2);
//D2Common.0x6FD8D5C0 (#10103)
D2COMMON_DLL_DECL void __stdcall DUNGEON_SetActCallbackFunc(Act* pAct, ACTCALLBACKFN pActCallbackFunction);
//D2Common.0x6FD8D600 (#10106)
D2COMMON_DLL_DECL void __stdcall DUNGEON_SaveKilledUnitGUID(Room1* pRoom, D2UnitGUID nUnitGUID);

//////////////////////////////////////
// Coordinates conversion functions	//
//      see doc/Coordinates.md		//
//////////////////////////////////////

//D2Common.0x6FD8D690 (#10107)
D2COMMON_DLL_DECL void __stdcall DUNGEON_ClientToGameTileCoords(int* pX, int* pY);
//D2Common.0x6FD8D870 (#10108)
D2COMMON_DLL_DECL void __stdcall DUNGEON_ClientToGameSubtileCoords(int* pX, int* pY);
//D2Common.0x6FD8D8A0 (#10109)
D2COMMON_DLL_DECL void __stdcall DUNGEON_ClientToGameCoords(int* pX, int* pY);

//D2Common.0x6FD8D6E0 (#10110)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GameTileToClientCoords(int* pX, int* pY);
//D2Common.0x6FD8D630 (#10111)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GameSubtileToClientCoords(int* pX, int* pY);
//D2Common.0x6FD8D660 (#10112)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GameToClientCoords(int* pX, int* pY);
//D2Common.0x6FD8D8C0 (#10113)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GameTileToSubtileCoords(int* pX, int* pY);

//D2Common.0x6FD8D710 (#10114)
D2COMMON_DLL_DECL void __stdcall DUNGEON_ClientTileDrawPositionToGameCoords(int nX, int nY, int* pX, int* pY);
//D2Common.0x6FD8D790 (#10115)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GameToClientTileDrawPositionCoords(int nX, int nY, int* pX, int* pY);

//D2Common.0x6FD8D7D0 (#10116)
D2COMMON_DLL_DECL void __stdcall DUNGEON_ClientSubileDrawPositionToGameCoords(int nX, int nY, int* pX, int* pY);
//D2Common.0x6FD8D830 (#10117)
D2COMMON_DLL_DECL void __stdcall DUNGEON_GameToClientSubtileDrawPositionCoords(int nX, int nY, int* pX, int* pY);

// Helper function, official name coming from D2Common.0x6FDBCF10
inline bool DungeonTestRoomGame(const Room1* pRoom, int nX, int nY)
{
	return nX >= pRoom->tCoords.nSubtileX && nX < (pRoom->tCoords.nSubtileX + pRoom->tCoords.nSubtileWidth)
        && nY >= pRoom->tCoords.nSubtileY && nY < (pRoom->tCoords.nSubtileY + pRoom->tCoords.nSubtileHeight);

}
// Helper function
inline Room1* __fastcall DUNGEON_GetRoomAtPosition(Room1* pRoom, int32_t nSubtileX, int32_t nSubtileY)
{
    if (!pRoom)
    {
        return nullptr;
    }

    if (DungeonTestRoomGame(pRoom, nSubtileX , nSubtileY))
    {
        return pRoom;
    }

    Room1** ppRoomList = nullptr;
    int32_t nNumRooms = 0;
    DUNGEON_GetAdjacentRoomsListFromRoom(pRoom, &ppRoomList, &nNumRooms);

    for (int32_t i = 0; i < nNumRooms; ++i)
    {
        Room1* pAdjacentRoom = ppRoomList[i];
        if (pAdjacentRoom && DungeonTestRoomGame(pAdjacentRoom, nSubtileX, nSubtileY))
        {
            return pAdjacentRoom;
        }
    }

    return nullptr;
}
