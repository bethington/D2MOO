#include "Drlg/D2DrlgActivate.h"

#include "Drlg/D2DrlgDrlg.h"
#include "Drlg/D2DrlgDrlgRoom.h"
#include "Drlg/D2DrlgPreset.h"
#include "Drlg/D2DrlgRoomTile.h"
#include <DataTbls/LevelsIds.h>

static int gStatsClientFreedRooms;
static int gStatsClientAllocatedRooms;
static int gStatsFreedRooms;
static int gStatsAllocatedRooms;

// D2Common.0x6FDE07B0
static void (__fastcall* gRoomExSetStatus[ROOMSTATUS_COUNT])(Room2*) =
{
	DRLGACTIVATE_RoomExSetStatus_ClientInRoom,
	DRLGACTIVATE_RoomExSetStatus_ClientInSight,
	DRLGACTIVATE_RoomExSetStatus_ClientOutOfSight,
	DRLGACTIVATE_RoomExSetStatus_Untile
};

// D2Common.0x6FDE07C0
static void (__fastcall* gRoomExUnsetStatus[ROOMSTATUS_COUNT])(Room2*) =
{
	DRLGACTIVATE_RoomExIdentifyRealStatus,
	DRLGACTIVATE_RoomExIdentifyRealStatus,
	DRLGACTIVATE_RoomExIdentifyRealStatus,
	DRLGACTIVATE_RoomExStatusUnset_Untile
};

//Helper functions
static void DRLGACTIVATE_RoomExStatuslink(Room2* pStatusRoomsListHead, Room2* pDrlgRoom)
{
	pDrlgRoom->pStatusNext = pStatusRoomsListHead;
	pDrlgRoom->pStatusPrev = pStatusRoomsListHead->pStatusPrev;

	pStatusRoomsListHead->pStatusPrev->pStatusNext = pDrlgRoom;
	pStatusRoomsListHead->pStatusPrev = pDrlgRoom;
}
static void DRLGACTIVATE_RoomExStatusUnlink(Room2* pDrlgRoom)
{
	if (pDrlgRoom->pStatusPrev && pDrlgRoom->pStatusNext)
	{
		pDrlgRoom->pStatusPrev->pStatusNext = pDrlgRoom->pStatusNext;
		pDrlgRoom->pStatusNext->pStatusPrev = pDrlgRoom->pStatusPrev;
		pDrlgRoom->pStatusPrev = nullptr;
		pDrlgRoom->pStatusNext = nullptr;
	}
}
// Returns true if status changed
static bool DRLGACTIVATE_UpdateRoomExStatusImpl(Room2* pDrlgRoom, DrlgRoomStatus nStatus)
{
	// Note: Lower value has priority over others
	if (pDrlgRoom->fRoomStatus > nStatus)
	{
		DRLGACTIVATE_RoomExStatusUnlink(pDrlgRoom);
		if (nStatus < ROOMSTATUS_COUNT)
		{
			DRLGACTIVATE_RoomExStatuslink(&pDrlgRoom->pLevel->pDrlg->tStatusRoomsLists[nStatus], pDrlgRoom);
		}
		pDrlgRoom->fRoomStatus = nStatus;
		return true;
	}
	return false;
}
static DrlgRoomStatus DRLGACTIVATE_RoomExFindFirstStatusWithRefCount(Room2* pDrlgRoom, DrlgRoomStatus nMaxStatus)
{
	for (int nFirstNonEmptyListStatus = 0; nFirstNonEmptyListStatus <= nMaxStatus && nFirstNonEmptyListStatus < ROOMSTATUS_COUNT; nFirstNonEmptyListStatus++)
	{
		if (pDrlgRoom->wRoomsInList[nFirstNonEmptyListStatus] != 0)
		{
			return (DrlgRoomStatus)nFirstNonEmptyListStatus;
		}
	}
	return nMaxStatus;
}

