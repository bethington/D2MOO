#pragma once

#include <Units/Units.h>

#include "D2PacketDef.h"


//D2Game.0x6FC3C640
int32_t __fastcall sub_6FC3C640(int32_t nClientId, int16_t nGameId, int16_t nClientCount, const char* szGameName);
//D2Game.0x6FC3C690
void __fastcall sub_6FC3C690(int32_t nClientId);
//D2Game.0x6FC3C6B0
void __fastcall sub_6FC3C6B0(int32_t nClientId);
//D2Game.0x6FC3C6D0
void __fastcall sub_6FC3C6D0(int32_t nClientId, uint32_t nErrorCode);
//D2Game.0x6FC3C6F0
void __fastcall D2GAME_PACKETS_SendHeaderOnlyPacket(GameClient* pClient, uint8_t nHeader);
//D2Game.0x6FC3C710
void __fastcall D2GAME_PACKETS_SendPacket_6FC3C710(GameClient* pClient, void* pPacket, int32_t nPacketSize);
//D2Game.0x6FC3C7C0
void __fastcall D2GAME_PACKETS_SendPacket0x01_6FC3C7C0(GameClient* pClient, char nHeader, Game* pGame);
//D2Game.0x6FC3C810
void __fastcall D2GAME_PACKETS_SendPacket0x03_6FC3C810(GameClient* pClient, char nHeader, char nAct, int32_t nInitSeed, int32_t nObjectSeed, int16_t nTownId);
//D2Game.0x6FC3C850
void __fastcall D2GAME_PACKETS_SendPacketSize06_6FC3C850(GameClient* pClient, DWORD nHeader, DWORD dwUnitType, DWORD dwUnitId);
//D2Game.0x6FC3C880
void __fastcall D2GAME_SendPacketSize05_6FC3C880(GameClient* pClient, char nHeader, int32_t nArg);
//D2Game.0x6FC3C8A0
void __fastcall D2GAME_PACKETS_SendPacket0x0C_6FC3C8A0(GameClient* pClient, char nHeader, UnitTypes nUnitType, int32_t nUnitGUID, char a5, char nHitClass, char nLifePct);
//D2Game.0x6FC3C8E0
void __fastcall D2GAME_PACKETS_SendPacket0x0E_6FC3C8E0(GameClient* pClient, char nHeader, char a3, int32_t a4, char a5, char a6, int32_t nAnimMode);
//D2Game.0x6FC3C920
void __fastcall D2GAME_PACKETS_SendPacket0x0D_6FC3C920(GameClient* pClient, char nPacketId, int32_t nUnitType, int32_t nUnitId, char a5, WORD nX, WORD nY, char a8, char nLife);
//D2Game.0x6FC3C9A0
void __fastcall D2GAME_PACKETS_SendPacket0x10_6FC3C9A0(GameClient* pClient, char nHeader, char a3, int32_t a4, char a5, char a6, int32_t a7, int16_t a8, int16_t a9);
//D2Game.0x6FC3CA00
void __fastcall D2GAME_PACKETS_SendPacket0x0F_6FC3CA00(GameClient* pClient, char nHeader, uint8_t bTOU, int32_t a4, char a5, int16_t a6, int16_t a7, char a8, int16_t a9, int16_t a10);
//D2Game.0x6FC3CA90
void __fastcall D2GAME_PACKETS_SendPacket0x68_6FC3CA90(GameClient* pClient, uint8_t nHeader, int32_t nUnitGUID, int8_t a4, int8_t a5, int32_t a6, int8_t a7);
//D2Game.0x6FC3CBC0
void __fastcall D2GAME_PACKETS_SendPacket0x67_6FC3CBC0(GameClient* pClient, uint8_t nHeader, int32_t nUnitGUID, int8_t a4, int16_t nX, int16_t nY, int8_t a7);
//D2Game.0x6FC3CCB0
void __fastcall D2GAME_PACKETS_SendPacket0x68_6FC3CCB0(GameClient* pClient, uint8_t nHeader, int32_t nUnitGUID, int8_t a4, int8_t a5, int32_t a6, int8_t a7, int8_t a8, int8_t a9);
//D2Game.0x6FC3CDE0
void __fastcall D2GAME_PACKETS_SendPacket0x67_6FC3CDE0(GameClient* pClient, uint8_t nHeader, int32_t nUnitGUID, int8_t a4, int16_t nX, int16_t nY, int8_t a7, int8_t a8, int8_t a9);
//D2Game.0x6FC3CEE0
void __fastcall D2GAME_PACKETS_SendPacket0x6A_6FC3CEE0(GameClient* pClient, char a2, int32_t nUnitGUID, char a4, char a5, int32_t a6, char nDirection);
//D2Game.0x6FC3CF30
void __fastcall D2GAME_PACKETS_SendPacket0x69_6FC3CF30(GameClient* pClient, char a2, int32_t a3, char a4, int16_t a5, int16_t a6, char a7, char a8);
//D2Game.0x6FC3CF90
void __fastcall D2GAME_PACKETS_SendPacket0x6C_6FC3CF90(GameClient* pClient, char a2, int32_t a3, char a4, char a5, int32_t a6, char a7, int16_t a8, int16_t a9);
//D2Game.0x6FC3D000
void __fastcall D2GAME_PACKETS_SendPacket0x6B_6FC3D000(GameClient* pClient, char a2, int32_t a3, char a4, int16_t a5, int16_t a6, char a7, char a8, int16_t a9, int16_t a10);
//D2Game.0x6FC3D080
void __fastcall D2GAME_PACKETS_SendPacket0x6D_6FC3D080(GameClient* pClient, DWORD dwUnitId, WORD nX, WORD nY, BYTE nUnitLife);
//D2Game.0x6FC3D0D0
void __fastcall D2GAME_PACKETS_SendPacket0x15_6FC3D0D0(GameClient* pClient, char a2, char a3, int32_t a4, int16_t a5, int16_t a6, char a7);
//D2Game.0x6FC3D120
void __fastcall D2GAME_PACKETS_SendPacket0x07_6FC3D120(GameClient* pClient, uint8_t nAreaId, uint16_t nTileX, uint16_t nTileY);
//D2Game.0x6FC3D160
void __fastcall D2GAME_PACKETS_SendPacket0x08_6FC3D160(GameClient* pClient, uint8_t nAreaId, uint16_t nTileX, uint16_t nTileY);
//D2Game.0x6FC3D1A0
void __fastcall D2GAME_PACKETS_SendPacket0x09_6FC3D1A0(GameClient* pClient, BYTE a2, BYTE a3, DWORD a4, BYTE a5, WORD a6, WORD a7);
//D2Game.0x6FC3D1F0
void __fastcall sub_6FC3D1F0(GameClient* pClient, int32_t nUnitGUID, uint8_t nClassId, const char* szName, int16_t nX, int16_t nY);
//D2Game.0x6FC3D300
void __fastcall D2GAME_PACKETS_SendPacket0x51_6FC3D300(GameClient* pClient, char nHeader, uint8_t nUnitType, int32_t nUnitGUID, int16_t nObjectId, int16_t nX, int16_t nY, char a8, char a9);
//D2Game.0x6FC3D3A0
void __fastcall D2GAME_PACKETS_SendPacket0x0A_RemoveObject_6FC3D3A0(GameClient* pClient, char alw0x0A, char nUnitType, int32_t nUnitId);
//D2Game.0x6FC3D3D0
void __fastcall D2GAME_PACKETS_SendPacket0x19_6FC3D3D0(GameClient* pClient, int32_t nValue, int32_t a3);
//D2Game.0x6FC3D410
void __fastcall D2GAME_PACKETS_SendPacket0x1A_B_C_6FC3D410(GameClient* pClient, int32_t nExperience, int32_t a3);
//D2Game.0x6FC3D480
void __fastcall D2GAME_PACKETS_SendPacket0x1D_E_F_6FC3D480(GameClient* pClient, uint16_t nStatId, uint32_t nValue);
//D2Game.0x6FC3D520
void __fastcall D2GAME_PACKETS_SendPacket0x9E_9F_A0_6FC3D520(GameClient* pClient, UnitAny* pUnit, uint16_t nStatId, uint32_t nValue);
//D2Game.0x6FC3D610
void __fastcall D2GAME_PACKETS_SendPacket0xA0_A1_A2_6FC3D610(GameClient* pClient, UnitAny* pUnit, uint16_t nStatId, uint32_t nBaseValue, uint32_t nFullValue);
//D2Game.0x6FC3D730
void __fastcall D2GAME_PACKETS_SendPacket0xA3_6FC3D730(GameClient* pClient, BYTE a2, WORD a3, WORD a4, BYTE a5, int32_t a6, BYTE a7, int32_t a8, int32_t a9, int32_t a10);
//D2Game.0x6FC3D7B0
void __fastcall D2GAME_PACKETS_SendPacket0xAB_6FC3D7B0(GameClient* pClient, uint8_t nUnitType, int32_t dwUnitId, uint8_t nUnitLife);
//D2Game.0x6FC3D7F0
void __fastcall D2GAME_PACKETS_SendPacket0xA5_6FC3D7F0(GameClient* pClient, uint8_t a2, int32_t a3, uint16_t a4);
//D2Game.0x6FC3D830
void __fastcall sub_6FC3D830(GameClient* pClient, BYTE nHeader, int32_t nUnitGUID, WORD nStatId, int32_t nValue);
//D2Game.0x6FC3D890
void __fastcall sub_6FC3D890(GameClient* pClient, uint16_t a2, uint16_t a3, uint16_t a4, uint8_t a5, uint8_t a6, uint16_t a7, uint16_t a8, uint8_t a9, uint8_t a10);
//D2Game.0x6FC3D9A0
void __fastcall sub_6FC3D9A0(GameClient* pClient, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint8_t a7, uint8_t a8);
//D2Game.0x6FC3DA90
void __fastcall D2GAME_PACKETS_SendPacket0x96_WalkVerify_6FC3DA90(GameClient* pClient, uint16_t a2, uint16_t a3, uint16_t a4, uint8_t a5, uint8_t a6);
//D2Game.0x6FC3DB50
void __fastcall D2GAME_PACKETS_SendPacket0x21_UpdateSkills_6FC3DB50(GameClient* pClient, UnitAny* pUnit, WORD nSkillId, BYTE nSkillLevel, BYTE a4);
//D2Game.0x6FC3DBE0
void __fastcall D2GAME_PACKETS_SendPacket0x22_6FC3DBE0(GameClient* pClient, BYTE nUnitType, int32_t nUnitGUID, int16_t nSkillId, char a5);
//D2Game.0x6FC3DC60
void __fastcall D2GAME_PACKETS_SendPacket0x23_6FC3DC60(GameClient* pClient, BYTE nUnitType, int32_t nUnitGUID, BYTE a4, WORD a5, int32_t a6);
//D2Game.0x6FC3DCA0
void __fastcall sub_6FC3DCA0(GameClient* pClient, UnitAny* pUnit);
//D2Game.0x6FC3DDF0
void __fastcall D2GAME_PACKETS_SendPacket0x26_ServerMessage_6FC3DDF0(GameClient* pClient, GSPacketSrv26* pMsg);
//D2Game.0x6FC3DEC0
void __fastcall D2GAME_PACKETS_SendPacket0x5A_6FC3DEC0(GameClient* pClient, GSPacketSrv5A* pPacket);
//D2Game.0x6FC3DF20
void __fastcall D2GAME_PACKETS_SendPacket0x27_6FC3DF20(GameClient* pClient, GSPacketSrv27* pPacket);
//D2Game.0x6FC3DF50
void __fastcall D2GAME_SendPacket0x53_6FC3DF50(GameClient* pClient, GSPacketSrv53* pPacket);
//D2Game.0x6FC3DF80
void __fastcall sub_6FC3DF80(GameClient* pClient1, UnitAny* pPlayer, GameClient* pClient2);
//D2Game.0x6FC3E090
void __fastcall D2GAME_PACKETS_SendPacket0x5C_6FC3E090(GameClient* pClient, DWORD dwUnitId);
//D2Game.0x6FC3E0B0
void __fastcall D2GAME_PACKETS_SendPacket0x77_Ui_6FC3E0B0(GameClient* pClient, BYTE nUiNo);
//D2Game.0x6FC3E0D0
void __fastcall sub_6FC3E0D0(GameClient* pClient, GSPacketSrv78* pPacket);
//D2Game.0x6FC3E100
void __fastcall D2GAME_PACKETS_SendPacket0x7A_6FC3E100(GameClient* pClient, char a2, char a3, int32_t a4, int32_t a5, int16_t a6);
//D2Game.0x6FC3E160
void __fastcall D2GAME_PACKETS_SendPacket0x81_6FC3E160(GameClient* pClient, char a2, int32_t a3, int32_t a4, int16_t a5, int32_t a6, int32_t a7, int32_t a8);
//D2Game.0x6FC3E1D0
void __fastcall D2GAME_PACKETS_SendPacket0x79_6FC3E1D0(GameClient* pClient, int32_t a2, char a3);
//D2Game.0x6FC3E200
void __fastcall sub_6FC3E200(GameClient* pClient, UnitAny* pUnit);
//D2Game.0x6FC3E3D0
UnkMonsterData* __fastcall sub_6FC3E3D0(UnitAny* pUnit);
//D2Game.0x6FC3E3F0
int32_t __stdcall sub_6FC3E3F0(UnkMonsterData* a1, int32_t nCurrentLifePercentage);
//D2Game.0x6FC3E440
int32_t __fastcall sub_6FC3E440(UnitAny* pUnit1, UnitAny* pUnit2, int32_t a3, int32_t a4);
//D2Game.0x6FC3E570
void __fastcall D2GAME_SendPacket0x9C_6FC3E570(GameClient* pClient, UnitAny* pItem, char nAction, DWORD dwFlag, int32_t bGamble);
//D2Game.0x6FC3E6F0
void __fastcall D2GAME_SendPacket0x9D_6FC3E6F0(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, char nAction, DWORD dwFlag, int32_t bGamble);
//D2Game.0x6FC3E850
void __fastcall D2GAME_SendP0x9C_ItemAction_AddToGround_6FC3E850(GameClient* pClient, UnitAny* pItem, DWORD dwFlag);
//D2Game.0x6FC3E870
void __fastcall D2GAME_SendP0x9C_ItemAction_GroundToCursor_6FC3E870(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD a4);
//D2Game.0x6FC3E8E0
void __fastcall D2GAME_SendP0x9C_ItemAction_DropToGround_6FC3E8E0(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD a4);
//D2Game.0x6FC3E930
void __fastcall D2GAME_SendP0x9C_ItemAction_OnGround_6FC3E930(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD a4);
//D2Game.0x6FC3E980
void __fastcall D2GAME_SendP0x9C_ItemAction_PutInContainer_6FC3E980(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD a4);
//D2Game.0x6FC3E9D0
void __fastcall D2GAME_UpdateClientItem_6FC3E9D0(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD dwCmdFlag);
//D2Game.0x6FC3EA50
void __fastcall D2GAME_SendP0x9D_ItemAction_Equip_6FC3EA50(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3EA70
void __fastcall D2GAME_SendP0x9D_ItemAction_IndirectlySwapBodyItem_6FC3EA70(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3EA90
void __fastcall D2GAME_SendP0x9D_ItemAction_Unequip_6FC3EA90(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3EAB0
void __fastcall D2GAME_SendP0x9D_ItemAction_SwapBodyItem_6FC3EAB0(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3EAD0
void __fastcall D2GAME_SendP0x9D_ItemAction_WeaponSwitch_6FC3EAD0(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3EAF0
void __fastcall D2GAME_SendP0x9C_ItemAction_AddQuantity_6FC3EAF0(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD a4);
//D2Game.0x6FC3EB40
void __fastcall D2GAME_SendP0x9C_ItemAction_SwapInContainer_6FC3EB40(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD a4);
//D2Game.0x6FC3EB90
void __fastcall D2GAME_SendP0x9C_ItemAction_PutInBelt_6FC3EB90(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD a4);
//D2Game.0x6FC3EBE0
void __fastcall D2GAME_SendP0x9C_ItemAction_RemoveFromBelt_6FC3EBE0(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlag);
//D2Game.0x6FC3EC00
void __fastcall D2GAME_SendP0x9C_ItemAction_SwapInBelt_6FC3EC00(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, DWORD a3);
//D2Game.0x6FC3EC20
void __fastcall D2GAME_PACKETS_SendPacket0x3E_6FC3EC20(GameClient* pClient, UnitAny* pItem, uint8_t a3, int32_t nStatId, uint32_t nValue, uint16_t a6);
//D2Game.0x6FC3EDC0
void __fastcall D2GAME_PACKETS_SendPacket0x3F_6FC3EDC0(GameClient* pClient, UnitAny* pUnit, char a3, int32_t a4, int32_t a5, int16_t a6);
//D2Game.0x6FC3EE20
void __fastcall D2GAME_SendP0x9D_ItemAction_AutoUnequip_6FC3EE20(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3EE40
void __fastcall D2GAME_PACKETS_SendPacket0x42_6FC3EE40(GameClient* pClient, UnitAny* pUnit);
//D2Game.0x6FC3EEA0
void __fastcall D2GAME_SendP0x9C_ItemAction_ToCursor_6FC3EEA0(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem);
//D2Game.0x6FC3EEC0
void __fastcall D2GAME_SendP0x9D_ItemAction_Unknown0x14_6FC3EEC0(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3EEE0
void __fastcall D2GAME_SendPacket0x47_6FC3EEE0(GameClient* pClient, UnitAny* pUnit);
//D2Game.0x6FC3EF40
void __fastcall D2GAME_SendPacket0x48_6FC3EF40(GameClient* pClient, UnitAny* pUnit, BYTE a3);
//D2Game.0x6FC3EFA0
void __fastcall D2GAME_SendP0x9D_ItemAction_UpdateStats_6FC3EFA0(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3EFC0
void __fastcall sub_6FC3EFC0(GameClient* pClient, UnitAny* pUnit1, UnitAny* pUnit2, int32_t a4, int32_t a5);
//D2Game.0x6FC3F040
void __fastcall D2GAME_SendP0x9D_ItemAction_Unknown0x16_6FC3F040(GameClient* pClient, UnitAny* pUnit, UnitAny* pItem, DWORD dwFlags);
//D2Game.0x6FC3F060
void __fastcall sub_6FC3F060(GameClient* pClient, uint8_t a2, int32_t a3, uint16_t a4, uint16_t a5, int32_t a6, uint16_t a7, uint8_t a8, int32_t a9);
//D2Game.0x6FC3F0C0
void __fastcall sub_6FC3F0C0(GameClient* pClient, uint8_t a2, int32_t a3, uint8_t nUnitType, int32_t nUnitGUID, uint16_t a6, uint16_t a7, uint8_t a8, int32_t a9);
//D2Game.0x6FC3F260
void __fastcall D2GAME_SendP0x9C_ItemAction_AddOrRemoveFromShop_6FC3F260(GameClient* pClient, UnitAny* pPlayer, UnitAny* pItem, char a3);
//D2Game.0x6FC3F2F0
void __fastcall D2GAME_SendPacket0x28_6FC3F2F0(GameClient* pClient, BYTE nHeader, BYTE a3, int32_t a4, BitBuffer* a5, BYTE a6);
//D2Game.0x6FC3F340
void __fastcall sub_6FC3F340(GameClient* pClient, BYTE nHeader, BYTE nUnitType, int32_t nUnitId, int32_t a5);
//D2Game.0x6FC3F370
void __fastcall D2GAME_PACKETS_SendPacket0x29_6FC3F370(GameClient* pClient, GSPacketSrv29* pPacket);
//D2Game.0x6FC3F380
void __fastcall sub_6FC3F380(GameClient* pClient, GSPacketSrv5D* pPacket);
//D2Game.0x6FC3F3B0
void __fastcall D2GAME_SendPacket0x2A_6FC3F3B0(GameClient* pClient, char a2, char a3, int32_t a4, int32_t a5, char a6);
//D2Game.0x6FC3F3F0
void __fastcall sub_6FC3F3F0(GameClient* pClient, GSPacketSrv2C* pPacket);
//D2Game.0x6FC3F410
void __fastcall D2GAME_SendPacket0x4E_6FC3F410(GameClient* pClient, GSPacketSrv4E* pPacket);
//D2Game.0x6FC3F440
void __fastcall D2GAME_SendPacket0x50_6FC3F440(GameClient* pClient, GSPacketSrv50* pPacket);
//D2Game.0x6FC3F480
void __fastcall sub_6FC3F480(GameClient* pClient, void* pPacket);
//D2Game.0x6FC3F490
void __fastcall D2GAME_SendPacket0x52_6FC3F490(GameClient* pClient, GSPacketSrv52* pPacket);
//D2Game.0x6FC3F4A0
void __fastcall D2GAME_PACKETS_SendPacket0x11_6FC3F4A0(GameClient* pClient, BYTE nUnitType, DWORD dwUnitId, WORD unk);
//D2Game.0x6FC3F4D0
void __fastcall sub_6FC3F4D0(GameClient* pClient, int32_t a2, char a3, int16_t a4, int16_t a5, int16_t a6, int16_t a7);
//D2Game.0x6FC3F520
void __fastcall D2GAME_SendPacket0x58_6FC3F520(GameClient* pClient, GSPacketSrv58* pPacket);
//D2Game.0x6FC3F550
void __fastcall sub_6FC3F550(GameClient* pClient, UnitAny* pUnit);
//D2Game.0x6FC3F590
void __fastcall D2GAME_PACKETS_SendPacket0x61_6FC3F590(GameClient* pClient, BYTE unk);
//D2Game.0x6FC3F5B0
void __fastcall D2GAME_PACKETS_SendPacket0x63_WaypointMenu_6FC3F5B0(GameClient* pClient, GSPacketSrv63* pPacket);
//D2Game.0x6FC3F5E0
void __fastcall D2GAME_PACKETS_SendPacket0x65_6FC3F5E0(GameClient* pClient, DWORD dwPlayerId, WORD nCount);
//D2Game.0x6FC3F610
void __fastcall sub_6FC3F610(GameClient* pClient, GSPacketSrv73* pPacket);
//D2Game.0x6FC3F640
void __fastcall D2GAME_PACKETS_SendPacket0x74_6FC3F640(GameClient* pClient, UnitAny* a2, UnitAny* a3, BYTE bAssign);
//D2Game.0x6FC3F690
void __fastcall SCMD_Send0x75_PartyRosterUpdate(UnitAny* pLocalPlayer, UnitAny* pOtherPlayer);
//D2Game.0x6FC3F720
void __fastcall D2GAME_PACKETS_SendPacket0x7B_6FC3F720(GameClient* pClient, BYTE slot, int16_t nSkill, int32_t nHand, int32_t nItemGUID);
//D2Game.0x6FC3F770
void __fastcall D2GAME_PACKETS_SendPacket0x7E_6FC3F770(GameClient* pClient);
//D2Game.0x6FC3F790
void __fastcall D2GAME_PACKETS_SendPacket0x82_6FC3F790(Game* pGame, GameClient* pClient, int32_t nPortalGUID, int32_t nPortalOwnerGUID);
//D2Game.0x6FC3F810
void __fastcall D2GAME_PACKETS_SendPacket0x8B_6FC3F810(GameClient* pClient, DWORD dwUnitId, BYTE bParty);
//D2Game.0x6FC3F840
void __fastcall sub_6FC3F840(Game* pGame, int32_t dwPlayerId1, int32_t dwPlayerId2, int16_t wRelationState);
//D2Game.0x6FC3F880
void __fastcall sub_6FC3F880(Game*, UnitAny* pPlayer, void* packet);
//D2Game.0x6FC3F8B0
void __fastcall D2GAME_PACKETS_SendPacket0x8C_6FC3F8B0(UnitAny* pUnit, DWORD dwPlayerId1, DWORD dwPlayerId2, WORD wRelationState);
//D2Game.0x6FC3F8F0
void __fastcall sub_6FC3F8F0(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC3F930
void __fastcall sub_6FC3F930(Game* pGame, UnitAny* pPlayer, void* packet);
//D2Game.0x6FC3F960
void __fastcall D2GAME_PACKETS_SendPacket0x8D_6FC3F960(UnitAny* pUnit, UnitAny* a2);
//D2Game.0x6FC3F9B0
void __fastcall D2GAME_SendPacket0x8E_CorpseAssign_ToAllPlayers_6FC3F9B0(Game* pGame, int32_t nUnitId, int32_t nCorpseId, BYTE bAssign);
//D2Game.0x6FC3F9F0
void __fastcall sub_6FC3F9F0(Game* pGame, UnitAny* pPlayer, void* pPacket);
//D2Game.0x6FC3FA20
void __fastcall D2GAME_PACKETS_SendPacket0x8E_6FC3FA20(GameClient* pClient, int32_t nUnitId, int32_t nCorpseId, BYTE bAssign);
//D2Game.0x6FC3FA50
void __fastcall sub_6FC3FA50(GameClient* pClient, void* a2);
//D2Game.0x6FC3FA60
void __fastcall D2GAME_SendPacket0x8A_6FC3FA60(GameClient* pClient, GSPacketSrv8A* pPacket);
//D2Game.0x6FC3FA90
void __fastcall D2GAME_PACKETS_SendPacket0x8F_6FC3FA90(GameClient* pClient);
//D2Game.0x6FC3FAC0
void __fastcall sub_6FC3FAC0(GameClient* pClient, GSPacketSrv91* pPacket);
//D2Game.0x6FC3FAF0
void __fastcall sub_6FC3FAF0(GameClient* pClient, uint32_t a2, uint16_t a3);
//D2Game.0x6FC3FB30
void __fastcall D2GAME_PACKETS_SendPacket0x9B_6FC3FB30(GameClient* pClient, int16_t a2, int32_t a3);
//D2Game.0x6FC3FB60
void __fastcall D2GAME_PACKETS_SendPacket0x97_6FC3FB60(GameClient* pClient);
//D2Game.0x6FC3FB80
void __fastcall D2GAME_PACKETS_SendPacket0x16_UnitsUpdate(GameClient* pClient);
//D2Game.0x6FC3FC00
void __fastcall D2GAME_PACKETS_SendPacket0xA4_6FC3FC00(GameClient* pClient, int16_t a2);
//D2Game.0x6FC3FC20
void __fastcall D2GAME_PACKETS_SendPacket0xA7_StateOn_6FC3FC20(GameClient* pClient, BYTE nUnitType, DWORD dwUnitId, uint16_t nState);
//D2Game.0x6FC3FC50
void __fastcall D2GAME_PACKETS_SendPacket0xA9_StateOff_6FC3FC50(GameClient* pClient, BYTE nUnitType, DWORD dwUnitId, uint16_t nState);
//D2Game.0x6FC3FC80
void __fastcall sub_6FC3FC80(GameClient* pClient, UnitAny* pUnit);
