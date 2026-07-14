#pragma once

#include "D2CommonDefinitions.h"
#include <D2Seed.h>
#include "MissileStream.h"
#include "Path/Path.h"

#pragma pack(1)
//TODO: Redo Header defs when .cpp is done

struct UnkMissileCalc
{
	int32_t field_0;
	int32_t field_4;
	int32_t field_8;
	int32_t field_C;
	int32_t field_10;
	int32_t field_14;
	int32_t field_18;
	int32_t field_1C;
	Seed pSeed;
};

struct MissileCalc
{
	UnitAny* pMissile;					//0x00
	UnitAny* pOwner;						//0x04
	int32_t nMissileId;							//0x08
	int32_t nMissileLevel;						//0x0C
};

struct MissileDamageData
{
	int32_t nFlags;								//0x00
	int32_t nMinDamage;							//0x04
	int32_t nMaxDamage;							//0x08
	int32_t nFireMinDamage;						//0x0C
	int32_t nFireMaxDamage;						//0x10
	int32_t nFireLength;						//0x14
	int32_t nLightMinDamage;					//0x18
	int32_t nLightMaxDamage;					//0x1C
	int32_t nMagicMinDamage;					//0x20
	int32_t nMagicMaxDamage;					//0x24
	int32_t nColdMinDamage;						//0x28
	int32_t nColdMaxDamage;						//0x2C
	int32_t nColdLength;						//0x30
	int32_t nPoisonMinDamage;					//0x34
	int32_t nPoisonMaxDamage;					//0x38
	int32_t nPoisonLength;						//0x3C
	int32_t nPoisonCount;						//0x40
	int32_t nLifeDrainMinDamage;				//0x44
	int32_t nLifeDrainMaxDamage;				//0x48
	int32_t nManaDrainMinDamage;				//0x4C
	int32_t nManaDrainMaxDamage;				//0x50
	int32_t nStaminaDrainMinDamage;				//0x54
	int32_t nStaminaDrainMaxDamage;				//0x58
	int32_t nStunLength;						//0x5C
	int32_t nBurningMin;						//0x60
	int32_t nBurningMax;						//0x64
	int32_t nBurnLength;						//0x68
	int32_t nDemonDamagePercent;				//0x6C
	int32_t nUndeadDamagePercent;				//0x70
	int32_t nDamageTargetAC;					//0x74
	int32_t nDamagePercent;						//0x78
};

struct MissileData
{
	uint32_t unk0x00;							//0x00 - some type of coords, see D2Common.11128, D2Common.11131 - D2Common.11134
	int16_t nStreamMissile;					//0x04
	int16_t nStreamRange;						//0x06
	int16_t nActivateFrame;					//0x08
	int16_t nSkill;							//0x0A
	int16_t nLevel;							//0x0C
	int16_t nTotalFrames;						//0x0E
	int16_t nCurrentFrame;					//0x10
	uint16_t unk0x12;							//0x12
	uint32_t fFlags;							//0x14
	int32_t nOwnerType;							//0x18
	D2UnitGUID dwOwnerGUID;						//0x1C
	int32_t nHomeType;							//0x20
	D2UnitGUID dwHomeGUID;						//0x24
	union
	{
		struct
		{
			int32_t nStatus;					//0x28 - used only by heatseeking projectiles
			PathPoint pCoords;		//0x2C - same, these are not coords, they are missile streams etc, see D2Common.11213 & D2Common.11214
		};

		Coord pTargetCoords;			//0x28
	};
	MissileStream* pStream;			//0x30
};

using MissileInitFunc = void(__fastcall*)(UnitAny*, int32_t);
struct Missile
{
	uint32_t dwFlags;							//0x00
	UnitAny* pOwner;							//0x04
	UnitAny* pOrigin;						//0x08
	UnitAny* pTarget;						//0x0C
	int32_t nMissile;							//0x10
	int32_t nX;									//0x14
	int32_t nY;									//0x18
	int32_t nTargetX;							//0x1C
	int32_t nTargetY;							//0x20
	int32_t nGfxArg;							//0x24
	int32_t nVelocity;							//0x28
	int32_t nSkill;								//0x2C
	int32_t nSkillLevel;						//0x30
	int32_t nLoops;								//0x34
	uint32_t unk0x38;							//0x38
	uint32_t unk0x3C;							//0x3C
	int32_t nFrame;								//0x40
	int32_t nActivateFrame;						//0x44
	int32_t nAttBonus;							//0x48
	int32_t nRange;								//0x4C
	int32_t nLightRadius;						//0x50
	MissileInitFunc pInitFunc;					//0x54
	int32_t pInitArgs;							//0x58
};
#pragma pack()

