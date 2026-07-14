#include "AI/AiGeneral.h"

#include <Drlg/D2DrlgPreset.h>
#include <D2Monsters.h>

#include "AI/AiUtil.h"
#include "MONSTER/Monster.h"
#include "UNIT/SUnit.h"
#include "UNIT/SUnitInactive.h"


//D2Game.0x6FCCED00
AiControl* __fastcall AIGENERAL_AllocAiControl(Game* pGame)
{
	AiControl* pAiControl = D2_CALLOC_STRC_POOL(pGame->pMemoryPool, AiControl);
	pAiControl->dwOwnerTypeEx = UNIT_MONSTER;
	pAiControl->dwOwnerType = UNIT_MONSTER;
	pAiControl->dwOwnerGUIDEx = -1;
	pAiControl->dwOwnerGUID = -1;

	return pAiControl;
}

//D2Game.0x6FCCED40
void __fastcall AIGENERAL_SetAiControlParam(UnitAny* pMonster, int32_t nIndex, int32_t nParamValue)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pMonster);
	if (!pAiControl)
	{
		return;
	}

	switch (nIndex)
	{
	case 1:
		pAiControl->dwAiParam[0] = nParamValue;
		break;

	case 2:
		pAiControl->dwAiParam[1] = nParamValue;
		break;

	case 3:
		pAiControl->dwAiParam[2] = nParamValue;
		break;
	default:
		break;
	}
}

//D2Game.0x6FCCED80
int32_t __fastcall AIGENERAL_GetAiControlParam(UnitAny* pUnit, int32_t nIndex)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return 0;
	}

	switch (nIndex)
	{
	case 1:
		return pAiControl->dwAiParam[0];

	case 2:
		return pAiControl->dwAiParam[1];

	case 3:
		return pAiControl->dwAiParam[2];
	default:
		break;
	}

	return 0;
}

//D2Game.0x6FCCEDC0
AiCmd* __fastcall AIGENERAL_AllocAiCommand(Game* pGame, UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return nullptr;
	}

	AiCmd* pAiCmd = D2_CALLOC_STRC_POOL(pGame->pMemoryPool, AiCmd);
	AiCmd* pLastCmd = pAiControl->pLastCmd;
	if (pLastCmd)
	{
		AiCmd* pCurrentCmd = pAiControl->pCurrentCmd;
		if (pLastCmd == pCurrentCmd)
		{
			pAiControl->pLastCmd = pAiCmd;
		}

		pAiCmd->pNextCmd = pCurrentCmd;
		pAiCmd->pPrevCmd = pAiControl->pCurrentCmd->pPrevCmd;
		pAiControl->pCurrentCmd->pPrevCmd = pAiCmd;
		pAiCmd->pPrevCmd->pNextCmd = pAiCmd;
	}
	else
	{
		pAiControl->pLastCmd = pAiCmd;
		pAiControl->pCurrentCmd = pAiCmd;
		pAiCmd->pNextCmd = pAiCmd;
		pAiCmd->pPrevCmd = pAiCmd;
	}

	return pAiCmd;
}

//D2Game.0x6FCCEE40
void __fastcall AIGENERAL_FreeCurrentAiCommand(Game* pGame, UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return;
	}

	AiCmd* pCurrentCmd = pAiControl->pCurrentCmd;
	if (!pCurrentCmd)
	{
		return;
	}

	if (pCurrentCmd == pCurrentCmd->pNextCmd)
	{
		pAiControl->pLastCmd = nullptr;
		pAiControl->pCurrentCmd = nullptr;
	}
	else
	{
		if (pAiControl->pLastCmd == pCurrentCmd)
		{
			pAiControl->pLastCmd = pCurrentCmd->pNextCmd;
		}

		pAiControl->pCurrentCmd = pCurrentCmd->pNextCmd;
		pCurrentCmd->pPrevCmd->pNextCmd = pCurrentCmd->pNextCmd;
		pCurrentCmd->pNextCmd->pPrevCmd = pCurrentCmd->pPrevCmd;
	}

	D2_FREE_POOL(pGame->pMemoryPool, pCurrentCmd);
}

//D2Game.0x6FCCEEB0
void __fastcall AIGENERAL_FreeAllAiCommands(Game* pGame, UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return;
	}

	while (pAiControl->pLastCmd)
	{
		AIGENERAL_FreeCurrentAiCommand(pGame, pUnit);
	}
}

