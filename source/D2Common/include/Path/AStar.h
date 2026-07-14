#pragma once

#include "D2CommonDefinitions.h"
#include "Path.h"
#include "D2Seed.h"

// Note: Very similar to D1's path finding, but using a cache for the open/closed set

#pragma pack(1)

struct PathFoWallNode					    //sizeof 0x38
{
	PathPoint tPoint;						//0x00
	int16_t nFScore;							//0x04
	int16_t nHeuristicDistanceToTarget;			//0x06
	int16_t nBestDistanceFromStart;				//0x08
	uint16_t wPad;								//0x0A
	PathFoWallNode* pBestParent;		    //0x0C
	PathFoWallNode* pChildren[8];			//0x10
	PathFoWallNode* pNextCachePoint;		//0x30
	PathFoWallNode* pNextSortedByFScore;	//0x34
};

struct PathFoWallContext						  //sizeof 0x32EC
{
	static const size_t CACHE_SIZE = 128;
	static const size_t STORAGE_SIZE = 200;
	static const size_t STACK_SIZE = 200;

	PathFoWallNode* aPendingCache[CACHE_SIZE];  //0x0000 Open set
	PathFoWallNode* aVisitedCache[CACHE_SIZE];  //0x0200 Closed set
	PathFoWallNode* pSortedListByFScore;		  //0x0400 Sorted in ascending order (best first)
	PathFoWallNode aNodesStorage[STORAGE_SIZE]; //0x0404 Allocator
	int32_t nNodesCount;							  //0x2FC4 Number of allocated nodes
	PathFoWallNode* aPointsStack[STACK_SIZE];   //0x2FC8 Stack used for propagation of best distance
	int32_t nStackCount;							  //0x32E8
};


#pragma pack()

//D2Common.0x6FDA69E0
int __fastcall PATH_AStar_ComputePath(PathInfo* pPathInfo);

//D2Common.0x6FDA6D10
int __fastcall PATH_AStar_PushToVisitedCache(PathFoWallContext* pContext, PathFoWallNode* pNode);

//1.10f: D2Common.0x6FDA6D50
//1.13c: D2Common.0x6FDCB3C0
BOOL __fastcall PATH_AStar_ExploreChildren(PathInfo* pPathInfo, PathFoWallContext* pContext, PathFoWallNode* a3, PathPoint tTargetCoord);

//1.10f: D2Common.0x6FDA7230
//1.13c: D2Common.0x6FDCAF70
int __stdcall PATH_AStar_Heuristic(PathPoint tPoint1, PathPoint tPoint2);
// Helper function
int16_t PATH_AStar_HeuristicForNeighbor(PathPoint tPoint, PathPoint tNeighbor);

//1.00:  D2Common.0x10057A10
//1.10f: D2Common.0x6FDA7280
//1.13c: D2Common.0x6FDCAF20
PathFoWallNode* __fastcall PATH_AStar_GetNodeFromPendingCache(PathFoWallContext* pContext, PathPoint tPathPoint);
//1.00:  D2Common.0x10057A10
//1.10f: D2Common.0x6FDA72D0
//1.13c: D2Common.0x6FDCAED0
PathFoWallNode* __fastcall PATH_AStar_FindPointInVisitedCache(PathFoWallContext* pContext, PathPoint tPathPoint);

//1.10f: D2Common.0x6FDA7320
//1.13c: D2Common.0x6FDCAE20
void __fastcall PATH_AStar_MakeCandidate(PathFoWallContext* pContext, PathFoWallNode* pNode);

//1.10f: D2Common.0x6FDA7390
//1.13c: D2Common.0x6FDCAC50
void __fastcall PATH_AStar_PropagateNewFScoreToChildren(PathFoWallContext* pContext, int nUnused, PathFoWallNode* pNewNode);

//1.10f: D2Common.0x6FDA7450
//1.13c: Inlined
PathFoWallNode* __fastcall PATH_AStar_GetNewNode(PathFoWallContext* pContext);

//1.10f: D2Common.0x6FDA7490
//1.13c: D2Common.0x6FDCAFB0
BOOL __fastcall PATH_AStar_EvaluateNeighbor(PathInfo* pPathInfo, PathFoWallContext* pContext, PathFoWallNode* pCurrentNode, PathPoint tNewPointCoord, PathPoint tTargetCoord);

//D2Common.0x6FDA78A0
signed int __fastcall PATH_AStar_FlushNodeToDynamicPath(PathFoWallNode* pNode, PathInfo* pPathInfo);

