#pragma once

#include <Units/Units.h>


//D2Game.0x6FCC5520
void __fastcall D2GAME_SUNITMSG_FirstFn_6FCC5520(Game* pGame, UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FCC58E0
void __fastcall D2GAME_STATES_SendUnitStates_6FCC58E0(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FCC5BE0
void __fastcall D2GAME_STATES_SendUnitStateUpdates_6FCC5BE0(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FCC5F00
void __fastcall D2GAME_STATES_SendStates_6FCC5F00(UnitAny* pUnit, GameClient* pClient, int32_t bPlayer);
//D2Game.0x6FCC5F20
void __fastcall sub_6FCC5F20(UnitAny* pItem, GameClient* pClient);
//D2Game.0x6FCC5F80
void __fastcall D2GAME_PACKETS_SendPacket0x0A_RemoveObject_6FCC5F80(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FCC5FA0
void __fastcall sub_6FCC5FA0(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FCC6080
void __fastcall D2GAME_UpdateUnit_6FCC6080(UnitAny* pPlayer, GameClient* pClient);
//D2Game.0x6FCC60D0
void __fastcall sub_6FCC60D0(UnitAny* pUnit, int16_t nSkillId, uint8_t nSkillLevel, uint8_t nUnitType, int32_t nUnitGUID, uint8_t a6);
//D2Game.0x6FCC6150
void __fastcall sub_6FCC6150(UnitAny* pUnit, int16_t nSkillId, uint8_t nSkillLevel, int16_t nX, int16_t nY, uint8_t a6);
//D2Game.0x6FCC61D0
void __fastcall D2GAME_MERCS_SendStat_6FCC61D0(UnitAny* pUnit, uint16_t nStatId, int32_t nValue);
//D2Game.0x6FCC6270
void __fastcall sub_6FCC6270(UnitAny* pUnit, uint8_t a2);
//D2Game.0x6FCC6300
void __fastcall sub_6FCC6300(UnitAny* pUnit, UnitAny* pTargetUnit, int16_t nSkillId, int16_t nSkillLevel, int32_t nX, int32_t nY, uint8_t a7);
//D2Game.0x6FCC63D0
void __fastcall sub_6FCC63D0(UnitAny* pUnit, int16_t a2);
//D2Game.0x6FCC6470
void __fastcall sub_6FCC6470(UnitAny* pUnit, int16_t a2);
//D2Game.0x6FCC64D0
void __fastcall sub_6FCC64D0(UnitAny* pUnit, uint8_t bLeftSkill, int16_t nSkillId, int32_t nOwnerGUID);
//D2Game.0x6FCC6540
void __fastcall sub_6FCC6540(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FCC6790
void __fastcall D2GAME_SUNITMSG_FreeUnitMessages_6FCC6790(UnitAny* pUnit);