//D2Common.0x6FD733D0
void __fastcall DRLGACTIVATE_RoomExSetStatus_ClientInRoom(Room2* pDrlgRoom)
{
	DRLGACTIVATE_UpdateRoomExStatusImpl(pDrlgRoom, ROOMSTATUS_CLIENT_IN_ROOM);
}

//Helper function
static void DRLGACTIVATE_InitRoomsInitTimeout(ActMisc* pDrlg)
{
	pDrlg->nRoomsInitTimeout = 2 * (DRLG_IsOnClient(pDrlg) == 0) + 5;
}
void DRLGACTIVATE_RoomEx_EnsureHasRoom(Room2* pDrlgRoom, bool bInitTimeoutCounter)
{
	if (pDrlgRoom->pRoom == nullptr && !(pDrlgRoom->dwFlags & DRLGROOMFLAG_HAS_ROOM))
	{
		ActMisc* pDrlg = pDrlgRoom->pLevel->pDrlg;
		if (!pDrlgRoom->nRoomsNear)
		{
			sub_6FD77BB0(pDrlg->pMempool, pDrlgRoom);
		}

		DRLGROOMTILE_InitRoomGrids(pDrlgRoom);
		DRLGROOMTILE_AddRoomMapTiles(pDrlgRoom);
		DRLG_CreateRoomForRoomEx(pDrlg, pDrlgRoom);
		++pDrlg->nRoomsInitSinceLastUpdate;
		++pDrlg->nAllocatedRooms;
		if (bInitTimeoutCounter)
		{
			DRLGACTIVATE_InitRoomsInitTimeout(pDrlg);
		}
	}
}

//D2Common.0x6FD73450
void __fastcall DRLGACTIVATE_RoomExSetStatus_ClientInSight(Room2* pDrlgRoom)
{
	DRLGACTIVATE_RoomEx_EnsureHasRoom(pDrlgRoom, true);
	DRLGACTIVATE_UpdateRoomExStatusImpl(pDrlgRoom, ROOMSTATUS_CLIENT_IN_SIGHT);
}

//D2Common.0x6FD73550
void __fastcall DRLGACTIVATE_RoomExSetStatus_ClientOutOfSight(Room2* pDrlgRoom)
{
	if (pDrlgRoom->dwFlags & DRLGROOMFLAG_TILELIB_LOADED 
		&& (pDrlgRoom->nType != DRLGTYPE_PRESET || (pDrlgRoom->dwFlags & DRLGROOMFLAG_PRESET_UNITS_ADDED) != 0))
	{

		if (DRLGACTIVATE_UpdateRoomExStatusImpl(pDrlgRoom, ROOMSTATUS_CLIENT_OUT_OF_SIGHT)
			&& pDrlgRoom->wRoomsInList[ROOMSTATUS_CLIENT_IN_SIGHT] != 0)
		{
			DRLGACTIVATE_RoomExSetStatus_ClientInSight(pDrlgRoom);
		}
	}
}

//D2Common.0x6FD736F0
void __fastcall DRLGACTIVATE_RoomExSetStatus_Untile(Room2* pDrlgRoom)
{
	if (!(pDrlgRoom->dwFlags & DRLGROOMFLAG_TILELIB_LOADED))
	{
		DRLGROOMTILE_LoadDT1FilesForRoom(pDrlgRoom);
	}

	if (pDrlgRoom->nType == DRLGTYPE_PRESET && !(pDrlgRoom->dwFlags & DRLGROOMFLAG_PRESET_UNITS_ADDED))
	{
		DRLGPRESET_SpawnHardcodedPresetUnits(pDrlgRoom);
	}
	DRLGACTIVATE_UpdateRoomExStatusImpl(pDrlgRoom, ROOMSTATUS_UNTILE);
}

