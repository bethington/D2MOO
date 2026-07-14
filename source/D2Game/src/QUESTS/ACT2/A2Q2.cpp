#include "QUESTS/ACT2/A2Q2.h"

#include <D2BitManip.h>

#include <DataTbls/LevelsIds.h>
#include <DataTbls/ObjectsIds.h>
#include <DataTbls/MonsterIds.h>
#include <D2Dungeon.h>
#include <D2Items.h>
#include <D2QuestRecord.h>

#include "GAME/Game.h"
#include "ITEMS/Items.h"
#include "OBJECTS/ObjMode.h"
#include "QUESTS/Quests.h"
#include "QUESTS/ACT2/A2Q3.h"
#include "QUESTS/ACT2/A2Q4.h"
#include "QUESTS/ACT2/A2Q6.h"
#include "UNIT/SUnit.h"


//D2Game.0x6FD35238
NPCMessageTable gpAct2Q2NpcMessages[] =
{
	{
		{
			{ MONSTER_CAIN2, 335, 0, 0 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 336, 0, 0 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 337, 0, 0 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 338, 0, 0 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 339, 0, 0 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 339, 0, 2 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 335, 0, 2 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 336, 0, 2 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 337, 0, 2 },
		},
		1
	},
	{
		{
			{ MONSTER_CAIN2, 338, 0, 2 },
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


//D2Game.0x6FCA01F0
bool __fastcall ACT2Q2_ActiveFilterCallback(QuestData* pQuest, int32_t nNpcId, UnitAny* pPlayer, BitBuffer* pQuestFlags, UnitAny* pNPC)
{
	if (nNpcId != MONSTER_CAIN2)
	{
		return false;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDPENDING) != 1 && ACT2Q2_CheckItemsAndState(pQuest, pPlayer, pQuestFlags->pBuffer))
	{
		return true;
	}

	return false;
}

//D2Game.0x6FCA0240
bool __fastcall ACT2Q2_CheckItemsAndState(QuestData* pQuestData, UnitAny* pUnit, uint8_t* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pQuestData->pGame->nDifficulty];

	*pData = -1;

	if (ITEMS_FindQuestItem(pQuestData->pGame, pUnit, ' tsh'))
	{
		if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM6))
		{
			*pData = 5;
			return false;
		}
		else
		{
			*pData = 4;
			return true;
		}
	}
	else
	{
		if (ITEMS_FindQuestItem(pQuestData->pGame, pUnit, ' xob'))
		{
			if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM2))
			{
				*pData = 3;
				return true;
			}
			*pData = 9;
		}

		if (ITEMS_FindQuestItem(pQuestData->pGame, pUnit, ' 1rt'))
		{
			if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN))
			{
				*pData = 0;
				return true;
			}
			*pData = 6;
		}

		if (ITEMS_FindQuestItem(pQuestData->pGame, pUnit, ' piv'))
		{
			if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_ENTERAREA))
			{
				*pData = 1;
				return true;
			}
			*pData = 7;
		}

		if (ITEMS_FindQuestItem(pQuestData->pGame, pUnit, ' fsm'))
		{
			if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM1))
			{
				*pData = 2;
				return true;
			}
			*pData = 8;
		}

		return false;
	}
}

