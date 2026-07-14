#pragma once

#include <Units/Units.h>
#include <UNIT/SUnitDmg.h>
#include <GAME/Event.h>

#pragma pack(push, 1)

struct ModeChange
{
	int32_t nMode;								//0x00
	UnitAny* pUnit;						//0x04
	UnitAny* pTargetUnit;				//0x08
	int32_t nX;									//0x0C
	int32_t nY;									//0x10
	uint8_t unk0x14[4];						//0x14
	int32_t unk0x18;							//0x18
	uint8_t unk0x1C;							//0x1C
	uint8_t unk0x1D[3];							//0x1D
};

// TODO: Better names
using MonModeCallback1 = int32_t(__fastcall*)(Game*, ModeChange*);
using MonModeCallback2 = int32_t(__fastcall*)(Game*, UnitAny*);
using MonModeCallback3 = void(__fastcall*)(Game*, UnitAny*);
struct MonModeCallbackTable
{
	MonModeCallback1 unk0x00;
	MonModeCallback2 unk0x04;
	MonModeCallback3 unk0x08;
	int32_t unk0x0C;
};

#pragma pack(pop)


//D2Game.0x6FC62770
int32_t __fastcall D2GAME_IsMonster_6FC62770(UnitAny* pUnit);
//D2Game.0x6FC62780
void __fastcall sub_6FC62780(UnitAny* pAttacker, UnitAny* pDefender, Game* pGame);
//D2Game.0x6FC627B0
void __fastcall sub_6FC627B0(UnitAny* pUnit, int32_t nMode);
//D2Game.0x6FC62D90
void __fastcall sub_6FC62D90(UnitAny* pUnit, Game* pGame);
//D2Game.0x6FC62DF0
void __stdcall sub_6FC62DF0(UnitAny* pUnit, Damage* pDamage);
//D2Game.0x6FC62E70
void __fastcall D2GAME_MONSTER_ApplyCriticalDamage_6FC62E70(UnitAny* pAttacker, UnitAny* pDefender, Damage* pDamage);
//D2Game.0x6FC62F50
uint8_t __fastcall sub_6FC62F50(UnitAny* pUnit);
//D2Game.0x6FC62F90
void __fastcall D2GAME_MONSTERMODE_ResetVelocityParams_6FC62F90(UnitAny* pUnit);
//D2Game.0x6FC62FC0
void __fastcall D2GAME_MONSTERMODE_SetVelocityParams_6FC62FC0(AiParam* pAiParam, int32_t a2, int32_t nVel, uint8_t a4);
//D2Game.0x6FC62FF0
AiParam* __fastcall D2GAME_MONSTERMODE_AllocParams_6FC62FF0(Game* pGame);
//D2Game.0x6FC63020
void __fastcall D2GAME_MONSTERMODE_FreeParams_6FC63020(Game* pGame, AiParam* pAiParam);
//D2Game.0x6FC63040
void __fastcall D2GAME_MONSTERMODE_Unk_6FC63040(Game* pGame, ModeChange* pModeChange, int32_t a3);
//D2Game.0x6FC631B0
void __fastcall sub_6FC631B0(Game* pGame, UnitAny* pUnit, int32_t a7, ModeChange* pModeChange);
//D2Game.0x6FC63440
void __fastcall D2GAME_MONSTER_ApplyStatRegen_6FC63440(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC63650
MonStatsTxt* __fastcall MONSTERMODE_GetMonStatsTxtRecord(int32_t nMonsterId);
//D2Game.0x6FC63680
void __fastcall sub_6FC63680(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC63940
void __fastcall sub_6FC63940(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC63A30
void __fastcall sub_6FC63A30(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC63B20
int32_t __fastcall D2GAME_RemoveModeChangeEventCallback_6FC63B20(Game* pGame, UnitAny* pMonster);
//D2Game.0x6FC63B30
int32_t __fastcall sub_6FC63B30(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC63E80
void __fastcall sub_6FC63E80(Game* pGame, UnitAny* pUnit, DWORD dwDir);
//D2Game.0x6FC63FD0
void __fastcall sub_6FC63FD0(Game* pGame, UnitAny* pAttacker);
//D2Game.0x6FC64090
void __fastcall sub_6FC64090(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC641D0
void __fastcall sub_6FC641D0(Game* pGame, UnitAny* pAttacker);
//D2Game.0x6FC64280
int32_t __fastcall sub_6FC64280(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC642C0
int32_t __fastcall sub_6FC642C0(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC64310
int32_t __fastcall sub_6FC64310(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC643D0
int32_t __fastcall sub_6FC643D0(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC643E0
int32_t __fastcall sub_6FC643E0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC64410
int32_t __fastcall sub_6FC64410(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC64450
int32_t __fastcall sub_6FC64450(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC64480
int32_t __fastcall sub_6FC64480(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC644E0
int32_t __fastcall sub_6FC644E0(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC64510
int32_t __fastcall sub_6FC64510(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC64540
int32_t __fastcall sub_6FC64540(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC645E0
int32_t __fastcall sub_6FC645E0(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC64790
int32_t __fastcall sub_6FC64790(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC64B10
int32_t __fastcall D2GAME_GetMonsterBaseId_6FC64B10(UnitAny* pUnit);
//D2Game.0x6FC64B50
int32_t __fastcall sub_6FC64B50(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC64B60
int32_t __fastcall sub_6FC64B60(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC64CD0
void __fastcall sub_6FC64CD0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC64E20
int32_t __fastcall sub_6FC64E20(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC64E60
int32_t __fastcall sub_6FC64E60(Game* pGame, ModeChange* pModeChange);
//D2Game.0x6FC64E90
int32_t __fastcall sub_6FC64E90(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC64F50
const MonModeCallbackTable* __fastcall MONSTERMODE_GetCallbackTableRecord(UnitAny* pUnit, int32_t nMode);
//D2Game.0x6FC65080
void __fastcall D2GAME_MONSTERS_AiFunction01_6FC65080(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC65150
void __fastcall D2GAME_MONSTERS_AiFunction02_6FC65150(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC65220
int32_t __stdcall D2GAME_ModeChange_6FC65220(Game* pGame, ModeChange* pModeChange, int32_t a3);
//D2Game.0x6FC65680
int32_t __fastcall sub_6FC65680(UnitAny* pUnit, int32_t nPathType, AiParam* pAiParam, int32_t dwNewDist);
//D2Game.0x6FC65780
void __stdcall MONSTERMODE_GetModeChangeInfo(UnitAny* pUnit, int32_t nMode, ModeChange* pModeChange);
//D2Game.0x6FC65890
void __fastcall D2GAME_MONSTERS_AiFunction13_6FC65890(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC658B0
void __fastcall D2GAME_MONSTERS_AiFunction07_6FC658B0(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC65920
void __fastcall D2GAME_MONSTERS_AiFunction11_6FC65920(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC65930
void __fastcall MONSTERMODE_EventHandler(Game* pGame, UnitAny* pUnit, EventTypes nEvent, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FC659B0
void __fastcall sub_6FC659B0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t a4);
