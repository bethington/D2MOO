#pragma once

#include "D2CommonDefinitions.h"

struct Level;

#pragma pack(1)


#pragma pack()


//Inlined in D2Common.0x6FD7D430
void __fastcall DRLGOUTDESR_PlaceDesertTransitionToTown(Level* pLevel);
//D2Common.0x6FD7D430
void __fastcall DRLGOUTDESR_InitAct2OutdoorLevel(Level* pLevel);
//D2Common.0x6FD7D870
void __fastcall DRLGOUTDESR_PlacePresetVariants(Level* pLevel, const int* pLevelPrestIds, unsigned int nVariants, BOOL bIterateFiles);
//D2Common.6FD7D950
void __fastcall DRLGOUTDESR_PlaceCliffs(Level* pLevel);
//D2Common.0x6FD7D9B0
void __fastcall DRLGOUTDESR_PlaceBorders(Level* pLevel);
//D2Common.0x6FD7D9F0
void __fastcall DRLGOUTDESR_AddExits(Level* pLevel);
//D2Common.0x6FD7DA60
void __fastcall DRLGOUTDESR_PlaceFillsInFarOasis(Level* pLevel);
//D2Common.0x6FD7DAC0
void __fastcall DRLGOUTDESR_PlaceRuinsInLostCity(Level* pLevel);
//D2Common.0x6FD7DB00
void __fastcall DRLGOUTDESR_PlaceFillsInLostCity(Level* pLevel);
//D2Common.6FD7DB70
void __fastcall DRLGOUTDESR_PlaceTombEntriesInCanyon(Level* pLevel);
//D2Common.0x6FD7DBC0
void __fastcall DRLGOUTDESR_PlaceFillsInCanyon(Level* pLevel);
