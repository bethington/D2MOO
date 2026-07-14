#include "QUESTS/ACT4/A4Q2.h"

#include <DataTbls/LevelsIds.h>
#include <DataTbls/MonsterIds.h>
#include <DataTbls/ObjectsIds.h>
#include <DataTbls/ObjectsTbls.h>
#include <Drlg/D2DrlgDrlg.h>
#include <D2Collision.h>
#include <D2DataTbls.h>
#include <D2Dungeon.h>
#include <D2QuestRecord.h>
#include <D2StatList.h>
#include <D2Waypoints.h>

#include "GAME/Clients.h"
#include "GAME/Game.h"
#include "GAME/Level.h"
#include "GAME/SCmd.h"
#include "ITEMS/Items.h"
#include "MONSTER/MonsterMode.h"
#include "MONSTER/MonsterSpawn.h"
#include "OBJECTS/Objects.h"
#include "OBJECTS/ObjMode.h"
#include "PLAYER/Player.h"
#include "PLAYER/PlrMsg.h"
#include "QUESTS/Quests.h"
#include "UNIT/Party.h"
#include "UNIT/SUnit.h"
#include "GAME/Task.h"


#pragma warning(disable: 28159)


//D2Game.0x6FD3AE10
NPCMessageTable gpAct4Q2NpcMessages[] =
{
	{
		{
			{ MONSTER_TYRAEL2, 681, 0, 1 },
		},
		1
	},
	{
		{
			{ MONSTER_TYRAEL2, 683, 0, 2 },
			{ MONSTER_CAIN4, 682, 0, 2 },
		},
		2
	},
	{
		{
			{ MONSTER_TYRAEL2, 684, 0, 1 },
			{ MONSTER_CAIN4, 685, 0, 1 },
		},
		2
	},
	{
		{
			{ MONSTER_TYRAEL2, 684, 0, 2 },
			{ MONSTER_CAIN4, 685, 0, 2 },
		},
		2
	},
	{
		{
			{ MONSTER_TYRAEL2, 20000, 0, 1 },
			{ MONSTER_CAIN4, 20001, 0, 1 },
		},
		2
	},
	{
		{
			{ MONSTER_TYRAEL2, 20000, 0, 2 },
			{ MONSTER_CAIN4, 20001, 0, 2 },
		},
		2
	},
	{
		{
			{ -1, 0, 0, 2 },
		},
		0
	}
};


//D2Game.0x6FCAE4C0
bool __fastcall ACT4Q2_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC)
{
	if (nNpcId == MONSTER_TYRAEL2)
	{
		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_COMPLETEDBEFORE) && pQuest->fState == 1
			|| !pQuest->pGame->bExpansion && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM3))
		{
			return 1;
		}

		if (pQuest->pGame->bExpansion && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM5))
		{
			return 1;
		}
	}
	else if (nNpcId == MONSTER_CAIN4)
	{
		if (!pQuest->pGame->bExpansion && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM2))
		{
			return 1;
		}

		if (!pQuest->pGame->bExpansion || !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED))
		{
			return 0;
		}

		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM4))
		{
			return 1;
		}
	}

	return 0;
}

//D2Game.0x6FCAE5B0
void __fastcall ACT4Q2_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) == 1)
	{
		return;
	}

	Room1* pRoom = UNITS_GetRoom(pUnit);
	if (!pRoom)
	{
		return;
	}

	const int32_t nLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);
	if (DRLG_GetActNoFromLevelId(nLevelId) != ACT_IV)
	{
		return;
	}

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_PRIMARYGOALDONE);
	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED);
	QUESTRECORD_ResetIntermediateStateFlags(pQuestFlags, QUESTSTATEFLAG_A4Q2);

	if (!pGame->bExpansion)
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM2);
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM3);
		CLIENTS_UpdateCharacterProgression(SUNIT_GetClientFromPlayer(pUnit, __FILE__, __LINE__), 4, pGame->nDifficulty);
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A4Q2_DIABLO);
	if (pQuestData && pQuestData->pQuestDataEx)
	{
		++((Act4Quest2*)pQuestData->pQuestDataEx)->nPlayerCount;
	}
}

//D2Game.0x6FCAE670
int32_t __fastcall ACT4Q2_IsChaosSanctumCleared(Game* pGame)
{
	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A4Q2_DIABLO);

	if (pQuestData && pQuestData->pQuestDataEx)
	{
		return ((Act4Quest2*)pQuestData->pQuestDataEx)->bSanctumCleared;
	}

	return 0;
}

