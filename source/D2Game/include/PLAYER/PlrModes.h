#pragma once

#include <Units/Units.h>
#include <GAME/Event.h>

#pragma pack(push, 1)
struct UnkPlrMode2
{
    int32_t nRightSkillId;
    int32_t nLeftSkillId;
    int32_t nRightSkillOwnerGUID;
    int32_t nLeftSkillOwnerGUID;
};
#pragma pack(pop)


//D2Game.0x6FC7F340
int32_t __fastcall D2GAME_PLRMODES_First_6FC7F340(Game* pGame, UnitAny* pUnit, int32_t nMode, int32_t a4, Skill* pSkill);
//D2Game.0x6FC7F550
void __fastcall PLRMODE_StartXY_Neutral(Game* pGame, UnitAny* pPlayer, int32_t nMode, int32_t nX, int32_t nY);
//D2Game.0x6FC7F5A0
void __fastcall PLRMODE_StartID_WalkRunKnockback(Game* pGame, UnitAny* a2, int32_t a3, UnitAny* pTargetUnit);
//D2Game.0x6FC7F600
void __fastcall sub_6FC7F600(Game* pGame, UnitAny* pPlayer, int32_t nMode);
//D2Game.0x6FC7F730
void __fastcall PLRMODE_StartXY_WalkRunKnockback(Game* pGame, UnitAny* pPlayer, int32_t nMode, int32_t nX, int32_t nY);
//D2Game.0x6FC7F780
int32_t __fastcall sub_6FC7F780(Game* pGame, UnitAny* pPlayer, int32_t a3, int32_t a4);
//D2Game.0x6FC7FB70
void __fastcall sub_6FC7FB70(UnitAny* pPlayer, int32_t a2);
//D2Game.0x6FC7FB90
int32_t __fastcall sub_6FC7FB90(UnitAny* pPlayer);
//D2Game.0x6FC7FBB0
int32_t __fastcall sub_6FC7FBB0(UnitAny* pPlayer);
//1.10f: D2Game.0x6FC7FBD0
//1.13c: D2Game.0x6FC99210
UnitAny* __fastcall D2GAME_CORPSE_Handler_6FC7FBD0(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY, Room1* pRoom);
//D2Game.0x6FC802F0
void __fastcall sub_6FC802F0(Game* pGame, int32_t nBodyloc, UnitAny* pItem, UnitAny* pUnit);
//D2Game.0x6FC803F0
void __fastcall sub_6FC803F0(Game* pGame, UnitAny* pItem, UnitAny* pPlayer);
//D2Game.0x6FC80440
void __fastcall sub_6FC80440(Game* pGame, UnitAny* pPlayer, UnitAny* pDeadBody);
//D2Game.0x6FC805B0
void __fastcall PLRMODE_StartID_Death(Game* pGame, UnitAny* pDefender, int32_t a3, UnitAny* pAttacker);
//D2Game.0x6FC80710
void __fastcall PLRMODE_StartXY_Dead(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4, int32_t a5);
//D2Game.0x6FC80870
void __fastcall PLRMODE_StartXY_Block(Game* pGame, UnitAny* pPlayer, int32_t nMode, int32_t nX, int32_t nY);
//D2Game.0x6FC808D0
int32_t __fastcall sub_6FC808D0(Game* pGame, UnitAny* pPlayer, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FC808E0
void __fastcall PLRMODE_StartXY_GetHit(Game* pGame, UnitAny* pPlayer, int32_t nMode, int32_t nX, int32_t nY);
//D2Game.0x6FC80940
void __fastcall PLRMODE_StartXY_AttackCastThrowKickSpecialSequence(Game* pGame, UnitAny* pPlayer, int32_t nMode, int32_t nX, int32_t nY);
//D2Game.0x6FC809B0
void __fastcall PLRMODE_StartID_AttackCastThrowKickSpecialSequence(Game* pGame, UnitAny* pPlayer, int32_t nMode, UnitAny* pTarget);
//D2Game.0x6FC80A30
void __fastcall sub_6FC80A30(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC80B90
void __fastcall sub_6FC80B90(Game* pGame, UnitAny* pPlayer, UnitAny* pWeapon);
//D2Game.0x6FC80E10
void __fastcall sub_6FC80E10(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC80EE0
int32_t __fastcall sub_6FC80EE0(Game* pGame, UnitAny* pPlayer, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FC80F80
void __fastcall D2GAME_EVENTS_StatRegen_6FC80F80(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC81250
void __fastcall sub_6FC81250(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC814F0
void __fastcall sub_6FC814F0(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC81560
void __fastcall sub_6FC81560(UnitAny* pUnit, UnkPlrMode2* a2);
//D2Game.0x6FC81600
void __fastcall sub_6FC81600(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FC81650
void __fastcall sub_6FC81650(Game* pGame, UnitAny* pPlayer, GameClient* pClient, int32_t a4);
//D2Game.0x6FC817D0
void __fastcall sub_6FC817D0(Game* pGame, UnitAny* pUnit, Skill* pUsedSkill, int32_t nMode, int32_t nX, int32_t nY, int32_t a7);
//D2Game.0x6FC81890
int32_t __fastcall sub_6FC81890(Game* pGame, UnitAny* pUnit, int32_t nMode);
//D2Game.0x6FC81A00
void __fastcall D2GAME_PLAYERMODE_Change_6FC81A00(Game* pGame, UnitAny* pPlayer, Skill* pSkill, BYTE nMode, int32_t nUnitType, int32_t nTargetGUID, int32_t bAllowReEnter);
//D2Game.0x6FC81B20
void __fastcall sub_6FC81B20(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC81B90
void __fastcall sub_6FC81B90(Game* pGame, UnitAny* pUnit, int32_t a3, int32_t a4);
//D2Game.0x6FC81BD0
void __fastcall D2GAME_EVENTS_Callback_6FC81BD0(Game* pGame, UnitAny* pUnit, EventTypes nEvent, int32_t dwArg, int32_t dwArgEx);
