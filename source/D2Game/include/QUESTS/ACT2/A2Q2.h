#pragma once

#include <QUESTS/Quests.h>

#pragma pack(1)

//HoradricStaff
struct Act2Quest2						//sizeof 0x30
{
	int unused0x00[5];						//0x00
	int nItemDropCount;						//0x14
	int nBaseStaffs;						//0x18
	int nCubedStaffs;						//0x1C
	int nHoradricCubes;						//0x20
	int nAmulets;							//0x24
	uint8_t bHoradricScrollDropped;			//0x28
	uint8_t bCubeDropped;					//0x29
	uint8_t bStaffCubed;					//0x2A
	uint8_t unk0x2B;						//0x2B TODO
	int nPlayerGUID;						//0x2C
};

#pragma pack()



//D2Game.0x6FCA01F0
bool __fastcall ACT2Q2_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC);
//D2Game.0x6FCA0240
bool __fastcall ACT2Q2_CheckItemsAndState(QuestData* pQuestData, UnitAny* pUnit, uint8_t* pData);
//D2Game.0x6FCA0370
void __fastcall ACT2Q2_UpdateHoradricItemCounts(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCA0490
bool __fastcall ACT2Q2_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus);
//D2Game.0x6FCA06C0
void __fastcall ACT2Q2_InitQuestData(QuestData* pQuestData);
//D2Game.0x6FCA07A0
void __fastcall ACT2Q2_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA08C0
void __fastcall ACT2Q2_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA09C0
void __fastcall ACT2Q2_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA09D0
void __fastcall ACT2Q2_Callback04_ItemPickedUp(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA0CF0
int32_t __fastcall ACT2Q2_IsQuestDone(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCA0D50
void __fastcall ACT2Q2_Callback05_ItemDropped(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA0DF0
void __fastcall ACT2Q2_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA0F10
void __fastcall ACT2Q2_Callback14_PlayerJoinedGame(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA0FC0
void __fastcall ACT2Q2_Callback09_PlayerDroppedWithQuestItem(QuestData* pQuestData, QuestArg* pQuestArg);
//D2Game.0x6FCA10B0
int32_t __fastcall OBJECTS_OperateFunction40_HoradricScrollChest(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FCA1180
int32_t __fastcall ACT2Q2_UnitIterate_DetermineHoradricScrollDropCount(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA11D0
int32_t __fastcall OBJECTS_OperateFunction41_StaffOfKingsChest(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FCA12C0
int32_t __fastcall ACT2Q2_UnitIterate_DetermineStaffDropCount(Game* pGame, UnitAny* pUnit, void* pData);
//D2Game.0x6FCA1330
int32_t __fastcall OBJECTS_OperateFunction39_HoradricCubeChest(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FCA1410
int32_t __fastcall ACT2Q2_UnitIterate_DetermineCubeDropCount(Game* pGame, UnitAny* pUnit, void* pData);
//
void __fastcall ACT2Q2_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg);
