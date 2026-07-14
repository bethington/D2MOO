#include "QUESTS/ACT3/A3Q6.h"

#include <DataTbls/LevelsIds.h>
#include <DataTbls/MonsterIds.h>
#include <DataTbls/ObjectsIds.h>
#include <DataTbls/ObjectsTbls.h>
#include <Drlg/D2DrlgDrlg.h>
#include <Drlg/D2DrlgPreset.h>
#include <D2Dungeon.h>
#include <D2Items.h>
#include <D2QuestRecord.h>

#include "AI/AiGeneral.h"
#include "GAME/Clients.h"
#include "GAME/Game.h"
#include "ITEMS/Items.h"
#include "MONSTER/MonsterSpawn.h"
#include "OBJECTS/Objects.h"
#include "QUESTS/Quests.h"
#include "UNIT/Party.h"
#include "UNIT/SUnit.h"


//D2Game.0x6FD3A040
NPCMessageTable gpAct3Q6NpcMessages[] =
{
	{
		{
			{ MONSTER_ORMUS, 628, 0, 1 },
		},
		1
	},
	{
		{
			{ MONSTER_ALKOR, 629, 0, 2 },
			{ MONSTER_ORMUS, 631, 0, 2 },
			{ MONSTER_MESHIF2, 633, 0, 2 },
			{ MONSTER_ASHEARA, 635, 0, 2 },
			{ MONSTER_HRATLI, 637, 0, 2 },
			{ MONSTER_CAIN3, 639, 0, 2 },
			{ MONSTER_NATALYA, 641, 0, 2 },
		},
		7
	},
	{
		{
			{ MONSTER_ALKOR, 630, 0, 2 },
			{ MONSTER_ORMUS, 632, 0, 2 },
			{ MONSTER_MESHIF2, 634, 0, 2 },
			{ MONSTER_HRATLI, 638, 0, 2 },
			{ MONSTER_CAIN3, 640, 0, 2 },
			{ MONSTER_NATALYA, 642, 0, 2 },
		},
		6
	},
	{
		{
			{ MONSTER_ALKOR, 643, 0, 2 },
			{ MONSTER_ORMUS, 645, 0, 2 },
			{ MONSTER_MESHIF2, 647, 0, 2 },
			{ MONSTER_ASHEARA, 649, 0, 2 },
			{ MONSTER_HRATLI, 651, 0, 2 },
			{ MONSTER_CAIN3, 653, 0, 2 },
			{ MONSTER_NATALYA, 655, 0, 2 },
		},
		7
	},
	{
		{
			{ MONSTER_ALKOR, 644, 0, 2 },
			{ MONSTER_ORMUS, 646, 0, 2 },
			{ MONSTER_MESHIF2, 648, 0, 2 },
			{ MONSTER_ASHEARA, 650, 0, 2 },
			{ MONSTER_HRATLI, 652, 0, 2 },
			{ MONSTER_CAIN3, 654, 0, 2 },
			{ MONSTER_NATALYA, 656, 0, 2 },
		},
		7
	},
	{
		{
			{ MONSTER_ALKOR, 657, 0, 1 },
			{ MONSTER_ORMUS, 658, 0, 1 },
			{ MONSTER_MESHIF2, 659, 0, 1 },
			{ MONSTER_ASHEARA, 660, 0, 1 },
			{ MONSTER_HRATLI, 661, 0, 1 },
			{ MONSTER_CAIN3, 662, 0, 1 },
			{ MONSTER_NATALYA, 663, 0, 1 },
		},
		7
	},
	{
		{
			{ MONSTER_ALKOR, 657, 0, 2 },
			{ MONSTER_ORMUS, 658, 0, 2 },
			{ MONSTER_MESHIF2, 659, 0, 2 },
			{ MONSTER_ASHEARA, 660, 0, 2 },
			{ MONSTER_HRATLI, 661, 0, 2 },
			{ MONSTER_CAIN3, 662, 0, 2 },
			{ MONSTER_NATALYA, 663, 0, 2 },
		},
		7
	},
	{
		{
			{ -1, 0, 0, 2 },
		},
		0
	}
};


//D2Game.0x6FCAC1F0
bool __fastcall ACT3Q6_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC)
{
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED) || nNpcId != MONSTER_ORMUS || pQuest->fState != 1)
	{
		return QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7) != 0;
	}

	return true;
}

