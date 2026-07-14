#pragma once

#include <Units/Units.h>
#include "GAME/Game.h"

#pragma pack(1)

struct PartyNode
{
	int nUnitGUID;
	PartyNode* pNext;
};

struct Party
{
	int16_t nPartyId;
	int16_t field_2;
	PartyNode* pPartyNodes;
	Party* pNext;
};

struct PartyControl
{
	int16_t field_0;
	int16_t field_2;
	Party* pParties;
};

using PartyCallbackFunction = void(__fastcall*)(Game*, UnitAny*, void*);

#pragma pack()

//D2Game.0x6FCB9B00
void __fastcall PARTY_AllocPartyControl(Game* pGame);
//D2Game.0x6FCB9BA0
void __fastcall PARTY_FreePartyControl(Game* pGame);
//D2Game.0x6FCB9C40
int16_t __fastcall sub_6FCB9C40(Game* pGame);
//D2Game.0x6FCB9D10
int32_t __fastcall sub_6FCB9D10(Game* pGame, int16_t nPartyId, UnitAny* pPlayer);
//D2Game.0x6FCB9E80
void __fastcall PARTY_LeaveParty(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBA0C0
int32_t __fastcall PARTY_GetLivingPartyMemberCountInSameLevel(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBA0E0
void __fastcall PARTY_CountLivingUnits(Game* pGame, UnitAny* pUnit, void* pLivingUnits);
//D2Game.0x6FCBA100
void __fastcall PARTY_IteratePartyMembers(Game* pGame, int16_t nPartyId, PartyCallbackFunction pCallback, void* pArgs);
//D2Game.0x6FCBA190
void __fastcall PARTY_IteratePartyMembersInSameLevel(Game* pGame, UnitAny* pUnit, PartyCallbackFunction pCallback, void* pArgs);
//D2Game.0x6FCBA270
int32_t __fastcall PARTY_ShareGoldDrop(Game* pGame, UnitAny* pUnit, int32_t nGoldValue);
//D2Game.0x6FCBA510
void __fastcall PARTY_CalculatePickAndDrop(UnitAny* pUnit, int32_t nValue, int32_t* pGoldToPick, int32_t* pGoldToDrop);
//D2Game.0x6FCBA550
void __fastcall PARTY_SynchronizeWithClient(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBA5F0
int16_t __fastcall PARTY_GetPartyIdForUnitOwner(Game* pGame, UnitAny* pUnit);
