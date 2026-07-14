#pragma once

#include <Units/Units.h>


enum PetTypes
{
	PETTYPE_NONE,
	PETTYPE_SINGLE,
	PETTYPE_VALKYRIE,
	PETTYPE_GOLEM,
	PETTYPE_SKELETON,
	PETTYPE_SKELETONMAGE,
	PETTYPE_REVIVE,
	PETTYPE_HIREABLE,
	PETTYPE_DOPPLEZON,
	PETTYPE_INVIS,
	PETTYPE_RAVEN,
	PETTYPE_SPIRITWOLF,
	PETTYPE_FENRIS,
	PETTYPE_TOTEM,
	PETTYPE_VINE,
	PETTYPE_GRIZZLY,
	PETTYPE_SHADOWWARRIOR,
	PETTYPE_ASSASSINTRAP,
	PETTYPE_PETTRAP,
	PETTYPE_HYDRA,
};


//D2Game.0x6FC7CAF0
void __fastcall PLAYERPETS_AllocPetList(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC7CD10
void __fastcall D2GAME_KillPlayerPets_6FC7CD10(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC7CDC0
void __fastcall sub_6FC7CDC0(Game* pGame, UnitAny* pPlayer, PetList* pPetList, int32_t a4, int32_t a5);
//D2Game.0x6FC7CF20
void __fastcall PLAYERPETS_PlayerIterate_SynchronizeWithClient(Game* pGame, UnitAny* pPlayer, void* pArg);
//D2Game.0x6FC7CF50
void __fastcall sub_6FC7CF50(Game* pGame, int32_t nPetGUID);
//D2Game.0x6FC7D060
void __fastcall PLAYERPETS_FreePetsFromPlayerData(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC7D150
void __fastcall sub_6FC7D150(Game* pGame, UnitAny* pPlayer, int32_t nPetType, int32_t nBaseMax);
//D2Game.0x6FC7D260
void __fastcall sub_6FC7D260(Game* pGame, UnitAny* pPlayer, int32_t nPetGUID, int32_t a4);
//D2Game.0x6FC7D390
void __fastcall PLAYERPETS_RemovePetFromList(Game* pGame, PetList* pPetList, int32_t nUnitGUID, int32_t a4);
//D2Game.0x6FC7D470
void __fastcall sub_6FC7D470(Game* pGame, UnitAny* pPlayer, UnitAny* pPet);
//D2Game.0x6FC7D5F0
int32_t __fastcall sub_6FC7D5F0(Game* pGame, UnitAny* pPlayer, UnitAny* pPet);
//D2Game.0x6FC7D720
void __fastcall PLAYERPETS_PlayerIterate_UpdateClient(Game* pGame, UnitAny* pPlayer, void* pArg);
//D2Game.0x6FC7D7A0
void __fastcall sub_6FC7D7A0(Game* pGame, UnitAny* pPlayer, UnitAny* pPet, int32_t nPetType, int32_t nPetMax);
//D2Game.0x6FC7D9D0
int32_t __fastcall sub_6FC7D9D0(UnitAny* pPlayer, int32_t nPetType);
//D2Game.0x6FC7DA40
int32_t __fastcall sub_6FC7DA40(Game* pGame, UnitAny* pPlayer, PetList* pPetList, UnitAny* pPet, PetInfo* pPetInfo);
//D2Game.0x6FC7DB90
void __fastcall PLAYERPETS_UpdatePetInfo(UnitAny* pUnit, int32_t nPetType, int32_t nUnitGUID, PetInfo* pPetInfo);
//D2Game.0x6FC7DBF0
void __fastcall sub_6FC7DBF0(Game* pGame, UnitAny* pPlayer, UnitAny* pPet, int32_t nPetType, PetInfo* pPetInfo);
//D2Game.0x6FC7DD10
int32_t __fastcall PLAYERPETS_GetPetTypeFromPetGUID(UnitAny* pPlayer, int32_t nPetGUID);
//D2Game.0x6FC7DD90
int32_t __fastcall sub_6FC7DD90(Game* pGame, UnitAny* pPlayer, UnitAny* pPet);
//D2Game.0x6FC7DEB0
PetInfo* __fastcall PLAYERPETS_GetPetInfoFromPetGUID(UnitAny* pUnit, int32_t nUnitGUID);
//D2Game.0x6FC7DF40
void __fastcall sub_6FC7DF40(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC7E2E0
PetList* __fastcall PLAYERPETS_GetPetListFromPetType(PlayerPet* pPlayerPets, int32_t nPetType);
//D2Game.0x6FC7E310
void __fastcall sub_6FC7E310(Game* pGame, UnitAny* pPlayer, int32_t a3, int32_t a4);
//D2Game.0x6FC7E550
void __fastcall sub_6FC7E550(Game* pGame, UnitAny* pPlayer, PetData* pPetData, Room1* pRoom, int32_t a5, int32_t a6);
//D2Game.0x6FC7E640
void __fastcall sub_6FC7E640(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC7E7C0
void __fastcall PLAYERPETS_IteratePets(Game* pGame, UnitAny* pPlayer, void(__fastcall* pCallback)(Game*, UnitAny*, UnitAny*, void*), void* a4);
//D2Game.0x6FC7E8B0
UnitAny* __fastcall sub_6FC7E8B0(Game* pGame, UnitAny* pPlayer, int32_t nPetType, int32_t a4);
//D2Game.0x6FC7E930
int32_t __fastcall PLAYERPETS_GetTotalPetCount(UnitAny* pPlayer);
//D2Game.0x6FC7E970
void __fastcall D2GAME_PLAYERPETS_Last_6FC7E970(GameClient* pClient, UnitAny* pPlayer);