//D2Game.0x6FCAC230
void __fastcall ACT3Q6_UnitIterate_SetPrimaryGoalDone(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED) == 1)
	{
		return;
	}

	Room1* pRoom = UNITS_GetRoom(pUnit);
	if (!pRoom)
	{
		return;
	}

	const int32_t nLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);
	if (DRLG_GetActNoFromLevelId(nLevelId) != ACT_III)
	{
		return;
	}

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE);
	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED);
	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7);

	GameClient* pClient = SUNIT_GetClientFromPlayer(pUnit, __FILE__, __LINE__);
	CLIENTS_UpdateCharacterProgression(pClient, 3, pGame->nDifficulty);

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A3Q6_MEPHISTO);
	if (pQuestData)
	{
		++((Act3Quest6*)pQuestData->pQuestDataEx)->nSoulstonesToDrop;
	}
}

//D2Game.0x6FCAC380
void __fastcall OBJECTS_InitFunction45_MephistoBridge(ObjInitFn* pOp)
{
	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A3Q6_MEPHISTO);
	if (!pQuestData)
	{
		return;
	}

	Act3Quest6* pQuestDataEx = (Act3Quest6*)pQuestData->pQuestDataEx;
	pQuestDataEx->bMephistoBridgeInitialized = 1;
	if (pOp->pObject)
	{
		pQuestDataEx->nMephistoBridgeGUID = pOp->pObject->dwUnitId;
	}
	else
	{
		pQuestDataEx->nMephistoBridgeGUID = -1;
	}

	UNITS_ChangeAnimMode(pOp->pObject, pQuestDataEx->nMephistoBridgeObjectMode);

	if (pQuestDataEx->nMephistoBridgeObjectMode != OBJMODE_OPENED)
	{
		EVENT_SetEvent(pOp->pGame, pOp->pObject, EVENTTYPE_QUESTFN, pOp->pGame->dwGameFrame + 20, 0, 0);
	}
}

//D2Game.0x6FCAC3E0
void __fastcall OBJECTS_InitFunction44_HellGatePortal(ObjInitFn* pOp)
{
	if (DUNGEON_GetLevelIdFromRoom(pOp->pRoom) == LEVEL_OUTERSTEPPES)
	{
		UNITS_ChangeAnimMode(pOp->pObject, OBJMODE_OPENED);
		return;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A3Q6_MEPHISTO);
	if (!pQuestData)
	{
		return;
	}

	Act3Quest6* pQuestDataEx = (Act3Quest6*)pQuestData->pQuestDataEx;
	pQuestDataEx->bHellGatePortalInitialized = 1;

	if (pOp->pObject)
	{
		pQuestDataEx->nHellGatePortalGUID = pOp->pObject->dwUnitId;
		UNITS_ChangeAnimMode(pOp->pObject, pQuestDataEx->nHellGatePortalObjectMode);
	}
	else
	{
		pQuestDataEx->nHellGatePortalGUID = -1;
		UNITS_ChangeAnimMode(pOp->pObject, pQuestDataEx->nHellGatePortalObjectMode);
	}
}

//D2Game.0x6FCAC450
void __fastcall ACT3Q6_InitQuestData(QuestData* pQuestData)
{
	memset(pQuestData->pfCallback, 0x00, sizeof(pQuestData->pfCallback));
	pQuestData->fState = 0;
	pQuestData->pfStatusFilter = nullptr;
	pQuestData->pNPCMessages = gpAct3Q6NpcMessages;
	pQuestData->bActive = 1;
	pQuestData->pfCallback[QUESTEVENT_MONSTERKILLED] = ACT3Q6_Callback08_MonsterKilled;
	pQuestData->pfCallback[QUESTEVENT_NPCACTIVATE] = ACT3Q6_Callback00_NpcActivate;
	pQuestData->pfCallback[QUESTEVENT_SCROLLMESSAGE] = ACT3Q6_Callback11_ScrollMessage;
	pQuestData->pfCallback[QUESTEVENT_CHANGEDLEVEL] = ACT3Q6_Callback03_ChangedLevel;
	pQuestData->pfCallback[QUESTEVENT_PLAYERSTARTEDGAME] = ACT3Q6_Callback13_PlayerStartedGame;
	pQuestData->pfCallback[QUESTEVENT_PLAYERLEAVESGAME] = ACT3Q6_Callback10_PlayerLeavesGame;
	pQuestData->pfSeqFilter = ACT3Q6_SeqCallback;
	pQuestData->nQuestFilter = QUESTSTATEFLAG_A3Q6;
	pQuestData->nInitNo = 6;
	pQuestData->pfActiveFilter = ACT3Q6_ActiveFilterCallback;

	Act3Quest6* pQuestDataEx = D2_ALLOC_STRC_POOL(pQuestData->pGame->pMemoryPool, Act3Quest6);
	memset(pQuestDataEx, 0x00, sizeof(Act3Quest6));
	pQuestData->pQuestDataEx = pQuestDataEx;
}