//D2Game.0x6FCAE690
void __fastcall ACT4Q2_InitQuestData(QuestData* pQuestData)
{
	memset(pQuestData->pfCallback, 0x00, sizeof(pQuestData->pfCallback));
	pQuestData->fState = 0;
	pQuestData->pfStatusFilter = 0;
	pQuestData->pfCallback[QUESTEVENT_MONSTERKILLED] = ACT4Q2_Callback08_MonsterKilled;
	pQuestData->pfCallback[QUESTEVENT_CHANGEDLEVEL] = ACT4Q2_Callback03_ChangedLevel;
	pQuestData->pfCallback[QUESTEVENT_NPCACTIVATE] = ACT4Q2_Callback00_NpcActivate;
	pQuestData->pfCallback[QUESTEVENT_NPCDEACTIVATE] = ACT4Q2_Callback02_NpcDeactivate;
	pQuestData->pfCallback[QUESTEVENT_SCROLLMESSAGE] = ACT4Q2_Callback11_ScrollMessage;
	pQuestData->pfCallback[QUESTEVENT_PLAYERSTARTEDGAME] = ACT4Q2_Callback13_PlayerStartedGame;
	pQuestData->pfCallback[QUESTEVENT_PLAYERJOINEDGAME] = ACT4Q2_Callback14_PlayerJoinedGame;
	pQuestData->pNPCMessages = gpAct4Q2NpcMessages;
	pQuestData->bActive = 1;
	pQuestData->nQuestFilter = QUESTSTATEFLAG_A4Q2;
	pQuestData->nInitNo = 4;
	pQuestData->pfActiveFilter = ACT4Q2_ActiveFilterCallback;

	Act4Quest2* pQuestDataEx = D2_ALLOC_STRC_POOL(pQuestData->pGame->pMemoryPool, Act4Quest2);
	memset(pQuestDataEx, 0x00, sizeof(Act4Quest2));

	pQuestData->pQuestDataEx = pQuestDataEx;
	pQuestData->pfSeqFilter = ACT4Q2_SeqCallback;
}

//D2Game.0x6FCAE750
bool __fastcall ACT4Q2_SeqCallback(QuestData* pQuestData)
{
	if (!pQuestData->fState && pQuestData->bNotIntro == 1)
	{
		QUESTS_StateDebug(pQuestData, 1, __FILE__, __LINE__);
	}

	return 1;
}

//D2Game.0x6FCAE780
void __fastcall ACT4Q2_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg)
{
	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;
	if (pQuestArg->nNPCNo == MONSTER_TYRAEL2)
	{
		switch (pQuestArg->nMessageIndex)
		{
		case 681:
		{
			pQuestDataEx->bTalkedToTyrael = 1;
			QUESTS_StateDebug(pQuestData, 2, __FILE__, __LINE__);
			SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT4Q2_UnitIterate_UpdateQuestStateFlags);
			QUESTS_NPCActivateSpeeches(pQuestArg->pGame, pQuestArg->pPlayer, pQuestArg->pTarget);
			return;
		}
		case 684:
		{
			QUESTRECORD_ClearQuestState(UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestData->pGame->nDifficulty], QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM3);
			return;
		}
		case 20000:
		{
			if (!pQuestData->pGame->bExpansion)
			{
				return;
			}

			QUESTRECORD_SetQuestState(UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestData->pGame->nDifficulty], QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM5);
			if (pQuestDataEx->bPortalToAct5Spawned)
			{
				return;
			}

			Coord pCoord = {};
			UNITS_GetCoords(pQuestArg->pTarget, &pCoord);
			pCoord.nX += 5;

			Room1* pRoom = UNITS_GetRoom(pQuestArg->pTarget);
			QUESTS_GetFreePosition(pRoom, &pCoord, 2, 0x400, &pRoom, 12);
			if (!pRoom)
			{
				return;
			}

			UnitAny* pPortal = SUNIT_AllocUnitData(UNIT_OBJECT, 566, pCoord.nX, pCoord.nY, pQuestArg->pGame, pRoom, 1, OBJMODE_OPERATING, 0);
			if (!pPortal)
			{
				return;
			}

			pQuestDataEx->unk0x44 = 0;
			pQuestDataEx->bPortalToAct5Spawned = 1;

			pPortal->dwFlags |= UNITFLAG_ISRESURRECT | UNITFLAG_ISINIT;
			return;
		}
		default:
			break;
		}
	}
	else if (pQuestArg->nNPCNo == MONSTER_CAIN4)
	{
		if (pQuestArg->nMessageIndex == 685)
		{
			QUESTRECORD_ClearQuestState(UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestData->pGame->nDifficulty], QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM2);
		}
		else if (pQuestArg->nMessageIndex == 20001)
		{
			QUESTRECORD_SetQuestState(UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestData->pGame->nDifficulty], QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM4);
		}
	}
}

//D2Game.0x6FCAE950
int32_t __fastcall ACT4Q2_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) == 1)
	{
		return 0;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A4Q2_DIABLO);
	if (!pQuestData)
	{
		return 0;
	}

	if (pQuestData->fState == 2)
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_STARTED);
		return 0;
	}

	if (pQuestData->fState == 3)
	{
		if (pQuestData->fLastState == 1)
		{
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_LEAVETOWN);
			return 0;
		}

		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_ENTERAREA);
	}

	return 0;
}

