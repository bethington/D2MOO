#pragma once

#include <Units/Units.h>

struct DrlgCoords;

#pragma pack(1)

struct ObjInitFn
{
	Game* pGame;						//0x00
	UnitAny* pObject;					//0x04
	Room1* pRoom;						//0x08
	ObjectControl* pObjectregion;		//0x0C
	ObjectsTxt* pObjectTxt;				//0x10
	int32_t nX;								//0x14
	int32_t nY;								//0x18
};
using ObjInitFunction = void(__fastcall*)(ObjInitFn*);

#pragma pack()

//D2Game.0x6FC70180
UnitAny* __fastcall OBJECTS_SpawnShrine(Game* pGame, Room1* pRoom, int32_t nClassId, int32_t nX, int32_t nY, int32_t nMode);
//D2Game.0x6FC70270
UnitAny* __fastcall OBJECTS_SpawnPresetChest(Game* pGame, Room1* pRoom, int32_t nClassId, int32_t nX, int32_t nY, int32_t nMode);
//D2Game.0x6FC70470
UnitAny* __fastcall OBJECTS_SpawnSpecialChest(Game* pGame, Room1* pRoom, int32_t nClassId, int32_t nX, int32_t nY, int32_t nMode);
//D2Game.0x6FC70520
UnitAny* __fastcall OBJECTS_SpawnArcaneSymbol(Game* pGame, Room1* pRoom, int32_t nClassId, int32_t nX, int32_t nY, int32_t nMode);
//D2Game.0x6FC70570
UnitAny* __fastcall OBJECTS_SpawnPresetObject(Game* pGame, Room1* pRoom, int32_t nClassId, int32_t nX, int32_t nY, int32_t nMode);
//D2Game.0x6FC70600
void __fastcall OBJECTS_InitHandler(Game* pGame, UnitAny* pObject, int32_t nUnitId, Room1* pRoom, int32_t nX, int32_t nY);
//D2Game.0x6FC70850
void __fastcall OBJECTS_InitFunction10_Unused(ObjInitFn* pOp);
//D2Game.0x6FC708B0
void __fastcall OBJECTS_InitFunction28_GoldPlaceHolder(ObjInitFn* pOp);
//D2Game.0x6FC70A50
void __fastcall OBJECTS_InitFunction01_Shrine(ObjInitFn* pOp);
//D2Game.0x6FC70BA0
int32_t __fastcall OBJECTS_GetShrineId(Game* pGame, uint8_t nShrineClass, int32_t nLevelId, Room1* pRoom);
//D2Game.0x6FC70CA0
void __fastcall OBJECTS_InitFunction22_Fire(ObjInitFn* pOp);
//D2Game.0x6FC70CF0
void __fastcall OBJECTS_InitFunction16_Well(ObjInitFn* pOp);
//D2Game.0x6FC70D10
void __fastcall OBJECTS_InitFunction24_SpikeFloorTrap(ObjInitFn* pOp);
//D2Game.0x6FC70D30
void __fastcall OBJECTS_InitFunction02_Urn(ObjInitFn* pOp);
//D2Game.0x6FC70DC0
void __fastcall OBJECTS_InitFunction27_GooPile(ObjInitFn* pOp);
//D2Game.0x6FC70E10
void __fastcall OBJECTS_InitFunction03_Chest(ObjInitFn* pOp);
//D2Game.0x6FC70F30
void __fastcall OBJECTS_InitFunction57_SparklyChest(ObjInitFn* pOp);
//D2Game.0x6FC71060
void __fastcall OBJECTS_InitFunction58_Fissure(ObjInitFn* pOp);
//D2Game.0x6FC710B0
void __fastcall OBJECTS_InitFunction59_VileDogAfterglow(ObjInitFn* pOp);
//D2Game.0x6FC71110
void __fastcall OBJECTS_InitFunction12_PermanentPortal(ObjInitFn* pOp);
//D2Game.0x6FC71360
void __fastcall OBJECTS_InitFunction08_Torch(ObjInitFn* pOp);
//D2Game.0x6FC71370
void __fastcall OBJECTS_InitFunction14_Brazier(ObjInitFn* pOp);
//D2Game.0x6FC71380
void __fastcall OBJECTS_InitFunction34_HellBrazierFire(ObjInitFn* pOp);
//D2Game.0x6FC713D0
void __fastcall OBJECTS_InitFunction11_Portal(ObjInitFn* pOp);
//D2Game.0x6FC71470
void __fastcall OBJECTS_InitFunction51_TrappedSoul(ObjInitFn* pOp);
//D2Game.0x6FC714A0
void __fastcall OBJECTS_InitFunction46_TrappedSoulPlaceHolder(ObjInitFn* pOp);
//D2Game.0x6FC718C0
UnitAny* __fastcall OBJECTS_PopulateFn1_CasketJarSarcophagusUrn(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nProbability);
//D2Game.0x6FC71F60
int32_t __cdecl sub_6FC71F60();
//D2Game.0x6FC71FB0
UnitAny* __fastcall OBJECTS_PopulateFn7_RogueOnStick(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nProbability);
//D2Game.0x6FC72340
void __fastcall OBJECTS_SpawnFliesOnCorpse(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC723F0
UnitAny* __fastcall OBJECTS_PopulateFn3_CommonObjects(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nChance);
//D2Game.0x6FC72510
UnitAny* __fastcall OBJECTS_CreateObject(Game* pGame, int32_t nClassId, int32_t nSizeX, int32_t nSizeY, Room1* pRoom);
//D2Game.0x6FC726D0
UnitAny* __fastcall OBJECTS_PopulateFn9_TrappedSoul(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nProbability);
//D2Game.0x6FC727F0
UnitAny* __fastcall OBJECTS_PopulateFn6_RogueGuardCorpse(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nProbability);
//D2Game.0x6FC728C0
UnitAny* __fastcall OBJECTS_PopulateFn8_Well(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nProbability);
//D2Game.0x6FC72C30
UnitAny* __fastcall OBJECTS_PopulateFn2_WaypointShrine(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nProbability);
//D2Game.0x6FC73050
int32_t __fastcall OBJECTS_RollRandomNumber(Seed* pSeed, int32_t nMin, int32_t nMax);
//D2Game.0x6FC730C0
UnitAny* __fastcall OBJECTS_PopulateFn4_Barrel(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nProbability);
//D2Game.0x6FC73550
UnitAny* __fastcall OBJECTS_PopulateFn5_Crate(Game* pGame, Room1* pRoom, uint8_t nDensity, uint32_t nObjectId, uint16_t nProbability);
//D2Game.0x6FC73A70
int32_t __fastcall OBJECTS_SpawnNothing(Game* pGame, Room1* pRoom, DrlgCoords* pDrlgCoords);
//D2Game.0x6FC73A80
int32_t __fastcall OBJECTS_SpawnBarrel(Game* pGame, Room1* pRoom, DrlgCoords* pDrlgCoords);
//D2Game.0x6FC73C50
int32_t __fastcall OBJECTS_SpawnNothing2(Game* pGame, Room1* pRoom, DrlgCoords* pDrlgCoords);
//D2Game.0x6FC73D80
int32_t __fastcall OBJECTS_SpawnArmorStand(Game* pGame, Room1* pRoom, DrlgCoords* pDrlgCoords);
//D2Game.0x6FC73DA0
void __fastcall OBJECTS_SpawnArmorStandOrWeaponRack(Game* pGame, Room1* pRoom, DrlgCoords* pDrlgCoords, int32_t bWeaponRack);
//D2Game.0x6FC740C0
int32_t __fastcall OBJECTS_SpawnWeaponRack(Game* pGame, Room1* pRoom, DrlgCoords* pDrlgCoords);
//D2Game.0x6FC740E0
void __fastcall OBJECTS_PopulationHandler(Game* pGame, Room1* pRoom);
//D2Game.0x6FC744B0
void __fastcall OBJECTS_FreeHoverMessage(Game* pGame, UnitAny* pObject);
//D2Game.0x6FC74520
void __fastcall OBJECTS_RemoveAll(Game* pGame);
//D2Game.0x6FC74590
void __fastcall OBJECTS_SetUnitIdInTimerArg(UnitAny* pPortal, int32_t nUnitId);
//D2Game.0x6FC745B0
int32_t __fastcall OBJECTS_GetUnitIdFromTimerArg(UnitAny* pUnit);