//D2Game.0x6FCAC510
void __fastcall ACT3Q6_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	static const int32_t nIndices[] =
	{
		-1, 0, 1, 2, 3, 4, 5, 6
	};

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	int32_t nNpcId = -1;
	if (pQuestArg->pTarget)
	{
		nNpcId = pQuestArg->pTarget->dwClassId;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, (QuestStateFlagIds)pQuestData->nQuestFilter, QFLAG_CUSTOM7))
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, 5);
		return;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED) == 1)
	{
		if (QUESTS_CheckPlayerGUID(pQuestData, (pQuestArg->pPlayer ? pQuestArg->pPlayer->dwUnitId : -1)) == 1)
		{
			QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, 6);
		}
	}
	else if (pQuestData->fState < 6)
	{
		const int32_t nIndex = nIndices[pQuestData->fState];
		if (nIndex != -1 && nIndex < 8)
		{
			QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, nIndex);
		}
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE) == 1)
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, nNpcId, 6);
	}
}

//D2Game.0x6FCAC610
void __fastcall ACT3Q6_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (pQuestArg->nOldLevel == LEVEL_KURASTDOCKTOWN)
	{
		QUESTS_QuickRemovePlayerGUID(pQuestData, pQuestArg);

		if (pQuestData->fState > 1 && pQuestData->fState <= 3)
		{
			BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];
			if (QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_REWARDGRANTED) != 1 && QUESTRECORD_GetQuestState(pQuestFlags, pQuestData->nQuestFilter, QFLAG_CUSTOM7) != 1)
			{
				if (pQuestData->fLastState < 3)
				{
					pQuestData->dwFlags &= 0xFFFFFF00;
					QUESTS_UnitIterate(pQuestData, 2, 0, ACT3Q6_UnitIterate_StatusCyclerEx, 0);
				}

				if (QUESTS_GetGlobalState(pQuestArg->pGame, QUESTSTATEFLAG_A3Q1, QFLAG_PRIMARYGOALDONE) == 1)
				{
					QUESTS_StateDebug(pQuestData, 4, __FILE__, __LINE__);
				}
				else
				{
					QUESTS_StateDebug(pQuestData, 5, __FILE__, __LINE__);
				}

				SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT3Q6_UnitIterate_UpdateQuestStateFlags);
			}
		}
	}

	if (pQuestArg->nNewLevel >= LEVEL_RUINEDFANE && pQuestData->bNotIntro && !pQuestData->fState)
	{
		pQuestData->fState = 1;
		pQuestData->pfCallback[QUESTEVENT_NPCDEACTIVATE] = ACT3Q6_Callback02_NpcDeactivate;
	}

	if (pQuestArg->nNewLevel == LEVEL_DURANCEOFHATELEV1)
	{
		if (pQuestData->fLastState == 2 || !pQuestData->fLastState)
		{
			pQuestData->dwFlags &= 0xFFFFFF00;
			QUESTS_UnitIterate(pQuestData, 3, 0, ACT3Q6_UnitIterate_StatusCyclerEx, 1);
		}

		if (pQuestData->fState != 1)
		{
			if (pQuestData->fLastState == 2 || !pQuestData->fLastState)
			{
				SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT3Q6_UnitIterate_UpdateQuestStateFlags);
			}
		}
		else if (QUESTS_GetGlobalState(pQuestArg->pGame, QUESTSTATEFLAG_A3Q1, QFLAG_PRIMARYGOALDONE) == 1)
		{
			QUESTS_StateDebug(pQuestData, 4, __FILE__, __LINE__);
			SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT3Q6_UnitIterate_UpdateQuestStateFlags);
		}
		else
		{
			QUESTS_StateDebug(pQuestData, 5, __FILE__, __LINE__);
			SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT3Q6_UnitIterate_UpdateQuestStateFlags);
		}
	}
	else if (pQuestArg->nNewLevel == LEVEL_DURANCEOFHATELEV3)
	{
		if (pQuestData->fLastState != 4)
		{
			pQuestData->dwFlags &= 0xFFFFFF00;
			QUESTS_UnitIterate(pQuestData, 4, 0, ACT3Q6_UnitIterate_StatusCyclerEx, 1);
		}

		if (pQuestData->fState == 4 || pQuestData->fState == 5)
		{
			SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT3Q6_UnitIterate_UpdateQuestStateFlags);
		}
		else if (QUESTS_GetGlobalState(pQuestArg->pGame, QUESTSTATEFLAG_A3Q1, QFLAG_PRIMARYGOALDONE) == 1)
		{
			QUESTS_StateDebug(pQuestData, 4, __FILE__, __LINE__);
			SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT3Q6_UnitIterate_UpdateQuestStateFlags);
		}
		else
		{
			QUESTS_StateDebug(pQuestData, 5, __FILE__, __LINE__);
			SUNIT_IterateUnitsOfType(pQuestData->pGame, 0, 0, ACT3Q6_UnitIterate_UpdateQuestStateFlags);
		}
	}
}