//D2Game.0x6FCAE9D0
void __fastcall ACT4Q2_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	static const int32_t nIndices[] = { -1, 0, 1, -1, -1 };

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	int32_t nNpcId = -1;
	if (pQuestArg->pTarget)
	{
		nNpcId = pQuestArg->pTarget->dwClassId;
	}

	if (!pQuestData->pGame->bExpansion)
	{
		if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM3) && nNpcId == MONSTER_TYRAEL2)
		{
			QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_TYRAEL2, 2);
		}
		else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM2) && nNpcId == MONSTER_CAIN4)
		{
			QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_CAIN4, 2);
		}
		else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM3))
		{
			if (nNpcId == MONSTER_CAIN4)
			{
				QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_CAIN4, 3);
			}
		}
		else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM2))
		{
			if (nNpcId == MONSTER_TYRAEL2)
			{
				QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_TYRAEL2, 3);
			}
		}
		return;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_REWARDGRANTED) == 1)
	{
		if (!pQuestData->pGame->bExpansion)
		{
			return;
		}

		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM5))
		{
			if (nNpcId == MONSTER_TYRAEL2)
			{
				QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_TYRAEL2, 4);
			}
			else if (nNpcId == MONSTER_CAIN4)
			{
				QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_CAIN4, 5);
			}
		}

		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM4))
		{
			if (nNpcId == MONSTER_CAIN4)
			{
				QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_CAIN4, 4);
			}
			else
			{
				if (nNpcId == MONSTER_TYRAEL2)
				{
					QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_TYRAEL2, 5);
				}
			}
		}
		return;
	}

	if ((pQuestData->fState < 4 || QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_PRIMARYGOALDONE)) && pQuestData->bNotIntro && pQuestData->fState)
	{
		const int32_t nIndex = nIndices[pQuestData->fState];
		if (nIndex != -1 && nIndex < 7)
		{
			QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, nIndex);
		}
	}
}

//D2Game.0x6FCAEBD0
void __fastcall ACT4Q2_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (pQuestArg->nOldLevel == LEVEL_THEPANDEMONIUMFORTRESS && pQuestData->fState == 2)
	{
		if (QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty], pQuestData->nQuestFilter, QFLAG_REWARDGRANTED) == 1)
		{
			return;
		}

		if (pQuestData->fState < 3)
		{
			QUESTS_StateDebug(pQuestData, 3, __FILE__, __LINE__);
		}

		if (pQuestData->fLastState < 1)
		{
			pQuestData->dwFlags &= 0xFFFFFF00;
			QUESTS_UnitIterate(pQuestData, 1, 0, ACT4Q2_UnitIterate_StatusCyclerEx, 0);
		}

		SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT4Q2_UnitIterate_UpdateQuestStateFlags);
	}

	if (pQuestArg->nNewLevel == LEVEL_CHAOSSANCTUM && pQuestData->bNotIntro)
	{
		if (pQuestData->fState < 3)
		{
			QUESTS_StateDebug(pQuestData, 3, __FILE__, __LINE__);
		}

		if (pQuestData->fLastState < 1)
		{
			pQuestData->dwFlags &= 0xFFFFFF00;
			QUESTS_UnitIterate(pQuestData, 1, 0, ACT4Q2_UnitIterate_StatusCyclerEx, 1);
		}

		SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT4Q2_UnitIterate_UpdateQuestStateFlags);
	}
}

//D2Game.0x6FCAECC0
int32_t __fastcall ACT4Q2_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_COMPLETEDBEFORE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_PRIMARYGOALDONE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_COMPLETEDNOW))
	{
		QUESTS_StatusCyclerEx(pGame, pUnit, QUEST_A4Q2_DIABLO);
	}

	return 0;
}

//D2Game.0x6FCAED30
void __fastcall ACT4Q2_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (!pQuestArg->pTarget || pQuestArg->pTarget->dwClassId != MONSTER_TYRAEL2)
	{
		return;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;

	if (pQuestDataEx->bTalkedToTyrael == 1)
	{
		pQuestData->dwFlags &= 0xFFFFFF00;
		QUESTS_UnitIterate(pQuestData, 1, 0, ACT4Q2_UnitIterate_StatusCyclerEx, 1);
		pQuestDataEx->bTalkedToTyrael = 0;
	}

	if (!pQuestData->pGame->bExpansion || !pQuestDataEx->unk0x44 || pQuestDataEx->bPortalToAct5Spawned)
	{
		return;
	}

	Coord pCoord = {};
	UNITS_GetCoords(pQuestArg->pTarget, &pCoord);
	pCoord.nX += 5;

	Room1* pRoom = UNITS_GetRoom(pQuestArg->pTarget);
	QUESTS_GetFreePosition(pRoom, &pCoord, 2, 0x400, &pRoom, 12);
	if (!pRoom)
	{
		return;
	}

	UnitAny* pPortal = SUNIT_AllocUnitData(UNIT_OBJECT, 566, pCoord.nX, pCoord.nY, pQuestArg->pGame, pRoom, 1, OBJMODE_OPERATING, 0);
	if (!pPortal)
	{
		return;
	}

	pQuestDataEx->unk0x44 = 0;
	pQuestDataEx->bPortalToAct5Spawned = 1;

	pPortal->dwFlags |= UNITFLAG_ISRESURRECT | UNITFLAG_ISINIT;
}

//D2Game.0x6FCAEE30
void __fastcall ACT4Q2_Callback14_PlayerJoinedGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (!pQuestArg->pGame->bExpansion)
	{
		return;
	}

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM5) && !QUESTRECORD_GetQuestState(pQuestFlags, 28, QFLAG_REWARDGRANTED))
	{
		QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM5);
	}
}

