#include "DEBUG/Debug.h"

#include <D2Dungeon.h>


#include "GAME/Game.h"
#include "UNIT/SUnit.h"


//D2Game.0x6FCCD2E0 (#10025)
UnitAny* __stdcall DEBUG_GetUnitFromTypeAndGUID(int32_t nUnitType, int32_t nUnitGUID)
{
    Game* pGame = sub_6FC3B160();
    if (!pGame)
    {
        return nullptr;
    }

    UnitAny* pUnit = SUNIT_GetServerUnit(pGame, nUnitType, nUnitGUID);
    GAME_LeaveGlobalGamesCriticalSection();
    return pUnit;
}

//D2Game.0x6FCCD310 (#10026)
Act* __stdcall DEBUG_GetAct(uint8_t nAct)
{
    Game* pGame = sub_6FC3B160();

    D2_ASSERT(pGame);

    Act* pAct = pGame->pAct[nAct];
    GAME_LeaveGlobalGamesCriticalSection();
    return pAct;
}

//D2Game.0x6FCCD350 (#10027)
int32_t __stdcall DEBUG_GetFreeMonsterIndex()
{
    Game* pGame = sub_6FC3B160();

    D2_ASSERT(pGame);

    int32_t nIndex = 0;
    for (int32_t i = 0; i < 128; ++i)
    {
        for (UnitAny* pUnit = pGame->pUnitList[1][i]; pUnit; pUnit = SUNIT_GetNextUnitFromList(pUnit))
        {
            ++nIndex;
        }
    }

    GAME_LeaveGlobalGamesCriticalSection();
    return nIndex;
}

//D2Game.0x6FCCD3B0 (#10028)
Room1* __stdcall DEBUG_GetRoomBySubtileCoordinates(uint8_t nAct, int32_t nX, int32_t nY)
{
    Game* pGame = sub_6FC3B160();

    D2_ASSERT(nAct < NUM_ACTS);

    Room1* pRoom = DUNGEON_FindRoomBySubtileCoordinates(pGame->pAct[nAct], nX, nY);
    GAME_LeaveGlobalGamesCriticalSection();
    return pRoom;
}
