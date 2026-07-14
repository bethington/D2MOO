#pragma once

#include <Units/Units.h>

#pragma pack(1)

enum EventFlags : uint16_t
{
	EVENTFLAG_0x1 = 0x1, // Some kind of lock or to prevent reentrancy ?
	EVENTFLAG_0x2 = 0x2,
	EVENTFLAG_0x4 = 0x4,
	EVENTFLAG_0x8 = 0x8,
};

enum EventTypes : uint8_t
{
	EVENTTYPE_MODECHANGE = 0,
	EVENTTYPE_ENDANIM = 1,
	EVENTTYPE_AITHINK = 2,
	EVENTTYPE_STATREGEN = 3,
	EVENTTYPE_TRAP = 4,
	EVENTTYPE_ACTIVESTATE = 5,
	EVENTTYPE_RESET = 5,
	EVENTTYPE_FREEHOVER = 6,
	EVENTTYPE_MONUMOD = 7,
	EVENTTYPE_QUESTFN = 7,
	EVENTTYPE_PERIODICSKILLS = 8,
	EVENTTYPE_PERIODICSTATS = 9,
	EVENTTYPE_AIRESET = 10,
	EVENTTYPE_DELAYEDPORTAL = 11,
	EVENTTYPE_REMOVESTATE = 12,
	EVENTTYPE_UPDATETRADE = 13,
	EVENTTYPE_REFRESHVENDOR = 13,
	EVENTTYPE_REMOVESKILLCOOLDOWN = 14,
	EVENTTYPE_COUNT = 15,
	EVENTTYPE_CUSTOM = 16,
};


struct TimerArg2
{
	int nUnitGUID;
	TimerArg2* pNext;
};

struct TimerArg
{
	TimerArg2* unk0x00;
	uint32_t dwUnitId;
};

using EventTimerCallback = int32_t(__fastcall*)(Game*, UnitAny*, EventTypes, int32_t, int32_t);
struct EventTimer
{
	EventTypes nEventType;
	int8_t padding0x01;
	uint16_t nFlags;
	int32_t nExpireFrame;
	UnitAny* pUnit;
	int32_t nUnitGUID;
	int32_t nUnitType;
	int32_t dwEventCustomId;
	int32_t dwEventCustomParam;
	EventTimer* pNextFreeEventTimer;
	EventTimer* unk0x20;
	EventTimer* pNext;
	EventTimer* pPrevious;
	EventTimerCallback pCallback;
};

struct EventTimerSlabList
{
	EventTimer tEventTimersStorage[600];
	EventTimer* pFreeEventTimerListHead;
	EventTimerSlabList* pNextSlab;
};

struct EventTimerQueue
{
	int32_t nArrayIndex;
	EventTimer* unk0x04[10][64];
	EventTimer* unk0xA04[5];
	EventTimer* unk0xA18;
	EventTimerSlabList* pSlabListHead;
};



#pragma pack()

//D2Game.0x6FC34840
void __fastcall D2GAME_EVENTS_Delete_6FC34840(Game* pGame, UnitAny* pUnit, EventTypes nEventType, int32_t nEventCustomId);
//D2Game.0x6FC34890
void __fastcall sub_6FC34890(Game* pGame, EventTimer* pTimer);
//D2Game.0x6FC349B0
void __fastcall sub_6FC349B0(Game* pGame, UnitAny* pUnit, int32_t nEvent, EventTimerCallback pCallback);
//D2Game.0x6FC349F0
void __fastcall sub_6FC349F0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC34A30
void __fastcall D2GAME_DeleteTimersOnUnit_6FC34A30(Game* pGame, UnitAny* pUnit);
//1.10f: D2Game.0x6FC34A80
//1.13c: D2Game.6FCAE4D0
void __fastcall EVENT_FreeEventQueue(Game* pGame);
//D2Game.0x6FC34AE0
void __fastcall EVENT_AllocEventQueue(Game* pGame);
//D2Game.0x6FC34BD0
void __fastcall EVENT_IterateEvents(Game* pGame);
//D2Game.0x6FC34CC0
void __fastcall EVENT_ExecutePlayerEvents(Game* pGame, EventTimerQueue* pTimerQueue, EventTimer* pEventTimer, int32_t a4);
//D2Game.0x6FC34DB0
void __fastcall EVENT_ExecuteMonsterEvents(Game* pGame, EventTimerQueue* pTimerQueue, EventTimer* pEventTimer, int32_t a4);
//D2Game.0x6FC34EA0
void __fastcall EVENT_ExecuteObjectEvents(Game* pGame, EventTimerQueue* pTimerQueue, EventTimer* pEventTimer, int32_t a4);
//D2Game.0x6FC34F90
void __fastcall EVENT_ExecuteMissileEvents(Game* pGame, EventTimerQueue* pTimerQueue, EventTimer* pEventTimer, int32_t a4);
//D2Game.0x6FC35080
void __fastcall EVENT_ExecuteItemEvents(Game* pGame, EventTimerQueue* pTimerQueue, EventTimer* pEventTimer, int32_t a4);
//D2Game.0x6FC35170
int32_t __fastcall EVENT_GetEventFrame(Game* pGame, UnitAny* pUnit, int32_t nEvent);
//D2Game.0x6FC351B0
void __fastcall EVENT_SetEvent(Game* pGame, UnitAny* pUnit, EventTypes nEventType, int32_t nExpireFrame, int32_t dwEventCustomId, int32_t dwEventCustomParam);
//D2Game.0x6FC351D0
void __fastcall D2GAME_InitTimer_6FC351D0(Game* pGame, UnitAny* pUnit, EventTypes nEventType, int32_t nExpireFrame, EventTimerCallback pfCallBack, int32_t dwEventCustomId, int32_t dwEventCustomParam);
//D2Game.0x6FC353D0
EventTimer** __fastcall sub_6FC353D0(Game* pGame, int32_t nUnitType, int32_t nExpireFrame);
//1.10f: D2Game.0x6FC35410
//1.13c: D2Game.0x6FCAE420
EventTimerSlabList* __fastcall EVENT_AllocTimerSlab(Game* pGame);
//D2Game.0x6FC35460
EventTimer* __fastcall EVENT_AllocateUnitTimer(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC35570
void __fastcall sub_6FC35570(Game* pGame, UnitAny* pUnit, EventTypes nEventType, int32_t dwEventCustomId, int32_t dwEventCustomParam);
//D2Game.0x6FC351D0
void __fastcall j_D2GAME_InitTimer_6FC351D0(Game* pGame, UnitAny* pUnit, EventTypes nEventType, int32_t nExpireFrame, EventTimerCallback pfCallBack, int32_t nSkillId, int32_t nSkillLevel);