//D2Game.0x6FCAEE80
void __fastcall ACT4Q2_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	if (pQuestArg->pGame->bExpansion && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM5) && !QUESTRECORD_GetQuestState(pQuestFlags, 28, QFLAG_REWARDGRANTED))
	{
		QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM5);
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_REWARDGRANTED) || QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_COMPLETEDBEFORE))
	{
		return;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_ENTERAREA) == 1)
	{
		pQuestData->fLastState = 2;
		pQuestData->fState = 3;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_LEAVETOWN) == 1)
	{
		pQuestData->fState = 3;
		pQuestData->fLastState = 1;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_STARTED) == 1)
	{
		pQuestData->fState = 2;
		pQuestData->fLastState = 1;
	}
}

//D2Game.0x6FCAEF40
void __fastcall ACT4Q2_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg)
{
	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;
	if (pQuestArg->pTarget && pQuestArg->pTarget->dwClassId == MONSTER_DIABLO)
	{
		if (!pQuestData->pGame->bExpansion)
		{
			QUESTS_TriggerFX(pQuestData->pGame, 13);
		}

		pQuestDataEx->bDiabloKilled = 1;

		Room1* pRoom = UNITS_GetRoom(pQuestArg->pTarget);
		if (!pRoom)
		{
			return;
		}

		bool bNotRewarded = false;
		pQuestDataEx->pRoom = pRoom;
		if (pQuestArg->pPlayer)
		{
			if (!pQuestDataEx->bTimerCreated)
			{
				pQuestDataEx->bNeedToSpawnDiablo = 0;
				pQuestDataEx->unk0x03 = 1;
				pQuestDataEx->bNeedToEndGame = 1;
				pQuestDataEx->bNeedToWarpPlayers = 1;
				pQuestDataEx->bClientsSaved = 0;
				pQuestDataEx->bTimerCreated = 1;
				pQuestDataEx->dwTickCount = GetTickCount();
				QUESTS_CreateTimer(pQuestData, ACT4Q2_SpawnDiablo, 1);
			}

			if (!pQuestData->bNotIntro)
			{
				return;
			}

			if (!QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty], QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED))
			{
				bNotRewarded = true;
			}
		}

		if (pQuestData->bNotIntro)
		{
			pQuestDataEx->nPlayerCount = 0;
			QUESTS_UnitIterate(pQuestData, 13, 0, ACT4Q2_UnitIterate_StatusCyclerEx, 1);
			SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT4Q2_UnitIterate_UpdateQuestStateFlags);
			SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT4Q2_UnitIterate_SetPrimaryGoalDoneForPartyMembers);
			SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT4Q2_UnitIterate_SetCompletionFlag);
			SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT4Q2_UnitIterate_AttachCompletionSound);

			if (bNotRewarded)
			{
				for(int nPlayer = 0; nPlayer < pQuestDataEx->nPlayerCount; nPlayer++)
				{
					D2Game_10034_Return(64);
				}
			}
		}

		return;
	}
	else
	{
		++pQuestDataEx->nBossesKilled;
		if (pQuestDataEx->nBossesKilled != 3)
		{
			return;
		}

		int32_t nCounter = 0;
		while (pQuestDataEx->bSealActivated[nCounter])
		{
			++nCounter;
			if (nCounter >= 5)
			{
				ACT4Q2_KillAllMonstersInCS(pQuestData->pGame, pQuestDataEx);

				if (pQuestDataEx->bTimerCreated)
				{
					return;
				}

				pQuestDataEx->dwTickCount = 0;
				pQuestDataEx->bNeedToSpawnDiablo = 1;
				pQuestDataEx->unk0x03 = 0;
				pQuestDataEx->bTimerCreated = 1;
				QUESTS_CreateTimer(pQuestData, ACT4Q2_SpawnDiablo, 1);
				return;
			}
		}
	}
}

//D2Game.0x6FCAF110
int32_t __fastcall ACT4Q2_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) == 1)
	{
		return 0;
	}

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_COMPLETEDNOW);
	QUESTS_SendLogUpdateEx(pUnit, QUEST_A4Q2_DIABLO, 0);
	return 0;
}

//D2Game.0x6FCAF160
int32_t __fastcall ACT4Q2_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData)
{
	if (!QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty], QUESTSTATEFLAG_A4Q2, QFLAG_PRIMARYGOALDONE))
	{
		return 0;
	}

	GSPacketSrv5D packet5D = {};
	packet5D.nHeader = 0x5D;
	packet5D.field_4 = 0;
	packet5D.nQuestId = QUEST_A4Q2_DIABLO;
	packet5D.nFlags = 2;
	D2GAME_PACKETS_SendPacket_6FC3C710(SUNIT_GetClientFromPlayer(pUnit, __FILE__, __LINE__), &packet5D, sizeof(packet5D));
	SUNIT_AttachSound(pUnit, 75, pUnit);
	return 0;
}

