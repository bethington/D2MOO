#pragma once

#include <Units/Units.h>
#include <UNIT/SUnitDmg.h>

#pragma pack(1)

typedef BOOL(__fastcall* SPELLPREPARE)(Game*, UnitAny*, UnitAny*, UnitAny*, int32_t, int32_t, int32_t);
typedef BOOL(__fastcall* SPELLDO)(Game*, UnitAny*, UnitAny*, UnitAny*, int32_t, int32_t, int32_t);

struct SpellTbl
{
	SPELLPREPARE pfSpellPrepare;			//0x00
	SPELLDO pfSpellDo;						//0x04
};

#pragma pack()

//D2Game.0x6FD02BA0
int32_t __fastcall SKILLITEM_pSpell01_Initializer(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD02BF0
int32_t __fastcall SKILLITEM_pSpell01_IdentifyItem(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD02C80
int32_t __fastcall SKILLITEM_pSpell02_CastPortal(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD02E10
int32_t __fastcall SKILLITEM_pSpell03_Potion(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD032D0
int32_t __fastcall SKILLITEM_pSpell04_Unused(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD03610
int32_t __fastcall SKILLITEM_pSpell05_RejuvPotion(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD039A0
int32_t __fastcall SKILLITEM_pSpell09_StaminaPotion(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD03BB0
int32_t __fastcall SKILLITEM_pSpell09_AntidoteThawingPotion(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD03C80
int32_t __fastcall SKILLITEM_pSpell08_ExperienceElixir(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD03CD0
int32_t __fastcall SKILLITEM_pSpell07_OpenCube(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD03D80
int32_t __fastcall SKILLITEM_pSpell10_CastFireBallOnTarget(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD03DF0
int32_t __fastcall SKILLITEM_pSpell11_CastFireBallToCoordinates(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY, int32_t nSkillId);
//D2Game.0x6FD03E40
int32_t __fastcall SKILLITEM_pSpell_Handler(Game* pGame, UnitAny* pUnit, UnitAny* pItem, UnitAny* pTarget, int32_t nX, int32_t nY);
//D2Game.0x6FD040B0
int32_t __fastcall SKILLS_SrvDo113_Scroll_Book(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04270
void __fastcall SKILLITEM_ActivateAura(Game* pGame, UnitAny* pUnit, int32_t a5, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04340
void __fastcall SKILLITEM_DeactivateAura(Game* pGame, UnitAny* pUnit, int32_t a4, int32_t nSkillId);
//D2Game.0x6FD043F0
int32_t __fastcall SKILLITEM_EventFunc06_AttackerTakesDamage(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD044B0
int32_t __fastcall SKILLITEM_EventFunc10_AttackerTakesLightDamage(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04570
int32_t __fastcall SKILLITEM_EventFunc11_ApplyFireDamage(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04630
int32_t __fastcall SKILLITEM_EventFunc12_ApplyColdDamage(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04720
int32_t __fastcall SKILLITEM_EventFunc07_Knockback(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04820
int32_t __fastcall SKILLITEM_EventFunc08_Howl(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD048B0
int32_t __fastcall SKILLITEM_EventFunc09_Stupidity(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD049D0
int32_t __fastcall SKILLITEM_EventFunc13_DamageToMana(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04B10
int32_t __fastcall SKILLITEM_EventFunc14_Freeze(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04C40
int32_t __fastcall SKILLITEM_CalculateOpenWoundsHpRegen(int32_t nLevel, const int32_t* pValues);
//D2Game.0x6FD04CF0
int32_t __fastcall SKILLITEM_EventFunc15_OpenWounds(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD04E50
int32_t __fastcall SKILLITEM_EventFunc16_CrushingBlow(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD050D0
int32_t __fastcall SKILLITEM_EventFunc17_ManaAfterKill(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD05160
int32_t __fastcall SKILLITEM_EventFunc28_HealAfterKill(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD051E0
int32_t __fastcall SKILLITEM_EventFunc18_HealAfterDemonKill(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD05270
int32_t __fastcall SKILLITEM_EventFunc19_Slow(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD053D0
int32_t __fastcall SKILLITEM_EventFunc20_SkillOnAttackHitKill(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD05520
int32_t __fastcall SKILLITEM_EventFunc21_SkillOnGetHit(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD05640
int32_t __fastcall SKILLITEM_EventFunc30_SkillOnDeathLevelup(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD05750
int32_t __fastcall SKILLITEM_EventFunc29_RestInPeace(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD05780
int32_t __fastcall SKILLITEM_TimerCallback_ReanimateMonster(Game* pGame, int32_t nEvent, UnitAny* pSource, UnitAny* pTarget, Damage* pDamage, int32_t nMonId, int32_t nOwnerId);
//D2Game.0x6FD05B60
int32_t __fastcall SKILLITEM_EventFunc31_Reanimate(Game* pGame, int32_t nEvent, UnitAny* pAttacker, UnitAny* pUnit, Damage* pDamage, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD05C20
int32_t __fastcall SKILLITEM_FindTargetPosition(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel);
//D2Game.0x6FD05D70
int32_t __fastcall SKILLITEM_HandleItemEffectSkill(Game* pGame, UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, UnitAny* pTargetUnit, int32_t nX, int32_t nY, int32_t* pUnitType, int32_t* pUnitGUID, int32_t* pX, int32_t* pY, int32_t a7);
//D2Game.0x6FD06020
int32_t __fastcall SKILLITEM_CastSkillOnTarget(UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, UnitAny* pTargetUnit, int32_t a5);
//D2Game.0x6FD060F0
int32_t __fastcall SKILLITEM_CastSkillOnPosition(UnitAny* pUnit, int32_t nSkillId, int32_t nSkillLevel, int32_t nX, int32_t nY, int32_t a0);