//D2Game.0x6FCA0370
void __fastcall ACT2Q2_UpdateHoradricItemCounts(Game* pGame, UnitAny* pUnit)
{
	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A2Q2_HORADRICSTAFF);
	if (pQuestData)
	{
		Act2Quest2* pQuestDataEx = (Act2Quest2*)pQuestData->pQuestDataEx;

		pQuestDataEx->bStaffCubed = 1;
		if (pUnit)
		{
			pQuestDataEx->nPlayerGUID = pUnit->dwUnitId;
		}
		else
		{
			pQuestDataEx->nPlayerGUID = -1;
		}

		--pQuestDataEx->nBaseStaffs;
		--pQuestDataEx->nAmulets;
		++pQuestDataEx->nCubedStaffs;
	}

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	QUESTS_UpdatePlayerFlags(pGame, pUnit);

	QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM7);

	QuestData* pQuestData11 = QUESTS_GetQuestData(pGame, QUEST_A2Q4_HORAZONTOME);
	if (!pQuestData11)
	{
		return;
	}

	Act2Quest4* pQuestDataEx11 = (Act2Quest4*)pQuestData11->pQuestDataEx;
	if (pQuestData11->bNotIntro)
	{
		if (!pQuestData11->fState)
		{
			QUESTS_StateDebug(pQuestData11, 1, __FILE__, __LINE__);
		}

		if (!pQuestData11->fLastState)
		{
			pQuestData11->dwFlags &= 0xFFFFFF00;
			QUESTS_UnitIterate(pQuestData11, 1, 0, ACT2Q4_UnitIterate_StatusCyclerEx, 1);
		}
	}
	else
	{
		pQuestDataEx11->bPalaceOpen = 1;
		if (pQuestDataEx11->nHaremBlockerObjectMode == OBJMODE_NEUTRAL)
		{
			pQuestDataEx11->bIsHaremBlockerNeutral = 1;
		}
		pQuestDataEx11->nHaremBlockerObjectMode = OBJMODE_OPENED;

		SUNIT_IterateUnitsOfType(pQuestData11->pGame, 0, 0, ACT2Q4_UnitIterate_UpdateQuestStateFlags);

		if (pQuestDataEx11->bHaremBlockerInitialized != 1)
		{
			return;
		}

		UnitAny* pHaremBlocker = SUNIT_GetServerUnit(pQuestData11->pGame, UNIT_OBJECT, pQuestDataEx11->nHaremBlockerGUID);
		if (!pHaremBlocker)
		{
			return;
		}

		UNITS_ChangeAnimMode(pHaremBlocker, OBJMODE_OPENED);
		UNITS_FreeCollisionPath(pHaremBlocker);
	}
}

//D2Game.0x6FCA0490
bool __fastcall ACT2Q2_StatusFilterCallback(QuestData* pQuest, UnitAny* pPlayer, BitBuffer* pGlobalFlags, BitBuffer* pFlags, uint8_t* pStatus)
{
	if (!QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A1COMPLETED, QFLAG_REWARDGRANTED))
	{
		*pStatus = 0;
		return true;
	}

	if (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED) || QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDPENDING))
	{
		if (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q6, QFLAG_REWARDGRANTED))
		{
			*pStatus = 2 * (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE) != 0) + 11;
			return true;
		}

		*pStatus = 6 - (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM6) != 0);
		return true;
	}

	*pStatus = 0;

	UnitAny* pHoradricScroll = ITEMS_FindQuestItem(pQuest->pGame, pPlayer, ' 1rt');
	UnitAny* pBaseStaff = ITEMS_FindQuestItem(pQuest->pGame, pPlayer, ' fsm');
	UnitAny* pBox = ITEMS_FindQuestItem(pQuest->pGame, pPlayer, ' xob');
	UnitAny* pAmulet = ITEMS_FindQuestItem(pQuest->pGame, pPlayer, ' piv');
	UnitAny* pCubedStaff = ITEMS_FindQuestItem(pQuest->pGame, pPlayer, ' tsh');

	if (!pBaseStaff || !pBox || !pAmulet)
	{
		if (!pCubedStaff)
		{
			if (pHoradricScroll && !QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN))
			{
				*pStatus = 1;
				return true;
			}

			QuestData* pQuestData10 = nullptr;
			Act2Quest2* pQuestDataEx = (Act2Quest2*)pQuest->pQuestDataEx;
			if (pQuestDataEx->bCubeDropped
				&& !pQuestDataEx->nHoradricCubes || !pQuestDataEx->nAmulets
				&& (pQuestData10 = QUESTS_GetQuestData(pQuest->pGame, QUEST_A2Q3_TAINTEDSUN)) != 0 // NOLINT(bugprone-assignment-in-if-condition)
				&& ((Act2Quest3*)pQuestData10->pQuestDataEx)->bAltarDestroyed || pQuestDataEx->bHoradricScrollDropped && !pQuestDataEx->nBaseStaffs)
			{
				*pStatus = 9;
			}

			if (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN))
			{
				*pStatus = 2;
				return true;
			}

			if (pAmulet || pBaseStaff || pBox)
			{
				*pStatus = 4;
				return true;
			}
			return true;
		}
		*pStatus = 6 - (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM6) != 0);
		return true;
	}

	if (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM2))
	{
		*pStatus = 3;
	}
	else
	{
		*pStatus = 2 * (QUESTRECORD_GetQuestState(pFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN) == 0) + 2;
	}

	return true;
}

