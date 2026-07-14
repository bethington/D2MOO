#include "QUESTS/ACT5/A5Q4.h"

#include <DataTbls/LevelsIds.h>
#include <DataTbls/MonsterIds.h>
#include <DataTbls/ObjectsIds.h>
#include <Drlg/D2DrlgDrlg.h>
#include <D2Dungeon.h>
#include <D2QuestRecord.h>
#include <D2Waypoints.h>

#include "GAME/Clients.h"
#include "GAME/Game.h"
#include "QUESTS/Quests.h"
#include "QUESTS/ACT5/A5Q3.h"
#include "SKILLS/Skills.h"
#include "UNIT/Party.h"
#include "UNIT/SUnit.h"


//D2Game.0x6FD3D828
NPCMessageTable gpAct5Q4NpcMessages[] =
{
	{
		{
			{ MONSTER_DREHYA, 20137, 0, 0, },
		},
		1
	},
	{
		{
			{ MONSTER_LARZUK, 20141, 0, 2, },
			{ MONSTER_CAIN6, 20139, 0, 2, },
			{ MONSTER_MALAH, 20140, 0, 2, },
			{ MONSTER_QUAL_KEHK, 20142, 0, 2, },
		},
		4
	},
	{
		{
			{ MONSTER_LARZUK, 20145, 0, 2, },
			{ MONSTER_CAIN6, 20144, 0, 2, },
			{ MONSTER_NIHLATHAK, 20143, 0, 2, },
			{ MONSTER_MALAH, 20146, 0, 2, },
			{ MONSTER_QUAL_KEHK, 20147, 0, 2, },
		},
		5
	},
	{
		{
			{ MONSTER_LARZUK, 20150, 0, 2, },
			{ MONSTER_CAIN6, 20149, 0, 2, },
			{ MONSTER_NIHLATHAK, 20148, 0, 0, },
			{ MONSTER_MALAH, 20151, 0, 2, },
			{ MONSTER_QUAL_KEHK, 20152, 0, 2, },
		},
		5
	},
	{
		{
			{ MONSTER_LARZUK, 20150, 0, 2, },
			{ MONSTER_CAIN6, 20149, 0, 2, },
			{ MONSTER_NIHLATHAK, 20148, 0, 2, },
			{ MONSTER_MALAH, 20151, 0, 2, },
			{ MONSTER_QUAL_KEHK, 20152, 0, 2, },
		},
		5
	},
	{
		{
			{ -1, 0, 0, 2 },
		},
		0
	},
	{
		{
			{ -1, 0, 0, 2 },
		},
		0
	},
	{
		{
			{ -1, 0, 0, 2 },
		},
		0
	}
};


//D2Game.0x6FCB59B0
void __fastcall ACT5Q4_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED) == 1 || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_COMPLETEDNOW) == 1)
	{
		return;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING) == 1 || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE) == 1)
	{
		return;
	}

	Room1* pRoom = UNITS_GetRoom(pUnit);
	if (!pRoom)
	{
		return;
	}

	const int32_t nLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);
	if (!nLevelId || DRLG_GetActNoFromLevelId(nLevelId) != ACT_V || (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDPENDING)))
	{
		return;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING))
	{
		return;
	}

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING);
	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE);
}

//D2Game.0x6FCB5A80
bool __fastcall ACT5Q4_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC)
{
	if (nNpcId != MONSTER_DREHYA)
	{
		return false;
	}

	if (pQuest->fState == 1)
	{
		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDPENDING)
			|| QUESTRECORD_GetQuestState(pQuestFlags, pQuest->nQuestFilter, QFLAG_REWARDGRANTED))
		{
			return false;
		}

		if (!QUESTRECORD_GetQuestState(pQuestFlags, pQuest->nQuestFilter, QFLAG_REWARDPENDING))
		{
			return true;
		}
	}
	else
	{
		if (!QUESTRECORD_GetQuestState(pQuestFlags, pQuest->nQuestFilter, QFLAG_PRIMARYGOALDONE))
		{
			return false;
		}

		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_ENTERAREA))
		{
			return true;
		}
	}

	return false;
}

