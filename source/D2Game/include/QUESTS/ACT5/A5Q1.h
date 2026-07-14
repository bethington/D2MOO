#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//Siege
struct Act5Quest1						//sizeof 0x18
{
	MapAI* pLarzukMapAI;					//0x00
	Room1* pRoom;							//0x04
	uint8_t bLarzukStartActivated;					//0x08
	uint8_t unused0x09[7];							//0x09
	int nLarzukGUID;							//0x10
	uint8_t unused0x14;							//0x14
	uint8_t bLarzukSpawned;						//0x15
	uint8_t bLarzukEndActivated;					//0x16
	uint8_t bLarzukMapAIChanged;					//0x17
};

#pragma pack()



//D2Game.0x6FCB1200
void __fastcall ACT5Q1_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB1280
bool __fastcall ACT5Q1_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCB1300
void __fastcall ACT5Q1_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCB13D0
bool __fastcall ACT5Q1_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
//D2Game.0x6FCB1470
void __fastcall ACT5Q1_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB14D0
int32_t __fastcall ACT5Q1_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB1540
void __fastcall ACT5Q1_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB16B0
int32_t __fastcall ACT5Q1_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB1740
void __fastcall ACT5Q1_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB1830
void __fastcall ACT5Q1_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB18E0
int32_t __fastcall ACT5Q1_UnitIterate_SetRewardPending(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB19A0
int32_t __fastcall ACT5Q1_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB19F0
int32_t __fastcall ACT5Q1_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB1A60
int32_t __fastcall ACT5Q1_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCB1AA0
void __fastcall ACT5Q1_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB1BA0
bool __fastcall ACT5Q1_SeqCallback(QuestData* pQuestData);
//D2Game.0x6FCB1C10
void __fastcall ACT5Q1_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCB1D10
void __fastcall ACT5Q1_SetRewardGranted(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCB1D90
void __fastcall OBJECTS_InitFunction71_LarzukStandard(ObjInitFn* pOp);
//D2Game.0x6FCB1E60
void __fastcall ACT5Q1_OnSiegeBossActivated(Game* pGame, UnitAny* pUnit);
//
void __fastcall ACT5Q1_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
//Inlined in D2Game.0x6FC975A0
void __fastcall ACT5Q1_ChangeLarzukMapAI(Game* pGame, UnitAny* pUnit, MapAI* pMapAi);
