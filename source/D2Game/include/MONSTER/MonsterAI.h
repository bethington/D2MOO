#pragma once

#include <Units/Units.h>
#include <UNIT/SUnitNpc.h>

#pragma pack(push, 1)
struct Unk
{
    int32_t nItemCode;
    uint8_t nBodyLoc;
    uint8_t unk0x05;
    uint8_t unk0x06;
    uint8_t unk0x07;
};
#pragma pack(pop)


//D2Game.0x6FC61190
void __fastcall sub_6FC61190(Game* pGame, UnitAny* pMonster, Unk* pItemCode, DWORD dwILvl, DWORD dwQuality);
//D2Game.0x6FC61270
void __fastcall sub_6FC61270(Game* pGame, UnitAny* pPlayer, UnitAny* pUnit, int16_t nId, MercData* pMercData, int32_t bDead);
//D2Game.0x6FC61490
void __fastcall MONSTERAI_SendMercStats(Game* pGame, UnitAny* pPlayer, DWORD dwZero);
//D2Game.0x6FC61610
void __fastcall MONSTERAI_UpdateMercStatsAndSkills(Game* pGame, UnitAny* pPlayer, UnitAny* pMerc, int32_t nLevel);
//D2Game.0x6FC61980
MonsterInteract* __fastcall MONSTERAI_AllocMonsterInteract(Game* pGame);
//D2Game.0x6FC619A0
void __fastcall MONSTERAI_FreeMonsterInteract(Game* pGame, MonsterInteract* pMonInteract);
//D2Game.0x6FC619F0
int32_t __fastcall MONSTERAI_GetInteractUnitCount(MonsterInteract* pMonInteract);
//D2Game.0x6FC61A10
int32_t __fastcall MONSTERAI_HasInteractUnit(MonsterInteract* pMonInteract);
//D2Game.0x6FC61A30
int32_t __fastcall MONSTERAI_IsInteractingWith(MonsterInteract* pMonInteract, UnitAny* pUnit);
//D2Game.0x6FC61A50
void __fastcall MONSTERAI_RemoveInteractInfoFor(Game* pGame, UnitAny* pMonster, MonsterInteract* pMonInteract);
//D2Game.0x6FC61AB0
void __fastcall sub_6FC61AB0(Game* pGame, UnitAny* pUnit, UnitAny* pNpc, MonsterInteract* pMonInteract);
//D2Game.0x6FC61AF0
void __fastcall D2GAME_NPCS_SetInteractTrading_6FC61AF0(UnitAny* pNPC, UnitAny* pPlayer);
//D2Game.0x6FC61B30
int32_t __fastcall sub_6FC61B30(UnitAny* pMonster, UnitAny* pPlayer);
//D2Game.0x6FC61B70
void __fastcall sub_6FC61B70(Game* pGame, UnitAny* pPlayer, UnitAny* pNpc, MonsterInteract* pMonInteract);
//D2Game.0x6FC61C70
void __fastcall sub_6FC61C70(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, MonsterInteract* pMonInteract, char a5);
//D2Game.0x6FC61E30
int32_t __fastcall sub_6FC61E30(UnitAny* pUnit, int32_t a2, int32_t a3);
//D2Game.0x6FC61EC0
int32_t __fastcall sub_6FC61EC0(UnitAny* pMonster);
//D2Game.0x6FC61EE0
int32_t __fastcall sub_6FC61EE0(UnitAny* pMonster);
//D2Game.0x6FC61F00
void __fastcall sub_6FC61F00(UnitAny* pMonster);
//D2Game.0xFC61F20
int32_t __fastcall sub_6FC61F20(UnitAny* pMonster, UnitAny* pUnit);
//D2Game.0x6FC61F70
void __fastcall D2GAME_MONSTERAI_Last_6FC61F70(Game* pGame, UnitAny* pMonster, void(__fastcall* pCallback)(UnitAny*));