//D2Common.0x6FD73790
void __fastcall DRLGACTIVATE_RoomExIdentifyRealStatus(Room2* pDrlgRoom)
{
	if (pDrlgRoom->fRoomStatus >= ROOMSTATUS_COUNT || pDrlgRoom->wRoomsInList[pDrlgRoom->fRoomStatus] == 0)
	{
		const DrlgRoomStatus nFirstStatusWithRefCount = DRLGACTIVATE_RoomExFindFirstStatusWithRefCount(pDrlgRoom, ROOMSTATUS_COUNT);
		if (pDrlgRoom->fRoomStatus != nFirstStatusWithRefCount)
		{
			DRLGACTIVATE_UpdateRoomExStatusImpl(pDrlgRoom, nFirstStatusWithRefCount);
		}
	}
}

//D2Common.0x6FD73880
void __fastcall DRLGACTIVATE_RoomExStatusUnset_Untile(Room2* pDrlgRoom)
{
	if (pDrlgRoom->fRoomStatus != ROOMSTATUS_COUNT)
	{
		DRLGACTIVATE_RoomExIdentifyRealStatus(pDrlgRoom);
		
		// We may unload the room if no status is now set
		if (pDrlgRoom->fRoomStatus == ROOMSTATUS_COUNT)
		{
			if (DRLG_IsOnClient(pDrlgRoom->pLevel->pDrlg))
			{
				DRLGROOMTILE_FreeRoom(pDrlgRoom, false);
			}
		}
	}
}

//D2Common.0x6FD73A30
void __fastcall DRLGACTIVATE_RoomExPropagateSetStatus(void* pMemPool, Room2* pDrlgRoom, uint8_t nStatus)
{
	if (pDrlgRoom->nRoomsNear == 0)
	{
		sub_6FD77BB0(pMemPool, pDrlgRoom);
	}

	for (int i = 0; i < pDrlgRoom->nRoomsNear; ++i)
	{
		Room2* pNearRoom = pDrlgRoom->ppRoomsNear[i];
		if (nStatus + 1 < ROOMSTATUS_COUNT)
		{
			DRLGACTIVATE_RoomExPropagateSetStatus(pMemPool, pNearRoom, nStatus + 1);
		}

		if (pNearRoom->fRoomStatus >= nStatus)
		{
			const DrlgRoomStatus nFirstStatusWithRefCount = DRLGACTIVATE_RoomExFindFirstStatusWithRefCount(pNearRoom, DrlgRoomStatus(nStatus));
			if (nFirstStatusWithRefCount == nStatus)
			{
				gRoomExSetStatus[nStatus](pNearRoom);
			}
		}
		++pNearRoom->wRoomsInList[nStatus];
	}
}

//D2Common.0x6FD73BE0
void __fastcall DRLGACTIVATE_RoomExPropagateUnsetStatus(Room2* pDrlgRoom, uint8_t nStatus)
{
	for (int i = 0; i < pDrlgRoom->nRoomsNear; ++i)
	{
		if (pDrlgRoom->ppRoomsNear[i])
		{
			--pDrlgRoom->ppRoomsNear[i]->wRoomsInList[nStatus];

			gRoomExUnsetStatus[nStatus](pDrlgRoom->ppRoomsNear[i]);

			if (nStatus + 1 < ROOMSTATUS_COUNT)
			{
				DRLGACTIVATE_RoomExPropagateUnsetStatus(pDrlgRoom->ppRoomsNear[i], nStatus + 1);
			}
		}
	}
}

// Helper functions
static void DRLGACTIVATE_RoomSetAndPropagateStatus(Room2* pDrlgRoom, DrlgRoomStatus nStatus)
{
	DRLGACTIVATE_RoomExPropagateSetStatus(pDrlgRoom->pLevel->pDrlg->pMempool, pDrlgRoom, nStatus + 1);
	if (pDrlgRoom->fRoomStatus >= nStatus)
	{
		const DrlgRoomStatus nFirstStatusWithRefCount = DRLGACTIVATE_RoomExFindFirstStatusWithRefCount(pDrlgRoom, nStatus);
		if (nFirstStatusWithRefCount == nStatus)
		{
			gRoomExSetStatus[nStatus](pDrlgRoom);
		}
	}
	++pDrlgRoom->wRoomsInList[nStatus];
}

