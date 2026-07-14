#include "QUESTS/ACT2/A2Q7.h"

#include <DataTbls/MonsterIds.h>
#include <D2QuestRecord.h>

#include "GAME/Game.h"
#include "QUESTS/Quests.h"


//D2Game.0x6FD37220
NPCMessageTable gpAct2Q7NpcMessages[] =
{
	{
		{
			{ MONSTER_ACT2GUARD4, 59, 0, 1, },
		},
		1
	},
	{
		{
			{ MONSTER_ACT2GUARD4, 60, 0, 1, },
		},
		1
	},
	{
		{
			{ MONSTER_ACT2GUARD4, 61, 0, 1, },
		},
		1
	},
	{
		{
			{ MONSTER_ACT2GUARD4, 62, 0, 1, },
		},
		1
	},
	{
		{
			{ MONSTER_ACT2GUARD4, 63, 0, 1, },
		},
		1
	},
	{
		{
			{ -1, 0, 0, 2 },
		},
		0
	}
};


//D2Game.0x6FCA6420
void __fastcall ACT2Q7_InitQuestData(QuestData* pQuestData)
{
	memset(pQuestData->pfCallback, 0x00, sizeof(pQuestData->pfCallback));
	pQuestData->pfCallback[QUESTEVENT_NPCACTIVATE] = ACT2Q7_Callback00_NpcActivate;
	pQuestData->pfCallback[QUESTEVENT_SCROLLMESSAGE] = ACT2Q7_Callback11_ScrollMessage;
	//pQuestData->pfCallback[8] = (QUESTCALLBACK)OBJECTS_InitFunction05_Door_6FCA6660;
	pQuestData->fState = 0;
	pQuestData->fLastState = 0;
	pQuestData->pNPCMessages = gpAct2Q7NpcMessages;
	pQuestData->bActive = 1;

	Act2Quest7* pQuestDataEx = D2_ALLOC_STRC_POOL(pQuestData->pGame->pMemoryPool, Act2Quest7);
	pQuestData->pQuestDataEx = pQuestDataEx;
	pQuestDataEx->unk0x00 = 0;

	pQuestData->nQuestFilter = QUESTSTATEFLAG_A2Q7;
	pQuestData->pfStatusFilter = ACT2Q7_StatusFilterCallback;
	pQuestData->pfActiveFilter = ACT2Q7_ActiveFilterCallback;
}

//D2Game.0x6FCA64B0
void __fastcall ACT2Q7_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (pQuestArg->nNPCNo != MONSTER_ACT2GUARD4)
	{
		return;
	}

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];
	if (pQuestArg->nMessageIndex == 59 || pQuestArg->nMessageIndex == 60)
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q7, QFLAG_PRIMARYGOALDONE);
		pQuestData->pfCallback[QUESTEVENT_NPCDEACTIVATE] = ACT2Q7_Callback02_NpcDeactivate;
	}
	else
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q7, QFLAG_REWARDGRANTED);
	}
}

//D2Game.0x6FCA6510
void __fastcall ACT2Q7_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (!pQuestArg->pTarget || pQuestArg->pTarget->dwClassId != MONSTER_ACT2GUARD4)
	{
		return;
	}

	Act2Quest7* pQuestDataEx = (Act2Quest7*)pQuestData->pQuestDataEx;
	if (pQuestDataEx->unk0x00 == 1)
	{
		pQuestDataEx->unk0x00 = 0;
		pQuestData->pfCallback[QUESTEVENT_NPCDEACTIVATE] = nullptr;
	}
}

//D2Game.0x6FCA6540
void __fastcall ACT2Q7_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (!pQuestArg->pTarget || pQuestArg->pTarget->dwClassId != MONSTER_ACT2GUARD4)
	{
		return;
	}

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q7, QFLAG_REWARDGRANTED)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q1, QFLAG_REWARDGRANTED)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q1, QFLAG_PRIMARYGOALDONE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q1, QFLAG_REWARDPENDING)
		|| QUESTS_GetGlobalState(pQuestArg->pGame, QUESTSTATEFLAG_A2Q1, QFLAG_PRIMARYGOALDONE))
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_ACT2GUARD4, (uint32_t)SEED_RollRandomNumber(&pQuestArg->pPlayer->pSeed) % 3 + 2);
		return;
	}

	QuestData* pQuestData8 = QUESTS_GetQuestData(pQuestData->pGame, 8);
	
	if (pQuestData8 && !pQuestData8->bNotIntro || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q7, QFLAG_PRIMARYGOALDONE))
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_ACT2GUARD4, (uint32_t)SEED_RollRandomNumber(&pQuestArg->pPlayer->pSeed) % 3 + 2);
	}
	else
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_ACT2GUARD4, SEED_RollRandomNumber(&pQuestArg->pPlayer->pSeed) & 1);
	}
}

//D2Game.0x6FCA6660
void __fastcall OBJECTS_InitFunction05_Door(ObjInitFn* pOp)
{
}

//D2Game.0x6FCA6680
bool __fastcall ACT2Q7_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC)
{
	if (nNpcId != MONSTER_ACT2GUARD4)
	{
		return false;
	}

	BitBuffer* pFlags = UNITS_GetPlayerData(pPlayer)->pQuestData[pQuest->pGame->nDifficulty];
	if (!QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q7, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q7, QFLAG_PRIMARYGOALDONE))
	{
		return true;
	}

	if (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q1, QFLAG_PRIMARYGOALDONE) == 1 && !QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q7, QFLAG_REWARDGRANTED))
	{
		return true;
	}

	return false;
}

//
bool __fastcall ACT2Q7_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus)
{
	return false;
}
