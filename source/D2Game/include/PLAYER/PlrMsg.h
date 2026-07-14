#pragma once

#include <Units/Units.h>

#include "D2PacketDef.h"



//D2Game.0x6FC81C00
void __fastcall sub_6FC81C00(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FC81CA0
void __fastcall sub_6FC81CA0(UnitAny* pUnit, GameClient* pClient, char a3);
//D2Game.0x6FC81D20
void __fastcall sub_6FC81D20(Game* pGame, UnitAny* pUnit, GameClient* pClient, int32_t nAnimMode);
//D2Game.0x6FC81E10
void __fastcall sub_6FC81E10(Game* pGame, UnitAny* pUnit, GameClient* pClient, int32_t nAnimMode);
//D2Game.0x6FC81F60
void __fastcall sub_6FC81F60(Game* pGame, UnitAny* pUnit, GameClient* pClient, int32_t nAnimMode);
//D2Game.0x6FC82010
void __fastcall sub_6FC82010(Game* pGame, UnitAny* pUnit, GameClient* pClient, int32_t nAnimMode);
//D2Game.0x6FC820C0
void __fastcall sub_6FC820C0(Game* pGame, UnitAny* pUnit, GameClient* pClient, int32_t nAnimMode);
//D2Game.0x6FC821C0
void __fastcall sub_6FC821C0(Game* pGame, UnitAny* pUnit, GameClient* pClient, int32_t nAnimMode);
//D2Game.0x6FC82270
void __fastcall sub_6FC82270(Game* pGame, UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FC822D0
void __fastcall D2GAME_UpdateAttribute_6FC822D0(UnitAny* pUnit, WORD nStat, uint32_t nNewValue, UnitAny* pPlayer);
//D2Game.0x6FC82360
int32_t __fastcall sub_6FC82360(UnitAny* pUnit, UnitAny* pUnit2, int32_t bSkipHpCheck);
//D2Game.0x6FC82830
void __fastcall sub_6FC82830(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FC828D0
int32_t __fastcall sub_6FC828D0(UnitAny* pPlayer, int32_t nUnitType, int32_t nUnitGUID, int32_t a4, Game* pGame);
//D2Game.0x6FC82CB0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x01_Walk_6FC82CB0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC82D10
int32_t __fastcall sub_6FC82D10(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nPacketSize, uint16_t* pX, uint16_t* pY);
//D2Game.0x6FC82ED0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x02_WalkToEntity_6FC82ED0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC83090
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x03_Run_6FC83090(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC830F0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x04_RunToEntity_6FC830F0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC832B0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x05_ShiftLeftClickSkill_6FC832B0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC83340
int32_t __fastcall sub_6FC83340(Game* pGame, UnitAny* pUnit, Skill* pSkill, int32_t nX, int32_t nY);
//D2Game.0x6FC83450
Skill* __fastcall sub_6FC83450(UnitAny* pUnit, Skill* pSkill);
//D2Game.0x6FC83550
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x06_LeftSkillOnUnit_6FC83550(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC836D0
int32_t __fastcall sub_6FC836D0(Game* pGame, UnitAny* pUnit, Skill* pSkill, int32_t nUnitType, int32_t nUnitGUID, int32_t a6);
//D2Game.0x6FC83890
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x07_ShiftLeftSkillOnUnit_6FC83890(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC83A10
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x08_ShiftLeftSkillHold_6FC83A10(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC83AC0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x09_LeftSkillOnUnitHold_6FC83AC0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC83C60
int32_t __fastcall sub_6FC83C60(UnitAny* pUnit, uint16_t nX, uint16_t nY, int32_t nMaxDistance);
//D2Game.0x6FC83D00
int32_t __fastcall sub_6FC83D00(UnitAny* pUnit, UnitAny* a2);
//D2Game.0x6FC83D40
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x0A_ShiftLeftSkillOnUnitHold_6FC83D40(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC83EE0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x0B_6FC83EE0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC83EF0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x0C_RightSkill_6FC83EF0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC83F80
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x0D_RightSkillOnUnit_6FC83F80(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84100
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x0E_ShiftRightSkillOnUnit_6FC84100(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84280
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x0F_RightSkillHold_6FC84280(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84330
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x10_RightSkillOnUnitHold_6FC84330(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC844D0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x11_ShiftRightSkillOnUnitHold_6FC844D0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84670
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x12_6FC84670(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84690
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x14_HandleOverheadChat_6FC84690(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC847B0
void __fastcall D2GAME_PACKETS_HandlePlayerMessage_6FC847B0(GameClient* pClient, void* pArgs);
//D2Game.0x6FC848A0
void __fastcall D2GAME_PACKETS_HandleWhisper_6FC848A0(GameClient* pClient, void* pArgs);
//D2Game.0x6FC84940
void __fastcall j_D2GAME_PACKETS_SendPacket0x26_ServerMessage_6FC3DDF0(GameClient* pClient, void* pMsg);
//D2Game.0x6FC84950
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x15_HandleChatMessage_6FC84950(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84C70
void __fastcall sub_6FC84C70(Game* pGame, const char* szMessage, uint8_t nColor);
//D2Game.0x6FC84CD0
void __fastcall sub_6FC84CD0(Game* pGame, const char* szMessage, uint8_t nColor);
//D2Game.0x6FC84D30
void __fastcall j_D2GAME_SendPacket0x5A_6FC3DEC0(GameClient* pClient, void* pPacket);
//D2Game.0x6FC84D40
void __fastcall sub_6FC84D40(Game* pGame, GSPacketSrv5A* pPacket5A);
//D2Game.0x6FC84D70
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x13_InteractWithEntity_6FC84D70(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84DB0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x16_PickItemOnGround_6FC84DB0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84E20
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x17_DropItemOnGround_6FC84E20(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC84ED0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x18_InsertItemInBuffer_6FC84ED0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC850C0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x19_RemoveItemFromBuffer_6FC850C0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85280
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x1A_EquipItem_6FC85280(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC853C0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x1B_Swap2HandedItem_6FC853C0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC854B0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x1C_RemoveBodyItem_6FC854B0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85550
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x1D_SwapCursorItemWithBody_6FC85550(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85690
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x1E_SwapTwo1HandedWithOne2HandedItem_6FC85690(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC857C0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x1F_SwapCursorBufferItems_6FC857C0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85990
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x20_UseGridItem_6FC85990(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85B50
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x27_UseItemAction_6FC85B50(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85C80
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x21_StackItems_6FC85C80(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85DA0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x22_UnstackItems_6FC85DA0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85E70
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x23_ItemToBelt_6FC85E70(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC85F50
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x24_ItemFromBelt_6FC85F50(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86040
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x25_SwitchBeltItem_6FC86040(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86150
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x26_UseBeltItem_6FC86150(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86310
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x28_SocketItem_6FC86310(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86450
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x29_ScrollToBook_6FC86450(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86520
int32_t __fastcall sub_6FC86520(Game* pGame, UnitAny* pPlayer, int32_t nItemGUID);
//D2Game.0x6FC866E0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x2A_ItemToCubeIndirect_6FC866E0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC867B0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x2C_2D_52_6FC867B0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC867C0
void __fastcall D2GAME_PlayerChangeAct_6FC867C0(Game* pGame, UnitAny* pUnit, DWORD dwDestLvl, DWORD nTileCalc);
//D2Game.0x6FC868C0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x2F_InitEntityChat_6FC868C0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC869F0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x30_TerminateEntityChat_6FC869F0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86AB0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x31_QuestMessage_6FC86AB0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86AE0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x32_BuyItemFromNpcBuffer_6FC86AE0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86B30
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x33_SellItemToNpcBuffer_6FC86B30(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86B70
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x35_Repair_6FC86B70(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86BA0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x34_IdentifyItemsWithNpc_6FC86BA0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86BD0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x36_HireMerc_6FC86BD0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86C00
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x62_ResurrectMerc_6FC86C00(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86C30
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x37_IdentifyFromGamble_6FC86C30(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86CB0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x38_EntityAction_6FC86CB0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86E50
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x39_PurchaseLife_6FC86E50(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86E80
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x3A_AddStatPoint_6FC86E80(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC86EF0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x3B_AddSkillPoint_6FC86EF0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87050
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x3C_SelectSkill_6FC87050(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC870F0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x3D_HighlightADoor_6FC870F0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87270
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x3E_ActivateInifussScroll_6FC87270(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87410
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x3F_PlayAudio_6FC87410(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87460
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x40_RequestQuestData_6FC87460(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87480
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x41_Resurrect_6FC87480(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC876B0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x5D_SquelchHostile_6FC876B0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87720
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x5E_InviteToParty_6FC87720(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87780
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x44_StaffInOrifice_6FC87780(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87930
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x45_ChangeTpLocation_6FC87930(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87B00
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x46_MercInteract_6FC87B00(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87CE0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x47_MoveMerc_6FC87CE0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87E20
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x48_TurnOffBusyState_6FC87E20(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC87E60
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x49_TakeOrCloseWp_6FC87E60(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nPacketSize);
//D2Game.0x6FC880A0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x4D_PlayNpcMessage_6FC880A0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC880F0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x4B_RequestEntityUpdate_6FC880F0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88170
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x4C_Transmogrify_6FC88170(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC881D0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x4F_ClickButton_6FC881D0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88210
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x50_DropGold_6FC88210(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88290
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x51_BindHotkeyToSkill_6FC88290(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88340
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x53_TurnStaminaOn_6FC88340(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88380
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x54_TurnStaminaOff_6FC88380(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC883B0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x58_QuestCompleted_6FC883B0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88400
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x59_MakeEntityMove_6FC88400(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88530
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x5F_UpdatePlayerPos_6FC88530(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC888A0
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x60_SwapWeapons_6FC888A0(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88930
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x61_DropPickupMercItem_6FC88930(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC88D10
int32_t __fastcall D2GAME_MERCS_EquipItem_6FC88D10(Game* pGame, UnitAny* pPlayer, UnitAny* pMerc, UnitAny* pItem);
//D2Game.0x6FC88F80
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x63_ShiftLeftClickItemToBelt_6FC88F80(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC89310
int32_t __fastcall D2GAME_PACKETCALLBACK_Rcv0x2E_42_43_64_65_6FC89310(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC89320
int32_t __fastcall D2GAME_PACKET_Handler_6FC89320(Game* pGame, UnitAny* pUnit, void* pPacket, int32_t nSize);
//D2Game.0x6FC89450
void __fastcall D2GAME_PLRMSG_Last_6FC89450(Game* pGame, UnitAny* pAttacker, UnitAny* pDefender);