static void DRLGACTIVATE_RoomUnsetAndPropagateStatus(Room2* pDrlgRoom, DrlgRoomStatus nStatus)
{
	if (pDrlgRoom->wRoomsInList[nStatus] != 0)
	{
		--pDrlgRoom->wRoomsInList[nStatus];

		gRoomExUnsetStatus[nStatus](pDrlgRoom);

		for (int i = 0; i < pDrlgRoom->nRoomsNear; ++i)
		{
			if (Room2 * pNearRoom = pDrlgRoom->ppRoomsNear[i])
			{
				--pNearRoom->wRoomsInList[nStatus + 1];
				gRoomExUnsetStatus[nStatus + 1](pNearRoom);
				DRLGACTIVATE_RoomExPropagateUnsetStatus(pNearRoom, nStatus + 2);
			}
		}
	}
}

//D2Common.0x6FD739A0
void __fastcall DRLGACTIVATE_SetClientIsInSight(ActMisc* pDrlg, int nLevelId, int nX, int nY, Room2* pDrlgRoomHint)
{
	Level* pLevel = DRLG_GetLevel(pDrlg, nLevelId);
	if (pDrlgRoomHint && pDrlgRoomHint->pLevel->nLevelId != nLevelId)
	{
		pDrlgRoomHint = nullptr;
	}
	Room2* pDrlgRoom = DRLG_GetRoomExFromCoordinates(nX, nY, pDrlg, pDrlgRoomHint, pLevel);

	if (pDrlgRoom->wRoomsInList[ROOMSTATUS_CLIENT_IN_SIGHT] == 0)
	{
		DRLGACTIVATE_RoomSetAndPropagateStatus(pDrlgRoom, ROOMSTATUS_CLIENT_IN_SIGHT);
	}
}

//D2Common.0x6FD73B40
void __fastcall DRLGACTIVATE_UnsetClientIsInSight(ActMisc* pDrlg, int nLevelId, int nX, int nY, Room2* pDrlgRoomHint)
{
	Level* pLevel = DRLG_GetLevel(pDrlg, nLevelId);
	if (pDrlgRoomHint && pDrlgRoomHint->pLevel->nLevelId != nLevelId)
	{
		pDrlgRoomHint = nullptr;
	}

	if (Room2* pDrlgRoom = DRLG_GetRoomExFromCoordinates(nX, nY, pDrlg, pDrlgRoomHint, pLevel))
	{
		DRLGACTIVATE_RoomUnsetAndPropagateStatus(pDrlgRoom, ROOMSTATUS_CLIENT_IN_SIGHT);
	}
}


//D2Common.0x6FD73C40
void __fastcall DRLGACTIVATE_ChangeClientRoom(Room2* pPreviousRoom, Room2* pNewRoom)
{
	if (pPreviousRoom == pNewRoom)
	{
		return;
	}

	if (pNewRoom)
	{
		DRLGACTIVATE_RoomSetAndPropagateStatus(pNewRoom, ROOMSTATUS_CLIENT_IN_ROOM);
	}

	if (pPreviousRoom)
	{
		DRLGACTIVATE_RoomUnsetAndPropagateStatus(pPreviousRoom, ROOMSTATUS_CLIENT_IN_ROOM);
	}
}

//D2Common.0x6FD73CF0
void __fastcall DRLGACTIVATE_InitializeRoomEx(Room2* pDrlgRoom)
{
	if (!(pDrlgRoom->dwFlags & DRLGROOMFLAG_TILELIB_LOADED))
	{
		DRLGROOMTILE_LoadDT1FilesForRoom(pDrlgRoom);
	}

	if (!(pDrlgRoom->dwFlags & DRLGROOMFLAG_PRESET_UNITS_ADDED) && (pDrlgRoom->nType == DRLGTYPE_PRESET))
	{
		DRLGPRESET_SpawnHardcodedPresetUnits(pDrlgRoom);
	}

	DRLGACTIVATE_RoomEx_EnsureHasRoom(pDrlgRoom, false);
}