//D2Game.0x6FCAC7F0
int32_t __fastcall ACT3Q6_UnitIterate_UpdateQuestStateFlags(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED) == 1 || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7) == 1)
	{
		return 0;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A3Q6_MEPHISTO);
	if (!pQuestData)
	{
		return 0;
	}

	switch (pQuestData->fState)
	{
	case 2:
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_STARTED);
		return 0;

	case 3:
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_LEAVETOWN);
		return 0;

	case 4:
		if (pQuestData->fLastState == 2)
		{
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_ENTERAREA);
		}
		else if (pQuestData->fLastState == 3)
		{
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM1);
		}
		else if (pQuestData->fLastState == 4)
		{
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM2);
		}
		return 0;

	case 5:
		if (pQuestData->fLastState == 2)
		{
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM3);
		}
		else if (pQuestData->fLastState == 3)
		{
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM4);
		}
		else if (pQuestData->fLastState == 4)
		{
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM5);
		}
		return 0;
	default:
		break;
	}

	return 0;
}

//D2Game.0x6FCAC910
int32_t __fastcall ACT3Q6_UnitIterate_StatusCyclerEx(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_COMPLETEDBEFORE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_COMPLETEDNOW))
	{
		QUESTS_StatusCyclerEx(pGame, pUnit, QUEST_A3Q6_MEPHISTO);
	}

	return 0;
}

//D2Game.0x6FCAC980
void __fastcall ACT3Q6_Callback02_NpcDeactivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (!pQuestArg->pTarget || pQuestArg->pTarget->dwClassId != MONSTER_ORMUS)
	{
		return;
	}

	Act3Quest6* pQuestDataEx = (Act3Quest6*)pQuestData->pQuestDataEx;
	if (pQuestDataEx->bOrmusActivated != 1)
	{
		return;
	}

	pQuestData->dwFlags &= 0xFFFFFF00;
	QUESTS_UnitIterate(pQuestData, 2, 0, ACT3Q6_UnitIterate_StatusCyclerEx, 1);
	pQuestDataEx->bOrmusActivated = 0;
	pQuestData->pfCallback[QUESTEVENT_NPCDEACTIVATE] = nullptr;
}