//D2Game.0x6FCA06C0
void __fastcall ACT2Q2_InitQuestData(QuestData* pQuestData)
{
	memset(pQuestData->pfCallback, 0x00, sizeof(pQuestData->pfCallback));
	pQuestData->pNPCMessages = gpAct2Q2NpcMessages;
	pQuestData->bActive = 1;

	Act2Quest2* pQuestDataEx = D2_ALLOC_STRC_POOL(pQuestData->pGame->pMemoryPool, Act2Quest2);
	pQuestData->pQuestDataEx = pQuestDataEx;
	pQuestData->nQuestFilter = QUESTSTATEFLAG_A2Q2;
	pQuestData->pfStatusFilter = ACT2Q2_StatusFilterCallback;
	pQuestData->nInitNo = 0;
	pQuestData->pfActiveFilter = ACT2Q2_ActiveFilterCallback;
	pQuestData->pfCallback[QUESTEVENT_CHANGEDLEVEL] = ACT2Q2_Callback03_ChangedLevel;
	pQuestData->pfCallback[QUESTEVENT_NPCACTIVATE] = ACT2Q2_Callback00_NpcActivate;
	pQuestData->pfCallback[QUESTEVENT_SCROLLMESSAGE] = ACT2Q2_Callback11_ScrollMessage;
	pQuestData->pfCallback[QUESTEVENT_PLAYERLEAVESGAME] = ACT2Q2_Callback10_PlayerLeavesGame;
	pQuestData->pfCallback[QUESTEVENT_ITEMPICKEDUP] = ACT2Q2_Callback04_ItemPickedUp;
	pQuestData->pfCallback[QUESTEVENT_ITEMDROPPED] = ACT2Q2_Callback05_ItemDropped;
	//pQuestData->pfCallback[8] = (QUESTCALLBACK)D2GAME_OBJECTS_InitFunction05_Door_6FCA6660;
	pQuestData->pfCallback[QUESTEVENT_PLAYERSTARTEDGAME] = ACT2Q2_Callback13_PlayerStartedGame;
	pQuestData->pfCallback[QUESTEVENT_PLAYERJOINEDGAME] = ACT2Q2_Callback14_PlayerJoinedGame;
	pQuestData->pfCallback[QUESTEVENT_PLAYERDROPPEDWITHQUESTITEM] = ACT2Q2_Callback09_PlayerDroppedWithQuestItem;
	memset(pQuestDataEx, 0x00, sizeof(Act2Quest2));
	pQuestData->fLastState = 13;
}

//D2Game.0x6FCA07A0
void __fastcall ACT2Q2_Callback11_ScrollMessage(QuestData* pQuestData, QuestArg* pQuestArg)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	if (pQuestArg->nNPCNo == MONSTER_CAIN2)
	{
		switch (pQuestArg->nMessageIndex)
		{
		case 335:
			QUESTS_DeleteItem(pQuestArg->pGame, pQuestArg->pPlayer, ' 1rt');
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN);
			break;

		case 336:
			pQuestData->dwFlags &= 0xFFFFFF00;
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_ENTERAREA);
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN);
			break;

		case 337:
			pQuestData->dwFlags &= 0xFFFFFF00;
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM1);
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN);
			break;

		case 338:
			pQuestData->dwFlags &= 0xFFFFFF00;
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM2);
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN);
			break;

		case 339:
			QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDPENDING);
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN);

			QUESTS_AddPlayerGUID(&pQuestData->tPlayerGUIDs, (pQuestArg->pPlayer ? pQuestArg->pPlayer->dwUnitId : -1));

			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM6);
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_ENTERAREA);
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM2);
			QUESTRECORD_SetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM1);
			QUESTS_NPCActivateSpeeches(pQuestArg->pGame, pQuestArg->pPlayer, pQuestArg->pTarget);
			break;

		default:
			return;
		}
	}
}