//D2Game.0x6FCAF1E0
bool __fastcall ACT4Q2_SpawnDiablo(Game* pGame, QuestData* pQuestData)
{
	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;
	if (pQuestDataEx->bNeedToSpawnDiablo)
	{
		++pQuestDataEx->dwTickCount;
		if (pQuestDataEx->dwTickCount < 10 || !pQuestDataEx->bDiabloStartPointInitialized)
		{
			return false;
		}

		UnitAny* pObject = SUNIT_GetServerUnit(pQuestData->pGame, UNIT_OBJECT, pQuestDataEx->nDiabloStartPointObjectGUID);
		if (!pObject)
		{
			return false;
		}

		const int32_t nX = CLIENTS_GetUnitX(pObject);
		const int32_t nY = CLIENTS_GetUnitY(pObject);

		Room1* pRoom = UNITS_GetRoom(pObject);

		UnitAny*   pDiablo = D2GAME_SpawnMonster_6FC69F10(pQuestData->pGame, pRoom, nX, nY, MONSTER_DIABLO, 1, -1, 0);
		if (!pDiablo) pDiablo = D2GAME_SpawnMonster_6FC69F10(pQuestData->pGame, pRoom, nX, nY, MONSTER_DIABLO, 1,  5, 0);
		if (!pDiablo) pDiablo = D2GAME_SpawnMonster_6FC69F10(pQuestData->pGame, pRoom, nX, nY, MONSTER_DIABLO, 1, 10, 0);

		if (pDiablo)
		{
			pDiablo->dwFlags |= UNITFLAG_ISRESURRECT | UNITFLAG_ISINIT;

			pQuestDataEx->bDiabloSpawned = 1;
			pQuestDataEx->bNeedToSpawnDiablo = 0;
			pQuestDataEx->bTimerCreated = 0;
			return true;
		}
		return false;
	}

	if (!pQuestDataEx->unk0x03)
	{
		return true;
	}

	if (pQuestDataEx->bNeedToEndGame && pQuestDataEx->dwTickCount + 95000 < GetTickCount())
	{
		pQuestDataEx->bTimerCreated = 0;
		pQuestDataEx->unk0x03 = 0;
		pQuestDataEx->bNeedToEndGame = 0;
		if (pQuestData->pGame->bExpansion)
		{
			return true;
		}

		sub_6FC37FB0(pGame, nullptr);
		return true;
	}

	if (pQuestDataEx->dwTickCount + 90000 >= GetTickCount())
	{
		if (pQuestDataEx->dwTickCount + 75000 < GetTickCount() && !pQuestDataEx->bClientsSaved)
		{
			pQuestDataEx->bClientsSaved = 1;
			sub_6FC37B10(pGame);
		}
		return false;
	}

	if (pQuestDataEx->bNeedToWarpPlayers)
	{
		if (!pQuestData->pGame->bExpansion)
		{
			SUNIT_IterateUnitsOfType(pGame, 0, 0, ACT4Q2_UnitIterate_WarpToTownEndGame);
		}
		pQuestDataEx->bNeedToWarpPlayers = 0;
	}

	return false;
}

//D2Game.0x6FCAF3D0
int32_t __fastcall ACT4Q2_UnitIterate_WarpToTownEndGame(Game* pGame, UnitAny* pUnit, void* pData)
{
	sub_6FC7BEC0(pGame, pUnit);
	UNITS_GetPlayerData(pUnit)->bBusy = 1;

	Act4Quest2* pQuestDataEx = (Act4Quest2*)QUESTS_GetQuestData(pGame, QUEST_A4Q2_DIABLO)->pQuestDataEx;
	pQuestDataEx->unk0x20 = pUnit;

	if (QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty], QUESTSTATEFLAG_A4Q2, QFLAG_PRIMARYGOALDONE))
	{
		LEVEL_WarpUnit(pGame, pUnit, LEVEL_THEPANDEMONIUMFORTRESS, 0);

		GSPacketSrv5D packet5D = {};
		packet5D.nHeader = 0x5D;
		packet5D.field_4 = 0;
		packet5D.nQuestId = QUEST_A4Q2_DIABLO;
		packet5D.nFlags = 1;
		D2GAME_PACKETS_SendPacket_6FC3C710(SUNIT_GetClientFromPlayer(pUnit, __FILE__, __LINE__), &packet5D, sizeof(packet5D));
	}
	else
	{
		GSPacketSrv50 packet50 = {};
		packet50.nHeader = 0x50;
		packet50.nQuestId = QUEST_A4Q2_DIABLO;
		D2GAME_PACKETS_SendPacket_6FC3C710(SUNIT_GetClientFromPlayer(pUnit, __FILE__, __LINE__), &packet50, sizeof(packet50));
	}

	return 0;
}

//D2Game.0x6FCAF4A0
int32_t __fastcall ACT4Q2_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData)
{
	if (!QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty], QUESTSTATEFLAG_A4Q2, QFLAG_PRIMARYGOALDONE))
	{
		return 0;
	}

	const int16_t nPartyId = SUNIT_GetPartyId(pUnit);
	if (nPartyId != -1)
	{
		PARTY_IteratePartyMembers(pGame, nPartyId, ACT4Q2_UnitIterate_SetPrimaryGoalDone, 0);
	}

	return 0;
}

//D2Game.0x6FCAF4F0
int32_t __fastcall ACT4Q2_UnitIterate_UpdatePlayerState(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	Room1* pRoom = UNITS_GetRoom(pUnit);
	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A4Q2_DIABLO);
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) == 1 || !pRoom || !pQuestData)
	{
		return 0;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;

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

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDPENDING))
	{
		return 0;
	}

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_PRIMARYGOALDONE);
	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED);
	QUESTRECORD_ResetIntermediateStateFlags(pQuestFlags, QUESTSTATEFLAG_A4Q2);

	if (!pGame->bExpansion)
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM2);
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_CUSTOM3);
		CLIENTS_UpdateCharacterProgression(SUNIT_GetClientFromPlayer(pUnit, __FILE__, __LINE__), 4, pGame->nDifficulty);
	}

	++pQuestDataEx->nPlayerCount;
	return 0;
}