//D2Game.0x6FCB5C00
void __fastcall ACT5Q4_InitQuestData(QuestData* pQuestData)
{
	memset(pQuestData->pfCallback, 0x00, sizeof(pQuestData->pfCallback));
	pQuestData->fState = 0;
	pQuestData->pfCallback[QUESTEVENT_PLAYERLEAVESGAME] = ACT5Q4_Callback10_PlayerLeavesGame;
	pQuestData->pfCallback[QUESTEVENT_CHANGEDLEVEL] = ACT5Q4_Callback03_ChangedLevel;
	pQuestData->pfCallback[QUESTEVENT_MONSTERKILLED] = ACT5Q4_Callback08_MonsterKilled;
	pQuestData->pfCallback[QUESTEVENT_NPCACTIVATE] = ACT5Q4_Callback00_NpcActivate;
	pQuestData->pfCallback[QUESTEVENT_NPCDEACTIVATE] = ACT5Q4_Callback02_NpcDeactivate;
	pQuestData->pfCallback[QUESTEVENT_SCROLLMESSAGE] = ACT5Q4_Callback11_ScrollMessage;
	pQuestData->pfCallback[QUESTEVENT_PLAYERSTARTEDGAME] = ACT5Q4_Callback13_PlayerStartedGame;
	pQuestData->pNPCMessages = gpAct5Q4NpcMessages;
	pQuestData->bActive = 1;
	
	Act5Quest4* pQuestDataEx = D2_ALLOC_STRC_POOL(pQuestData->pGame->pMemoryPool, Act5Quest4);
	pQuestData->pQuestDataEx = pQuestDataEx;
	pQuestData->nQuestFilter = QUESTSTATEFLAG_A5Q4;
	pQuestData->pfStatusFilter = ACT5Q4_StatusFilterCallback;
	pQuestData->nInitNo = 4;
	pQuestData->pfActiveFilter = ACT5Q4_ActiveFilterCallback;
	pQuestData->pfSeqFilter = ACT5Q4_SeqCallback;
	pQuestData->nSeqId = 35;
	memset(pQuestDataEx, 0x00, sizeof(Act5Quest4));
	QUESTS_ResetPlayerGUIDCount(&pQuestDataEx->tPlayerGUIDs);
}

//D2Game.0x6FCB5CD0
bool __fastcall ACT5Q4_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus)
{
	*pStatus = 0;

	if (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED))
	{
		return true;
	}

	if (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING) || QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE))
	{
		*pStatus = (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A5Q4, QFLAG_ENTERAREA) != 0) + 4;
	}
	else if (pQuest->bNotIntro)
	{
		if (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A5Q4, QFLAG_COMPLETEDNOW))
		{
			*pStatus = 12;
		}
		else if (pQuest->fState < 4)
		{
			*pStatus = pQuest->fLastState;
		}
	}

	return true;
}

//D2Game.0x6FCB5D60
void __fastcall ACT5Q4_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (!pQuestArg->pTarget || pQuestArg->pTarget->dwClassId != MONSTER_DREHYA)
	{
		return;
	}

	Act5Quest4* pQuestDataEx = (Act5Quest4*)pQuestData->pQuestDataEx;

	if (pQuestDataEx->bDrehyaActivated == 1)
	{
		pQuestData->dwFlags &= 0xFFFFFF00;
		QUESTS_UnitIterate(pQuestData, 1, 0, ACT5Q4_UnitIterate_StatusCyclerEx, 1);
		pQuestDataEx->bDrehyaActivated = 0;
	}

	if (!pQuestDataEx->bNeedsToOpenPortal || pQuestDataEx->bPortalCreated)
	{
		return;
	}

	const int32_t nX = CLIENTS_GetUnitX(pQuestArg->pTarget);
	const int32_t nY = CLIENTS_GetUnitY(pQuestArg->pTarget);

	Room1* pRoom = UNITS_GetRoom(pQuestArg->pTarget);
	if (D2GAME_CreatePortalObject_6FD13DF0(pQuestData->pGame, nullptr, pRoom, nX + 10, nY + 5, LEVEL_NIHLATHAKSTEMPLE, nullptr, OBJECT_PERMANENT_TOWN_PORTAL, 1))
	{
		pQuestDataEx->bPortalCreated = 1;
		pQuestDataEx->bNeedsToOpenPortal = 0;
	}
}

//D2Game.0x6FCB5E70
int32_t __fastcall ACT5Q4_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_COMPLETEDBEFORE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_COMPLETEDNOW))
	{
		QUESTS_StatusCyclerEx(pGame, pUnit, QUEST_A5Q4_NIHLATHAK);
	}

	return 0;
}