//D2Common.0x6FD73D80
Room1* __fastcall DRLGACTIVATE_StreamRoomAtCoords(ActMisc* pDrlg, int nX, int nY)
{
	if (Room2* pDrlgRoom = DRLG_GetRoomExFromCoordinates(nX, nY, pDrlg, 0, 0))
	{
		DRLGACTIVATE_InitializeRoomEx(pDrlgRoom);
		return pDrlgRoom->pRoom;
	}
	return nullptr;
}

//D2Common.0x6FD73E30
void __fastcall DRLGACTIVATE_InitializeRoomExStatusLists(ActMisc* pDrlg)
{
	for (int nStatus = 0; nStatus < ROOMSTATUS_COUNT; ++nStatus)
	{
		Room2* pDrlgRoomStatusList = &pDrlg->tStatusRoomsLists[nStatus];
		pDrlgRoomStatusList->fRoomStatus = nStatus;
		pDrlgRoomStatusList->pStatusNext = pDrlgRoomStatusList;
		pDrlgRoomStatusList->pStatusPrev = pDrlgRoomStatusList;
	}
}

//Helper function
static Room2* DRLGACTIVATE_RoomExStatusList_GetFirst(ActMisc* pDrlg, DrlgRoomStatus nStatus)
{
	const Room2* pStatusListHead = &pDrlg->tStatusRoomsLists[nStatus];
	if (pStatusListHead != pStatusListHead->pStatusNext) // If not empty
	{
		return pStatusListHead->pStatusNext;
	}
	return nullptr;
}

//D2Common.0x6FD73E60
Room1* __fastcall DRLGACTIVATE_GetARoomInClientSight(ActMisc* pDrlg)
{
	if (Room2* pDrlgRoom = DRLGACTIVATE_RoomExStatusList_GetFirst(pDrlg, ROOMSTATUS_CLIENT_IN_ROOM))
	{
		return pDrlgRoom->pRoom;
	}
	if (Room2* pDrlgRoom = DRLGACTIVATE_RoomExStatusList_GetFirst(pDrlg, ROOMSTATUS_CLIENT_IN_SIGHT))
	{
		return pDrlgRoom->pRoom;
	}
	return nullptr;
}

//D2Common.0x6FD73E90
Room1* __fastcall DRLGACTIVATE_GetARoomInSightButWithoutClient(ActMisc* pDrlg, Room2* pDrlgRoom)
{
	if (Room2* pNextStatusRoom = pDrlgRoom->pStatusNext)
	{
		if (pDrlgRoom->fRoomStatus != ROOMSTATUS_CLIENT_IN_ROOM || pNextStatusRoom != &pDrlg->tStatusRoomsLists[ROOMSTATUS_CLIENT_IN_ROOM])
		{
			return pNextStatusRoom->pRoom;
		}

		if (Room2* pFirstRoomInSight = DRLGACTIVATE_RoomExStatusList_GetFirst(pDrlg, ROOMSTATUS_CLIENT_IN_SIGHT))
		{
			return pFirstRoomInSight->pRoom;
		}
	}

	return nullptr;
}

//D2Common.0x6FD73EF0 (#10015)
void __fastcall DRLGACTIVATE_GetRoomsAllocationStats(int* pOutStatsClientAllocatedRooms, int* pOutStatsClientFreedRooms, int* pOutStatsAllocatedRooms, int* pOutStatsFreedRooms)
{
	*pOutStatsClientAllocatedRooms = gStatsClientAllocatedRooms;
	*pOutStatsClientFreedRooms = gStatsClientFreedRooms;
	*pOutStatsAllocatedRooms = gStatsAllocatedRooms;
	*pOutStatsFreedRooms = gStatsFreedRooms;
}

