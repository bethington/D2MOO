#pragma once

#include <D2StatList.h>
#include <Units/Units.h>
#include <Units/UnitFinds.h>
#include <UNIT/SUnitDmg.h>
#include <UNIT/SUnitEvent.h>
#include <AI/AiStates.h>

struct ItemStatCostTxt;
struct MissilesTxt;
struct PetTypeTxt;
struct StatesTxt;

using CurseCallbackFunc = void(__fastcall*)(Game*, UnitAny*, UnitAny*, int32_t);


using SkillStartFunc = int32_t(__fastcall*)(Game*, UnitAny*, int32_t, int32_t);
using SkillDoFunc = int32_t(__fastcall*)(Game*, UnitAny*, int32_t, int32_t);
extern const SkillDoFunc gpSkillSrvDoFnTable_6FD40A20[];
extern const int32_t gnSkillSrvDoFns;

#pragma pack(push, 1)
struct AuraCallback
{
	Game* pGame;  //+00
	UnitAny* pOwner; //+04
	int32_t nCounter;       //+08
	void* pArgs;       //+0C
};

struct StaticFieldCallbackArg
{
	int32_t nMinDamage;       //+00
	int32_t nDamagePct;       //+04
	int32_t nStaticFieldMin;  //+08
	int32_t nElementalLength; //+0C
	int32_t nElementalType;   //+10
};

struct UnkNecSkill
{
	Game* pGame;
	UnitAny* pUnit;
	int32_t unk0x08;
	int32_t bUpdateAnimRate;
	int32_t nSkillId;
	int32_t nSkillLevel;
	int32_t nDuration;
	int32_t nAuraStat[6];
	int32_t nAuraStatCalc[6];
	int32_t nAuraTargetState;
	int32_t nAuraEvent[3];
	int32_t nAuraEventFunc[3];
};

struct Unk6FD106F0
{
	UnitAny* pUnit;
	int32_t nDistance;
	int32_t nX;
	int32_t nY;
	int32_t(__fastcall* pCallback)(Game*, UnitAny*);
};

struct Unk6FD10880
{
	UnitAny* unk0x00;
	int32_t unk0x04;
	UnitAny* unk0x08;
	int32_t unk0x0C;
	int32_t unk0x10;
	int32_t unk0x14;
};

struct Effect
{
	uint32_t nFlags;
	UnitAny* pUnit;
	int32_t nSkillId;
	int32_t nX;
	int32_t nY;
	int32_t nParam;
	int32_t nSkillLevel;
	int32_t unk0x1C;
	void* pfAreaEffect;
};

struct MonSkillInfo
{
	int32_t nMinDamage;
	int32_t nMaxDamage;
	int32_t nToHit;
};

struct Curse
{
	UnitAny* pUnit;								//0x00
	UnitAny* pTarget;							//0x04
	int32_t nSkill;									//0x08
	int32_t nSkillLevel;							//0x0C
	int32_t nDuration;								//0x10
	int32_t nStat;									//0x14
	int32_t nStatValue;								//0x18
	int32_t nState;									//0x1C
	StatListRemoveCallback pStateRemoveCallback;	//0x20
};

struct SummonArg
{
	uint32_t dwFlags;						//0x00
	UnitAny* pOwner;						//0x04
	int32_t nHcIdx;							//0x08
	AiSpecialState nAiSpecialState;		//0x0C
	int32_t nMonMode;						//0x10
	Coord pPosition;					//0x14		
	int32_t nPetType;						//0x1C
	int32_t nPetMax;						//0x20
};

#pragma pack(pop)


