#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Radament
struct Act2Quest1						//sizeof 0x14
{
	uint8_t bRadamentKilled;				//0x00
	uint8_t pad0x01[3];						//0x01
	Room1* pRoom;						//0x04
	uint8_t bAtmaActivated;					//0x08
	uint8_t unk0x09;						//0x09 TODO
	uint8_t bStatusTimerStarted;			//0x0A
	uint8_t pad0x0B;						//0x0B
	int nSkillBookDropCount;				//0x0C
	uint8_t bRewardPending;					//0x10
	uint8_t pad0x11[3];						//0x11
};

#pragma pack()



//D2Game.0x6FC9F5A0
bool __fastcall ACT2Q1_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FC9F610
void __fastcall ACT2Q1_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9F690
void __fastcall ACT2Q1_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC9F770
void __fastcall ACT2Q1_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9F7C0
int32_t __fastcall ACT2Q1_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9F830
void __fastcall ACT2Q1_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9FA20
int32_t __fastcall ACT2Q1_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9FAB0
void __fastcall ACT2Q1_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9FBB0
void __fastcall ACT2Q1_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9FCB0
int32_t __fastcall ACT2Q1_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9FD10
int32_t __fastcall ACT2Q1_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9FE70
bool __fastcall ACT2Q1_Timer_StatusCycler(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FC9FEA0
int32_t __fastcall ACT2Q1_UnitIterate_DetermineSkillBookDropCount(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9FF20
void __fastcall ACT2Q1_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9FFA0
bool __fastcall ACT2Q1_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCA0010
void __fastcall ACT2Q1_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA0120
void __fastcall ACT2Q1_OnRadamentActivated(Game* pGame, UnitAny* pUnit);
//
void __fastcall ACT2Q1_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