//D2Game.0x6FCAF630
void __fastcall ACT4Q2_KillAllMonstersInCS(Game* pGame, Act4Quest2* pQuestDataEx)
{
	if (pQuestDataEx->bSanctumCleared)
	{
		return;
	}

	pQuestDataEx->bSanctumCleared = 1;
	QUESTS_TriggerFX(pGame, 12);

	for (Room1* pRoom = DUNGEON_GetRoomFromAct(pGame->pAct[ACT_IV]); pRoom; pRoom = pRoom->pRoomNext)
	{
		if (DUNGEON_GetLevelIdFromRoom(pRoom) == LEVEL_CHAOSSANCTUM)
		{
			for (UnitAny* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext)
			{
				if (pUnit->dwUnitType == UNIT_MONSTER && pUnit->dwClassId != MONSTER_DIABLO && pUnit->dwClassId != MONSTER_DIABLOCLONE && !SUNIT_IsDead(pUnit) && STATLIST_GetUnitAlignment(pUnit) == UNIT_ALIGNMENT_EVIL)
				{
					ModeChange modeChange = {};
					MONSTERMODE_GetModeChangeInfo(pUnit, MONMODE_DEATH, &modeChange);
					D2GAME_ModeChange_6FC65220(pGame, &modeChange, 1);
				}
			}
		}
	}
}

//D2Game.0x6FCAF6D0
void __fastcall OBJECTS_InitFunction55_DiabloStartPoint(ObjInitFn* pOp)
{
	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A4Q2_DIABLO);
	if (!pQuestData)
	{
		return;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;
	if (pQuestDataEx->bDiabloSpawned)
	{
		return;
	}

	pQuestDataEx->bDiabloStartPointInitialized = 1;
	int32_t nObjectId = -1;
	if (pOp->pObject)
	{
		nObjectId = pOp->pObject->dwUnitId;
	}
	pQuestDataEx->nDiabloStartPointObjectGUID = nObjectId;

	int32_t nCounter = 0;
	while (pQuestDataEx->bSealActivated[nCounter])
	{
		++nCounter;
		if (nCounter >= 5)
		{
			if (pQuestDataEx->nBossesKilled != 3)
			{
				return;
			}

			ACT4Q2_KillAllMonstersInCS(pOp->pGame, pQuestDataEx);
			if (pQuestDataEx->bTimerCreated)
			{
				return;
			}

			pQuestDataEx->dwTickCount = 0;
			pQuestDataEx->bNeedToSpawnDiablo = 1;
			pQuestDataEx->unk0x03 = 0;
			pQuestDataEx->bTimerCreated = 1;
			QUESTS_CreateTimer(pQuestData, ACT4Q2_SpawnDiablo, 1);
			return;
		}
	}
}

//D2Game.0x6FCAF760
int32_t __fastcall OBJECTS_OperateFunction54_DiabloSeal(ObjOperateFn* pOp, int32_t nOperate)
{
	if (!pOp || !pOp->pObject)
	{
		return 0;
	}

	UnitAny* pObject = pOp->pObject;

	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A4Q2_DIABLO);

	if (pObject->dwAnimMode != OBJMODE_NEUTRAL || !pQuestData || !pQuestData->pQuestDataEx)
	{
		return 0;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;
	Coord* pCoord = &pQuestDataEx->pSealCoords[0];
	UNITS_GetCoords(pOp->pObject, pCoord);

	const int32_t nX = pQuestDataEx->pSealCoords[0].nX - 12;
	const int32_t nY = pQuestDataEx->pSealCoords[0].nY - 52;
	pQuestDataEx->pSealCoords[0].nX = nX;
	pQuestDataEx->pSealCoords[0].nY = nY;

	Room1* pRoom = D2GAME_GetRoom_6FC52070(UNITS_GetRoom(pOp->pObject), nX, nY);
	if (!pRoom)
	{
		return 0;
	}

	Room1* pNewRoom = nullptr;
	QUESTS_GetFreePosition(pRoom, pCoord, 3, COLLIDE_MASK_PLACEMENT, &pNewRoom, 13);
	if (pNewRoom && SUNIT_AllocUnitData(UNIT_OBJECT, 131, pCoord->nX, pCoord->nY, pOp->pGame, pNewRoom, 1, 0, 0))
	{
		DUNGEON_ToggleHasPortalFlag(pNewRoom, 0);
		OBJECTS_OperateFunction52_DiabloSeal(pOp, nOperate);
	}

	return 0;
}