//D2Game.0x6FCCEEF0
AiCmd* __fastcall AIGENERAL_GetCurrentAiCommandFromUnit(UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return nullptr;
	}

	return pAiControl->pCurrentCmd;
}

//D2Game.0x6FCCEF10
AiCmd* __fastcall AIGENERAL_GetAiCommandFromParam(UnitAny* pUnit, int32_t nCmdParam, int32_t bSet)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return nullptr;
	}

	AiCmd* pCurrentAiCmd = pAiControl->pCurrentCmd;
	if (!pCurrentAiCmd)
	{
		return nullptr;
	}

	AiCmd* pAiCmd = pCurrentAiCmd->pNextCmd;
	while (pAiCmd->nCmdParam[0] != nCmdParam)
	{
		if (pAiCmd == pCurrentAiCmd)
		{
			break;
		}

		pAiCmd = pAiCmd->pNextCmd;
	}

	if (pAiCmd->nCmdParam[0] != nCmdParam)
	{
		return nullptr;
	}

	if (bSet)
	{
		pAiControl->pCurrentCmd = pAiCmd;
	}

	return pAiCmd;
}

//D2Game.0x6FCCEF70
void __fastcall AIGENERAL_AllocCommandsForMinions(Game* pGame, UnitAny* pUnit, AiCmd* pAiCmd)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl || !pAiControl->pGame)
	{
		return;
	}

	AiControl* pOwnerAiControl = AIGENERAL_GetAiControlFromUnit(SUNIT_GetServerUnit(pAiControl->pGame, pAiControl->dwOwnerTypeEx, pAiControl->dwOwnerGUIDEx));
	if (!pOwnerAiControl)
	{
		return;
	}

	for (MinionList* pMinionList = pOwnerAiControl->pMinionList; pMinionList; pMinionList = pMinionList->pNext)
	{
		UnitAny* pMinion = SUNIT_GetServerUnit(pOwnerAiControl->pGame, UNIT_MONSTER, pMinionList->dwMinionGUID);
		if (pMinion)
		{
			AIGENERAL_CopyAiCommand(pGame, pMinion, pAiCmd);
		}
	}
}

//D2Game.0x6FCCF050
AiCmd* __fastcall AIGENERAL_CopyAiCommand(Game* pGame, UnitAny* pUnit, AiCmd* pAiCmd)
{
	AiCmd* pCopy = AIGENERAL_AllocAiCommand(pGame, pUnit);
	memcpy(pCopy->nCmdParam, pAiCmd->nCmdParam, sizeof(pCopy->nCmdParam));

	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (pAiControl)
	{
		pAiControl->pCurrentCmd = pCopy;
	}

	return pCopy;
}

//D2Game.0x6FCCF090
AiCmd* __fastcall AIGENERAL_SetCurrentAiCommand(Game* pGame, UnitAny* pUnit, int32_t nCmdParam, int32_t bSet)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return 0;
	}

	AiCmd* pAiCmd = AIGENERAL_GetAiCommandFromParam(pUnit, nCmdParam, bSet);
	if (!pAiCmd)
	{
		const int32_t nCmdParams[5] = { nCmdParam, 0, 0, 0, 0 };
		pAiControl->pCurrentCmd = AIGENERAL_AllocAiCommand(pGame, pUnit);
		memcpy(pAiControl->pCurrentCmd->nCmdParam, nCmdParams, sizeof(pAiControl->pCurrentCmd->nCmdParam));
	}

	return AIGENERAL_GetAiCommandFromParam(pUnit, nCmdParam, bSet);
}