//D2Common.0x6FD73F20 (#10003)
void __stdcall DRLGACTIVATE_Update(ActMisc* pDrlg)
{
	if (DRLG_IsOnClient(pDrlg))
	{
		gStatsClientAllocatedRooms = pDrlg->nAllocatedRooms;
		gStatsClientFreedRooms = pDrlg->nFreedRooms;
	}
	else
	{
		gStatsAllocatedRooms = pDrlg->nAllocatedRooms;
		gStatsFreedRooms = pDrlg->nFreedRooms;
	}

	if (pDrlg->nRoomsInitSinceLastUpdate > 1)
	{
		pDrlg->nRoomsInitSinceLastUpdate = 0;
		return;
	}

	--pDrlg->nRoomsInitTimeout;

	if (pDrlg->nRoomsInitTimeout == 0)
	{
		DRLGACTIVATE_InitRoomsInitTimeout(pDrlg);

		if (!pDrlg->pDrlgRoom || pDrlg->pDrlgRoom->fRoomStatus != ROOMSTATUS_CLIENT_OUT_OF_SIGHT)
		{
			pDrlg->pDrlgRoom = pDrlg->tStatusRoomsLists[ROOMSTATUS_CLIENT_OUT_OF_SIGHT].pStatusNext;
		}


		if (Room2* pDrlgRoomExListHead = pDrlg->pDrlgRoom)
		{
			Room2* pCurRoomEx;
			for (pCurRoomEx = pDrlgRoomExListHead ; pCurRoomEx != pDrlgRoomExListHead ; pCurRoomEx = pCurRoomEx->pStatusNext)
			{
				if (pCurRoomEx != &pDrlg->tStatusRoomsLists[ROOMSTATUS_CLIENT_OUT_OF_SIGHT])
				{
					DRLGACTIVATE_RoomEx_EnsureHasRoom(pCurRoomEx, false);
				}
				if (pDrlg->nRoomsInitSinceLastUpdate != 0)
				{
					break;
				}
			}

			pDrlg->pDrlgRoom = pCurRoomEx;
			pDrlg->nRoomsInitSinceLastUpdate = 0;
		}
	}
}

//D2Common.0x6FD74060
BOOL __fastcall DRLGACTIVATE_TestRoomCanUnTile(Room2* pDrlgRoom)
{
	if (pDrlgRoom->dwFlags & DRLGROOMFLAG_HASPORTAL)
	{
		return FALSE;
	}

	D2_ASSERT(!DRLG_IsOnClient(DRLGROOM_GetDrlgFromRoomEx(pDrlgRoom)));

	if (pDrlgRoom->fRoomStatus <= 1)
	{
		return FALSE;
	}

	if (DRLG_IsTownLevel(pDrlgRoom->pLevel->nLevelId) || pDrlgRoom->pLevel->nLevelId == LEVEL_ROCKYSUMMIT)
	{
		for (Room2* pCurrentRoomEx = pDrlgRoom->pLevel->pFirstRoomEx; pCurrentRoomEx; pCurrentRoomEx = pCurrentRoomEx->pDrlgRoomNext)
		{
			if (pCurrentRoomEx->fRoomStatus <= 1)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

//D2Common.0x6FD740F0
void __fastcall DRLGACTIVATE_ToggleHasPortalFlag(Room2* pDrlgRoom, BOOL bReset)
{
	if (bReset)
	{
		pDrlgRoom->dwFlags &= ~DRLGROOMFLAG_HASPORTAL;
	}
	else
	{
		pDrlgRoom->dwFlags |= DRLGROOMFLAG_HASPORTAL;
	}
}

//D2Common.0x6FD74110
uint8_t __fastcall DRLGACTIVATE_GetRoomStatusFlags(Room2* pDrlgRoom)
{
	return pDrlgRoom->fRoomStatus;
}
