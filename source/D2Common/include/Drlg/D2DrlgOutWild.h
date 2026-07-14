#pragma once

#include "D2CommonDefinitions.h"
#include <Drlg/D2DrlgDrlgVer.h>

struct Level;

#pragma pack(1)


#pragma pack()

//D2Common.0x6FD84CA0
void __fastcall DRLGOUTWILD_GetBridgeCoords(Level* pLevel, int* pX, int* pY);
//D2Common.0x6FD84D30
void __fastcall DRLGOUTWILD_InitAct1OutdoorLevel(Level* pLevel);
//D2Common.0x6FD85060
BOOL __fastcall DRLGOUTWILD_TestSpawnRiver(Level* pLevel, int nX);
//D2Common.0x6FD850B0
void __fastcall DRLGOUTWILD_SpawnRiver(Level* pLevel, int nX);
//D2Common.0x6FD85300
BOOL __fastcall sub_6FD85300(DrlgVertex* pDrlgVertex);
//D2Common.0x6FD85350
BOOL __fastcall sub_6FD85350(DrlgVertex* pDrlgVertex);
//D2Common.0x6FD85390
BOOL __fastcall DRLGOUTWILD_SpawnCliffCaves(Level* pLevel, int nX, int nY);
//D2Common.0x6FD853F0
void __fastcall DRLGOUTWILD_SpawnTownTransitionsAndCaves(Level* pLevel);
//D2Common.0x6FD85520
void __fastcall DRLGOUTWILD_SpawnSpecialPresets(Level* pLevel);
//D2Common.0x6FD85920
void __fastcall DRLGOUTWILD_SpawnCottage(Level* pLevel, int nLvlPrestId, int a3);
