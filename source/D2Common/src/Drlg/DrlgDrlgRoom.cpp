#include "Drlg/D2DrlgDrlgRoom.h"

#include "D2DataTbls.h"
#include "Drlg/D2DrlgDrlg.h"
#include "Drlg/D2DrlgDrlgLogic.h"
#include "Drlg/D2DrlgDrlgWarp.h"
#include "Drlg/D2DrlgOutRoom.h"
#include "Drlg/D2DrlgPreset.h"
#include "Drlg/D2DrlgRoomTile.h"
#include "D2Dungeon.h"
#include "D2Seed.h"


//D2Common.0x6FD771C0
Room2* __fastcall DRLGROOM_AllocRoomEx(Level* pLevel, int nType)
{
	Room2* pDrlgRoom = D2_CALLOC_STRC_POOL(pLevel->pDrlg->pMempool, Room2);

	pDrlgRoom->pLevel = pLevel;
	pDrlgRoom->nType = nType;
	pDrlgRoom->fRoomStatus = 4;
	SEED_InitLowSeed(&pDrlgRoom->pSeed, (int)SEED_RollRandomNumber(&pLevel->pSeed));
	pDrlgRoom->dwInitSeed = (unsigned int)SEED_RollRandomNumber(&pDrlgRoom->pSeed);

	if (pLevel->dwFlags & DRLGLEVELFLAG_AUTOMAP_REVEAL)
	{
		pDrlgRoom->dwFlags |= DRLGROOMFLAG_AUTOMAP_REVEAL;
	}

	if (nType == DRLGTYPE_MAZE)
	{
		DRLGOUTROOM_AllocDrlgOutdoorRoom(pDrlgRoom);
	}
	else if (nType == DRLGTYPE_PRESET)
	{
		DRLGPRESET_AllocPresetRoomData(pDrlgRoom);
	}

	return pDrlgRoom;
}

//D2Common.0x6FD77280
void __fastcall sub_6FD77280(Room2* pDrlgRoom, BOOL bClient, uint32_t nFlags)
{
	pDrlgRoom->pRoom = NULL;

	pDrlgRoom->dwOtherFlags = nFlags & 1;

	if (pDrlgRoom->dwFlags & DRLGROOMFLAG_HAS_ROOM)
	{
		DRLGROOMTILE_FreeRoom(pDrlgRoom, bClient == 0);
	}
}

//D2Common.0x6FD772B0
void __fastcall DRLGROOM_FreeRoomTiles(void* pMemPool, Room2* pDrlgRoom)
{
	RoomTile* pRoomTile = NULL;
	RoomTile* pNext = NULL;

	pRoomTile = pDrlgRoom->pRoomTiles;
	while (pRoomTile)
	{
		pNext = pRoomTile->pNext;
		D2_FREE_POOL(pMemPool, pRoomTile);
		pRoomTile = pNext;
	}
	pDrlgRoom->pRoomTiles = NULL;
}

