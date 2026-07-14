#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//DenOfEvil
struct Act1Quest1						//sizeof 0x8C
{
	QuestGUID pQuestGUID;					//0x00
	uint8_t bFinished;								//0x84
	uint8_t bEnteredDen;							//0x85
	uint8_t bAkaraActivated;						//0x86
	uint8_t bTimerActive;							//0x87
	int nMonstersLeft;							//0x88
};

#pragma pack()



//D2Game.0x6FC97920
void __fastcall ACT1Q1_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC97990
int32_t __fastcall ACT1Q1_GetMonstersToBeKilled(QuestData* pQuestData);
//D2Game.0x6FC979A0
bool __fastcall ACT1Q1_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FC97A10
void __fastcall ACT1Q1_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC97AE0
void __fastcall ACT1Q1_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC97B30
int32_t __fastcall ACT1Q1_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC97BA0
void __fastcall ACT1Q1_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC97D30
int32_t __fastcall ACT1Q1_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC97DC0
void __fastcall ACT1Q1_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC97ED0
void __fastcall ACT1Q1_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC980C0
int32_t __fastcall ACT1Q1_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC98120
int32_t __fastcall ACT1Q1_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC98160
int32_t __fastcall ACT1Q1_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC981B0
bool __fastcall ACT1Q1_Timer_StatusCycler(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FC981E0
void __fastcall ACT1Q1_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC98330
bool __fastcall ACT1Q1_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FC983A0
void __fastcall ACT1Q1_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC98430
bool __fastcall ACT1Q1_CanClientFXBeTriggered(QuestData* pQuestData);
//
void __fastcall ACT1Q1_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
