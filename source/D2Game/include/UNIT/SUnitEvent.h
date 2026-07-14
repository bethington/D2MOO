#pragma once

#include <Units/Units.h>
#include "GAME/Event.h"
#include "SUnitDmg.h"

using D2UnitEventCallbackFunction = int32_t(__fastcall*)(Game*, int32_t, UnitAny*, UnitAny*, Damage*, int32_t, int32_t);

enum UnitEventTypes: uint8_t // Events.txt
{
	UNITEVENT_HITBYMISSILE,
	UNITEVENT_DAMAGEDINMELEE,
	UNITEVENT_DAMAGEDBYMISSILE,
	UNITEVENT_ATTACKEDINMELEE,
	UNITEVENT_DOACTIVE,
	UNITEVENT_DOMELEEDMG,
	UNITEVENT_DOMISSILEDMG,
	UNITEVENT_DOMELEEATTACK,
	UNITEVENT_DOMISSILEATTACK,
	UNITEVENT_KILL,
	UNITEVENT_KILLED,
	UNITEVENT_ABSORBDAMAGE,
	UNITEVENT_LEVELUP,
	UNITEVENT_DEATH, // Monster dies
};

enum UnitEventFlags : uint16_t {
	UNITEVENTFLAG_IN_CALLBACK = 1 << 0, // Prevents destruction during callback
	UNITEVENTFLAG_SHOULD_BE_FREED = 1 << 1,
};

struct UnitEvent
{
	UnitEventTypes nUnitEvent;			//0x00 Event from events.txt. NOT the same thing as timer events
	uint8_t unk0x01;						//0x01
	uint16_t nFlags;						//0x02 UnitEventFlags
	uint32_t nQueueNo;						//0x04
	int32_t unk0x08;						//0x08
	int32_t nGUID1;							//0x0C First identifier
	int32_t nGUID2;							//0x10 Second identifier
	D2UnitEventCallbackFunction pCallback;	//0x14
	UnitEvent* pPrevious;				//0x18  Seems to be any kind of id, not necessariyl the prevtimer ?
	UnitEvent* pNext;					//0x1C
};

//D2Game.0x6FCC3610
void __fastcall SUNITEVENT_FreeEventList(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCC3650
UnitEvent* __fastcall SUNITEVENT_Register(Game* pGame, UnitAny* pUnit, UnitEventTypes nUnitEvent, int32_t nGUID1, int32_t nGUID2, D2UnitEventCallbackFunction pCallback, uint32_t nQueueNo, int32_t a8);
//D2Game.0x6FCC36D0
void __fastcall SUNITEVENT_Unregister(Game* pGame, UnitAny* pUnit, int32_t nTimerQueueNo, int32_t a4);
//D2Game.0x6FCC3750
UnitEvent* __fastcall SUNITEVENT_GetEvent(Game* pGame, UnitAny* pUnit, int32_t nTimerQueueNo, int32_t a4, int32_t nGUID1);
//D2Game.0x6FCC3790
void __fastcall SUNITEVENT_Trigger(Game* pGame, UnitEventTypes nUnitEvent, UnitAny* pUnit, UnitAny* pEventSourceUnit, Damage* pDamage);
