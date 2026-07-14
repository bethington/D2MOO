#pragma once

#include <Units/Units.h>


#pragma pack(push, 1)
struct UnkAiStrc1
{
	UnitAny* pTarget;
	int32_t nDistance;
};
#pragma pack(pop)


//D2Game.0x6FCCF9D0
UnitAny* __fastcall sub_6FCCF9D0(Game* pGame, UnitAny* pUnit, AiControl* pAiControl, int32_t* pDistance, int32_t* pCombat);
//D2Game.0x6FCCFC00
UnitAny* __fastcall sub_6FCCFC00(Game* pGame, UnitAny* pUnit, AiControl* pAiControl, int32_t* pDistance, int32_t* pCombat);
//D2Game.0x6FCCFD40
UnitAny* __fastcall sub_6FCCFD40(Game* pGame, UnitAny* pUnit, AiControl* pAiControl, int32_t* pDistance, int32_t* pCombat, int32_t nMaxDistance);
//D2Game.0x6FCCFD70
UnitAny* __fastcall sub_6FCCFD70(Game* pGame, UnitAny* pUnit, int32_t* bCloseToTarget);
//D2Game.0x6FCCFDE0
UnitAny* __fastcall sub_6FCCFDE0(Game* pGame, UnitAny* pNPC, UnitAny* pPlayer, void* pCallbackArg);
//D2Game.0x6FCCFEA0
int32_t __fastcall AITACTICS_ChangeModeAndTargetUnit(Game* pGame, UnitAny* pUnit, int32_t nMode, UnitAny* pTargetUnit);
//D2Game.0x6FCCFEE0
int32_t __fastcall AITACTICS_ChangeModeAndTargetCoordinates(Game* pGame, UnitAny* pUnit, int32_t nMode, int32_t nX, int32_t nY);
//D2Game.0x6FCCFF20
int32_t __fastcall AITACTICS_UseSequenceSkill(Game* pGame, UnitAny* pUnit, int32_t nSkillId, UnitAny* pTargetUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCCFFB0
int32_t __fastcall AITACTICS_UseSkill(Game* pGame, UnitAny* pUnit, uint8_t nMode, int32_t nSkillId, UnitAny* pTarget, int32_t nX, int32_t nY);
//D2Game.0x6FCD00A0
void __fastcall AITACTICS_IdleInNeutralMode(Game* pGame, UnitAny* pUnit, int32_t nFrames);
//D2Game.0x6FCD0110
void __fastcall AITACTICS_Idle(Game* pGame, UnitAny* pUnit, int32_t nFrames);
//D2Game.0x6FCD0150
void __fastcall sub_6FCD0150(Game* pGame, UnitAny* pUnit, int32_t nFrames);
//D2Game.0x6FCD01B0
void __fastcall AITACTICS_SetVelocity(UnitAny* pUnit, int32_t a2, int32_t nVel, uint8_t a4);
//D2Game.0x6FCD0220
int32_t __fastcall AITACTICS_WalkToTargetUnitWithFlags(Game* pGame, UnitAny* pUnit, UnitAny* pTargetUnit, uint8_t nFlag);
//D2Game.0x6FCD0240
int32_t __fastcall AITACTICS_MoveToTarget(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nMode, int32_t nX, int32_t nY, uint8_t bStep, uint8_t nFlags);
//D2Game.0x6FCD03B0
int32_t __fastcall AITACTICS_WalkToTargetUnit(Game* pGame, UnitAny* pUnit, UnitAny* pTarget);
//D2Game.0x6FCD03D0
int32_t __fastcall AITACTICS_RunToTargetUnitWithFlags(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, uint8_t nFlags);
//D2Game.0x6FCD03F0
int32_t __fastcall AITACTICS_RunToTargetUnit(Game* pGame, UnitAny* pUnit, UnitAny* pTarget);
//D2Game.0x6FCD0410
int32_t __fastcall sub_6FCD0410(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, uint8_t nFlags);
//D2Game.0x6FCD0460
int32_t __fastcall AITACTICS_WalkToTargetCoordinates(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCD0480
int32_t __fastcall AITACTICS_RunToTargetCoordinates(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCD04A0
int32_t __fastcall AITACTICS_WalkToTargetCoordinatesDeleteAiEvent(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCD04C0
int32_t __fastcall AITACTICS_RunToTargetCoordinatesDeleteAiEvent(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCD04E0
int32_t __fastcall AITACTICS_WalkToTargetCoordinatesNoSteps(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY);
//D2Game.0x6FCD0500
int32_t __fastcall AITACTICS_WalkToTargetUnitWithSteps(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, uint8_t bStep);
//D2Game.0x6FCD0530
int32_t __fastcall AITACTICS_RunToTargetUnitWithSteps(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, uint8_t bStep);
//D2Game.0x6FCD0560
int32_t __fastcall D2GAME_AICORE_Escape_6FCD0560(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nMaxDistance, int32_t bDeleteAiEventCallback);
//D2Game.0x6FCD06D0
int32_t __fastcall sub_6FCD06D0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nMaxDistance, int32_t bDeleteAiEventCallback);
//D2Game.0x6FCD0840
int32_t __fastcall AITACTICS_WalkCloseToUnit(Game* pGame, UnitAny* pUnit, int32_t nMaxDistance);
//D2Game.0x6FCD09D0
int32_t __fastcall sub_6FCD09D0(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nMaxDistance);
//D2Game.0x6FCD0B60
int32_t __fastcall D2GAME_AICORE_WalkToOwner_6FCD0B60(Game* pGame, UnitAny* pUnit, UnitAny* pOwner, int32_t nMaxDistance);
//D2Game.0x6FCD0D00
int32_t __fastcall AITACTICS_RunCloseToTargetUnit(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nMaxDistance);
//D2Game.0x6FCD0E80
int32_t __fastcall sub_6FCD0E80(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t a4, int32_t bDeleteAiEventCallback);
//D2Game.0x6FCD0F10
void __fastcall AITACTICS_AddMessage(Game* pGame, UnitAny* pUnit, UnitAny* pScrollTarget, uint16_t wMessage, int32_t bScrollMessage);
//D2Game.0x6FCD1020
int32_t __fastcall AITACTICS_ChangeModeAndTargetCoordinatesOneStep(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY, int32_t nMode);
//D2Game.0x6FCD1070
int32_t __fastcall AITACTICS_ChangeModeAndTargetCoordinatesNoStep(Game* pGame, UnitAny* pUnit, int32_t nX, int32_t nY, int32_t nMode);
//D2Game.0x6FCD10C0
int32_t __fastcall AITACTICS_MoveInRadiusToTarget(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t nMode, int32_t a5, int32_t a6);
//D2Game.0x6FCD12C0
int32_t __fastcall AITACTICS_WalkInRadiusToTarget(Game* pGame, UnitAny* pUnit, UnitAny* pTarget, int32_t a4, int32_t a5);
//D2Game.0x6FCD12E0
int32_t __fastcall AITACTICS_WalkAroundTargetWithScaledDistance(Game* pGame, UnitAny* pUnit, AiControl* pAiControl, UnitAny* pTarget, int32_t nScale);
//D2Game.0x6FCD1430
UnitAny* __fastcall AITACTICS_GetTargetMinion(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FCD1490
void __fastcall AITACTICS_UseSkillInRange(UnitAny* pUnit, int32_t nRange, uint16_t wSkillId, uint8_t nMode);
