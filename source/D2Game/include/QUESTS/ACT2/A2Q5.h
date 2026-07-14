#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Summoner
struct Act2Quest5						//sizeof 0x0C
{
	uint8_t bSummonerKilled;				//0x00
	uint8_t unk0x01;						//0x01 TODO
	uint8_t pad0x02[2];						//0x02
	Room1* pRoom;						//0x04
	uint8_t bStatusTimerStarted;			//0x08
	uint8_t unk0x09;						//0x09 TODO
	uint8_t pad0x0A[2];						//0x0A
};

#pragma pack()


//D2Game.0x6FCA3DB0
bool __fastcall ACT2Q5_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCA3E20
void __fastcall ACT2Q5_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA3EA0
void __fastcall ACT2Q5_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCA3F50
void __fastcall ACT2Q5_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA4040
void __fastcall ACT2Q5_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA4140
int32_t __fastcall ACT2Q5_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA41B0
void __fastcall ACT2Q5_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA4270
int32_t __fastcall ACT2Q5_UnitIterate_SetRewardPending(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA4380
bool __fastcall ACT2Q5_Timer_StatusCycler(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FCA43D0
int32_t __fastcall ACT2Q5_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA4420
int32_t __fastcall ACT2Q5_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA4470
int32_t __fastcall ACT2Q5_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA44D0
void __fastcall ACT2Q5_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA4500
void __fastcall ACT2Q5_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA4560
void __fastcall ACT2Q5_OnSummonerActivated(Game* pGame);
//D2Game.0x6FCA45E0
int32_t __fastcall ACT2Q5_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//
void __fastcall ACT2Q5_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
