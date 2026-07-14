#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//BloodRaven
struct Act1Quest2						//sizeof 0x0C
{
	uint8_t unk0x00;								//0x00 TODO
	uint8_t unk0x01;								//0x01 TODO
	uint8_t bBloodravenKilled;						//0x02
	uint8_t bKashyaActivated;						//0x03
	int unk0x04;								//0x04 TODO
	int nBloodravenGUID;						//0x08
};

#pragma pack()

//D2Game.0x6FC98450
void __fastcall ACT1Q2_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC984C0
bool __fastcall ACT1Q2_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FC98530
void __fastcall ACT1Q2_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC98600
void __fastcall ACT1Q2_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC98660
int32_t __fastcall ACT1Q2_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC986D0
int32_t __fastcall ACT1Q2_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC98760
void __fastcall ACT1Q2_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC988F0
void __fastcall ACT1Q2_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC989D0
void __fastcall ACT1Q2_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC98AB0
bool __fastcall ACT1Q2_Timer_StatusCycler(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FC98AD0
int32_t __fastcall ACT1Q2_UnitIterate_SetRewardPending(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC98C00
int32_t __fastcall ACT1Q2_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC98C60
int32_t __fastcall ACT1Q2_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC98CA0
int32_t __fastcall ACT1Q2_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC98CF0
void __fastcall ACT1Q2_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC98DD0
bool __fastcall ACT1Q2_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FC98E60
void __fastcall ACT1Q2_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//
void __fastcall ACT1Q2_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