//D2Common.0x6FD772F0
void __fastcall DRLGROOM_FreeRoomEx(Room2* pDrlgRoom)
{
	PresetUnit* pNextPresetUnit = NULL;
	Room2* pCurrentRoomEx = NULL;
	Room2* pNextRoomEx = NULL;
	DrlgOrth* pNextDrlgOrth = NULL;
	DrlgOrth* pNextOrth = NULL;
	DrlgOrth* pOrth = NULL;
	void* pMemPool = NULL;

	pMemPool = pDrlgRoom->pLevel->pDrlg->pMempool;

	DRLGROOM_FreeRoomTiles(pMemPool, pDrlgRoom);

	if (pDrlgRoom->ppRoomsNear)
	{
		D2_FREE_POOL(pMemPool, pDrlgRoom->ppRoomsNear);
		pDrlgRoom->nRoomsNear = 0;
		pDrlgRoom->ppRoomsNear = NULL;
	}

	if (pDrlgRoom->nType == DRLGTYPE_MAZE)
	{
		DRLGOUTROOM_FreeDrlgOutdoorRoom(pDrlgRoom);
	}
	else if (pDrlgRoom->nType == DRLGTYPE_PRESET)
	{
		DRLGPRESET_FreePresetRoomData(pDrlgRoom);
	}

	
	for (PresetUnit* pPresetUnit = pDrlgRoom->pPresetUnits; pPresetUnit; pPresetUnit = pNextPresetUnit)
	{
		pNextPresetUnit = pPresetUnit->pNext;
		DRLGPRESET_FreePresetUnit(pMemPool, pPresetUnit);
	}

	for (DrlgOrth* pDrlgOrth = pDrlgRoom->pDrlgOrth; pDrlgOrth; pDrlgOrth = pNextDrlgOrth)
	{
		pNextDrlgOrth = pDrlgOrth->pNext;
		if (pDrlgOrth->bInit == 1)
		{
			pCurrentRoomEx = pDrlgOrth->pDrlgRoom;
			pOrth = pDrlgRoom->pDrlgOrth;
			pNextOrth = pOrth->pNext;

			if (pOrth->bInit == 1 && pOrth->pDrlgRoom == pCurrentRoomEx)
			{
				pDrlgRoom->pDrlgOrth = pNextOrth;
				pNextOrth = pOrth;

				D2_FREE_POOL(pMemPool, pNextOrth);
			}
			else
			{
				while (pNextOrth)
				{
					if (pNextOrth->bInit == 1 && pNextOrth->pDrlgRoom == pCurrentRoomEx)
					{
						pOrth->pNext = pNextOrth->pNext;
						D2_FREE_POOL(pMemPool, pNextOrth);
						break;
					}

					pOrth = pNextOrth;
					pNextOrth = pNextOrth->pNext;
				}
			}

			pOrth = pCurrentRoomEx->pDrlgOrth;
			pNextOrth = pOrth->pNext;
			if (pOrth->bInit == 1 && pOrth->pDrlgRoom == pDrlgRoom)
			{
				pCurrentRoomEx->pDrlgOrth = pNextOrth;
				pNextOrth = pOrth;
				D2_FREE_POOL(pMemPool, pNextOrth);
			}
			else
			{
				while (pNextOrth)
				{
					if (pNextOrth->bInit == 1 && pNextOrth->pDrlgRoom == pDrlgRoom)
					{
						pOrth->pNext = pNextOrth->pNext;
						D2_FREE_POOL(pMemPool, pNextOrth);
						break;
					}

					pOrth = pNextOrth;
					pNextOrth = pNextOrth->pNext;
				}
			}
		}
	}

	for (DrlgOrth* pDrlgOrth = pDrlgRoom->pDrlgOrth; pDrlgOrth; pDrlgOrth = pNextDrlgOrth)
	{
		pNextDrlgOrth = pDrlgOrth->pNext;
		D2_FREE_POOL(pMemPool, pDrlgOrth);
	}

	pCurrentRoomEx = pDrlgRoom->pLevel->pFirstRoomEx;
	if (pCurrentRoomEx == pDrlgRoom)
	{
		pDrlgRoom->pLevel->pFirstRoomEx = pDrlgRoom->pDrlgRoomNext;
		--pDrlgRoom->pLevel->nRooms;
	}
	else
	{
		pNextRoomEx = pCurrentRoomEx->pDrlgRoomNext;
		while (pNextRoomEx)
		{
			if (pNextRoomEx == pDrlgRoom)
			{
				pCurrentRoomEx->pDrlgRoomNext = pDrlgRoom->pDrlgRoomNext;
				--pDrlgRoom->pLevel->nRooms;
				break;
			}

			pCurrentRoomEx = pNextRoomEx;
			pNextRoomEx = pCurrentRoomEx->pDrlgRoomNext;
		}
	}

	DRLGROOMTILE_FreeTileGrid(pDrlgRoom);
	DRLGLOGIC_FreeDrlgCoordList(pDrlgRoom);
	D2_FREE_POOL(pMemPool, pDrlgRoom);
}

//D2Common.0x6FD774F0
void __fastcall DRLGROOM_FreeRoomData(void* pMemPool, DrlgOrth* pDrlgRoomData)
{
	DrlgOrth* pNext = NULL;
	
	for (DrlgOrth* pRoomData = pDrlgRoomData; pRoomData; pRoomData = pNext)
	{
		pNext = pRoomData->pNext;
		D2_FREE_POOL(pMemPool, pRoomData);
	}
}