//D2Game.0x6FD0F8B0
int32_t __fastcall sub_6FD0F8B0(Game* pGame, UnitAny* pUnit, int32_t nValue);
//D2Game.0x6FD0FA00
int32_t __fastcall sub_6FD0FA00(UnitAny* pUnit, UnitAny* pTarget, uint32_t nAuraFilter);
//D2Game.0x6FD0FDD0
void __fastcall sub_6FD0FDD0(UnitAny* pUnit);
//D2Game.0x6FD0FE50
void __fastcall sub_6FD0FE50(Game* pGame, UnitAny* pUnit, int32_t(__fastcall* pCallback)(AuraCallback*, UnitAny*), void* pArgs);
//D2Game.0x6FD0FE80
void __fastcall sub_6FD0FE80(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY, int32_t nAuraRange, uint32_t nAuraFilter, int32_t(__fastcall* pCallback)(AuraCallback*, UnitAny*), void* pCallbackArgs, int32_t bCheckMonAuraFlag, const char* szFile, int32_t nLine);
//D2Game.0x6FD10140
void __fastcall sub_6FD10140(Game* pGame, UnitAny* pAttackerArg, UnitAny* pUnit, Damage* pDamage, int32_t nUnused);
//D2Game.0x6FD10200
int32_t __fastcall sub_6FD10200(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY, int32_t nAuraRange, Damage* pDamage, uint32_t nAuraFilter);
//D2Game.0x6FD10250
void __fastcall sub_6FD10250(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nState, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD10360
int32_t __fastcall sub_6FD10360(Game* pGame, uint32_t nFlags, UnitAny* pUnit, int32_t nDistance, int32_t(__fastcall* pCallback)(UnitAny*, void*), void* pArgs);
//D2Game.0x6FD10630
UnitAny* __fastcall SKILLS_FindAuraTarget(Game* pGame, UnitAny* pUnit, int32_t nAuraRange, int32_t(__fastcall* pCallback)(Game*, UnitAny*));
//D2Game.0x6FD106F0
int32_t __fastcall sub_6FD106F0(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FD10790
int32_t __fastcall sub_6FD10790(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY, int32_t nAuraRange, uint32_t nAuraFilter);
//D2Game.0x6FD107E0
int32_t __fastcall sub_6FD107E0(AuraCallback* pAuraCallback, UnitAny* pUnit);
//D2Game.0x6FD107F0
UnitAny* __fastcall sub_6FD107F0(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY, int32_t nAuraRange, uint32_t nAuraFilter, int32_t a7, int32_t* a8);
//D2Game.0x6FD10880
int32_t __fastcall sub_6FD10880(AuraCallback* pAuraCallback, UnitAny* pUnit);

PetTypeTxt* __fastcall DATATBLS_GetPetTypeTxtRecord(int32_t nPetType);
//D2Game.0x6FD108D0
void __fastcall sub_6FD108D0(Game* pGame, UnitAny* pUnit, int32_t nSkillId);
//D2Game.0x6FD10A50
int32_t __fastcall D2GAME_SKILLMANA_Consume_6FD10A50(Game* pGame, UnitAny* pPlayer, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD10C90
int32_t __fastcall D2GAME_SKILLMANA_AuraConsume_6FD10C90(UnitAny* pUnit, int32_t nManaCost);
//D2Game.0x6FD10CE0
int32_t __fastcall sub_6FD10CE0(UnitAny* pUnit);
//D2Game.0x6FD10D90
UnitFindData* __fastcall sub_6FD10D90(Game* pGame, UnitAny* pUnit, int32_t nSize, int32_t nX, int32_t nY);
//D2Game.0x6FD10E20
void __fastcall sub_6FD10E20(Game* pGame, UnitFindData* pUnitFindData);
//D2Game.0x6FD10E50
void __fastcall sub_6FD10E50(UnitAny* pUnit, int32_t nState, StatList* pStatList);
//D2Game.0x6FD10EC0
StatList* __fastcall sub_6FD10EC0(Curse* pCurse);
//D2Game.0x6FD11260
StatList* __fastcall sub_6FD11260(UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillId, int32_t nSkillLevel, int32_t nStatId, int32_t nState, StatListRemoveCallback pfRemoveStatCallback);
//D2Game.0x6FD11340
int32_t __fastcall sub_6FD11340(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FD11420
UnitAny* __fastcall sub_6FD11420(Game* pGame, int32_t nMissileId, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t nX, int32_t nY, int32_t nTargetX, int32_t nTargetY, int32_t bCheckItemQuantity);
//D2Game.0x6FD115E0
UnitAny* __fastcall D2GAME_CreateMissile_6FD115E0(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t nMissileId, int32_t nX, int32_t nY);
//D2Game.0x6FD11710
UnitAny* __fastcall sub_6FD11710(Game* pGame, int32_t nMissileId, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t nX, int32_t nY, int32_t nTargetX, int32_t nTargetY, int32_t bSkipCheck);
//D2Game.0x6FD118C0
int32_t __fastcall sub_6FD118C0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FD119C0
int32_t __fastcall sub_6FD119C0(UnitAny* pUnit);
//D2Game.0x6FD11AE0
void __fastcall sub_6FD11AE0(Game* pGame, UnitAny* pUnit, UnitAny* pItem);
//D2Game.0x6FD11B50
int32_t __fastcall D2GAME_CheckSkillIfUnitsClassSkill_6FD11B50(UnitAny* pUnit, int32_t nSkillId);
//D2Game.0x6FD11BA0
void __fastcall sub_6FD11BA0(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD11C00
void __fastcall D2GAME_SKILLS_SetDelay_6FD11C00(Game* pGame, UnitAny* pUnit, int32_t nDelay);
//D2Game.0x6FD11C90
int32_t __fastcall sub_6FD11C90(UnitAny* pUnit, int32_t nStateId, int32_t a3);
//D2Game.0x6FD11D90
void __fastcall sub_6FD11D90(UnitAny* pUnit, Damage* pDamage, int32_t nLength, int32_t nSkillId);
//D2Game.0x6FD11E40
void __fastcall sub_6FD11E40(UnitAny* pUnit, Damage* pDamage, int32_t nElementalType, int32_t nDamage, int32_t nLength, int32_t* pResistStatId, int32_t* pElementalType);
//D2Game.0x6FD11FF0
int32_t __fastcall SKILLS_SrvSt01_Attack_LeftHandSwing(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD12060
int32_t __fastcall SKILLS_SrvSt65_Throw_LeftHandThrow(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD120A0
int32_t __fastcall SKILLS_SrvSt02_Kick(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD12150
int32_t __fastcall SKILLS_SrvSt03_Unsummon(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD12210
int32_t __fastcall SKILLS_SrvDo001_Attack_LeftHandSwing(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD123D0
int32_t __fastcall SKILLS_SrvDo002_Kick_PowerStrike_MonIceSpear_Impale_Bash_Stun_Concentrate_BearSmite_Vengeance_Berserk_FireClaws(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD12670
int32_t __fastcall SKILLS_SrvDo004_Unsummon(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD126B0
int32_t __fastcall SKILLS_SrvDo003_Throw(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD12800
int32_t __fastcall SKILLS_SrvDo005_LeftHandThrow(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD12950
int32_t __fastcall sub_6FD12950(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t a5);
//D2Game.0x6FD12BA0
int32_t __fastcall D2GAME_SKILLS_Handler_6FD12BA0(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t a5, int32_t a6, int32_t a7);
//D2Game.0x6FD12FD0
int32_t __fastcall sub_6FD12FD0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FD13220
int32_t __fastcall SKILL_ComputePeriodicRate(Game* pGame, UnitAny* pUnit, SkillsTxt* pSkillsTxtRecord, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD13260
int32_t __fastcall sub_6FD13260(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t a5);
//D2Game.0x6FD13330
int32_t __fastcall sub_6FD13330(Game* pGame, UnitAny* pUnit, int32_t a5, int32_t nSkillId, int32_t nSkillLevel, int32_t a6);
//D2Game.0x6FD13410
void __fastcall sub_6FD13410(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FD13470
void __fastcall D2GAME_MONSTERS_AiFunction09_6FD13470(Game* pGame, UnitAny* pUnit, int32_t nSkillIdArg, int32_t a4);
//D2Game.0x6FD13610
void __fastcall D2GAME_MONSTERS_AiFunction10_6FD13610(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t nSkillId);
//D2Game.0x6FD136E0
int32_t __fastcall sub_6FD136E0(UnitAny* pUnit, int32_t nSkillId);
//D2Game.0x6FD13800
void __fastcall D2GAME_AssignSkill_6FD13800(UnitAny* pUnit, int32_t a2, int32_t nSkillId, int32_t nFlags);
//D2Game.0x6FD13B20
UnitAny* __fastcall D2GAME_CreateLinkPortal_6FD13B20(Game* pGame, UnitAny* pPortal, int32_t nDestLevel, int32_t nSourceLevel);
//D2Game.0x6FD13DF0
int32_t __fastcall D2GAME_CreatePortalObject_6FD13DF0(Game* pGame, UnitAny* pUnit, Room1* pRoom, int32_t nX, int32_t nY, int32_t nDestLevel, UnitAny** ppSourceUnit, int32_t nObjectId, int32_t bPerm);
//D2Game.0x6FD14020
int32_t __fastcall D2GAME_GetXAndYFromTargetUnit_6FD14020(Game* pGame, UnitAny* pUnit, int32_t* pX, int32_t* pY);
//D2Game.0x6FD140D0
void __fastcall sub_6FD140D0(int32_t* pX, int32_t* pY);
//D2Game.0x6FD14120
void __fastcall sub_6FD14120(int32_t* pX, int32_t* pY);
//D2Game.0x6FD14170
void __fastcall sub_6FD14170(Game* pGame, UnitAny* pOwner, UnitAny* a3, int32_t nMissileId, int32_t nSkillId, int32_t nSkillLevel, int32_t nVelocity);
//D2Game.0x6FD14260
void __fastcall sub_6FD14260(Game* pGame, int32_t nMissiles, Missile* missileParams);
//D2Game.0x6FD14370
void __fastcall D2GAME_MONSTERS_AiFunction06_6FD14370(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD14400
void __fastcall sub_6FD14400(Game* pGame, UnitAny* pUnit, int32_t nDoFunc, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD14430
UnitAny* __fastcall D2GAME_SummonPet_6FD14430(Game* pGame, SummonArg* pSummonArg);
//D2Game.0x6FD146D0
void __fastcall sub_6FD146D0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FD14770
void __fastcall sub_6FD14770(Game* pGame, int32_t nFlags, UnitAny* pUnit, int32_t nSkillLevel, int32_t nRange, CurseCallbackFunc pfCurse);
//D2Game.0x6FD149B0
void __fastcall sub_6FD149B0(Game* pGame, Effect* pEffect);
//D2Game.0x6FD14BD0
void __fastcall sub_6FD14BD0(Game* pGame, int32_t nFlags, int32_t nX, int32_t nY, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, void* pfAreaEffect);
//D2Game.0x6FD14C30
void __fastcall sub_6FD14C30(UnitAny* pUnit);
//D2Game.0x6FD14C60
void __fastcall D2GAME_SetSkills_6FD14C60(UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t bRemove);
//D2Game.0x6FD14CC0
void __fastcall D2GAME_RefreshPassiveSkills_6FD14CC0(UnitAny* pUnit);
//D2Game.0x6FD14D20
void __fastcall sub_6FD14D20(UnitAny* pUnit, int32_t nUnused, MonSkillInfo* pMonSkillInfo);
//D2Game.0x6FD14DD0
int32_t __fastcall D2GAME_RollElementalDamage_6FD14DD0(UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD14EC0
void __fastcall D2GAME_RollPhysicalDamage_6FD14EC0(UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD14F70
void __fastcall sub_6FD14F70(Game* pGame, UnitAny* pUnused, UnitAny* pUnit, int32_t nDelay);
//D2Game.0x6FD15080
void __fastcall sub_6FD15080(UnitAny* pUnit, int32_t a2);
//D2Game.0x6FD150A0
int32_t __fastcall sub_6FD150A0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget);
//D2Game.0x6FD15190
int32_t __fastcall sub_6FD15190(UnitAny* pUnit, AiSpecialState nAiSpecialState);
//D2Game.0x6FD15210
UnitAny* __fastcall sub_6FD15210(UnitAny* pUnit, UnitAny* pTarget, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD15320
int32_t __fastcall sub_6FD15320(UnitAny* pUnit, UnitFindArg* pArg);
//D2Game.0x6FD15340
int32_t __fastcall sub_6FD15340(Game* pGame, UnitAny* pUnit, int32_t nMissileId);
//D2Game.0x6FD15460
int32_t __fastcall sub_6FD15460(UnitAny* pUnit);
//D2Game.0x6FD15470
void __fastcall sub_6FD15470(UnitAny* pUnit, int32_t nAttackRate);
//D2Game.0x6FD154D0
void __fastcall sub_6FD154D0(UnitAny* pUnit);
//D2Game.0x6FD15500
int32_t __fastcall sub_6FD15500(UnitAny* pUnit);
//D2Game.0x6FD15580
int32_t __fastcall D2GAME_GetSummonIdFromSkill_6FD15580(UnitAny* pUnit, int32_t bFromMonster, int32_t nSkillId, int32_t nSkillLevel, int32_t* pSpawnMode, int32_t* pX, int32_t* pY);
//D2Game.0x6FD155E0
void __fastcall sub_6FD155E0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, SkillsTxt* pSkillsTxtRecord, int32_t nSkillId, int32_t nSkillLevel, Damage* pDamage, int32_t a8);
//D2Game.0x6FD15650
void __fastcall sub_6FD15650(Game* pGame, UnitAny* pOwner, UnitAny* pUnit, uint16_t wResultFlags, int32_t a5, Damage* pDamage, int32_t a7);
//D2Game.0x6FD156A0
UnitEvent* __fastcall sub_6FD156A0(Game* pGame, UnitAny* pUnit, UnitEventTypes nUnitEventType, int32_t a4, int32_t a5, int32_t nEventFunc, int32_t a7, int32_t a8);

//D2Game.0x6FC808D0
int32_t __fastcall SKILLS_SrvSt18_Attract(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);

StatesTxt* __fastcall SKILLS_GetStatesTxtRecord(int32_t nStateId);
ItemStatCostTxt* __fastcall SKILLS_GetItemStatCostTxtRecord(int32_t nStatId);
MissilesTxt* __fastcall SKILLS_GetMissilesTxtRecord(int32_t nMissileId);
