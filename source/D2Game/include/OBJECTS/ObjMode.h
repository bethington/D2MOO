#pragma once

#include <Units/Units.h>
#include <Units/UnitFinds.h>

#pragma pack(1)

using ObeliskPowerUpFunction = int32_t(__fastcall*)(Game*, UnitAny*, int32_t);

struct ObeliskPowerUp
{
	ObeliskPowerUpFunction pPowerUpCallback;//0x00
	uint32_t nChance;						//0x04
	int32_t nValue;							//0x08
};


struct ObjOperateFn
{
	Game* pGame;						//0x00
	UnitAny* pObject;					//0x04
	UnitAny* pPlayer;					//0x08
	ObjectControl* pObjectregion;		//0x0C
	int32_t nObjectIdx;						//0x10
};
using ObjOperateFunction = int32_t(__fastcall*)(ObjOperateFn*, int32_t);

using ObjShrineFunction = void(__fastcall* )(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
struct ShrineTable
{
	ObjShrineFunction pfShrineCallback;		//0x00
	int32_t unk0x04;						//0x04
	int32_t unk0x08;						//0x08
};

#pragma pack()

//D2Game.0x6FC748A0
void __fastcall sub_6FC748A0(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74A40
void __fastcall sub_6FC74A40(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74AC0
void __fastcall sub_6FC74AC0(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74B00
void __fastcall sub_6FC74B00(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74B40
void __fastcall sub_6FC74B40(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74CA0
void __fastcall sub_6FC74CA0(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74D10
void __fastcall sub_6FC74D10(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74DF0
void __fastcall sub_6FC74DF0(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74F60
void __fastcall D2GAME_SpikeTraps_6FC74F60(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC750D0
void __fastcall sub_6FC750D0(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC75250
void __fastcall D2GAME_OBJMODE_InvokeEventFunction_6FC75250(Game* pGame, UnitAny* pObject, int32_t nEventType);
//D2Game.0x6FC752A0
void __fastcall sub_6FC752A0(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FC75350
void __fastcall sub_6FC75350(Game* pGame, UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FC753E0
int32_t __fastcall OBJECTS_OperateFunction27_TeleportPad(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC75530
int32_t __fastcall OBJECTS_OperateFunction30_ExplodingChest(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC755A0
int32_t __fastcall OBJECTS_OperateFunction08_Door(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC75730
int32_t __fastcall OBJECTS_OperateFunction16_TrapDoor(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC757E0
int32_t __fastcall OBJECTS_OperateFunction47_Stair(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC758F0
int32_t __fastcall OBJECTS_OperateFunction61_HarrogathMainGate(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC759F0
int32_t __fastcall OBJECTS_OperateFunction50_Stair(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC75A10
int32_t __fastcall OBJECTS_OperateFunction29_SlimeDoor(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC75AC0
void __fastcall D2GAME_OBJECTS_TrapHandler8_9_6FC75AC0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC75B40
void __fastcall D2GAME_SpawnTrapMonster_6FC75B40(ObjOperateFn* pOp, int32_t nMonsterId, int32_t a3);
//D2Game.0x6FC75BC0
void __fastcall D2GAME_OBJECTS_TrapHandler5_7_6FC75BC0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC75C70
void __fastcall D2GAME_OBJECTS_TrapHandler1_6FC75C70(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC75D00
void __fastcall D2GAME_OBJECTS_TrapHandler2_6_6FC75D00(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC75D90
void __fastcall D2GAME_OBJECTS_TrapHandler3_6FC75D90(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC75E20
void __fastcall D2GAME_OBJECTS_TrapHandler4_6FC75E20(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC75EB0
void __fastcall sub_6FC75EB0(ObjOperateFn* pOp);
//Inlined in OBJECTS_OperateFunction04_Chest
__forceinline void __fastcall OBJECTS_ChestEnd(ObjOperateFn* pOp, int32_t nType);
//D2Game.0x6FC76030
int32_t __fastcall OBJECTS_OperateFunction04_Chest(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC764B0
void __fastcall  D2GAME_SetTrapCallback_6FC764B0(ObjOperateFn* pOp, uint8_t nTrapType);
//D2Game.0x6FC76570
int32_t __fastcall OBJECTS_OperateFunction17_Obelisk(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC766B0
void __fastcall D2GAME_SHRINES_Health_6FC766B0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC766F0
void __fastcall D2GAME_SHRINES_FillMana_6FC766F0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC76730
void __fastcall D2GAME_SHRINES_Refill_6FC76730(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC76790
void __fastcall D2GAME_SHRINES_ExchangeHealth_6FC76790(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC767F0
void __fastcall D2GAME_SHRINES_ExchangeMana_6FC767F0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC76850
void __fastcall D2GAME_SHRINES_Enirhs_6FC76850(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC76880
void __fastcall D2GAME_SHRINES_Portal_6FC76880(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC76910
void __fastcall D2GAME_SHRINES_Gem_6FC76910(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC76A60
int32_t __fastcall sub_6FC76A60(Game* pGame, UnitAny* pPlayer, int32_t nItemId, UnitAny* pItem);
//D2Game.0x6FC76BC0
void __fastcall D2GAME_SHRINES_Storm_6FC76BC0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC76E90
int32_t __fastcall sub_6FC76E90(UnitAny* pUnit, UnitFindArg* pArg);
//D2Game.0x6FC76ED0
void __fastcall D2GAME_SHRINES_Monster_6FC76ED0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC76F60
int32_t __fastcall sub_6FC76F60(UnitAny* pMonster, UnitAny* pUnit1);
//D2Game.0x6FC770D0
void __fastcall D2GAME_SHRINES_Exploding_6FC770D0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC773B0
void __fastcall D2GAME_SHRINES_Poison_6FC773B0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC77690
void __fastcall D2GAME_SHRINES_CombatBoost_6FC77690(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC77750
int32_t __fastcall sub_6FC77750(ShrinesTxt* pShrinesTxtRecord, int32_t nStatId, int32_t nValue, UnitAny* pUnit);
//D2Game.0x6FC779C0
void __fastcall D2GAME_SHRINES_Stamina_6FC779C0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC77AB0
void __fastcall sub_6FC77AB0(UnitAny* pUnit, int32_t nState, struct StatList* pStatList);
//D2Game.0x6FC77AE0
void __fastcall D2GAME_SHRINES_DefensiveBoost_6FC77AE0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC77BA0
void __fastcall D2GAME_SHRINES_SkillBoost_6FC77BA0(ObjOperateFn* pOp, ShrinesTxt* pShrinesTxtRecord);
//D2Game.0x6FC77C10
void __fastcall sub_6FC77C10(UnitAny* pUnit, int32_t nState, struct StatList* pStatList);
//D2Game.0x6FC77C30
int32_t __fastcall OBJECTS_OperateFunction02_Shrine(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC77E80
int32_t __fastcall OBJECTS_OperateFunction01_Casket(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC77F70
int32_t __fastcall OBJECTS_OperateFunction68_EvilUrn(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78120
int32_t __fastcall OBJECTS_OperateFunction33_WirtsBody(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC781C0
int32_t __fastcall OBJECTS_OperateFunction03_Urn_Basket_Jar(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78290
int32_t __fastcall OBJECTS_OperateFunction14_Corpse(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78340
void __fastcall sub_6FC78340(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC78390
int32_t __fastcall OBJECTS_OperateFunction51_JungleStash(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78470
int32_t __fastcall OBJECTS_OperateFunction18_SecretDoor(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC784E0
int32_t __fastcall OBJECTS_OperateFunction26_BookShelf(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC785D0
int32_t __fastcall OBJECTS_OperateFunction19_ArmorStand(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78640
int32_t __fastcall OBJECTS_OperateFunction20_WeaponRack(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC786B0
int32_t __fastcall OBJECTS_OperateFunction05_Barrel(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC787F0
int32_t __fastcall OBJECTS_OperateFunction07_ExplodingBarrel(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78940
int32_t __fastcall OBJECTS_OperateFunction13_TorchTiki(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78970
int32_t __fastcall OBJECTS_OperateFunction11_Torch(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC789C0
int32_t __fastcall OBJECTS_OperateHandler(Game* pGame, UnitAny* pPlayer, int32_t nObjectType, int32_t nObjectGUID, int32_t* pResult);
//D2Game.0x6FC78B20
int32_t __fastcall sub_6FC78B20(Game* pGame, UnitAny* pUnit, int32_t nObjectType, int32_t nObjectGUID);
//D2Game.0x6FC78BB0
void __fastcall sub_6FC78BB0(Game* pGame, int32_t nUnused, int32_t nObjectGUID);
//D2Game.0x6FC78C90
int32_t __fastcall OBJECTS_OperateFunction34_ArcaneSanctuaryPortal(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78D30
int32_t __fastcall OBJECTS_OperateFunction46_HellGatePortal(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC78E00
int32_t __fastcall OBJECTS_OperateFunction15_Portal(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC79310
int32_t __fastcall OBJECTS_OperateFunction43_DurielPortal(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC79490
int32_t __fastcall OBJECTS_OperateFunction23_Waypoint(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC79600
void __fastcall D2GAME_WAYPOINT_Unk_6FC79600(Game* pGame, UnitAny* pPlayer, int32_t nWaypointGUID, int32_t nLevelId);
//D2Game.0x6FC797A0
int32_t __fastcall OBJMODE_ObeliskPowerUp_IncreaseMana(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FC797E0
int32_t __fastcall OBJMODE_ObeliskPowerUp_IncreaseEnergy(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FC79800
int32_t __fastcall OBJMODE_ObeliskPowerUp_IncreaseDexterity(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FC79820
int32_t __fastcall OBJMODE_ObeliskPowerUp_IncreaseVitality(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FC79840
int32_t __fastcall OBJMODE_ObeliskPowerUp_IncreaseStrength(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FC79860
int32_t __fastcall OBJMODE_ObeliskPowerUp_IncreaseHitpoints(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FC798A0
int32_t __fastcall OBJMODE_ObeliskPowerUp_IncreaseSkillpoints(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FC798C0
int32_t __fastcall OBJMODE_MainObeliskHandler(Game* pGame, int32_t nPlayerGUID, int32_t nObjectGUID, int32_t nItemGUID, int16_t nState);
//D2Game.0x6FC79B50
void __fastcall sub_6FC79B50(Game* pGame, UnitAny* pUnit, UnitAny* pObject);
//D2Game.0x6FC79C00
void __fastcall sub_6FC79C00(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC79C30
void __fastcall sub_6FC79C30(Game* pGame, int32_t nUnused, int32_t nPortalGUID, int32_t nDestLevel);
//D2Game.0x6FC79D90
int32_t __fastcall OBJECTS_OperateFunction22_Well(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC7A000
void __fastcall OBJMODE_PetIterate_Heal(Game* pGame, UnitAny* pUnit, UnitAny* pUnit2, void* pUnitHealed);
//D2Game.0x6FC7A070
int32_t __fastcall OBJECTS_OperateFunction48_TrappedSoul(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC7A140
UnitAny* __fastcall OBJMODE_DropItemWithCodeAndQuality(Game* pGame, UnitAny* pUnit, uint32_t dwCode, int32_t nItemQuality);
//D2Game.0x6FC7A220
UnitAny* __fastcall OBJMODE_DropFromChestTCWithQuality(ObjOperateFn* pOp, int32_t nItemQuality);
