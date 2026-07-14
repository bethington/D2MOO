#include "GAME/Level.h"

#include <D2Collision.h>
#include <D2Dungeon.h>
#include <D2Environment.h>
#include <D2Skills.h>
#include <Units/UnitRoom.h>
#include <Path/PathMisc.h>


#include "GAME/Arena.h"
#include "GAME/Clients.h"
#include "GAME/SCmd.h"
#include "ITEMS/ItemMode.h"
#include "ITEMS/Items.h"
#include "MISSILES/Missiles.h"
#include "MONSTER/Monster.h"
#include "MONSTER/MonsterMsg.h"
#include "OBJECTS/Objects.h"
#include "OBJECTS/ObjMode.h"
#include "PLAYER/Player.h"
#include "PLAYER/PlayerPets.h"
#include "PLAYER/PlrModes.h"
#include "QUESTS/ACT2/A2Q3.h"
#include "UNIT/SUnit.h"
#include "UNIT/SUnitMsg.h"
#include "UNIT/SUnitProxy.h"


//D2Game.0x6FC3BBA0
void __fastcall LEVEL_UpdateUnitsInAdjacentRooms(Game* pGame, Room1* pRoom, GameClient* pClient)
{
    // TODO: v10
    if (!pGame->pAct[CLIENTS_GetActNo(pClient)]->bHasPendingUnitListUpdates)
    {
        return;
    }

    D2_ASSERT(pRoom);

    Room1** ppRoomList = nullptr;
    int32_t nNumRooms = 0;
    DUNGEON_GetAdjacentRoomsListFromRoom(pRoom, &ppRoomList, &nNumRooms);

    for (int32_t i = 0; i < nNumRooms; ++i)
    {
        Room1* pAdjacentRoom = ppRoomList[i];
        D2_ASSERT(pAdjacentRoom);

        UnitAny* pUnit = pAdjacentRoom->pUnitUpdate;
        while (pUnit)
        {
            UnitAny* pNextUnit = pUnit->pChangeNextUnit;

            int32_t v10 = 0;
            int32_t bProcessUnit = 0;
            if (!(pUnit->dwFlags & UNITFLAG_INITSEEDSET) || CLIENTS_GetPlayerFromClient(pClient, 0) == pUnit)
            {
                bProcessUnit = 1;
            }
            else if (pUnit->dwUnitType != UNIT_MISSILE)
            {
                D2GAME_SUNITMSG_FirstFn_6FCC5520(pGame, pUnit, pClient);
                v10 = 1;
                bProcessUnit = 1;
            }

            if (bProcessUnit)
            {
                switch (pUnit->dwUnitType)
                {
                case UNIT_PLAYER:
                    sub_6FC81650(pGame, pUnit, pClient, v10);
                    break;

                case UNIT_MONSTER:
                    sub_6FC659E0(pGame, pUnit, pClient, v10);
                    break;

                case UNIT_OBJECT:
                    sub_6FC75350(pGame, pUnit, pClient);
                    break;

                case UNIT_ITEM:
                    sub_6FC420B0(pUnit, pClient);
                    break;

                default:
                    break;
                }
            }

            pUnit = pNextUnit;
        }
    }
}

//D2Game.0x6FC3BD10
void __fastcall LEVEL_RemoveUnitsExceptClientPlayer(Room1* pRoom, GameClient* pClient)
{
    D2_ASSERT(pRoom);

    Room1** ppRoomList = nullptr;
    int32_t nNumRooms = 0;
    DUNGEON_GetAdjacentRoomsListFromRoom(pRoom, &ppRoomList, &nNumRooms);

    UnitAny* pUnit = CLIENTS_GetPlayerFromClient(pClient, 0);
    D2_ASSERT(pUnit);

    for (int32_t i = 0; i < nNumRooms; ++i)
    {
        for (DrlgDelete* pDrlgDelete = DUNGEON_GetDrlgDeleteFromRoom(ppRoomList[i]); pDrlgDelete; pDrlgDelete = pDrlgDelete->pNext)
        {
            if (pDrlgDelete->nUnitType != pUnit->dwUnitType || pDrlgDelete->nUnitGUID != pUnit->dwUnitId)
            {
                D2GAME_PACKETS_SendPacket0x0A_RemoveObject_6FC3D3A0(pClient, 0x0A, pDrlgDelete->nUnitType, pDrlgDelete->nUnitGUID);
            }
        }
    }
}