//D2Game.0x6FCAC9D0
void __fastcall ACT3Q6_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];
	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7) && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED))
	{
		return;
	}

	const int16_t nMessageIndex = pQuestArg->nMessageIndex;
	if (pQuestArg->nNPCNo != MONSTER_ORMUS || nMessageIndex != 628)
	{
		if (nMessageIndex == 658 || nMessageIndex == 657 || nMessageIndex == 659 || nMessageIndex == 660 || nMessageIndex == 661 || nMessageIndex == 662 || nMessageIndex == 663)
		{
			if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7))
			{
				if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE))
				{
					pQuestData->dwFlags &= 0xFFFFFF00;
					QUESTS_UnitIterate(pQuestData, 13, 0, ACT3Q6_UnitIterate_StatusCyclerEx, 0);
					QUESTS_StateDebug(pQuestData, 7, __FILE__, __LINE__);
				}
				QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7);
				QUESTS_AddPlayerGUID(&pQuestData->tPlayerGUIDs, (pQuestArg->pPlayer ? pQuestArg->pPlayer->dwUnitId : -1));
			}
			QUESTS_NPCActivateSpeeches(pQuestArg->pGame, pQuestArg->pPlayer, pQuestArg->pTarget);
		}
	}
	else
	{
		if (QUESTS_GetGlobalState(pQuestArg->pGame, QUESTSTATEFLAG_A3Q1, QFLAG_PRIMARYGOALDONE) == 1)
		{
			QUESTS_StateDebug(pQuestData, 2, __FILE__, __LINE__);
		}
		else
		{
			QUESTS_StateDebug(pQuestData, 3, __FILE__, __LINE__);
		}

		((Act3Quest6*)pQuestData->pQuestDataEx)->bOrmusActivated = 1;
		QUESTS_NPCActivateSpeeches(pQuestArg->pGame, pQuestArg->pPlayer, pQuestArg->pTarget);
	}
}

//D2Game.0x6FCACB30
void __fastcall ACT3Q6_Callback08_MonsterKilled(QuestData* pQuestData, QuestArg* pQuestArg)
{
	QUESTS_DebugOutput(pQuestData->pGame, "killed boss for quest", __FILE__, __LINE__);
	QUESTS_StateDebug(pQuestData, 6, __FILE__, __LINE__);

	Act3Quest6* pQuestDataEx = (Act3Quest6*)pQuestData->pQuestDataEx;
	pQuestData->pfCallback[QUESTEVENT_PLAYERLEAVESGAME] = ACT3Q6_Callback10_PlayerLeavesGame;
	if (pQuestData->bNotIntro)
	{
		if (pQuestArg->pPlayer)
		{
			BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestData->pGame->nDifficulty];
			if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED))
			{
				//D2Game_Return(62);

				if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7))
				{
					++pQuestDataEx->nSoulstonesToDrop;
				}

				QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE);
				QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED);
				QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7);

				GameClient* pClient = SUNIT_GetClientFromPlayer(pQuestArg->pPlayer, __FILE__, __LINE__);
				CLIENTS_UpdateCharacterProgression(pClient, 3, pQuestArg->pGame->nDifficulty);
			}
		}

		SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT3Q6_UnitIterate_UpdateQuestStateAfterMonsterKill);
		SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT3Q6_UnitIterate_SetPrimaryGoalDoneForPartyMembers);
		SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT3Q6_UnitIterate_SetCompletionFlag);
		SUNIT_IterateUnitsOfType(pQuestArg->pGame, 0, pQuestArg->pTarget, ACT3Q6_UnitIterate_AttachCompletionSound);

		if (!pQuestDataEx->bTimerStarted)
		{
			QUESTS_CreateTimer(pQuestData, ACT3Q6_Timer_StatusCycler, 12);
			pQuestDataEx->bTimerStarted = 1;
		}
	}

	QUESTS_SetGlobalState(pQuestArg->pGame, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE);

	if (pQuestDataEx->bHellGatePortalInitialized)
	{
		UnitAny* pObject = SUNIT_GetServerUnit(pQuestData->pGame, UNIT_OBJECT, pQuestDataEx->nHellGatePortalGUID);
		if (pObject)
		{
			UNITS_ChangeAnimMode(pObject, OBJMODE_OPERATING);
			ObjectsTxt* pObjectsTxtRecord = DATATBLS_GetObjectsTxtRecord(pObject->dwClassId);
			EVENT_SetEvent(pQuestData->pGame, pObject, EVENTTYPE_ENDANIM, pQuestData->pGame->dwGameFrame + (pObjectsTxtRecord->dwFrameCnt[1] >> 8), 0, 0);
		}
	}
	pQuestDataEx->nHellGatePortalObjectMode = OBJMODE_OPENED;

	pQuestArg->pTarget->dwDropItemCode = ' ssm';
	for (int32_t i = 0; i < pQuestDataEx->nSoulstonesToDrop; ++i)
	{
		int32_t nItemLevel = 0;
		if (D2GAME_DropItemAtUnit_6FC4FEC0(pQuestArg->pGame, pQuestArg->pTarget, ITEMQUAL_NORMAL, &nItemLevel, 0, -1, 0))
		{
			pQuestDataEx->bSoulStoneDropped = 1;
			++pQuestDataEx->nSoulStonesDropped;
		}
	}

	//D2Game_Return();
	QUESTS_TriggerFX(pQuestData->pGame, 11);
}

