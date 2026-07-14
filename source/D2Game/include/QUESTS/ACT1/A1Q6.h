#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Andariel
struct Act1Quest6			//sizeof 0x198
{
	QuestGUID pGUID1;		//0x00
	QuestGUID pGUID2;		//0x84
	QuestGUID pGUID3;		//0x108
	int nAndarielGUID;			//0x18C
	short unk0x190;				//0x190 TODO
	short nTimerInvocations;	//0x192
	uint8_t bAndarielKilled;	//0x194
	uint8_t bCainActivated;		//0x195
	short pad0x196;				//0x196
};

#pragma pack()



//D2Game.0x6FC9DFF0
void __fastcall ACT1Q6_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9E0E0
bool __fastcall ACT1Q6_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FC9E1B0
void __fastcall ACT1Q6_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FC9E2B0
void __fastcall ACT1Q6_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9E300
int32_t __fastcall ACT1Q6_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9E370
void __fastcall ACT1Q6_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9E510
int32_t __fastcall ACT1Q6_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9E5A0
void __fastcall ACT1Q6_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9E7A0
void __fastcall ACT1Q6_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9E900
void __fastcall ACT1Q6_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FC9EB50
int32_t __fastcall ACT1Q6_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9EBA0
void __fastcall ACT1Q6_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pArgs);
//D2Game.0x6FC9EBF0
int32_t __fastcall ACT1Q6_UnitIterate_SetRewardPending(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9ECD0
int32_t __fastcall ACT1Q6_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9ED30
int32_t __fastcall ACT1Q6_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9ED80
bool __fastcall ACT1Q6_Timer_StatusCycler(Game* pGame, QuestData* pQuest);
//D2Game.0x6FC9EE00
int32_t __fastcall ACT1Q6_UnitIterate_CreatePortalToTown(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FC9EE70
bool __fastcall ACT1Q6_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FC9EE90
bool __fastcall ACT1Q6_Timer_StateDebug(Game* pGame, QuestData* pQuestData);
//D2Game.0x6FC9EEB0
void __fastcall ACT1Q6_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