//D2Game.0x6FCAF8D0
int32_t __fastcall OBJECTS_OperateFunction55_DiabloSeal(ObjOperateFn* pOp, int32_t nOperate)
{
	if (!pOp || !pOp->pObject)
	{
		return 0;
	}

	UnitAny* pObject = pOp->pObject;

	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A4Q2_DIABLO);

	if (pObject->dwAnimMode != OBJMODE_NEUTRAL || !pQuestData || !pQuestData->pQuestDataEx)
	{
		return 0;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;
	Coord* pCoord = &pQuestDataEx->pSealCoords[1];
	UNITS_GetCoords(pOp->pObject, pCoord);

	const int32_t nX = pQuestDataEx->pSealCoords[1].nX - 39;
	const int32_t nY = pQuestDataEx->pSealCoords[1].nY + 33;
	pQuestDataEx->pSealCoords[1].nX = nX;
	pQuestDataEx->pSealCoords[1].nY = nY;

	Room1* pRoom = D2GAME_GetRoom_6FC52070(UNITS_GetRoom(pOp->pObject), nX, nY);
	if (!pRoom)
	{
		return 0;
	}

	Room1* pNewRoom = nullptr;
	QUESTS_GetFreePosition(pRoom, pCoord, 3, COLLIDE_MASK_PLACEMENT, &pNewRoom, 14);
	if (pNewRoom && SUNIT_AllocUnitData(UNIT_OBJECT, 131, pCoord->nX, pCoord->nY, pOp->pGame, pNewRoom, 1, 0, 0))
	{
		DUNGEON_ToggleHasPortalFlag(pNewRoom, 0);
		OBJECTS_OperateFunction52_DiabloSeal(pOp, nOperate);
	}

	return 0;
}

//D2Game.0x6FCAFA40
int32_t __fastcall OBJECTS_OperateFunction56_DiabloSeal(ObjOperateFn* pOp, int32_t nOperate)
{
	if (!pOp || !pOp->pObject)
	{
		return 0;
	}

	UnitAny* pObject = pOp->pObject;

	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A4Q2_DIABLO);

	if (pObject->dwAnimMode != OBJMODE_NEUTRAL || !pQuestData || !pQuestData->pQuestDataEx)
	{
		return 0;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;
	Coord* pCoord = &pQuestDataEx->pSealCoords[2];
	UNITS_GetCoords(pOp->pObject, pCoord);

	const int32_t nX = pQuestDataEx->pSealCoords[2].nX + 32;
	const int32_t nY = pQuestDataEx->pSealCoords[2].nY + 16;
	pQuestDataEx->pSealCoords[2].nX = nX;
	pQuestDataEx->pSealCoords[2].nY = nY;

	Room1* pRoom = D2GAME_GetRoom_6FC52070(UNITS_GetRoom(pOp->pObject), nX, nY);
	if (!pRoom)
	{
		return 0;
	}

	Room1* pNewRoom = nullptr;
	QUESTS_GetFreePosition(pRoom, pCoord, 3, COLLIDE_MASK_PLACEMENT, &pNewRoom, 15);
	if (pNewRoom && SUNIT_AllocUnitData(UNIT_OBJECT, 131, pCoord->nX, pCoord->nY, pOp->pGame, pNewRoom, 1, 0, 0))
	{
		DUNGEON_ToggleHasPortalFlag(pNewRoom, 0);
		OBJECTS_OperateFunction52_DiabloSeal(pOp, nOperate);
	}

	return 0;
}

//D2Game.0x6FCAFBB0
int32_t __fastcall OBJECTS_OperateFunction52_DiabloSeal(ObjOperateFn* pOp, int32_t nOperate)
{
	if (!pOp || !pOp->pObject)
	{
		return 0;
	}

	UnitAny* pObject = pOp->pObject;

	if (pObject->dwAnimMode != OBJMODE_NEUTRAL)
	{
		return 0;
	}

	UNITS_ChangeAnimMode(pObject, OBJMODE_OPERATING);

	ObjectsTxt* pObjectsTxtRecord = DATATBLS_GetObjectsTxtRecord(pOp->nObjectIdx);
	EVENT_SetEvent(pOp->pGame, pObject, EVENTTYPE_ENDANIM, pOp->pGame->dwGameFrame + (pObjectsTxtRecord->dwFrameCnt[1] >> 7), 0, 0);

	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A4Q2_DIABLO);
	if (!pQuestData)
	{
		return 0;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;

	switch (pObject->dwClassId)
	{
	case 392:
		pQuestDataEx->bSealActivated[0] = 1;
		break;

	case 393:
		pQuestDataEx->bSealActivated[1] = 1;
		break;

	case 394:
		pQuestDataEx->bSealActivated[2] = 1;
		break;

	case 395:
		pQuestDataEx->bSealActivated[3] = 1;
		break;

	case 396:
		pQuestDataEx->bSealActivated[4] = 1;
		break;

	default:
		break;
	}

	int32_t nCounter = 0;
	while (pQuestDataEx->bSealActivated[nCounter])
	{
		++nCounter;

		if (nCounter >= 5)
		{
			if (pQuestDataEx->nBossesKilled != 3)
			{
				return 0;
			}

			ACT4Q2_KillAllMonstersInCS(pOp->pGame, pQuestDataEx);
			if (pQuestDataEx->bTimerCreated)
			{
				return 0;
			}

			pQuestDataEx->dwTickCount = 0;
			pQuestDataEx->bNeedToSpawnDiablo = 1;
			pQuestDataEx->unk0x03 = 0;
			pQuestDataEx->bTimerCreated = 1;
			QUESTS_CreateTimer(pQuestData, ACT4Q2_SpawnDiablo, 1);
			return 0;
		}
	}

	return 0;
}