//D2Game.0x6FCA08C0
void __fastcall ACT2Q2_Callback00_NpcActivate(QuestData* pQuestData, QuestArg* pQuestArg)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	if (!pQuestArg->pTarget || pQuestArg->pTarget->dwClassId != MONSTER_CAIN2)
	{
		return;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDPENDING))
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_CAIN2, 4);
		return;
	}

	uint8_t pData = 0;
	if (ACT2Q2_CheckItemsAndState(pQuestData, pQuestArg->pPlayer, &pData) && pData != uint8_t(-1))
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_CAIN2, pData);
		return;
	}

	if (QUESTS_CheckPlayerGUID(pQuestData, (pQuestArg->pPlayer ? pQuestArg->pPlayer->dwUnitId : -1)))
	{
		QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_CAIN2, 5);
		return;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED) == 1)
	{
		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE) == 1) // TODO: This condition looks weird
		{
			return;
		}
	}

	if (pData > 9)
	{
		if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN))
		{
			return;
		}

		pData = 6;
	}

	QUESTS_InitScrollTextChain(pQuestData, pQuestArg->pTextControl, MONSTER_CAIN2, pData);
}

//D2Game.0x6FCA09C0
void __fastcall ACT2Q2_Callback03_ChangedLevel(QuestData* pQuestData, QuestArg* pQuestArg)
{
	if (pQuestArg->nOldLevel == LEVEL_LUTGHOLEIN)
	{
		QUESTS_QuickRemovePlayerGUID(pQuestData, pQuestArg);
	}
}

//D2Game.0x6FCA09D0
void __fastcall ACT2Q2_Callback04_ItemPickedUp(QuestData* pQuestData, QuestArg* pQuestArg)
{
	int32_t nItemId = -1;
	if (pQuestArg->pTarget)
	{
		nItemId = pQuestArg->pTarget->dwClassId;
	}

	ItemsTxt* pItemsTxtRecord = DATATBLS_GetItemsTxtRecord(nItemId);
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	pQuestData->dwFlags &= 0xFFFFFF00;

	Room1* pRoom = UNITS_GetRoom(pQuestArg->pPlayer);
	const int32_t nLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);

	// TODO: Remove useless code
	if (pQuestData->nActNo == DRLG_GetActNoFromLevelId(nLevelId))
	{
		pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];
		if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDBEFORE))
		{
			if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE))
			{
				// TODO: Maybe was meant to be SetQuestState?
				QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDNOW);
			}
		}
	}

	if (pItemsTxtRecord->dwCode != ' 1rt' || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN))
	{
		if (pItemsTxtRecord->dwCode == ' piv' || pItemsTxtRecord->dwCode == ' xob')
		{
			if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN))
			{
				pQuestData->fLastState = 2;
				if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDBEFORE) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE))
				{
					QUESTS_StatusCyclerEx(pQuestData->pGame, pQuestArg->pPlayer, QUEST_A2Q2_HORADRICSTAFF);
					return;
				}
				if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDNOW))
				{
					return;
				}
			}
			else
			{
				pQuestData->fLastState = 6;
				if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDBEFORE) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE))
				{
					QUESTS_StatusCyclerEx(pQuestData->pGame, pQuestArg->pPlayer, QUEST_A2Q2_HORADRICSTAFF);
					return;
				}
				if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDNOW))
				{
					return;
				}
			}
		}
		else if (pItemsTxtRecord->dwCode == ' fsm')
		{
			if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN))
			{
				pQuestData->fLastState = 2;
				if (!ACT2Q2_IsQuestDone(pQuestData->pGame, pQuestArg->pPlayer))
				{
					return;
				}
			}
			else
			{
				pQuestData->fLastState = 6;
				if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDBEFORE) || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE))
				{
					QUESTS_StatusCyclerEx(pQuestData->pGame, pQuestArg->pPlayer, QUEST_A2Q2_HORADRICSTAFF);
					return;
				}
				if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDNOW))
				{
					return;
				}
			}
		}
		else
		{
			return;
		}
	}
	else
	{
		pQuestData->fLastState = 1;

		if ((QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED)
			|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDBEFORE)) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE))
		{
			if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDNOW))
			{
				return;
			}
		}
	}

	QUESTS_StatusCyclerEx(pQuestData->pGame, pQuestArg->pPlayer, QUEST_A2Q2_HORADRICSTAFF);
}