//D2Game.0x6FC3BDE0
void __fastcall LEVEL_FreeDrlgDeletes(Game* pGame)
{
    for (int32_t i = 0; i < NUM_ACTS; ++i)
    {
        Act* pAct = pGame->pAct[i];
        if (pAct && pAct->bHasPendingRoomDeletions)
        {
            for (Room1* pRoom = DUNGEON_GetRoomFromAct(pAct); pRoom; pRoom = pRoom->pRoomNext)
            {
                DUNGEON_FreeDrlgDelete(pRoom);
            }

            pAct->bHasPendingRoomDeletions = FALSE;
        }
    }
}

//D2Game.0x6FC3BE40
void __fastcall LEVEL_AddClient(Game* pGame, Room1* pRoom, GameClient* pClient)
{
    D2_ASSERT(pRoom);

    DrlgCoords drlgCoords = {};
    DUNGEON_GetRoomCoordinates(pRoom, &drlgCoords);
    D2GAME_PACKETS_SendPacket0x07_6FC3D120(pClient, DUNGEON_GetLevelIdFromRoom(pRoom), drlgCoords.nTileXPos, drlgCoords.nTileYPos);

    DUNGEON_AddClientToRoom(pRoom, pClient);

    if (pRoom->nNumClients <= 1u)
    {
        for (UnitAny* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext)
        {
            if (pUnit->dwUnitType == UNIT_MONSTER)
            {
                MONSTER_UpdateAiCallbackEvent(pGame, pUnit);
            }
        }
    }

    UnitAny* pPlayer = CLIENTS_GetPlayerFromClient(pClient, 0); 
    for (UnitAny* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext)
    {
        if (pPlayer != pUnit)
        {
            D2GAME_SUNITMSG_FirstFn_6FCC5520(pGame, pUnit, pClient);
        }
    }
}

//D2Game.0x6FC3BF00
void __fastcall LEVEL_RemoveClient(Game* pGame, Room1* pRoom, GameClient* pClient)
{
    D2_ASSERT(pRoom);

    for (UnitAny* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext)
    {
        D2GAME_PACKETS_SendPacket0x0A_RemoveObject_6FCC5F80(pUnit, pClient);
    }

    DUNGEON_RemoveClientFromRoom(pRoom, pClient);

    if (!pRoom->nNumClients)
    {
        for (UnitAny* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext)
        {
            if (pUnit->dwUnitType == UNIT_MONSTER)
            {
                MONSTER_DeleteEvents(pGame, pUnit);
            }
        }
    }

    DrlgCoords drlgCoords = {};
    DUNGEON_GetRoomCoordinates(pRoom, &drlgCoords);
    D2GAME_PACKETS_SendPacket0x08_6FC3D160(pClient, DUNGEON_GetLevelIdFromRoom(pRoom), drlgCoords.nTileXPos, drlgCoords.nTileYPos);
}

//D2Game.0x6FC3BFB0
void __fastcall LEVEL_RemoveClientFromAdjacentRooms(Room1* pRoom, GameClient* pClient)
{
    D2_ASSERT(pRoom);

    Room1** ppRoomList = nullptr;
    int32_t nNumRooms = 0;
    DUNGEON_GetAdjacentRoomsListFromRoom(pRoom, &ppRoomList, &nNumRooms);

    for (int32_t i = 0; i < nNumRooms; ++i)
    {
        DUNGEON_RemoveClientFromRoom(ppRoomList[i], pClient);
    }
}