//D2Game.0x6FCCF190
void __fastcall AIGENERAL_FreeAiControl(Game* pGame, AiControl* pAiControl)
{
	if (!pAiControl)
	{
		return;
	}

	if (pAiControl->pMapAi)
	{
		DRLGPRESET_FreeMapAI(pGame->pMemoryPool, pAiControl->pMapAi);
	}

	MinionList* pNext = nullptr;
	for (MinionList* i = pAiControl->pMinionList; i; i = pNext)
	{
		pNext = i->pNext;
		D2_FREE_POOL(pGame->pMemoryPool, i);
	}

	for (AiCmd* i = pAiControl->pLastCmd; i; i = pAiControl->pLastCmd)
	{
		AiCmd* pCurrentCmd = pAiControl->pCurrentCmd;
		if (!pCurrentCmd)
		{
			break;
		}

		if (pCurrentCmd == pCurrentCmd->pNextCmd)
		{
			pAiControl->pLastCmd = nullptr;
			pAiControl->pCurrentCmd = nullptr;
		}
		else
		{
			if (i == pCurrentCmd)
			{
				pAiControl->pLastCmd = pCurrentCmd->pNextCmd;
			}

			pAiControl->pCurrentCmd = pCurrentCmd->pNextCmd;

			pCurrentCmd->pPrevCmd->pNextCmd = pCurrentCmd->pNextCmd;
			pCurrentCmd->pNextCmd->pPrevCmd = pCurrentCmd->pPrevCmd;
		}

		D2_FREE_POOL(pGame->pMemoryPool, pCurrentCmd);
	}

	D2_FREE_POOL(pGame->pMemoryPool, pAiControl);
}

//D2Game.0x6FCCF240
MapAI** __stdcall AIGENERAL_GetMapAiFromUnit(UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return nullptr;
	}

	return &pAiControl->pMapAi;
}

//D2Game.0x6FCCF270
void __fastcall AIGENERAL_SetOwnerData(Game* pGame, UnitAny* pUnit, int32_t nOwnerGUID, int32_t nOwnerType, int32_t bSetFlag1, int32_t bSetFlag2)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);

	if (bSetFlag2)
	{
		AIUTIL_ToggleAiControlFlag(pAiControl, 2u, 1);
	}

	if (bSetFlag1)
	{
		AIUTIL_ToggleAiControlFlag(pAiControl, 1u, 1);
	}

	pAiControl->dwOwnerGUIDEx = nOwnerGUID;
	pAiControl->dwOwnerTypeEx = nOwnerType;
	pAiControl->pGame = pGame;
}

//D2Game.0x6FCCF2D0
void __fastcall AIGENERAL_GetOwnerData(UnitAny* pUnit, int32_t* pUnitGUID, int32_t* pUnitType)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (pAiControl && pAiControl->pGame)
	{
		*pUnitGUID = pAiControl->dwOwnerGUIDEx;
		*pUnitType = pAiControl->dwOwnerTypeEx;
	}
	else
	{
		*pUnitGUID = -1;
	}
}

//D2Game.0x6FCCF320
UnitAny* __fastcall AIGENERAL_GetMinionOwner(UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (pAiControl && pAiControl->pGame)
	{
		return SUNIT_GetServerUnit(pAiControl->pGame, pAiControl->dwOwnerTypeEx, pAiControl->dwOwnerGUIDEx);
	}

	return nullptr;
}

//D2Game.0x6FCCF360
void __fastcall AIGENERAL_AllocMinionList(Game* pGame, UnitAny* pUnit, UnitAny* pMinion)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	MinionList* pMinionList = D2_CALLOC_STRC_POOL(pGame->pMemoryPool, MinionList);

	if (pMinion)
	{
		pMinionList->dwMinionGUID = pMinion->dwUnitId;
	}
	else
	{
		pMinionList->dwMinionGUID = -1;
	}

	pMinionList->pNext = pAiControl->pMinionList;
	pAiControl->pMinionList = pMinionList;
}

//D2Game.0x6FCCF3C0
void __fastcall AIGENERAL_FreeMinionList(UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl || !pAiControl->pGame)
	{
		return;
	}

	AiControl* pOwnerAiControl = AIGENERAL_GetAiControlFromUnit(SUNIT_GetServerUnit(pAiControl->pGame, pAiControl->dwOwnerTypeEx, pAiControl->dwOwnerGUIDEx));

	MinionList* pPreviousMinionList = nullptr;
	for (MinionList* pMinionList = pOwnerAiControl->pMinionList; pMinionList; pMinionList = pMinionList->pNext)
	{
		if (pMinionList->dwMinionGUID == pUnit->dwUnitId)
		{
			UnitAny* pMinion = SUNIT_GetServerUnit(pOwnerAiControl->pGame, UNIT_MONSTER, pMinionList->dwMinionGUID);
			if (pMinion && pMinion->dwUnitType == UNIT_MONSTER && pMinion->pMonsterData)
			{
				pMinion->pMonsterData->pAiControl->dwOwnerGUIDEx = -1;
				pMinion->pMonsterData->pAiControl->dwOwnerTypeEx = 1;
				pMinion->pMonsterData->pAiControl->pGame = pOwnerAiControl->pGame;
			}

			if (pPreviousMinionList)
			{
				pPreviousMinionList->pNext = pMinionList->pNext;
			}
			else
			{
				pOwnerAiControl->pMinionList = pMinionList->pNext;
			}

			D2_FREE_POOL(pOwnerAiControl->pGame->pMemoryPool, pMinionList);
			return;
		}

		pPreviousMinionList = pMinionList;
	}
}