//D2Game.0x6FCACD80
int32_t __fastcall ACT3Q6_UnitIterate_UpdateQuestStateAfterMonsterKill(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	Room1* pRoom = UNITS_GetRoom(pUnit);
	if (!pRoom || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED) || DUNGEON_GetLevelIdFromRoom(pRoom) != LEVEL_DURANCEOFHATELEV3)
	{
		return 0;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A3Q6_MEPHISTO);
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7))
	{
		return 0;
	}

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE);
	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED);
	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7);

	GameClient* pClient = SUNIT_GetClientFromPlayer(pUnit, __FILE__, __LINE__);
	CLIENTS_UpdateCharacterProgression(pClient, 3, pGame->nDifficulty);

	++((Act3Quest6*)pQuestData->pQuestDataEx)->nSoulstonesToDrop;

	return 0;
}

//D2Game.0x6FCACE30
bool __fastcall ACT3Q6_Timer_StatusCycler(Game* pGame, QuestData* pQuestData)
{
	Act3Quest6* pQuestDataEx = (Act3Quest6*)pQuestData->pQuestDataEx;

	if (pQuestData->fLastState != 4)
	{
		pQuestData->dwFlags &= 0xFFFFFF00;
		QUESTS_UnitIterate(pQuestData, 4, 0, ACT3Q6_UnitIterate_StatusCyclerEx, 1);
	}
	pQuestDataEx->bTimerStarted = 0;

	return 1;
}

//D2Game.0x6FCACE60
int32_t __fastcall ACT3Q6_UnitIterate_SetPrimaryGoalDoneForPartyMembers(Game* pGame, UnitAny* pUnit, void* pData)
{
	if (!QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty], QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE))
	{
		return 0;
	}

	const int16_t nPartyId = SUNIT_GetPartyId(pUnit);
	if (nPartyId != -1)
	{
		PARTY_IteratePartyMembers(pGame, nPartyId, ACT3Q6_UnitIterate_SetPrimaryGoalDone, 0);
	}

	return 0;
}

//D2Game.0x6FCACEB0
int32_t __fastcall ACT3Q6_UnitIterate_SetCompletionFlag(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED) != 1 && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7) != 1)
	{
		QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_COMPLETEDNOW);
		QUESTS_SendLogUpdateEx(pUnit, QUEST_A3Q6_MEPHISTO, 0);
	}

	return 0;
}

//D2Game.0x6FCACF10
int32_t __fastcall ACT3Q6_UnitIterate_AttachCompletionSound(Game* pGame, UnitAny* pUnit, void* pData)
{
	if (QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty], QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE))
	{
		SUNIT_AttachSound(pUnit, 66, pUnit);
	}

	return 0;
}

//D2Game.0x6FCACF50
bool __fastcall ACT3Q6_SeqCallback(QuestData* pQuestData)
{
	if (!pQuestData->fState && pQuestData->bNotIntro)
	{
		pQuestData->fState = 1;
		pQuestData->pfCallback[QUESTEVENT_NPCDEACTIVATE] = ACT3Q6_Callback02_NpcDeactivate;
	}

	return true;
}