//D2Game.0x6FC3C010
void __fastcall LEVEL_SynchronizeDayNightCycleWithClient(Game* pGame, GameClient* pClient)
{
    const uint8_t nAct = CLIENTS_GetActNo(pClient);
    D2GAME_PACKETS_SendPacket0x03_6FC3C810(pClient, 3, nAct, pGame->dwInitSeed, pGame->dwObjSeed, DUNGEON_GetTownLevelIdFromAct(pGame->pAct[nAct]));

    int32_t nCycleIndex = 0;
    int32_t nTicks = 0;
    int32_t nEclipse = 0;
    ENVIRONMENT_GetCycleIndex_Ticks_EclipseFromAct(pGame->pAct[nAct], &nCycleIndex, &nTicks, &nEclipse);
    
    GSPacketSrv53 packet53 = {};
    packet53.nHeader = 0x53u;
    packet53.unk0x01 = nCycleIndex;
    packet53.unk0x05 = nTicks;
    packet53.unk0x09 = nEclipse;
    D2GAME_SendPacket0x53_6FC3DF50(pClient, &packet53);
}

//D2Game.0x6FC3C0B0
void __fastcall LEVEL_ChangeAct(Game* pGame, GameClient* pClient, int32_t nDestinationLevelId, int32_t nTileCalc)
{
    if (ARENA_IsInArenaMode(pGame))
    {
        return;
    }

    const uint8_t nNewAct = DRLG_GetActNoFromLevelId(nDestinationLevelId);
    D2_ASSERT(nNewAct != pClient->nAct);

    LEVEL_LoadAct(pGame, nNewAct);

    CLIENTS_SetClientState(pClient, CLIENTSTATE_CHANGING_ACT);

    UnitAny* pUnit = CLIENTS_GetPlayerFromClient(pClient, 0);
    D2_ASSERT(pUnit);

    if (!pGame->bExpansion && pUnit->dwUnitType == UNIT_PLAYER)
    {
        D2GAME_KillPlayerPets_6FC7CD10(pGame, pUnit);
    }

    SKILLS_RemoveTransformStatesFromShapeshiftedUnit(pUnit);

    int32_t nX = 0;
    int32_t nY = 0;
    Room1* pSpawnRoom = DUNGEON_FindActSpawnLocationEx(pGame->pAct[nNewAct], nDestinationLevelId, nTileCalc, &nX, &nY, UNITS_GetUnitSizeX(pUnit));
    if (!pSpawnRoom)
    {
        return;
    }

    Coord spawnPoint = {};
    spawnPoint.nX = nX;
    spawnPoint.nY = nY;

    Room1* pRoom = COLLISION_GetFreeCoordinatesEx(pSpawnRoom, &spawnPoint, UNITS_GetUnitSizeX(pUnit), 0x1C89u, 5);
    if (!pRoom)
    {
        return;
    }

    nX = spawnPoint.nX;
    nY = spawnPoint.nY;

    if (!D2Common_10229(pUnit->pDynamicPath, pUnit, nullptr, 0, 0))
    {
        FOG_DisplayAssert("fRet", __FILE__, __LINE__);
        exit(-1);
    }

    sub_6FCBB190(pGame, pUnit, UNITS_GetRoom(pUnit));

    if (!D2Common_10229(pUnit->pDynamicPath, pUnit, pRoom, nX, nY))
    {
        FOG_DisplayAssert("fRet", __FILE__, __LINE__);
        exit(-1);
    }

    CLIENTS_SetRoomInClient(pClient, nullptr);
    D2GAME_PACKETS_SendHeaderOnlyPacket(pClient, 5u);
    CLIENTS_SetActNo(pClient, nNewAct);
    pUnit->nAct = nNewAct;
    pUnit->pDrlgAct = pGame->pAct[nNewAct];
    LEVEL_SynchronizeDayNightCycleWithClient(pGame, pClient);
    CLIENTS_SetRoomInClient(pClient, pRoom);
    UNITROOM_RefreshUnit(pUnit);
    pUnit->dwFlagEx |= UNITFLAGEX_TELEPORTED;
    sub_6FCBB190(pGame, pUnit, nullptr);
    UNITROOM_RefreshUnit(pUnit);

    if (pUnit->dwUnitType == UNIT_PLAYER)
    {
        sub_6FC7E310(pGame, pUnit, CLIENTS_GetUnitX(pUnit), CLIENTS_GetUnitY(pUnit));
    }
}

