#pragma once

#include <Units/Units.h>
#include <Drlg/D2DrlgPreset.h>

#include <GAME/Event.h>

#pragma pack(push, 1)
struct UnitInfo		// sizeof(0x34)
{
	int32_t nUnitType;		// 0x00
	int32_t nClassId;		// 0x04
	int32_t nUnitGUID;		// 0x08
	int32_t nAnimMode;		// 0x0C
	int32_t nX;				// 0x10
	int32_t nY;				// 0x14
	int32_t nOwnerType;		// 0x18
	int32_t nOwnerGUID;		// 0x1C
	int32_t unk0x20;		// 0x20
	char szDescription[16];	// 0x24
};

struct UnitDescriptionList
{
	uint32_t nClassId;					//0x00
	uint32_t nCount;					//0x04
	uint32_t nDefaultCount;				//0x08 Always 0 in 1.10f. Used to be added to nCount for display on the server
	char szDescription[128];			//0x0C
	UnitDescriptionList* pNext;	//0x8C
}; // Size=0x90
#pragma pack(pop)


//D2Game.0x6FCBAEE0
void __fastcall SUNIT_RemoveUnit(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBB160
UnitAny* __fastcall SUNIT_GetNextUnitFromList(UnitAny* pUnit);
//D2Game.0x6FCBB190
void __fastcall sub_6FCBB190(Game* pGame, UnitAny* pUnit, Room1* pRoom);
//D2Game.0x6FCBB440
void __fastcall SUNIT_WarpPlayer(Game* pGame, UnitAny* pTarget, UnitAny* pRoomTile);
//D2Game.0x6FCBB630
void __fastcall SUNIT_InitSeed(UnitAny* pUnit, Seed* pSeed);
//D2Game.0x6FCBB6C0
UnitAny* __fastcall SUNIT_AllocUnitData(int32_t nUnitType, int32_t nClassId, int32_t nX, int32_t nY, Game* pGame, Room1* pRoom, char a7, int32_t nMode, DWORD a3);
//D2Game.0x6FCBBB00
UnitAny* __fastcall SUNIT_GetServerUnit(Game* pGame, int32_t nUnitType, int32_t nUnitGUID);
//D2Game.0x6FCBBB70
UnitAny* __fastcall SUNIT_GetOwner(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBBC00
UnitAny* __fastcall SUNIT_GetLastAttacker(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBBCB0
void __fastcall SUNIT_Add(UnitAny* pUnit, int32_t nX, int32_t nY, Game* pGame, Room1* pRoom, int32_t a6);
//D2Game.0x6FCBBFE0
void __fastcall SUNIT_Restore(Game* pGame, UnitAny* pUnit, Room1* pRoom, int32_t nX, int32_t nY);
//D2Game.0x6FCBC280
void __fastcall SUNIT_InitClientInPlayerData(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FCBC2E0
GameClient* __fastcall SUNIT_GetClientFromPlayer(UnitAny* pPlayer, const char* szFile, int32_t nLine);
//D2Game.0x6FCBC300
void __fastcall sub_6FCBC300(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBC480
void __fastcall SUNIT_AttachSound(UnitAny* pUnit, int32_t nUpdateType, UnitAny* pUpdateUnit);
//D2Game.0x6FCBC4D0
void __fastcall sub_6FCBC4D0(UnitAny* pUnit);
//D2Game.0x6FCBC590
UnitAny* __fastcall SUNIT_CreatePresetUnit(Game* pGame, Room1* pRoom, int32_t nUnitType, int32_t nClassId, int32_t nX, int32_t nY, int32_t nMode, int32_t nUnitFlags);
//D2Game.0x6FCBC6F0
void __fastcall SUNIT_SpawnPresetUnit(Game* pGame, Room1* pRoom, PresetUnit* pPresetUnit);
//D2Game.0x6FCBC780
void __fastcall SUNIT_SpawnPresetUnitsInRoom(Game* pGame, Room1* pRoom);
//D2Game.0x6FCBC7E0
void __fastcall sub_6FCBC7E0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBC900
UnitAny* __stdcall SUNIT_GetTargetUnit(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBC930
int32_t __fastcall sub_6FCBC930(Game* pGame, UnitAny* pMissile);
//D2Game.0x6FCBC9A0
void __fastcall SUNIT_SetCombatMode(Game* pGame, UnitAny* pUnit, int32_t nMode);
//D2Game.0x6FCBC9C0
void __fastcall sub_6FCBC9C0(UnitAny* pFirst, UnitAny* pSecond);
//D2Game.0x6FCBCB30
UnitAny* __fastcall SUNIT_GetPortalOwner(Game* pGame, UnitAny* pPortal);
//D2Game.0x6FCBCC40
void __fastcall SUNIT_IterateUnitsOfType(Game* pGame, int32_t nType, void* pArg, int32_t(__fastcall* pfIterate)(Game*, UnitAny*, void*));
//D2Game.0x6FCBCD70
void __fastcall SUNIT_IterateLivingPlayers(Game* pGame, void(__fastcall* pfIterate)(Game*, UnitAny*, void*), void* pArg);
//D2Game.0x6FCBCE30
void __fastcall SUNIT_SetTimerOnUnit(UnitAny* pUnit, EventTimer* pTimer);
//D2Game.0x6FCBCE40
EventTimer* __fastcall SUNIT_GetTimerFromUnit(UnitAny* pUnit);
//D2Game.0x6FCBCE50
void __fastcall D2GAME_DeletePlayerPerFrameEvents_6FCBCE50(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBCE70
void __fastcall sub_6FCBCE70(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBCFD0
void __fastcall sub_6FCBCFD0(Game* pGame, UnitAny* pUnit, int32_t a3);
//D2Game.0x6FCBD120
void __fastcall sub_6FCBD120(Game* pGame, UnitAny* pUnit, int32_t a3);
//D2Game.0x6FCBD260
void __fastcall D2GAME_SKILLS_RewindSkillEx_6FCBD260(Game* pGame, UnitAny* pUnit, int32_t a3);
//D2Game.0x6FCBD3A0
void __fastcall sub_6FCBD3A0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBD3D0
void __fastcall SUNIT_FillUnitInfo(UnitAny* pUnit, UnitInfo* pInfo);
//D2Game.0x6FCBD4D0
UnitAny* __fastcall SUNIT_GetInteractUnit(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBD550
void __fastcall SUNIT_RemoveAllItemsFromInventory(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCBD730
Game* __fastcall SUNIT_GetGameFromUnit(UnitAny* pUnit);
//D2Game.0x6FCBD760
void __fastcall D2GAME_SetNecropetFlag_6FCBD760(UnitAny* pUnit, int32_t nFlag);
//D2Game.0x6FCBD790
uint32_t __fastcall D2GAME_GetNecropetFlag_6FCBD790(UnitAny* pUnit);
//D2Game.0x6FCBD7C0
void __fastcall D2GAME_SetSparkChest_6FCBD7C0(UnitAny* pUnit, uint8_t nSparkChest);
//D2Game.0x6FCBD7F0
uint8_t __fastcall D2GAME_CheckIfSparklyChest_6FCBD7F0(UnitAny* pUnit);
//D2Game.0x6FCBD820
int32_t __stdcall SUNIT_GetInteractInfo(UnitAny* pUnit, int32_t* pInteractUnitType, int32_t* pInteractUnitGUID);
//D2Game.0x6FCBD840
void __stdcall SUNIT_SetInteractInfo(UnitAny* pUnit, int32_t nInteractUnitType, int32_t nInteractUnitGUID);
//D2Game.0x6FCBD890
void __stdcall SUNIT_ResetInteractInfo(UnitAny* pUnit);
//D2Game.0x6FCBD8B0
int32_t __fastcall SUNIT_IsDead(UnitAny* pUnit);
//D2Game.0x6FCBD900
int32_t __fastcall sub_6FCBD900(Game* pGame, UnitAny* pUnit, UnitAny* pTarget);
//D2Game.0x6FCBDA10
UnitAny** __fastcall SUNIT_GetUnitList(int32_t nUnitType, Game* pGame, int32_t nUnitGUID);
//D2Game.0x6FCBDA40
int32_t __fastcall SUNIT_CanPetBeTargetedBySkill(Game* pGame, UnitAny* pOwner, UnitAny* pPet, int32_t nSkillId);
//D2Game.0x6FCBDAD0
int32_t __fastcall SUNIT_CanAllyBeTargetedBySkill(Game* pGame, UnitAny* pOwner, UnitAny* pPet, int32_t nSkillId);
//D2Game.0x6FCBDC60
int32_t __fastcall SUNIT_AreUnitsAligned(Game* pGame, UnitAny* pUnit1, UnitAny* pUnit);
//D2Game.0x6FCBDD30
void __fastcall sub_6FCBDD30(UnitAny* pUnit, uint8_t nAlignNew, int32_t a3);
//D2Game.0x6FCBDE90
void __fastcall sub_6FCBDE90(UnitAny* pUnit, int32_t bSetUninterruptable);
//D2Game.0x6FCBDF90
int32_t __fastcall sub_6FCBDF90(Game* pGame, UnitAny* pPlayer, int32_t a3, int32_t a4, int32_t a5);
//D2Game.0x6FCBDFE0
int32_t __fastcall sub_6FCBDFE0(Game* pGame, UnitAny* pUnit, Room1* pInputRoom, int32_t nX, int32_t nY, int32_t a6, int32_t a7);
//D2Game.0x6FCBE2D0
int16_t __fastcall SUNIT_GetPartyId(UnitAny* pPlayer);