//D2Game.0x6FCB5EE0
bool __fastcall ACT5Q4_SeqCallback(QuestData* pQuestData)
{
	if (pQuestData->fState < 4 && pQuestData->bNotIntro)
	{
		if (!pQuestData->fState)
		{
			QUESTS_StateDebug(pQuestData, 1, __FILE__, __LINE__);
		}
		return true;
	}

	QuestData* pQuest = QUESTS_GetQuestData(pQuestData->pGame, pQuestData->nSeqId);
	if (!pQuest)
	{
		return false;
	}

	if (IsBadCodePtr((FARPROC)pQuestData->pfSeqFilter))
	{
		FOG_DisplayAssert("pQuestInfo->pSequence", __FILE__, __LINE__);
		exit(-1);
	}
	return pQuestData->pfSeqFilter(pQuest);
}

//D2Game.0x6FCB5F70
void __fastcall ACT5Q4_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg)
{
	Act5Quest4* pQuestDataEx = (Act5Quest4*)pQuestData->pQuestDataEx;

	if (pQuestArg->nNPCNo != MONSTER_DREHYA)
	{
		return;
	}

	if (pQuestArg->nMessageIndex == 20137)
	{
		if (!pQuestData->bNotIntro)
		{
			return;
		}

		QUESTS_StateDebug(pQuestData, 2, __FILE__, __LINE__);
		pQuestDataEx->bDrehyaActivated = 1;
		pQuestDataEx->bNeedsToOpenPortal = 1;
		SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT5Q4_UnitIterate_UpdateQuestStateFlags);
	}
	else if (pQuestArg->nMessageIndex == 20148)
	{
		pQuestData->dwFlags &= 0xFFFFFF00;
		QUESTS_UnitIterate(pQuestData, pQuestData->fLastState, 0, ACT5Q4_UnitIterate_StatusCyclerEx, 1);

		BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestData->pGame->nDifficulty];

		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_ENTERAREA);

		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE) && (pQuestData->bNotIntro || !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING)))
		{
			return;
		}

		QUESTS_StateDebug(pQuestData, 5, __FILE__, __LINE__);

		if (pQuestData->fState < 4 && pQuestData->bNotIntro)
		{
			if (!pQuestData->fState)
			{
				QUESTS_StateDebug(pQuestData, 1, __FILE__, __LINE__);
			}
			return;
		}

		QuestData* pQuest = QUESTS_GetQuestData(pQuestData->pGame, pQuestData->nSeqId);
		if (!pQuest)
		{
			return;
		}

		if (IsBadCodePtr((FARPROC)pQuestData->pfSeqFilter))
		{
			FOG_DisplayAssert("pQuestInfo->pSequence", __FILE__, __LINE__);
			exit(-1);
		}
		pQuestData->pfSeqFilter(pQuest);
	}
}

//D2Game.0x6FCB6100
int32_t __fastcall ACT5Q4_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDPENDING))
	{
		return 0;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED) == 1 || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING) == 1)
	{
		return 0;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A5Q4_NIHLATHAK);
	if (!pQuestData)
	{
		return 0;
	}

	if (pQuestData->fState == 2)
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_STARTED);
	}
	else if (pQuestData->fState == 3)
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_LEAVETOWN);
	}

	return 0;
}

//D2Game.0x6FCB6190
void __fastcall ACT5Q4_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	static const int32_t nIndices[] =
	{
		-1, 0, 1, 2, 3, 4
	};

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestData->pGame->nDifficulty];

	int32_t nNpcId = -1;
	if (pQuestArg->pTarget)
	{
		nNpcId = pQuestArg->pTarget->dwClassId;
	}

	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDPENDING))
	{
		return;
	}

	if (nNpcId != MONSTER_DREHYA || pQuestData->fState != 1)
	{
		if (QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_REWARDPENDING))
		{
			if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_ENTERAREA))
			{
				QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, 4);
			}
			else
			{
				QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, 3);
			}
		}
		else
		{
			if (QUESTS_CheckPlayerGUID(pQuestData, (pQuestArg->pPlayer ? pQuestArg->pPlayer->dwUnitId : -1)) == 1)
			{
				QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, 4);
			}
			else if (QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_REWARDGRANTED) != 1 && (pQuestData->fState < 4 || QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_PRIMARYGOALDONE)) && pQuestData->bNotIntro)
			{
				const int32_t nIndex = nIndices[pQuestData->fState];
				if (nIndex != -1 && nIndex < 8)
				{
					QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, nIndex);
				}
			}
		}
	}
	else
	{
		if (!QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_REWARDPENDING) && !QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_REWARDGRANTED))
		{
			QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_DREHYA, 0);
		}
	}
}