//D2Game.0x6FCAFCB0
void __fastcall ACT4Q2_SpawnSealBoss(Game* pGame, UnitAny* pUnit)
{
	Coord pCoord = {};

	UNITS_GetCoords(pUnit, &pCoord);
	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A4Q2_DIABLO);
	if (!pQuestData)
	{
		return;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;

	Coord* pSealCoords = nullptr;
	int32_t nMonId = 0;
	if (pCoord.nX == pQuestDataEx->pSealCoords[0].nX && pCoord.nY == pQuestDataEx->pSealCoords[0].nY)
	{
		nMonId = sgptDataTables->nSuperUniqueIds[36];
		pSealCoords = &pQuestDataEx->pSealCoords[0];
	}
	else if (pCoord.nX == pQuestDataEx->pSealCoords[1].nX && pCoord.nY == pQuestDataEx->pSealCoords[1].nY)
	{
		nMonId = sgptDataTables->nSuperUniqueIds[37];
		pSealCoords = &pQuestDataEx->pSealCoords[1];
	}
	else if (pCoord.nX == pQuestDataEx->pSealCoords[2].nX && pCoord.nY == pQuestDataEx->pSealCoords[2].nY)
	{
		nMonId = sgptDataTables->nSuperUniqueIds[38];
		pSealCoords = &pQuestDataEx->pSealCoords[2];
	}
	else
	{
		return;
	}

	if (!QUESTS_SpawnMonster(pGame, pUnit, pSealCoords, 2, nMonId))
	{
		EVENT_SetEvent(pGame, pUnit, EVENTTYPE_QUESTFN, pGame->dwGameFrame + 10, 0, 0);
	}
}

//D2Game.0x6FCAFD80
int32_t __fastcall ACT4Q2_HasDiabloBeenKilled(Game* pGame)
{
	if (pGame->bExpansion)
	{
		return 0;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A4Q2_DIABLO);
	if (!pQuestData)
	{
		return 1;
	}

	return ((Act4Quest2*)pQuestData->pQuestDataEx)->bDiabloKilled;
}

//D2Game.0x6FCAFDB0
void __fastcall OBJECTS_InitFunction78_LastLastPortal(ObjInitFn* pOp)
{
	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A4Q2_DIABLO);
	if (!pQuestData)
	{
		return;
	}

	Act4Quest2* pQuestDataEx = (Act4Quest2*)pQuestData->pQuestDataEx;
	if (pQuestDataEx->nLastLastPortalObjectMode == OBJMODE_OPENED)
	{
		UNITS_ChangeAnimMode(pOp->pObject, OBJMODE_OPENED);
	}
	else
	{
		pQuestDataEx->nLastLastPortalObjectMode = OBJMODE_OPENED;
		UNITS_ChangeAnimMode(pOp->pObject, OBJMODE_OPERATING);
	}
}

//D2Game.0x6FCAFDF0
int32_t __fastcall OBJECTS_OperateFunction73_LastLastPortal(ObjOperateFn* pOp, int32_t nOperate)
{
	if (!pOp || !pOp->pGame)
	{
		return 0;
	}

	if (!pOp->pGame->bExpansion)
	{
		return 1;
	}

	PlayerData* pPlayerData = UNITS_GetPlayerData(pOp->pPlayer);
	BitBuffer* pQuestFlags = pPlayerData->pQuestData[pOp->pGame->nDifficulty];
	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4Q2, QFLAG_PRIMARYGOALDONE))
	{
		SUNIT_AttachSound(pOp->pPlayer, 19, pOp->pPlayer);
		return 0;
	}

	Room1* pRoom = UNITS_GetRoom(pOp->pPlayer);
	if (!pRoom)
	{
		return 1;
	}

	const int32_t nLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);
	if (DRLG_GetActNoFromLevelId(nLevelId) != ACT_IV)
	{
		return 1;
	}

	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A4COMPLETED, QFLAG_REWARDGRANTED))
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4COMPLETED, QFLAG_REWARDGRANTED);
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A4COMPLETED, QFLAG_PRIMARYGOALDONE);

		GameClient* pClient = SUNIT_GetClientFromPlayer(pOp->pPlayer, "..\\D2Game/Quests/a4q2.cpp", 1370);
		if (pClient && !PLAYER_IsBusy(pOp->pPlayer))
		{
			SUNIT_ResetInteractInfo(pOp->pPlayer);
			pPlayerData->bBusy = 1;

			GSPacketSrv5D packet5D = {};
			packet5D.nHeader = 0x5D;
			packet5D.nQuestId = QUEST_A4Q2_DIABLO;
			packet5D.nFlags = 2;
			D2GAME_PACKETS_SendPacket_6FC3C710(pClient, &packet5D, sizeof(packet5D));

			GSPacketSrv61 packet61 = {};
			packet61.nHeader = 0x61;
			packet61.unk0x001 = 5;
			D2GAME_PACKETS_SendPacket_6FC3C710(pClient, &packet61, sizeof(packet61));
		}
	}

	D2GAME_PlayerChangeAct_6FC867C0(pOp->pGame, pOp->pPlayer, LEVEL_HARROGATH, 5);
	QUESTS_UpdatePlayerFlags(pOp->pGame, pOp->pPlayer);

	int16_t nWpNo = 0;
	if (WAYPOINTS_GetWaypointNoFromLevelId(LEVEL_HARROGATH, &nWpNo))
	{
		WAYPOINTS_ActivateWaypoint(pPlayerData->pWaypointData[pOp->pGame->nDifficulty], nWpNo);
	}

	return 1;
}