//D2Game.0x6FCA0CF0
int32_t __fastcall ACT2Q2_IsQuestDone(Game* pGame, UnitAny* pUnit)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];

	if (!QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED) && !QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDBEFORE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_PRIMARYGOALDONE)
		|| QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_COMPLETEDNOW) != 0)
	{
		return 1;
	}

	return 0;
}

//D2Game.0x6FCA0D50
void __fastcall ACT2Q2_Callback05_ItemDropped(QuestData* pQuestData, QuestArg* pQuestArg)
{
	int32_t nItemId = -1;
	if (pQuestArg->pTarget)
	{
		nItemId = pQuestArg->pTarget->dwClassId;
	}

	ItemsTxt* pItemsTxtRecord = DATATBLS_GetItemsTxtRecord(nItemId);

	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	if (pItemsTxtRecord->dwCode == ' piv' && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_ENTERAREA))
	{
		QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_ENTERAREA);
	}

	if (pItemsTxtRecord->dwCode == ' xob' && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM2))
	{
		QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM2);
	}

	if (pItemsTxtRecord->dwCode == ' fsm' && QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM1))
	{
		QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM1);
	}
}

//D2Game.0x6FCA0DF0
void __fastcall ACT2Q2_Callback13_PlayerStartedGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pQuestArg->pPlayer)->pQuestData[pQuestArg->pGame->nDifficulty];

	UnitAny* pBaseStaff = ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' fsm');
	UnitAny* pBox = ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' xob');
	UnitAny* pAmulet = ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' piv');
	UnitAny* pCubedStaff = ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' tsh');

	Act2Quest2* pQuestDataEx = (Act2Quest2*)pQuestData->pQuestDataEx;
	if (pBaseStaff)
	{
		++pQuestDataEx->nBaseStaffs;
	}

	if (pAmulet)
	{
		++pQuestDataEx->nAmulets;
	}

	if (pBox)
	{
		++pQuestDataEx->nHoradricCubes;
	}

	if (pCubedStaff)
	{
		++pQuestDataEx->nCubedStaffs;
	}

	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED))
	{
		return;
	}

	QuestData* pQuestData13 = QUESTS_GetQuestData(pQuestArg->pGame, QUEST_A2Q6_DURIEL);
	if (!pQuestData13 || !pQuestData13->bNotIntro || ((Act2Quest6*)pQuestData13->pQuestDataEx)->bDurielLairTombOpen || ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' tsh'))
	{
		return;
	}

	if (pBaseStaff)
	{
		if (!pBox)
		{
			QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM2);

			if (!pAmulet)
			{
				QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_ENTERAREA);
			}
			return;
		}

		if (pAmulet)
		{
			return;
		}
	}
	else
	{
		QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM1);
	}

	if (!pBox)
	{
		QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_CUSTOM2);
	}

	if (!pAmulet)
	{
		QUESTRECORD_ClearQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_ENTERAREA);
	}
}

