#pragma once

#include "D2CommonDefinitions.h"
#include <D2Seed.h>
#include "Path.h"
#pragma pack(1)

#pragma pack()

//D2Common.0x6FDAA880
BOOL __fastcall sub_6FDAA880(PathInfo* pPathInfo, int* pTestDir, PathPoint pPoint, int* pDirection);

//D2Common.0x6FDABA50
int __stdcall  sub_6FDABA50(PathPoint pPoint1, PathPoint pPoint2);
//1.10f: D2Common.0x6FDAB6A0
//1.13c: D2Common.0x6FD8E190
int __stdcall sub_6FDAB6A0(PathPoint pPoint1, PathPoint pPoint2);
//D2Common.0x6FDAB750
int __fastcall sub_6FDAB750(int nX1, int nY1, int nX2, int nY2);
//1.10f: D2Common.0x6FDAB770
//1.13c: D2Common.0x6FD8E5F0 (Inlined everywhere, but not stripped)
int __stdcall sub_6FDAB770(PathPoint pPoint1, PathPoint pPoint2);
//D2Common.0x6FDAB7D0
void __fastcall sub_6FDAB7D0(int* pTestDir, int nUnused, PathPoint pPoint1, PathPoint pPoint2);

//D2Common.0x6FDAB0B0
int __fastcall sub_6FDAB0B0(PathInfo* pPathInfo);
//1.10f: Inlined
//1.13c: D2Common.0x6FD85E00
BOOL PATH_PreparePathTargetForPathUpdate(PathInfo* pPathInfo);
//D2Common.0x6FDAA9F0
int __fastcall PATH_Toward_6FDAA9F0(PathInfo* pPathInfo);
//1.00:  D2Common.0x1005BB30
//1.10f: D2Common.0x6FDAABF0
BOOL __fastcall PATH_CheckCollisionsToNextPosition(Path* pDynamicPath, PathPoint* pGameCoord);
//D2Common.0x6FDAB130
int __fastcall PATH_Straight_Compute(PathInfo* pPathInfo);
//D2Common.0x6FDAB270
int __fastcall PATH_Knockback_Server(PathInfo* pPathInfo);
//D2Common.0x6FDAB1E0
int __fastcall PATH_Leap_6FDAB1E0(PathInfo* pPathInfo);
//D2Common.0x6FDAB240
int __fastcall PATH_Knockback_Client(PathInfo* pPathInfo);
//D2Common.0x6FDAB0C0
int __fastcall PATH_BackupTurn_Compute(PathInfo* pPathInfo);
//D2Common.0x6FDAB790
void __fastcall PATH_GetDirections_6FDAB790(int* pTestDir, PathPoint pPoint1, PathPoint pPoint2);

//D2Common.0x6FDAB3C0
signed int __fastcall PATH_ComputePathBlessedHammer_6FDAB3C0(Path* pDynamicPath);
//1.00:  D2Common.0x1005BCA0
//1.10f: D2Common.0x6FDAAD10
//1.13c: D2Common.0x6FDB6C20
BOOL __fastcall PATH_RayTrace(Path* pDynamicPath, PathPoint* pPathDestination, PathPoint tStartCoord);

//D2Common.0x6FDAB4A0
int __fastcall PATH_ComputePathChargedBolt_6FDAB4A0(Path* pDynamicPath, Seed* pSeed);

//D2Common.0x6FDAB610
int __fastcall sub_6FDAB610(int nX1, int nY1, int nX2, int nY2);

//D2Common.0x6FDAC700 (#10215)
D2COMMON_DLL_DECL int __stdcall PATH_ComputeDirection(int nX1, int nY1, int nX2, int nY2);

//D2Common.0x6FDAC760
int __stdcall PATH_ComputeDirectionFromPreciseCoords_6FDAC760(DWORD dwStartPrecisionX, DWORD dwStartPrecisionY, DWORD dwTargetPrecisionX, DWORD dwTargetPrecisionY);

//1.00:  D2Common.0x1005E9C0
//1.10f: D2Common.0x6FDAC790
//1.13c: D2Common.0x6FD5D240
void __fastcall PATH_ComputeVelocityAndDirectionVectorsToNextPoint(Path* pPath, BOOL bNormalizeDirectionIfSamePos, BOOL bForceDirectionNormalization);


//1.10f: D2Common.0x6FDAC8F0 (#10236)
//1.13c: D2Common.0x6FD5D820 (#10831)
D2COMMON_DLL_DECL int __stdcall D2Common_10236(UnitAny* pUnit, int a2);

//D2Common.0x6FDAC9A0 (#10226)
D2COMMON_DLL_DECL BOOL __stdcall D2Common_10226(UnitAny* pUnit, signed int a2);


//D2Common.0x6FDAD530 (#10227)
D2COMMON_DLL_DECL BOOL __stdcall D2Common_10227(UnitAny* pUnit);

//D2Common.0x6FDAD590 (#10229)
D2COMMON_DLL_DECL BOOL __stdcall D2Common_10229(Path* pDynamicPath, UnitAny* pUnit, Room1* pDestRoom, uint32_t nDestX, uint32_t nDestY);

//D2Common.0x6FDADA20
void __fastcall PATH_RecacheRoom(Path* pDynamicPath, Room1* pHintRoom);