//D2Game.0x6FCB6320
void __fastcall ACT5Q4_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (!pQuestData->bNotIntro)
	{
		return;
	}

	Room1* pRoom = UNITS_GetRoom(pQuestArg->pTarget);
	if (!pRoom)
	{
		return;
	}

	Act5Quest4* pQuestDataEx = (Act5Quest4*)pQuestData->pQuestDataEx;
	pQuestDataEx->pRoom = pRoom;

	SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT5Q4_UnitIterate_SetRewardPending);
	SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT5Q3_UnitIterate_SetPrimaryGoalDoneForPartyMembers);
	SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT5Q4_UnitIterate_SetCompletionFlag);
	SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT5Q4_UnitIterate_AttachCompletionSound);

	QUESTS_SetGlobalState(pQuestData->pGame, QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE);
	QUESTS_TriggerFX(pQuestData->pGame, 17);
	QUESTS_StateDebug(pQuestData, 4, __FILE__, __LINE__);

	if (!pQuestDataEx->bTimerActive)
	{
		pQuestDataEx->bTimerActive = 1;
		QUESTS_CreateTimer(pQuestData, ACT5Q4_Timer_StatusCycler, 8);
	}
}

//D2Game.0x6FCB63F0
int32_t __fastcall ACT5Q4_UnitIterate_SetRewardPending(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	Room1* pRoom = UNITS_GetRoom(pUnit);
	if (!pRoom || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_COMPLETEDNOW) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED))
	{
		return 0;
	}

	Act5Quest4* pQuestDataEx = (Act5Quest4*)QUESTS_GetQuestData(pGame, QUEST_A5Q4_NIHLATHAK)->pQuestDataEx;

	if (pRoom != pQuestDataEx->pRoom)
	{
		Room1** ppRoomList = nullptr;
		int32_t nNumRooms = 0;
		DUNGEON_GetAdjacentRoomsListFromRoom(pRoom, &ppRoomList, &nNumRooms);

		int32_t nCounter = 0;
		while (nCounter < nNumRooms)
		{
			if (ppRoomList[nCounter] == pQuestDataEx->pRoom)
			{
				break;
			}

			++nCounter;
		}

		if (nCounter >= nNumRooms)
		{
			return 0;
		}
	}

	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q3, QFLAG_REWARDPENDING))
	{
		return 0;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING))
	{
		return 0;
	}

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING);
	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE);
	return 0;
}

//D2Game.0x6FCB6500
int32_t __fastcall ACT5Q4_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData)
{
	if (!QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty], QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE))
	{
		return 0;
	}

	const int16_t nPartyId = SUNIT_GetPartyId(pUnit);
	if (nPartyId != -1)
	{
		PARTY_IteratePartyMembers(pGame, nPartyId, ACT5Q3_UnitIterate_SetPrimaryGoalDone, 0);
	}

	return 0;
}

//D2Game.0x6FCB6550
int32_t __fastcall ACT5Q4_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED) != 1
		&& QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING) != 1
		&& QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE) != 1)
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_COMPLETEDNOW);
		QUESTS_SendLogUpdateEx(pUnit, QUEST_A5Q4_NIHLATHAK, 0);
	}

	return 0;
}

//D2Game.0x6FCB65C0
int32_t __fastcall ACT5Q4_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData)
{
	if (QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty], QUESTSTATEFLAG_A5Q4, QFLAG_PRIMARYGOALDONE))
	{
		SUNIT_AttachSound(pUnit, 82, pUnit);
	}

	return 0;
}

//D2Game.0x6FCB6600
bool __fastcall ACT5Q4_Timer_StatusCycler(Game* pGame, QuestData* pQuestData)
{
	Act5Quest4* pQuestDataEx = (Act5Quest4*)pQuestData->pQuestDataEx;

	if (pQuestData->fLastState != 4)
	{
		pQuestData->dwFlags &= 0xFFFFFF00;
		QUESTS_UnitIterate(pQuestData, 4, 0, ACT5Q4_UnitIterate_StatusCyclerEx, 1);
	}

	pQuestDataEx->bTimerActive = 0;
	return true;
}

