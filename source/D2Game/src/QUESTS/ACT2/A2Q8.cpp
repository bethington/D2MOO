#include "QUESTS/ACT2/A2Q8.h"

#include <DataTbls/MonsterIds.h>
#include <D2QuestRecord.h>

#include "GAME/Game.h"
#include "QUESTS/Quests.h"


//D2Game.0x6FD376B8
NPCMessageTable gpAct2Q8NpcMessages[] =
{
	{
		{
			{ MONSTER_ACT2GUARD5, 303, 0, 1, },
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


//D2Game.0x6FCA66F0
void __fastcall ACT2Q8_InitQuestData(QuestData* pQuestData)
{
	memset(pQuestData->pfCallback, 0x00, sizeof(pQuestData->pfCallback));
	pQuestData->fState = 0;
	pQuestData->fLastState = 0;
	pQuestData->pfCallback[QUESTEVENT_SCROLLMESSAGE] = ACT2Q8_Callback11_ScrollMessage;
	pQuestData->pfCallback[QUESTEVENT_NPCACTIVATE] = ACT2Q8_Callback00_NpcActivate;
	pQuestData->pNPCMessages = gpAct2Q8NpcMessages;
	pQuestData->bActive = 1;

	Act2Quest8* pQuestDataEx = D2_ALLOC_STRC_POOL(pQuestData->pGame->pMemoryPool, Act2Quest8);
	pQuestData->pQuestDataEx = pQuestDataEx;
	pQuestDataEx->unk0x00[0] = 0;
	pQuestDataEx->unk0x00[1] = 0;

	pQuestData->nQuestFilter = QUESTSTATEFLAG_A2Q8;
	pQuestData->pfStatusFilter = ACT2Q8_StatusFilterCallback;
	pQuestData->pfActiveFilter = ACT2Q8_ActiveFilterCallback;
}

//D2Game.0x6FCA6770
void __fastcall ACT2Q8_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (pQuestArg->nNPCNo == MONSTER_ACT2GUARD5 && pQuestArg->nMessageIndex == 303)
	{
		QUESTRECORD_SetQuestState(UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty], QUESTSTATEFLAG_A2Q8, 0);
	}
}

//D2Game.0x6FCA67B0
void __fastcall ACT2Q8_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (!pQuestArg->pTarget || pQuestArg->pTarget->dwClassId != MONSTER_ACT2GUARD5)
	{
		return;
	}

	QuestData* pQuestData13 = QUESTS_GetQuestData(pQuestData->pGame, QUEST_A2Q6_DURIEL);
	if (!pQuestData13 || !pQuestData13->bNotIntro || pQuestData13->fState >= 2)
	{
		return;
	}

	QuestData* pQuestData10 = QUESTS_GetQuestData(pQuestData->pGame, QUEST_A2Q3_TAINTEDSUN);
	if (pQuestData10 && pQuestData10->bNotIntro && pQuestData10->fState < 4)
	{
		return;
	}

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestData->pGame->nDifficulty];
	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q6, QFLAG_REWARDPENDING) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q6, QFLAG_REWARDGRANTED))
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_ACT2GUARD5, 0);
	}
}

//D2Game.0x6FCA6850
bool __fastcall ACT2Q8_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC)
{
	if (nNpcId != MONSTER_ACT2GUARD5)
	{
		return false;
	}

	BitBuffer* pFlags = UNITS_GetPlayerData(pPlayer)->pQuestData[pQuest->pGame->nDifficulty];
	QuestData* pQuestData = QUESTS_GetQuestData(pQuest->pGame, 13);
	if (!pQuestData)
	{
		return false;
	}

	return pQuestData->bNotIntro && pQuestData->fState < 2
		&& QUESTS_GetGlobalState(pQuest->pGame, QUESTSTATEFLAG_A2Q3, QFLAG_PRIMARYGOALDONE)
		&& !QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q6, QFLAG_REWARDPENDING)
		&& !QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q6, QFLAG_REWARDGRANTED);
}

//
bool __fastcall ACT2Q8_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus)
{
	return false;
}