//D2Common.0x6FD77520
void __fastcall DRLGROOM_AllocDrlgOrthsForRooms(Room2* pDrlgRoom1, Room2* pDrlgRoom2, int nDirection)
{
	DrlgOrth* pDrlgOrth = NULL;
	DrlgOrth* pNew = NULL;

	pDrlgOrth = pDrlgRoom1->pDrlgOrth;
	while (pDrlgOrth)
	{
		if (pDrlgOrth->pDrlgRoom == pDrlgRoom2)
		{
			break;
		}
		pDrlgOrth = pDrlgOrth->pNext;
	}

	if (!pDrlgOrth)
	{
		pNew = D2_CALLOC_STRC_POOL(pDrlgRoom1->pLevel->pDrlg->pMempool, DrlgOrth);
		pNew->pNext = pDrlgRoom1->pDrlgOrth;
		pDrlgRoom1->pDrlgOrth = pNew;
		pNew->pDrlgRoom = pDrlgRoom2;
		pNew->nDirection = nDirection;
		pNew->bInit = 1;
		pNew->pBox = &pDrlgRoom2->pDrlgCoord;
	}

	pDrlgOrth = pDrlgRoom2->pDrlgOrth;
	while (pDrlgOrth)
	{
		if (pDrlgOrth->pDrlgRoom == pDrlgRoom1)
		{
			break;
		}
		pDrlgOrth = pDrlgOrth->pNext;
	}

	if (!pDrlgOrth)
	{
		pNew = D2_CALLOC_STRC_POOL(pDrlgRoom2->pLevel->pDrlg->pMempool, DrlgOrth);
		pNew->pNext = pDrlgRoom2->pDrlgOrth;
		pDrlgRoom2->pDrlgOrth = pNew;
		pNew->pDrlgRoom = pDrlgRoom1;
		pNew->nDirection = ((uint8_t)nDirection - 2) & 3;
		pNew->bInit = 1;
		pNew->pBox = &pDrlgRoom1->pDrlgCoord;
	}
}

//D2Common.0x6FD77600
void __fastcall DRLGROOM_AddOrth(DrlgOrth** ppDrlgOrth, Level* pLevel, int nDirection, BOOL bIsPreset)
{
	DrlgOrth* pPrevious = NULL;
	DrlgOrth* pNext = NULL;
	DrlgOrth* pNew = NULL;

	pNew = D2_CALLOC_STRC_POOL(pLevel->pDrlg->pMempool, DrlgOrth);

	pNew->pLevel = pLevel;
	pNew->nDirection = nDirection;
	pNew->bPreset = bIsPreset;
	pNew->bInit = 0;
	pNew->pBox = &pLevel->pLevelCoords;

	if (!*ppDrlgOrth)
	{
		*ppDrlgOrth = pNew;
	}
	else
	{
		pNext = (*ppDrlgOrth)->pNext;
		pPrevious = *ppDrlgOrth;
		if (pNext)
		{
			do
			{
				if (sub_6FD776B0(pNext, pNew))
				{
					break;
				}

				pPrevious = pNext;
				pNext = pNext->pNext;
			}
			while (pNext);
		}
		else
		{
			if (sub_6FD776B0(*ppDrlgOrth, pNew))
			{
				pNew->pNext = *ppDrlgOrth;
				*ppDrlgOrth = pNew;
				return;
			}
		}

		pNew->pNext = pNext;
		pPrevious->pNext = pNew;
	}
}