//D2Game.0x6FCACF80
void __fastcall ACT3Q6_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_REWARDGRANTED) == 1
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM7) == 1
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3COMPLETED, QFLAG_REWARDGRANTED) == 1)
	{
		QUESTS_SetGlobalState(pQuestData->pGame, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE);
		((Act3Quest6*)pQuestData->pQuestDataEx)->nHellGatePortalObjectMode = OBJMODE_OPENED;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM5) == 1)
	{
		pQuestData->fState = 5;
		pQuestData->fLastState = 4;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM4) == 1)
	{
		pQuestData->fState = 5;
		pQuestData->fLastState = 3;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM3) == 1)
	{
		pQuestData->fState = 5;
		pQuestData->fLastState = 2;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM2) == 1)
	{
		pQuestData->fState = 4;
		pQuestData->fLastState = 4;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_CUSTOM1) == 1)
	{
		pQuestData->fState = 4;
		pQuestData->fLastState = 3;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_ENTERAREA) == 1)
	{
		pQuestData->fState = 4;
		pQuestData->fLastState = 2;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_LEAVETOWN) == 1)
	{
		pQuestData->fState = 2;
		pQuestData->fLastState = 2;
	}
	else if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A3Q6, QFLAG_STARTED) == 1)
	{
		pQuestData->fState = 3;
		pQuestData->fLastState = 2;
	}
}

//D2Game.0x6FCAD0C0
void __fastcall OBJECTS_InitFunction52_NatalyaStart(ObjInitFn* pOp)
{
	if (QUESTS_GetGlobalState(pOp->pGame, QUESTSTATEFLAG_A3Q6, QFLAG_PRIMARYGOALDONE))
	{
		return;
	}

	Act3Quest6* pQuestDataEx = (Act3Quest6*)QUESTS_GetQuestData(pOp->pGame, QUEST_A3Q6_MEPHISTO)->pQuestDataEx;

	if (pQuestDataEx->bNatalyaSpawned)
	{
		UnitAny* pNatalya = SUNIT_GetServerUnit(pOp->pGame, UNIT_MONSTER, pQuestDataEx->nNatalyaGUID);
		if (pNatalya && pNatalya->dwClassId == MONSTER_NATALYA)
		{
			return;
		}
	}

	UnitAny* pNatalya = D2GAME_SpawnMonster_6FC69F10(pOp->pGame, pOp->pRoom, pOp->nX, pOp->nY, MONSTER_NATALYA, 1, -1, 0);
	if (!pNatalya)
	{
		pNatalya = D2GAME_SpawnMonster_6FC69F10(pOp->pGame, pOp->pRoom, pOp->nX, pOp->nY, MONSTER_NATALYA, 1, 3, 0);
	}

	if (!pNatalya)
	{
		pQuestDataEx->bNatalyaSpawned = 0;
		return;
	}

	pQuestDataEx->bNatalyaSpawned = 1;
	if (pNatalya)
	{
		pQuestDataEx->nNatalyaGUID = pNatalya->dwUnitId;
	}
	else
	{
		pQuestDataEx->nNatalyaGUID = -1;
	}

	if (pNatalya && pQuestDataEx->pNatalyaMapAI && pQuestDataEx->pNatalyaMapAI->pPosition && !pQuestDataEx->bNatalyaMapAIChanged)
	{
		DRLGPRESET_ChangeMapAI(&pQuestDataEx->pNatalyaMapAI, AIGENERAL_GetMapAiFromUnit(pNatalya));
		pQuestDataEx->bNatalyaMapAIChanged = 1;
	}
}

//D2Game.0x6FCAD1A0
void __fastcall ACT3Q6_SetObjectModes(Game* pGame)
{
	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A3Q6_MEPHISTO);
	if (!pQuestData)
	{
		return;
	}

	Act3Quest6* pQuestDataEx = (Act3Quest6*)pQuestData->pQuestDataEx;

	pQuestDataEx->nHellGatePortalObjectMode = OBJMODE_OPENED;
	pQuestDataEx->nMephistoBridgeObjectMode = OBJMODE_OPENED;

	if (pQuestDataEx->bHellGatePortalInitialized)
	{
		UnitAny* pObject = SUNIT_GetServerUnit(pGame, UNIT_OBJECT, pQuestDataEx->nHellGatePortalGUID);
		if (pObject)
		{
			UNITS_ChangeAnimMode(pObject, OBJMODE_OPERATING);
		}
	}

	if (pQuestDataEx->bMephistoBridgeInitialized)
	{
		UnitAny* pObject = SUNIT_GetServerUnit(pGame, UNIT_OBJECT, pQuestDataEx->nMephistoBridgeGUID);
		if (pObject)
		{
			UNITS_ChangeAnimMode(pObject, OBJMODE_OPENED);
		}
	}
}

//
void __fastcall ACT3Q6_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	QUESTS_RemovePlayerGUID(pQuestData, pQuestArg);
}