//D2Game.0x6FCCF4B0
void __fastcall sub_6FCCF4B0(UnitAny* pUnit)
{
	if (!pUnit || pUnit->dwUnitType != UNIT_MONSTER || !pUnit->pMonsterData)
	{
		return;
	}

	AiControl* pAiControl = pUnit->pMonsterData->pAiControl;
	if (!pAiControl || !pAiControl->pGame)
	{
		return;
	}

	UnitAny* pOwner = SUNIT_GetServerUnit(pAiControl->pGame, pAiControl->dwOwnerTypeEx, pAiControl->dwOwnerGUIDEx);
	if (!pOwner || pOwner != pUnit)
	{
		return;
	}

	MinionList* pMinionList = pAiControl->pMinionList;
	while (pMinionList)
	{
		MinionList* pNext = pMinionList->pNext;
		UnitAny* pMinion = SUNIT_GetServerUnit(pAiControl->pGame, UNIT_MONSTER, pMinionList->dwMinionGUID);
		if (pMinion && pMinion->dwUnitType == UNIT_MONSTER && pMinion->pMonsterData)
		{
			pMinion->pMonsterData->pAiControl->dwOwnerGUIDEx = -1;
			pMinion->pMonsterData->pAiControl->dwOwnerTypeEx = UNIT_MONSTER;
			pMinion->pMonsterData->pAiControl->pGame = pAiControl->pGame;
		}

		D2_FREE_POOL(pAiControl->pGame->pMemoryPool, pMinionList);
		pMinionList = pNext;
	}

	pAiControl->pMinionList = nullptr;
}

//D2Game.0x6FCCF590
void __fastcall AIGENERAL_FreeAllMinionLists(Game* pGame, MinionList* pMinionList)
{
	while (pMinionList)
	{
		MinionList* pNext = pMinionList->pNext;
		D2_FREE_POOL(pGame->pMemoryPool, pMinionList);
		pMinionList = pNext;
	}
}

//D2Game.0x6FCCF5C0
void __fastcall AIGENERAL_ExecuteCallbackOnMinions(UnitAny* pUnit, void* a2, void* a3, void(__fastcall* pfnParty)(UnitAny*, void*, void*))
{
	D2_ASSERT(pfnParty);

	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl || !pAiControl->pGame)
	{
		return;
	}

	AiControl* pOwnerAiControl = AIGENERAL_GetAiControlFromUnit(SUNIT_GetServerUnit(pAiControl->pGame, pAiControl->dwOwnerTypeEx, pAiControl->dwOwnerGUIDEx));
	for (MinionList* pMinionList = pOwnerAiControl->pMinionList; pMinionList; pMinionList = pMinionList->pNext)
	{
		UnitAny* pMinion = SUNIT_GetServerUnit(pOwnerAiControl->pGame, UNIT_MONSTER, pMinionList->dwMinionGUID);
		if (pMinion)
		{
			pfnParty(pMinion, a2, a3);
		}
	}
}

//D2Game.0x6FCCF680
void __fastcall AIGENERAL_GetAiControlInfo(UnitAny* pUnit, int32_t* pOwnerGUID, int32_t* pOwnerType, int32_t* pAiControlFlag1, int32_t* pAiControlFlag2, MinionList** ppMinionList)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (pAiControl && pAiControl->dwOwnerTypeEx == UNIT_MONSTER)
	{
		*pOwnerGUID = pAiControl->dwOwnerGUIDEx;
		*pOwnerType = pAiControl->dwOwnerTypeEx;
		*pAiControlFlag1 = AIUTIL_CheckAiControlFlag(pAiControl, 1u);
		*pAiControlFlag2 = AIUTIL_CheckAiControlFlag(pAiControl, 2u);
		*ppMinionList = pAiControl->pMinionList;
		pAiControl->dwOwnerGUIDEx = -1;
		pAiControl->pMinionList = nullptr;
	}
	else
	{
		*pOwnerGUID = -1;
	}
}

