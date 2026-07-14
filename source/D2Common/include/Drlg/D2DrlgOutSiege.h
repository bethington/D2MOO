#pragma once

#include "D2CommonDefinitions.h"

struct Level;

#pragma pack(1)


#pragma pack()

//D2Common.0x6FD84100
int __fastcall sub_6FD84100(Level* pLevel);
//D2Common.0x6FD84110
void __fastcall DRLGOUTSIEGE_InitAct5OutdoorLevel(Level* pLevel);
//D2Common.0x6FD844F0
void __fastcall DRLGOUTSIEGE_PlaceCaves(Level* pLevel);
//D2Common.0x6FD84580
void __fastcall DRLGOUTSIEGE_PlaceBarricadeEntrancesAndExits(Level* pLevel);
//D2Common.0x6FD846C0
void __fastcall sub_6FD846C0(Level* pLevel);
//D2Common.0x6FD84700
void __fastcall DRLGOUTSIEGE_AddACt5SecondaryBorder(Level* pLevel);
//D2Common.0x6FD84780
int __fastcall sub_6FD84780(Level* pLevel, int nStyle, int a3);
//D2Common.0x6FD84820
BOOL __fastcall sub_6FD84820(Level* pLevel, int nX, int nY, int a4, int a5, unsigned int a6);
//D2Common.0x6FD84870
void __fastcall DRLGOUTSIEGE_PlaceSpecialPresets(Level* pLevel);
//D2Common.0x6FD84910
void __fastcall DRLGOUTSIEGE_PlacePrisons(Level* pLevel);
//D2Common.0x6FD84BB0
void __fastcall DRLGOUTSIEGE_ConnectBarricadeAndSiege(Level* pLevel);