//D2Game.0x6FCA0F10
void __fastcall ACT2Q2_Callback14_PlayerJoinedGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	Act2Quest2* pQuestDataEx = (Act2Quest2*)pQuestData->pQuestDataEx;
	if (ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' fsm'))
	{
		++pQuestDataEx->nBaseStaffs;
	}

	if (ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' piv'))
	{
		++pQuestDataEx->nAmulets;
	}

	if (ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' xob'))
	{
		++pQuestDataEx->nHoradricCubes;
	}

	if (ITEMS_FindQuestItem(pQuestArg->pGame, pQuestArg->pPlayer, ' tsh'))
	{
		++pQuestDataEx->nCubedStaffs;
	}

	if (!pQuestDataEx->unk0x2B || pQuestDataEx->nCubedStaffs <= 0 && (!pQuestDataEx->nHoradricCubes || !pQuestDataEx->nAmulets || !pQuestDataEx->nBaseStaffs))
	{
		return;
	}

	QuestData* pQuestData13 = QUESTS_GetQuestData(pQuestData->pGame, QUEST_A2Q6_DURIEL);
	if (!pQuestData13 || !pQuestData13->bNotIntro)
	{
		return;
	}

	Act2Quest6* pQuestDataEx13 = (Act2Quest6*)pQuestData13->pQuestDataEx;
	if (pQuestDataEx13->bStaffItemsRemoved != 1)
	{
		pQuestDataEx13->bHoradricStaffIsMissing = 0;
	}
}

//D2Game.0x6FCA0FC0
void __fastcall ACT2Q2_Callback09_PlayerDroppedWithQuestItem(QuestData* pQuestData, QuestArg* pQuestArg)
{
	Act2Quest2* pQuestDataEx = (Act2Quest2*)pQuestData->pQuestDataEx;

	switch (ITEMS_GetBaseCode(pQuestArg->pTarget))
	{
	case ' xob':
		--pQuestDataEx->nHoradricCubes;
		break;

	case ' tsh':
		--pQuestDataEx->nCubedStaffs;
		break;

	case ' fsm':
		--pQuestDataEx->nBaseStaffs;
		break;

	case ' piv':
		--pQuestDataEx->nAmulets;
		break;
	default:
		break;
	}

	if (pQuestDataEx->unk0x2B || pQuestDataEx->nCubedStaffs)
	{
		return;
	}

	if (!pQuestDataEx->bCubeDropped || pQuestDataEx->nHoradricCubes)
	{
		if (pQuestDataEx->nAmulets)
		{
			return;
		}
		QuestData* pQuestData10 = QUESTS_GetQuestData(pQuestData->pGame, QUEST_A2Q3_TAINTEDSUN);
		if(pQuestData10 == nullptr || !((Act2Quest3*)pQuestData10->pQuestDataEx)->bAltarDestroyed)
		{
			if (!pQuestDataEx->bHoradricScrollDropped || pQuestDataEx->nBaseStaffs)
			{
				return;
			}
		}
	}

	QuestData* pQuestData13 = QUESTS_GetQuestData(pQuestData->pGame, QUEST_A2Q6_DURIEL);
	if (pQuestData13 && pQuestData13->bNotIntro)
	{
		Act2Quest6* pQuestDataEx13 = (Act2Quest6*)pQuestData13->pQuestDataEx;
		if (pQuestDataEx13->bStaffItemsRemoved != 1)
		{
			pQuestDataEx13->bHoradricStaffIsMissing = 1;
			if (pQuestData13->pGame->nGameType == 3)
			{
				pQuestDataEx13->nQuestStatus = 8;
				pQuestDataEx->unk0x2B = 1;
				return;
			}
			pQuestDataEx13->nQuestStatus = 9;
		}
	}
	pQuestDataEx->unk0x2B = 1;
}