//D2Common.0x6FDB9F30 (#11115)
D2COMMON_DLL_DECL void __stdcall MISSILE_AllocMissileData(UnitAny* pMissile);
//D2Common.0x6FDB9F80 (#11116)
D2COMMON_DLL_DECL void __stdcall MISSILE_FreeMissileData(UnitAny* pMissile);
//D2Common.0x6FDB9FC0 (#11117)
D2COMMON_DLL_DECL uint32_t __stdcall MISSILE_GetFlags(UnitAny* pMissile);
//D2Common.0x6FDB9FE0 (#11118)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetFlags(UnitAny* pMissile, uint32_t dwFlags);
//D2Common.0x6FDBA000 (#11119)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetLevel(UnitAny* pMissile, uint16_t nLevel);
//D2Common.0x6FDBA020 (#11120)
D2COMMON_DLL_DECL uint32_t __stdcall MISSILE_GetLevel(UnitAny* pMissile);
//D2Common.0x6FDBA040 (#11126)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetSkill(UnitAny* pMissile, int nSkill);
//D2Common.0x6FDBA080 (#11127)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetSkill(UnitAny* pMissile);
//D2Common.0x6FDBA0A0 (#11121)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetTotalFrames(UnitAny* pMissile, int nTotalFrames);
//D2Common.0x6FDBA0E0 (#11122)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetTotalFrames(UnitAny* pMissile);
//D2Common.0x6FDBA100 (#11123)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetCurrentFrame(UnitAny* pMissile, int nCurrentFrame);
//D2Common.0x6FDBA140 (#11124)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetCurrentFrame(UnitAny* pMissile);
//D2Common.0x6FDBA160 (#11125)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetRemainingFrames(UnitAny* pMissile);
//D2Common.0x6FDBA190 (#11128)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetClassId(UnitAny* pMissile);
//D2Common.0x6FDBA1B0 (#11129)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetOwner(UnitAny* pMissile, UnitAny* pOwner);
//D2Common.0x6FDBA230 (#11130)
D2COMMON_DLL_DECL BOOL __stdcall MISSILE_CheckUnitIfOwner(UnitAny* pMissile, UnitAny* pUnit);
//D2Common.0x6FDBA2B0 (#11131)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetStreamMissile(UnitAny* pMissile, uint16_t nStreamMissile);
//D2Common.0x6FDBA2D0 (#11132)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetStreamMissile(UnitAny* pMissile);
//D2Common.0x6FDBA300 (#11133)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetStreamRange(UnitAny* pMissile, short nStreamRange);
//D2Common.0x6FDBA320 (#11134)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetStreamRange(UnitAny* pMissile);
//D2Common.0x6FDBA340 (#11135)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetHitClass(UnitAny* pMissile);
//D2Common.0x6FDBA390 (#11136)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetActivateFrame(UnitAny* pMissile, int nActivateFrame);
//D2Common.0x6FDBA3D0 (#11137)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetActivateFrame(UnitAny* pMissile);
//D2Common.0x6FDBA3F0 (#11138)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetAnimSpeed(UnitAny* pMissile);
//D2Common.0x6FDBA410 (#11139)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetAnimSpeed(UnitAny* pMissile, int nAnimSpeed);
//D2Common.0x6FDBA450
void __fastcall MISSILE_SetStream(UnitAny* pMissile, MissileStream* pStream);
//D2Common.0x6FDBA470
MissileStream* __fastcall MISSILE_GetStream(UnitAny* pMissile);
//D2Common.0x6FDBA490 (#11140)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetTargetX(UnitAny* pMissile, int nTargetX);
//D2Common.0x6FDBA4B0 (#11141)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetTargetX(UnitAny* pMissile);
//D2Common.0x6FDBA4D0 (#11142)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetTargetY(UnitAny* pMissile, int nTargetY);
//D2Common.0x6FDBA4F0 (#11143)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetTargetY(UnitAny* pMissile);
//D2Common.0x6FDBA510 (#11144)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetHomeType(UnitAny* pMissile, UnitAny* pTarget);
//D2Common.0x6FDBA550 (#11145)
D2COMMON_DLL_DECL void __stdcall MISSILE_GetHomeType(UnitAny* pMissile, int* nHomeType, D2UnitGUID* nHomeGUID);
//D2Common.0x6FDBA5B0 (#11217)
D2COMMON_DLL_DECL void __stdcall MISSILE_CalculateDamageData(MissileDamageData* pMissileDamageData, UnitAny* pOwner, UnitAny* pOrigin, UnitAny* pMissile, int nLevel);
//D2Common.0x6FDBADF0
BOOL __fastcall MISSILE_HasBonusStats(UnitAny* pUnit, UnitAny* pItem);
//D2Common.0x6FDBAED0
void __fastcall MISSILE_AddStatsToDamage(MissileDamageData* pMissileDamageData, UnitAny* pMissile, uint8_t nShift);
//D2Common.0x6FDBB060
void __fastcall MISSILE_CalculateFinalDamage(MissileDamageData* pMissileDamageData, int nSrcDamage);
//D2Common.0x6FDBB1B0
int __fastcall MISSILE_CalculateMasteryBonus(UnitAny* pUnit, int nElemType, int nSrcDamage);
//D2Common.0x6FDBB2E0 (#11218)
D2COMMON_DLL_DECL void __stdcall MISSILE_SetDamageStats(UnitAny* pOwner, UnitAny* pMissile, MissileDamageData* pMissileDamageData, int nLevel);
//D2Common.0x6FDBB5A0 (#11285)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetMinDamage(UnitAny* pMissile, UnitAny* pOwner, int nMissileId, int nLevel);
//D2Common.0x6FDBB710 (#11286)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetMaxDamage(UnitAny* pMissile, UnitAny* pOwner, int nMissileId, int nLevel);
//D2Common.0x6FDBB880 (#11289)
D2COMMON_DLL_DECL uint8_t __stdcall MISSILE_GetElemTypeFromMissileId(int nMissileId);
//D2Common.0x6FDBB8C0 (#11287)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetMinElemDamage(UnitAny* pMissile, UnitAny* pOwner, int nMissileId, int nLevel);
//D2Common.0x6FDBBA30 (#11288)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetMaxElemDamage(UnitAny* pMissile, UnitAny* pOwner, int nMissileId, int nLevel);
//D2Common.0x6FDBBBA0 (#11221)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetElementalLength(int nUnused, UnitAny* pMissile, int nMissileId, int nLevel);
//D2Common.0x6FDBBC50 (#11290)
D2COMMON_DLL_DECL int __stdcall MISSILE_GetSpecialParamValue(UnitAny* pMissile, UnitAny* pOwner, uint8_t nParamId, int nMissileId, int nLevel);
//D2Common.0x6FDBC060
int __fastcall MISSILE_GetCalcParamValue(int32_t nParamId, void* pUserData);
//D2Common.0x6FDBC080
int __fastcall MISSILE_GetMinimum(int a1, int a2, int a3, void* pUserData);
//D2Common.0x6FDBC090
int __fastcall MISSILE_GetMaximum(int a1, int a2, int a3, void* pUserData);
//D2Common.0x6FDBC0A0
int __fastcall MISSILE_GetRandomNumberInRange(int nMin, int nMax, int nUnused, void* pUserData);
//D2Common.0x6FDBC120
int __fastcall MISSILE_GetSpecialParamValueForSkillMissile(int nSkillId, int nParamId, int nUnused, void* pUserData);
//D2Common.0x6FDBC170 (#11284)
D2COMMON_DLL_DECL int __stdcall MISSILE_EvaluateMissileFormula(UnitAny* pMissile, UnitAny* pOwner, unsigned int nCalc, int nMissileId, int nLevel);