//D2Game.0x6FC3C410
void __fastcall LEVEL_WarpUnit(Game* pGame, UnitAny* pPlayer, int32_t nDestinationLevelId, int32_t nTileCalc)
{
    GameClient* pClient = SUNIT_GetClientFromPlayer(pPlayer, __FILE__, __LINE__);
    D2_ASSERT(pClient);

    int32_t nSourceLevelId = 0;
    Room1* pRoom = UNITS_GetRoom(pPlayer);
    if (pRoom)
    {
        nSourceLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);
    }

    const uint8_t nDestinationAct = DRLG_GetActNoFromLevelId(nDestinationLevelId);
    if (nDestinationAct != CLIENTS_GetActNo(pClient))
    {
        SUNITPROXY_UpdateNpcsOnActChange(pGame, pPlayer, nSourceLevelId, nDestinationLevelId);
        LEVEL_ChangeAct(pGame, pClient, nDestinationLevelId, nTileCalc);
        return;
    }

    int32_t nX = 0;
    int32_t nY = 0;
    Room1* pSpawnRoom = DUNGEON_FindActSpawnLocationEx(pGame->pAct[nDestinationAct], nDestinationLevelId, nTileCalc, &nX, &nY, UNITS_GetUnitSizeX(pPlayer));
    if (pSpawnRoom)
    {
        sub_6FCBDFE0(pGame, pPlayer, pSpawnRoom, nX, nY, 0, 0);
    }
}

//D2Game.0x6FC3C510
void __fastcall LEVEL_LoadAct(Game* pGame, uint8_t nAct)
{
    if (pGame->pAct[nAct])
    {
        return;
    }

    int32_t nTownLevelId = 0;
    if (ARENA_IsInArenaMode(pGame))
    {
        nTownLevelId = ARENA_GetAlternateStartTown(pGame);
    }
    else
    {
        nTownLevelId = DUNGEON_GetTownLevelIdFromActNo(nAct);
    }

    pGame->pAct[nAct] = DUNGEON_AllocAct(nAct, pGame->dwInitSeed, 0, pGame, pGame->nDifficulty, pGame->pMemoryPool, nTownLevelId, 0, 0);
    ACT2Q3_BrightenEnvironment(pGame, nAct);
}

//D2Game.0x6FC3C580
void __fastcall LEVEL_RemoveAllUnits(Game* pGame)
{
    MONSTER_RemoveAll(pGame);
    OBJECTS_RemoveAll(pGame);
    PLAYER_RemoveAllPlayers(pGame);
    ITEMS_RemoveAll(pGame);
    MISSILES_RemoveAll(pGame);

    D2_ASSERT(pGame);

    UnitAny* pTile = pGame->pTileList;
    while (pTile)
    {
        UnitAny* pNextTile = pTile->pListNext;
        SUNIT_RemoveUnit(pGame, pTile);
        pTile = pNextTile;
    }
    pGame->pTileList = nullptr;
}

//D2Game.0x6FC3C5B0
void __fastcall LEVEL_UpdateQueuedUnitsInAllActs(Game* pGame)
{
    for (int32_t i = 0; i < NUM_ACTS; ++i)
    {
        Act* pAct = pGame->pAct[i];
        if (pAct && pAct->bHasPendingUnitListUpdates)
        {
            for (Room1* pRoom = DUNGEON_GetRoomFromAct(pAct); pRoom; pRoom = pRoom->pRoomNext)
            {
                for (UnitAny* pUnit = pRoom->pUnitUpdate; pUnit; pUnit = pUnit->pChangeNextUnit)
                {
                    sub_6FCBC300(pGame, pUnit);
                }

                UNITROOM_ClearUpdateQueue(pRoom);
            }

            pAct->bHasPendingUnitListUpdates = FALSE;
        }
    }

    if (pGame->pArenaCtrl)
    {
        pGame->pArenaCtrl->fFlags &= 0xFFFFFBFF;
    }
}