//D2Game.0x6FCA10B0
int32_t __fastcall OBJECTS_OperateFunction40_HoradricScrollChest(ObjOperateFn* pOp, int32_t nOperate)
{
	if (!pOp || !pOp->pObject)
	{
		return 0;
	}

	UnitAny* pObject = pOp->pObject;

	if (!QUESTS_SetObjectSelection(pOp))
	{
		return 1;
	}

	pObject->dwDropItemCode = ' 1rt';

	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A2Q2_HORADRICSTAFF);
	if (pQuestData && pQuestData->pQuestDataEx)
	{
		Act2Quest2* pQuestDataEx = (Act2Quest2*)pQuestData->pQuestDataEx;

		pQuestDataEx->nItemDropCount = 0;
		SUNIT_IterateUnitsOfType(pOp->pGame, 0, pOp->pPlayer, ACT2Q2_UnitIterate_DetermineHoradricScrollDropCount);

		for (int32_t i = 0; i < pQuestDataEx->nItemDropCount; ++i)
		{
			int32_t nItemLevel = 0;
			D2GAME_DropItemAtUnit_6FC4FEC0(pOp->pGame, pObject, ITEMQUAL_UNIQUE, &nItemLevel, 0, -1, 1);
		}
	}
	OBJMODE_DropFromChestTCWithQuality(pOp, ITEMQUAL_MAGIC);

	Seed* pSeed = QUESTS_GetGlobalSeed(pOp->pGame);

	const int32_t nCounter = ITEMS_RollRandomNumber(pSeed) % 5 + 5;
	for (int32_t i = 0; i < nCounter; ++i)
	{
		OBJMODE_DropItemWithCodeAndQuality(pOp->pGame, pObject, ' dlg', 2);
	}

	return 1;
}

//D2Game.0x6FCA1180
int32_t __fastcall ACT2Q2_UnitIterate_DetermineHoradricScrollDropCount(Game* pGame, UnitAny* pUnit, void* pData)
{
	BitBuffer* pQuestFlags = UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty];
	if (QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED) == 1 || QUESTRECORD_GetQuestState(pQuestFlags, QUESTSTATEFLAG_A2Q2, QFLAG_LEAVETOWN))
	{
		return 0;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A2Q2_HORADRICSTAFF);
	if (pQuestData)
	{
		++((Act2Quest2*)pQuestData->pQuestDataEx)->nItemDropCount;
	}

	return 0;
}

//D2Game.0x6FCA11D0
int32_t __fastcall OBJECTS_OperateFunction41_StaffOfKingsChest(ObjOperateFn* pOp, int32_t nOperate)
{
	if (!pOp || !pOp->pObject)
	{
		return 0;
	}

	UnitAny* pObject = pOp->pObject;

	if (!QUESTS_SetObjectSelection(pOp))
	{
		return 1;
	}

	pObject->dwDropItemCode = ' fsm';

	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A2Q2_HORADRICSTAFF);
	if (pQuestData && pQuestData->pQuestDataEx)
	{
		Act2Quest2* pQuestDataEx = (Act2Quest2*)pQuestData->pQuestDataEx;

		pQuestDataEx->nItemDropCount = 0;
		SUNIT_IterateUnitsOfType(pOp->pGame, 0, pOp->pPlayer, ACT2Q2_UnitIterate_DetermineStaffDropCount);

		for (int32_t i = 0; i < pQuestDataEx->nItemDropCount; ++i)
		{
			int32_t nItemLevel = 0;
			UnitAny* pItem = D2GAME_DropItemAtUnit_6FC4FEC0(pOp->pGame, pObject, ITEMQUAL_UNIQUE, &nItemLevel, 0, -1, 1);
			if (pItem)
			{
				ITEMS_SetItemFlag(pItem, IFLAG_IDENTIFIED, 1);

				pQuestDataEx->bHoradricScrollDropped = 1;
				++pQuestDataEx->nHoradricCubes;
			}
		}
	}
	OBJMODE_DropFromChestTCWithQuality(pOp, ITEMQUAL_MAGIC);

	Seed* pSeed = QUESTS_GetGlobalSeed(pOp->pGame);

	const int32_t nCounter = ITEMS_RollRandomNumber(pSeed) % 5 + 5;
	for (int32_t i = 0; i < nCounter; ++i)
	{
		OBJMODE_DropItemWithCodeAndQuality(pOp->pGame, pObject, ' dlg', 2);
	}

	return 1;
}

