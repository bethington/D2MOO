#include "UNIT/SUnitEvent.h"

#include <Fog.h>

#include "GAME/Game.h"

//D2Game.0x6FCC3610
void __fastcall SUNITEVENT_FreeEventList(Game* pGame, UnitAny* pUnit)
{
    if (!pUnit)
    {
        return;
    }

    UnitEvent* pUnitEvent = pUnit->pSrvUnitEventsList;
    while (pUnitEvent)
    {
        UnitEvent* pNext = pUnitEvent->pNext;
        D2_FREE_POOL(pGame->pMemoryPool, pUnitEvent);
        pUnitEvent = pNext;
    }

    pUnit->pSrvUnitEventsList = nullptr;
}

//D2Game.0x6FCC3650
UnitEvent* __fastcall SUNITEVENT_Register(Game* pGame, UnitAny* pUnit, UnitEventTypes nUnitEvent, int32_t nGUID1, int32_t nGUID2, D2UnitEventCallbackFunction pCallback, uint32_t nQueueNo, int32_t a8)
{
    if (!pUnit)
    {
        return nullptr;
    }

    UnitEvent* pUnitEvent = D2_CALLOC_STRC_POOL(pGame->pMemoryPool, UnitEvent);

    pUnitEvent->nUnitEvent = nUnitEvent;
    pUnitEvent->nGUID1 = nGUID1;
    pUnitEvent->nGUID2 = nGUID2;
    pUnitEvent->pCallback = pCallback;
    pUnitEvent->pNext = pUnit->pSrvUnitEventsList;
    pUnitEvent->nQueueNo = nQueueNo;
    pUnitEvent->unk0x08 = a8;

    if (pUnit->pSrvUnitEventsList)
    {
        pUnit->pSrvUnitEventsList->pPrevious = pUnitEvent;
    }

    pUnit->pSrvUnitEventsList = pUnitEvent;
    return pUnitEvent;
}

// Helper function
static void __fastcall SUNITEVENT_FreeEvent(Game* pGame, UnitAny* pUnit, UnitEvent* pUnitEvent)
{
	UnitEvent* pNext = pUnitEvent->pNext;
	if (pUnitEvent->pPrevious)
	{
		pUnitEvent->pPrevious->pNext = pNext;
	}
	else
	{
		pUnit->pSrvUnitEventsList = pNext;
	}

	if (pUnitEvent->pNext)
	{
		pUnitEvent->pNext->pPrevious = pUnitEvent->pPrevious;
	}

	D2_FREE_POOL(pGame->pMemoryPool, pUnitEvent);
}

//D2Game.0x6FCC36D0
void __fastcall SUNITEVENT_Unregister(Game* pGame, UnitAny* pUnit, int32_t nTimerQueueNo, int32_t a4)
{
    if (!pUnit)
    {
        return;
    }

    UnitEvent* pUnitEvent = pUnit->pSrvUnitEventsList;
    while (pUnitEvent)
    {
        UnitEvent* pNext = pUnitEvent->pNext;
        if (pUnitEvent->nQueueNo == nTimerQueueNo && pUnitEvent->unk0x08 == a4)
        {
            if (pUnitEvent->nFlags & UNITEVENTFLAG_IN_CALLBACK)
            {
                pUnitEvent->nFlags |= UNITEVENTFLAG_SHOULD_BE_FREED;
            }
            else
            {
				SUNITEVENT_FreeEvent(pGame, pUnit, pUnitEvent);
            }
        }

        pUnitEvent = pNext;
    }
}

//D2Game.0x6FCC3750
UnitEvent* __fastcall SUNITEVENT_GetEvent(Game* pGame, UnitAny* pUnit, int32_t nTimerQueueNo, int32_t a4, int32_t nGUID1)
{
    if (!pUnit)
    {
        return nullptr;
    }

    for (UnitEvent* pUnitEvent = pUnit->pSrvUnitEventsList; pUnitEvent; pUnitEvent = pUnitEvent->pNext)
    {
        if (pUnitEvent->nQueueNo == nTimerQueueNo && pUnitEvent->unk0x08 == a4 && pUnitEvent->nGUID1 == nGUID1)
        {
            return pUnitEvent;
        }
    }

    return nullptr;
}

//D2Game.0x6FCC3790
void __fastcall SUNITEVENT_Trigger(Game* pGame, UnitEventTypes nUnitEvent, UnitAny* pUnit, UnitAny* pEventSourceUnit, Damage* pDamage)
{
    if (!pUnit)
    {
        return;
    }

    UnitEvent* pUnitEvent = pUnit->pSrvUnitEventsList;
    while (pUnitEvent)
    {
        UnitEvent* pNext = pUnitEvent->pNext;
        if (pUnitEvent->nUnitEvent == nUnitEvent)
        {
            pUnitEvent->nFlags |= UNITEVENTFLAG_IN_CALLBACK;
            pUnitEvent->pCallback(pGame, nUnitEvent, pUnit, pEventSourceUnit, pDamage, pUnitEvent->nGUID1, pUnitEvent->nGUID2);
            pUnitEvent->nFlags &= ~UNITEVENTFLAG_IN_CALLBACK;

            if ((pUnitEvent->nFlags & UNITEVENTFLAG_SHOULD_BE_FREED) || !pUnitEvent->nQueueNo)
            {
				SUNITEVENT_FreeEvent(pGame, pUnit, pUnitEvent);
            }
        }
        pUnitEvent = pNext;
    }
}
