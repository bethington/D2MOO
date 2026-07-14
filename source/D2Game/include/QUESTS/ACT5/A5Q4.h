#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Nihlathak
struct Act5Quest4						//sizeof 0x90
{
	QuestGUID tPlayerGUIDs;					//0x00
	uint8_t unused0x84;						//0x84
	uint8_t bTimerActive;					//0x85
	uint8_t bDrehyaActivated;				//0x86
	uint8_t bNeedsToOpenPortal;				//0x87
	uint8_t bPortalCreated;					//0x88
	uint8_t bWaypointNotActivated;			//0x89
	uint8_t pad0x8A[2];						//0x8A
	Room1* pRoom;						//0x8C
};

#pragma pack()



//D2Game.0x6FCB59B0
void __fastcall ACT5Q4_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB5A80
bool __fastcall ACT5Q4_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCB5C00
void __fastcall ACT5Q4_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCB5CD0
bool __fastcall ACT5Q4_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
//D2Game.0x6FCB5D60
void __fastcall ACT5Q4_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB5E70
int32_t __fastcall ACT5Q4_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB5EE0
bool __fastcall ACT5Q4_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCB5F70
void __fastcall ACT5Q4_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB6100
int32_t __fastcall ACT5Q4_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB6190
void __fastcall ACT5Q4_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB6320
void __fastcall ACT5Q4_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB63F0
int32_t __fastcall ACT5Q4_UnitIterate_SetRewardPending(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB6500
int32_t __fastcall ACT5Q4_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB6550
int32_t __fastcall ACT5Q4_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB65C0
int32_t __fastcall ACT5Q4_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB6600
bool __fastcall ACT5Q4_Timer_StatusCycler(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FCB6630
void __fastcall ACT5Q4_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB6750
void __fastcall ACT5Q4_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB67C0
void __fastcall ACT5Q4_SetRewardGranted(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCB6800
void __fastcall ACT5Q4_OnNihlathakActivated(Game* pGame);
//D2Game.0x6FCB6840
void __fastcall ACT5Q4_AnyaOpenPortal(Game* pGame, UnitAny* pUnit);
//
void __fastcall ACT5Q4_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