//D2Game.0x6FCB6630
void __fastcall ACT5Q4_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	Act5Quest4* pQuestDataEx = (Act5Quest4*)pQuestData->pQuestDataEx;
	if (pQuestData->bNotIntro)
	{
		BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];
		if (!QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_REWARDPENDING))
		{
			if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_LEAVETOWN))
			{
				pQuestDataEx->bNeedsToOpenPortal = 1;
				QUESTS_UnitIterate(pQuestData, 2, 0, ACT5Q4_UnitIterate_StatusCyclerEx, 1);
				QUESTS_StateDebug(pQuestData, 3, __FILE__, __LINE__);
			}
			else if (QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_STARTED))
			{
				pQuestDataEx->bNeedsToOpenPortal = 1;
				QUESTS_UnitIterate(pQuestData, 1, 0, ACT5Q4_UnitIterate_StatusCyclerEx, 1);
				QUESTS_StateDebug(pQuestData, 2, __FILE__, __LINE__);
			}
		}
	}
	else
	{
		pQuestData->fState = 5;

		WaypointData* pWaypointFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pWaypointData[pQuestArg->pGame->nDifficulty];
		int16_t nWpNo = 0;
		if (WAYPOINTS_GetWaypointNoFromLevelId(LEVEL_HALLSOFDEATHSCALLING, &nWpNo) && !WAYPOINTS_IsActivated(pWaypointFlags, nWpNo))
		{
			pQuestDataEx->bWaypointNotActivated = 1;
		}
	}
}

//D2Game.0x6FCB6750
void __fastcall ACT5Q4_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg)
{
	Act5Quest4* pQuestDataEx = (Act5Quest4*)pQuestData->pQuestDataEx;

	if (pQuestArg->nOldLevel == LEVEL_HARROGATH && pQuestData->fState == 2)
	{
		QUESTS_StateDebug(pQuestData, 3, __FILE__, __LINE__);
		SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT5Q4_UnitIterate_UpdateQuestStateFlags);
	}

	if (pQuestArg->nNewLevel == LEVEL_NIHLATHAKSTEMPLE && pQuestData->fLastState == 1)
	{
		pQuestData->dwFlags &= 0xFFFFFF00;
		QUESTS_UnitIterate(pQuestData, 2, 0, ACT5Q4_UnitIterate_StatusCyclerEx, 1);
		pQuestDataEx->bDrehyaActivated = 0;
	}
}

//D2Game.0x6FCB67C0
void __fastcall ACT5Q4_SetRewardGranted(Game* pGame, UnitAny* pUnit)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDGRANTED);
	QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_REWARDPENDING);

	QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A5Q4, QFLAG_COMPLETEDBEFORE);//TODO: Needed? Set?
}

//D2Game.0x6FCB6800
void __fastcall ACT5Q4_OnNihlathakActivated(Game* pGame)
{
	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A5Q4_NIHLATHAK);

	if (!pQuestData || !pQuestData->bNotIntro || pQuestData->fLastState >= 3)
	{
		return;
	}

	pQuestData->dwFlags &= 0xFFFFFF00;
	QUESTS_UnitIterate(pQuestData, 3, 0, ACT5Q4_UnitIterate_StatusCyclerEx, 1);
}

//D2Game.0x6FCB6840
void __fastcall ACT5Q4_AnyaOpenPortal(Game* pGame, UnitAny* pUnit)
{
	Act5Quest4* pQuestDataEx = (Act5Quest4*)QUESTS_GetQuestData(pGame, QUEST_A5Q4_NIHLATHAK)->pQuestDataEx;
	if (!pQuestDataEx->bWaypointNotActivated)
	{
		return;
	}

	Room1* pRoom = UNITS_GetRoom(pUnit);
	const int32_t nLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);
	if (DRLG_GetActNoFromLevelId(nLevelId) != ACT_V || !D2GAME_CreatePortalObject_6FD13DF0(pGame, nullptr, pRoom, CLIENTS_GetUnitX(pUnit) + 10, CLIENTS_GetUnitY(pUnit) + 5, LEVEL_NIHLATHAKSTEMPLE, nullptr, OBJECT_PERMANENT_TOWN_PORTAL, 1))
	{
		return;
	}

	pQuestDataEx->bWaypointNotActivated = 0;
}

//
void __fastcall ACT5Q4_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	Act5Quest4* pQuestDataEx = (Act5Quest4*)pQuestData->pQuestDataEx;

	if (pQuestArg->pPlayer)
	{
		QUESTS_FastRemovePlayerGUID(&pQuestData->tPlayerGUIDs, pQuestArg->pPlayer->dwUnitId);
		QUESTS_FastRemovePlayerGUID(&pQuestDataEx->tPlayerGUIDs, pQuestArg->pPlayer->dwUnitId);
	}
	else
	{
		QUESTS_FastRemovePlayerGUID(&pQuestData->tPlayerGUIDs, -1);
		QUESTS_FastRemovePlayerGUID(&pQuestDataEx->tPlayerGUIDs, -1);
	}
}