//D2Game.0x6FCA12C0
int32_t __fastcall ACT2Q2_UnitIterate_DetermineStaffDropCount(Game* pGame, UnitAny* pUnit, void* pData)
{
	if (QUESTRECORD_GetQuestState(UNITS_GetPlayerData(pUnit)->pQuestData[pGame->nDifficulty], QUESTSTATEFLAG_A2Q2, QFLAG_REWARDGRANTED))
	{
		return 0;
	}

	if (ITEMS_FindQuestItem(pGame, pUnit, ' fsm') || ITEMS_FindQuestItem(pGame, pUnit, ' tsh'))
	{
		return 0;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A2Q2_HORADRICSTAFF);
	if (pQuestData)
	{
		++((Act2Quest2*)pQuestData->pQuestDataEx)->nItemDropCount;
	}

	return 0;
}

//D2Game.0x6FCA1330
int32_t __fastcall OBJECTS_OperateFunction39_HoradricCubeChest(ObjOperateFn* pOp, int32_t nOperate)
{
	if (!pOp || !pOp->pObject)
	{
		return 0;
	}

	UnitAny* pObject = pOp->pObject;

	if (!QUESTS_SetObjectSelection(pOp))
	{
		return 1;
	}

	pObject->dwDropItemCode = ' xob';
	QuestData* pQuestData = QUESTS_GetQuestData(pOp->pGame, QUEST_A2Q2_HORADRICSTAFF);
	if (pQuestData && pQuestData->pQuestDataEx)
	{
		Act2Quest2* pQuestDataEx = (Act2Quest2*)pQuestData->pQuestDataEx;

		pQuestDataEx->nItemDropCount = 0;
		SUNIT_IterateUnitsOfType(pOp->pGame, 0, pOp->pPlayer, ACT2Q2_UnitIterate_DetermineCubeDropCount);

		for (int32_t i = 0; i < pQuestDataEx->nItemDropCount; ++i)
		{
			int32_t nItemLevel = 0;
			if (D2GAME_DropItemAtUnit_6FC4FEC0(pOp->pGame, pObject, ITEMQUAL_NORMAL, &nItemLevel, 0, -1, 1))
			{
				pQuestDataEx->bCubeDropped = 1;
				++pQuestDataEx->nHoradricCubes;
			}
		}
	}
	OBJMODE_DropFromChestTCWithQuality(pOp, ITEMQUAL_MAGIC);

	Seed* pSeed = QUESTS_GetGlobalSeed(pOp->pGame);

	const int32_t nCounter = ITEMS_RollRandomNumber(pSeed) % 5 + 5;
	for (int32_t i = 0; i < nCounter; ++i)
	{
		OBJMODE_DropItemWithCodeAndQuality(pOp->pGame, pObject, ' dlg', 2);
	}

	return 1;
}

//D2Game.0x6FCA1410
int32_t __fastcall ACT2Q2_UnitIterate_DetermineCubeDropCount(Game* pGame, UnitAny* pUnit, void* pData)
{
	if (ITEMS_FindQuestItem(pGame, pUnit, ' xob'))
	{
		return 0;
	}

	QuestData* pQuestData = QUESTS_GetQuestData(pGame, QUEST_A2Q2_HORADRICSTAFF);
	if (pQuestData)
	{
		++((Act2Quest2*)pQuestData->pQuestDataEx)->nItemDropCount;
	}

	return 0;
}

//
void __fastcall ACT2Q2_Callback10_PlayerLeavesGame(QuestData* pQuestData, QuestArg* pQuestArg)
{
	QUESTS_RemovePlayerGUID(pQuestData, pQuestArg);
}