//D2Common.0x6FD776B0
BOOL __fastcall sub_6FD776B0(DrlgOrth* pDrlgOrth1, DrlgOrth* pDrlgOrth2)
{
	if (pDrlgOrth1->nDirection <= pDrlgOrth2->nDirection)
	{
		if (pDrlgOrth1->nDirection == pDrlgOrth2->nDirection)
		{
			switch (pDrlgOrth2->nDirection)
			{
			case 0:
				if (pDrlgOrth1->pBox->nPosY <= pDrlgOrth2->pBox->nPosY)
				{
					return FALSE;
				}
				break;

			case 1:
				if (pDrlgOrth1->pBox->nPosX >= pDrlgOrth2->pBox->nPosX)
				{
					return FALSE;
				}
				break;

			case 2:
				if (pDrlgOrth1->pBox->nPosY >= pDrlgOrth2->pBox->nPosY)
				{
					return FALSE;
				}
				break;

			case 3:
				if (pDrlgOrth1->pBox->nPosX <= pDrlgOrth2->pBox->nPosX)
				{
					return FALSE;
				}
				break;

			default:
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

// Helper function
void DRLG_ComputeManhattanDistance(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int* pDistanceX, int* pDistanceY)
{
	// Negative distance means we are "inside" the other rectangle
	if (pDrlgCoord1->nPosX >= pDrlgCoord2->nPosX)
	{
		*pDistanceX = pDrlgCoord1->nPosX - pDrlgCoord2->nWidth - pDrlgCoord2->nPosX;
	}
	else
	{
		*pDistanceX = pDrlgCoord2->nPosX - pDrlgCoord1->nWidth - pDrlgCoord1->nPosX;
	}

	if (pDrlgCoord1->nPosY >= pDrlgCoord2->nPosY)
	{
		*pDistanceY = pDrlgCoord1->nPosY - pDrlgCoord2->nHeight - pDrlgCoord2->nPosY;
	}
	else
	{
		*pDistanceY = pDrlgCoord2->nPosY - pDrlgCoord1->nHeight - pDrlgCoord1->nPosY;
	}
}


//D2Common.0x6FD77740
// Compute manhattan distance between rectangles and returns true if distance is greater or equal than nMargin
BOOL __fastcall DRLG_GetRectanglesManhattanDistanceAndCheckNotOverlapping(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int nMaxDistance, int* pDistanceX, int* pDistanceY)
{
	DRLG_ComputeManhattanDistance(pDrlgCoord1, pDrlgCoord2, pDistanceX, pDistanceY);
	return *pDistanceX >= nMaxDistance || *pDistanceY >= nMaxDistance;
}

//D2Common.0x6FD777B0
// Compute manhattan distance between rectangles and returns true if distance is greater or equal than nMargin
BOOL __fastcall DRLG_CheckNotOverlappingUsingManhattanDistance(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int nMaxDistanceToAssumeCollision)
{
	int nSignedDistanceX = 0;
	int nSignedDistanceY = 0;
	return DRLG_GetRectanglesManhattanDistanceAndCheckNotOverlapping(pDrlgCoord1, pDrlgCoord2, nMaxDistanceToAssumeCollision, &nSignedDistanceX, &nSignedDistanceY);
}

//D2Common.0x6FD77800
BOOL __fastcall DRLG_CheckOverlappingWithOrthogonalMargin(DrlgCoord* pDrlgCoord1, DrlgCoord* pDrlgCoord2, int nOrthogonalDistanceMax)
{
	int nSignedDistanceX = 0;
	int nSignedDistanceY = 0;
	DRLG_ComputeManhattanDistance(pDrlgCoord1, pDrlgCoord2, &nSignedDistanceX, &nSignedDistanceY);

	if (nOrthogonalDistanceMax)
	{
		if (nSignedDistanceX == 0 && nSignedDistanceY <= nOrthogonalDistanceMax)
		{
			return TRUE;
		}

		if (nSignedDistanceY == 0 && nSignedDistanceX <= nOrthogonalDistanceMax)
		{
			return TRUE;
		}
	}
	else
	{
		if (nSignedDistanceX <= 0 && nSignedDistanceY <= 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//D2Common.0x6FD77890
BOOL __fastcall DRLGMAZE_CheckRoomNotOverlaping(Level* pLevel, Room2* pDrlgRoom1, Room2* pIgnoredRoom, int nMargin)
{
	for (Room2* pCurrentRoomEx = pLevel->pFirstRoomEx; pCurrentRoomEx; pCurrentRoomEx = pCurrentRoomEx->pDrlgRoomNext)
	{
		if (pCurrentRoomEx != pDrlgRoom1 && pCurrentRoomEx != pIgnoredRoom)
		{
			if (!DRLG_CheckNotOverlappingUsingManhattanDistance(&pDrlgRoom1->pDrlgCoord, &pCurrentRoomEx->pDrlgCoord, nMargin))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

//D2Common.0x6FD77910
void __fastcall DRLGROOM_AddRoomExToLevel(Level* pLevel, Room2* pDrlgRoom)
{
	pDrlgRoom->pDrlgRoomNext = pLevel->pFirstRoomEx;
	pLevel->pFirstRoomEx = pDrlgRoom;
	++pLevel->nRooms;
}

//D2Common.0x6FD77930
BOOL __fastcall DRLGROOM_AreXYInsideCoordinates(DrlgCoord* pDrlgCoord, int nX, int nY)
{
	if (nX >= pDrlgCoord->nPosX && nY >= pDrlgCoord->nPosY && nX < pDrlgCoord->nPosX + pDrlgCoord->nWidth && nY < pDrlgCoord->nPosY + pDrlgCoord->nHeight)
	{
		return TRUE;
	}

	return FALSE;
}

//D2Common.0x6FD77980
BOOL __fastcall DRLGROOM_AreXYInsideCoordinatesOrOnBorder(DrlgCoord* pDrlgCoord, int nX, int nY)
{
	if (nX >= pDrlgCoord->nPosX && nY >= pDrlgCoord->nPosY && nX <= pDrlgCoord->nPosX + pDrlgCoord->nWidth && nY <= pDrlgCoord->nPosY + pDrlgCoord->nHeight)
	{
		return TRUE;
	}

	return FALSE;
}

//D2Common.0x6FD779D0
BOOL __fastcall DRLGROOM_CheckLOSDraw(Room2* pDrlgRoom)
{
	if (pDrlgRoom->nType == DRLGTYPE_MAZE)
	{
		return TRUE;
	}
	else if (pDrlgRoom->nType == DRLGTYPE_PRESET)
	{
		return pDrlgRoom->dwFlags & DRLGROOMFLAG_NO_LOS_DRAW;
	}
		
	return FALSE;
}

//D2Common.0x6FD779F0
int __fastcall sub_6FD779F0(Room2* pDrlgRoom)
{
	if (pDrlgRoom->nType == DRLGTYPE_MAZE)
	{
		return pDrlgRoom->pOutdoor->dwFlags & 0x80;
	}

	return 0;
}

//D2Common.0x6FD77A00
int __fastcall DRLGROOM_CheckWaypointFlags(Room2* pDrlgRoom)
{
	return pDrlgRoom->dwFlags & (DRLGROOMFLAG_HAS_WAYPOINT | DRLGROOMFLAG_HAS_WAYPOINT_SMALL);
}

//D2Common.0x6FD77A10
int __fastcall DRLGROOM_GetLevelId(Room2* pDrlgRoom)
{
	return pDrlgRoom->pLevel->nLevelId;
}

//D2Common.0x6FD77A20
int __fastcall DRLGROOM_GetWarpDestinationLevel(Room2* pDrlgRoom, int nSourceLevel)
{
	int nDestinationLevel = 0;
	LvlWarpTxt* pLvlWarpTxtRecord = nullptr;
	Room1* pRoom = DRLGWARP_GetDestinationRoom(pDrlgRoom, nSourceLevel, &nDestinationLevel, &pLvlWarpTxtRecord);
	D2_ASSERT(pRoom);
	
	pDrlgRoom = DUNGEON_GetRoomExFromRoom(pRoom);
	D2_ASSERT(pDrlgRoom);
	
	D2_ASSERT(pDrlgRoom->pLevel);
	return pDrlgRoom->pLevel->nLevelId;
}

//D2Common.0x6FD77AB0
int __fastcall DRLGROOM_GetLevelIdFromPopulatedRoom(Room2* pDrlgRoom)
{
	if (!pDrlgRoom)
	{
		FOG_DisplayWarning("ptRoom", __FILE__, __LINE__);
	}

	if (pDrlgRoom->dwFlags & DRLGROOMFLAG_POPULATION_ZERO)
	{
		return 0;
	}
	else
	{
		return pDrlgRoom->pLevel->nLevelId;
	}
}

//D2Common.0x6FD77AF0
BOOL __fastcall DRLGROOM_HasWaypoint(Room2* pDrlgRoom)
{
	if (!pDrlgRoom)
	{
		FOG_DisplayWarning("ptRoom", __FILE__, __LINE__);
	}

	return DRLGROOM_CheckWaypointFlags(pDrlgRoom) != 0;
}

//D2Common.0x6FD77B20
const char* __fastcall DRLGROOM_GetPickedLevelPrestFilePathFromRoomEx(Room2* pDrlgRoom)
{
	if (pDrlgRoom->nType == DRLGTYPE_PRESET)
	{
		return DRLGPRESET_GetPickedLevelPrestFilePathFromRoomEx(pDrlgRoom);
	}
	else
	{
		return "None";
	}
}

//D2Common.0x6FD77B50
int __fastcall DRLGROOM_ReorderNearRoomList(Room2* pDrlgRoom, Room1** ppRoomList)
{
	int nRooms = 0;

	for (int i = 0; i < pDrlgRoom->nRoomsNear; ++i)
	{
		if (pDrlgRoom->ppRoomsNear[i]->pRoom)
		{
			ppRoomList[nRooms] = pDrlgRoom->ppRoomsNear[i]->pRoom;
			++nRooms;
		}
	}

	for (int i = nRooms; i < pDrlgRoom->nRoomsNear; ++i)
	{
		ppRoomList[i] = NULL;
	}

	return nRooms;
}

//D2Common.0x6FD77BB0
void __fastcall sub_6FD77BB0(void* pMemPool, Room2* pDrlgRoom)
{
	Room2* ppNearRooms[30] = {};
	Level* pLevel = NULL;
	int* pDestinationVisArray = 0;
	int* pSourceVisArray = 0;
	int nWarpDestination = 0;
	int nDestinationVisCount = 0;
	int nSourceVisCount = 0;
	int nFlags = 0;
	int nX = 0;
	int nY = 0;
	BOOL bSkip = FALSE;
	uint8_t nWarpId = 0;

	pDrlgRoom->nRoomsNear = 0;

	for (Room2* pCurrentRoomEx = pDrlgRoom->pLevel->pFirstRoomEx; pCurrentRoomEx; pCurrentRoomEx = pCurrentRoomEx->pDrlgRoomNext)
	{
		if (pDrlgRoom->nTileXPos >= pCurrentRoomEx->nTileXPos)
		{
			nX = pDrlgRoom->nTileXPos - pCurrentRoomEx->nTileWidth - pCurrentRoomEx->nTileXPos;
		}
		else
		{
			nX = pCurrentRoomEx->nTileXPos - pDrlgRoom->nTileWidth - pDrlgRoom->nTileXPos;
		}

		if (pDrlgRoom->nTileYPos >= pCurrentRoomEx->nTileYPos)
		{
			nY = pDrlgRoom->nTileYPos - pCurrentRoomEx->nTileHeight - pCurrentRoomEx->nTileYPos;
		}
		else
		{
			nY = pCurrentRoomEx->nTileYPos - pDrlgRoom->nTileHeight - pDrlgRoom->nTileYPos;
		}

		if (nX < 6 && nY < 6)
		{
			ppNearRooms[pDrlgRoom->nRoomsNear] = pCurrentRoomEx;
			++pDrlgRoom->nRoomsNear;
		}
	}

	DRLGROOM_SortRoomListByPosition(ppNearRooms, pDrlgRoom->nRoomsNear);

	pDrlgRoom->ppRoomsNear = (Room2**)D2_ALLOC_POOL(pMemPool, sizeof(Room2*) * pDrlgRoom->nRoomsNear);

	for (int i = 0; i < pDrlgRoom->nRoomsNear; ++i)
	{
		pDrlgRoom->ppRoomsNear[i] = ppNearRooms[i];
	}

	if (pDrlgRoom->dwFlags & DRLGROOMFLAG_HAS_WARP_MASK)
	{
		nFlags = DRLGROOMFLAG_HAS_WARP_0;
		nWarpId = 0;

		do
		{
			if (nFlags & pDrlgRoom->dwFlags)
			{
				if (nWarpId >= 8)
				{
					FOG_DisplayWarning("ptRoom1 && bExit1 < LEVEL_VIS_MAX", __FILE__, __LINE__);
				}

				pSourceVisArray = DRLGROOM_GetVisArrayFromLevelId(pDrlgRoom->pLevel->pDrlg, pDrlgRoom->pLevel->nLevelId);

				pLevel = DRLG_GetLevel(pDrlgRoom->pLevel->pDrlg, pSourceVisArray[nWarpId]);

				pDestinationVisArray = DRLGROOM_GetVisArrayFromLevelId(pDrlgRoom->pLevel->pDrlg, pSourceVisArray[nWarpId]);
				nWarpDestination = DRLGWARP_GetWarpDestinationFromArray(pDrlgRoom->pLevel, nWarpId);

				if (!pLevel->pFirstRoomEx)
				{
					DRLG_InitLevel(pLevel);
				}

				if (nWarpDestination != -1)
				{
					nSourceVisCount = 0;
					nDestinationVisCount = 0;
					for (int i = 0; i < nWarpId; ++i)
					{
						if (pSourceVisArray[i] == pSourceVisArray[nWarpId])
						{
							++nSourceVisCount;
						}
					}

					for (int i = 0; i < 8; ++i)
					{
						if (pDestinationVisArray[i] == pDrlgRoom->pLevel->nLevelId)
						{
							if (nSourceVisCount == nDestinationVisCount)
							{
								if (sub_6FD77F00(pMemPool, pDrlgRoom, nWarpId, pLevel->pFirstRoomEx, i, nWarpDestination))
								{
									bSkip = TRUE;
								}
								break;
							}

							++nDestinationVisCount;
						}
					}
				}

				if (!bSkip)
				{
					for (int i = 0; i < 8; ++i)
					{
						if (pDestinationVisArray[i] == pDrlgRoom->pLevel->nLevelId && sub_6FD77F00(pMemPool, pDrlgRoom, nWarpId, pLevel->pFirstRoomEx, i, nWarpDestination))
						{
							break;
						}
					}
				}
			}

			nFlags <<= 1;

			++nWarpId;
		}
		while (nFlags & DRLGROOMFLAG_HAS_WARP_MASK);
	}

	if (!DRLG_IsTownLevel(pDrlgRoom->pLevel->nLevelId))
	{
		for (int i = 0; i < pDrlgRoom->nRoomsNear; ++i)
		{
			if (DRLG_IsTownLevel(pDrlgRoom->ppRoomsNear[i]->pLevel->nLevelId))
			{
				pDrlgRoom->dwFlags |= DRLGROOMFLAG_POPULATION_ZERO;
				return;
			}
		}
	}
}

//D2Common.0x6FD77EB0
void __fastcall DRLGROOM_SortRoomListByPosition(Room2** ppRoomList, int nListSize)
{
	Room2* pDrlgRoom1 = NULL;
	Room2* pDrlgRoom2 = NULL;

	for (int j = nListSize - 1; j > 0; --j)
	{
		for (int i = 0; i < nListSize - 1; ++i)
		{
			pDrlgRoom1 = ppRoomList[i + 1];
			pDrlgRoom2 = ppRoomList[i];

			if (pDrlgRoom2->nTileXPos >= pDrlgRoom1->nTileXPos + pDrlgRoom1->nTileWidth || pDrlgRoom2->nTileYPos >= pDrlgRoom1->nTileYPos + pDrlgRoom1->nTileHeight)
			{
				ppRoomList[i] = pDrlgRoom1;
				ppRoomList[i + 1] = pDrlgRoom2;
			}
		}
	}
}

//D2Common.0x6FD77F00
BOOL __fastcall sub_6FD77F00(void* pMemPool, Room2* pDrlgRoom1, uint8_t nWarpId, Room2* pDrlgRoom2, char nWarpFlag, int nDirection)
{
	RoomTile* pRoomTile = NULL;
	int nX = 0;
	int nY = 0;
	BOOL bResult = FALSE;

	while (pDrlgRoom2)
	{
		if (((1 << (nWarpFlag + 4)) & pDrlgRoom2->dwFlags))
		{
			if (nDirection == -1)
			{
				if (pDrlgRoom1->nTileXPos >= pDrlgRoom2->nTileXPos)
				{
					nX = pDrlgRoom1->nTileXPos - pDrlgRoom2->nTileWidth - pDrlgRoom2->nTileXPos;
				}
				else
				{
					nX = pDrlgRoom2->nTileXPos - pDrlgRoom1->nTileWidth - pDrlgRoom1->nTileXPos;
				}

				if (pDrlgRoom1->nTileYPos >= pDrlgRoom2->nTileYPos)
				{
					nY = pDrlgRoom1->nTileYPos - pDrlgRoom2->nTileHeight - pDrlgRoom2->nTileYPos;
				}
				else
				{
					nY = pDrlgRoom2->nTileYPos - pDrlgRoom1->nTileHeight - pDrlgRoom1->nTileYPos;
				}

				if (nX < 6 && nY < 6)
				{
					++pDrlgRoom1->nRoomsNear;
					pDrlgRoom1->ppRoomsNear = (Room2**)D2_REALLOC_POOL(pMemPool, pDrlgRoom1->ppRoomsNear, sizeof(Room2*) * pDrlgRoom1->nRoomsNear);

					D2_ASSERT(pDrlgRoom1->ppRoomsNear);

					pDrlgRoom1->ppRoomsNear[pDrlgRoom1->nRoomsNear - 1] = pDrlgRoom2;
					DRLGROOM_SortRoomListByPosition(pDrlgRoom1->ppRoomsNear, pDrlgRoom1->nRoomsNear);

					bResult = TRUE;
				}
			}
			else
			{
				++pDrlgRoom1->nRoomsNear;
				pDrlgRoom1->ppRoomsNear = (Room2**)D2_REALLOC_POOL(pMemPool, pDrlgRoom1->ppRoomsNear, sizeof(Room2*) * pDrlgRoom1->nRoomsNear);

				D2_ASSERT(pDrlgRoom1->ppRoomsNear);
				pDrlgRoom1->ppRoomsNear[pDrlgRoom1->nRoomsNear - 1] = pDrlgRoom2;
				DRLGROOM_SortRoomListByPosition(pDrlgRoom1->ppRoomsNear, pDrlgRoom1->nRoomsNear);

				pRoomTile = D2_CALLOC_STRC_POOL(pDrlgRoom1->pLevel->pDrlg->pMempool, RoomTile);
				pRoomTile->pDrlgRoom = pDrlgRoom2;
				pRoomTile->pLvlWarpTxtRecord = DRLGWARP_GetLvlWarpTxtRecordFromWarpIdAndDirection(pDrlgRoom1->pLevel, nWarpId, 'b');
				pRoomTile->bEnabled = 1;
				pRoomTile->pNext = pDrlgRoom1->pRoomTiles;
				pDrlgRoom1->pRoomTiles = pRoomTile;

				return TRUE;
			}
		}

		pDrlgRoom2 = pDrlgRoom2->pDrlgRoomNext;
	}

	return bResult;
}

//D2Common.0x6FD780E0
PresetUnit* __fastcall DRLGROOM_AllocPresetUnit(Room2* pDrlgRoom, void* pMemPool, int nUnitType, int nIndex, int nMode, int nX, int nY)
{
	PresetUnit* pPresetUnit = D2_CALLOC_STRC_POOL(pMemPool, PresetUnit);

	pPresetUnit->nUnitType = nUnitType;
	pPresetUnit->nMode = nMode;
	pPresetUnit->nIndex = nIndex;
	pPresetUnit->nYpos = nY;
	pPresetUnit->nXpos = nX;

	if (pDrlgRoom)
	{
		pPresetUnit->pNext = pDrlgRoom->pPresetUnits;
		pDrlgRoom->pPresetUnits = pPresetUnit;
	}
	else
	{
		pPresetUnit->pNext = NULL;
	}

	return pPresetUnit;
}

//D2Common.0x6FD78160
PresetUnit* __fastcall DRLGROOM_GetPresetUnits(Room2* pDrlgRoom)
{
	if (!(pDrlgRoom->dwFlags & DRLGROOMFLAG_AUTOMAP_REVEAL))
	{
		if (pDrlgRoom->dwFlags & DRLGROOMFLAG_PRESET_UNITS_SPAWNED)
		{
			return NULL;
		}

		pDrlgRoom->dwFlags |= DRLGROOMFLAG_PRESET_UNITS_SPAWNED;
	}

	return pDrlgRoom->pPresetUnits;
}

//D2Common.0x6FD78190
void __fastcall DRLGROOM_SetRoom(Room2* pDrlgRoom, Room1* pRoom)
{
	pDrlgRoom->pRoom = pRoom;
}

//D2Common.0x6FD781A0
void __fastcall DRLGROOM_GetRGB_IntensityFromRoomEx(Room2* pDrlgRoom, uint8_t* pIntensity, uint8_t* pRed, uint8_t* pGreen, uint8_t* pBlue)
{
	LevelDefBin* pLevelDefRecord = DATATBLS_GetLevelDefRecord(pDrlgRoom->pLevel->nLevelId);

	*pIntensity = pLevelDefRecord->nIntensity;
	*pRed = pLevelDefRecord->nRed;
	*pGreen = pLevelDefRecord->nGreen;
	*pBlue = pLevelDefRecord->nBlue;
}

//D2Common.0x6FD781E0
int* __fastcall DRLGROOM_GetVisArrayFromLevelId(ActMisc* pDrlg, int nLevelId)
{
	DrlgWarp* pDrlgWarp = pDrlg->pWarp;

	while (pDrlgWarp)
	{
		if (!pDrlgWarp->nLevel)
		{
			FOG_DisplayWarning("ptVisInfo->eLevelId != LEVEL_ID_NONE", __FILE__, __LINE__);
		}

		if (nLevelId == pDrlgWarp->nLevel)
		{
			return pDrlgWarp->nVis;
		}

		pDrlgWarp = pDrlgWarp->pNext;
	}

	return DATATBLS_GetLevelDefRecord(nLevelId)->dwVis;
}

//D2Common.0x6FD78230
ActMisc* __fastcall DRLGROOM_GetDrlgFromRoomEx(Room2* pRoom)
{
	D2_ASSERT(pRoom);
	D2_ASSERT(pRoom->pLevel);
	D2_ASSERT(pRoom->pLevel->pDrlg);
	return pRoom->pLevel->pDrlg;
}