//D2Game.0x6FCCF710
void __fastcall AIGENERAL_SetAiControlInfo(Game* pGame, UnitAny* pUnit, DWORD nOwnerGUID, DWORD nOwnerType, int32_t bSetFlag1, int32_t bSetFlag2, MinionList* pMinionList)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return;
	}

	AIGENERAL_FreeAllMinionLists(pGame, pAiControl->pMinionList);

	if (bSetFlag2)
	{
		AIUTIL_ToggleAiControlFlag(pAiControl, 2u, 1);
	}

	if (bSetFlag1)
	{
		AIUTIL_ToggleAiControlFlag(pAiControl, 1u, 1);
	}

	pAiControl->dwOwnerGUIDEx = nOwnerGUID;
	pAiControl->dwOwnerTypeEx = nOwnerType;
	pAiControl->pGame = pGame;
	pAiControl->pMinionList = pMinionList;
}

//D2Game.0x6FCCF7C0
void __fastcall AIGENERAL_UpdateMinionList(UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!AIUTIL_CheckAiControlFlag(pAiControl, 1u))
	{
		return;
	}

	if (!AIUTIL_CheckAiControlFlag(pAiControl, 2u))
	{
		for (MinionList* i = pAiControl->pMinionList; i; i = i->pNext)
		{
			AIGENERAL_SetOwnerData(pAiControl->pGame, SUNIT_GetServerUnit(pAiControl->pGame, UNIT_MONSTER, i->dwMinionGUID), -1, UNIT_MONSTER, 0, 0);
		}
		return;
	}

	MinionList* pMinionList = pAiControl->pMinionList;
	MinionList* pList = pAiControl->pMinionList;

	while (pMinionList)
	{
		UnitAny* pMinion = SUNIT_GetServerUnit(pAiControl->pGame, UNIT_MONSTER, pMinionList->dwMinionGUID);
		if (pMinion)
		{
			if (pMinion->dwUnitType == UNIT_MONSTER)
			{
				AIGENERAL_SetOwnerData(pAiControl->pGame, pMinion, pMinionList->dwMinionGUID, UNIT_MONSTER, 1, 1);
			}

			if (pUnit && pUnit->dwUnitType == UNIT_MONSTER)
			{
				AIGENERAL_SetOwnerData(pAiControl->pGame, pUnit, pMinionList->dwMinionGUID, UNIT_MONSTER, 0, 0);
			}

			if (pMinion->dwUnitType == UNIT_MONSTER)
			{
				AiControl* pMinionAiControl = AIGENERAL_GetAiControlFromUnit(pMinion);

				MinionList* pNewList = D2_CALLOC_STRC_POOL(pAiControl->pGame->pMemoryPool, MinionList);
				pNewList->dwMinionGUID = pUnit ? pUnit->dwUnitId : -1;

				if (pMinionAiControl->pMinionList)
				{
					pNewList->pNext = pMinionAiControl->pMinionList;
				}
				pMinionAiControl->pMinionList = pNewList;
			}

			do
			{
				UnitAny* pTemp = SUNIT_GetServerUnit(pAiControl->pGame, UNIT_MONSTER, pList->dwMinionGUID);
				if (pTemp && pTemp != pMinion)
				{
					AIGENERAL_SetOwnerData(pAiControl->pGame, pTemp, pMinionList->dwMinionGUID, UNIT_MONSTER, 0, 0);
					AIGENERAL_AllocMinionList(pAiControl->pGame, pMinion, pTemp);
				}
				pList = pList->pNext;
			}
			while (pList);
			return;
		}

		pMinionList = pMinionList->pNext;
	}
}

//D2Game.0x6FCCF9B0
int32_t __fastcall AIGENERAL_GetMinionSpawnClassId(UnitAny* pUnit)
{
	AiControl* pAiControl = AIGENERAL_GetAiControlFromUnit(pUnit);
	if (!pAiControl)
	{
		return 0;
	}

	return pAiControl->nMinionSpawnClassId;
}
