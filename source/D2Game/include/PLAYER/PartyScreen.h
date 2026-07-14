#pragma once

#include <Units/Units.h>


//D2Game.0x6FC7AB50
void __fastcall PARTYSCREEN_ToggleLootability(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, int32_t nParam, int32_t* pFailure);
//D2Game.0x6FC7AC20
void __fastcall PARTYSCREEN_SendEventUpdateToClient(GameClient* pClient, int32_t nUnitGUID, uint8_t nType);
//D2Game.0x6FC7AD10
void __fastcall PARTYSCREEN_ToggleIgnore(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, int32_t nParam, int32_t* pFailure);
//D2Game.0x6FC7AD70
void __fastcall PARTYSCREEN_ToggleSquelch(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, int32_t nParam, int32_t* pFailure);
//D2Game.0x6FC7ADD0
void __fastcall D2GAME_PARTYSCREEN_Command8_6FC7ADD0(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, int32_t nParam, int32_t* pFailure);
//D2Game.0x6FC7AED0
void __fastcall sub_6FC7AED0(Game* pGame, UnitAny* pLocalPlayer, void* a3);
//D2Game.0x6FC7B030
void __fastcall PARTYSCREEN_InvitationCancelled(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, int32_t nParam, int32_t* pFailure);
//D2Game.0x6FC7B0E0
void __fastcall PARTYSCREEN_ReceivedInvitation(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, int32_t nParam, int32_t* pFailure);
//D2Game.0x6FC7B160
void __fastcall PARTYSCREEN_PlayerLeftParty(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, int32_t nParam, int32_t* pFailure);
//D2Game.0x6FC7B190
void __fastcall PARTYSCREEN_PartyIterate_PlayerLeftParty(Game* pGame, UnitAny* pPlayer1, void* pArg);
//D2Game.0x6FC7B1D0
void __fastcall PARTYSCREEN_ToggleHostile(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, int32_t nParam, int32_t* pFailure);
//D2Game.0x6FC7B3F0
void __fastcall sub_6FC7B3F0(Game* pGame, UnitAny* pPlayer1, void* pArg);
//D2Game.0x6FC7B450
void __fastcall sub_6FC7B450(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC7B460
void __fastcall sub_6FC7B460(Game* pGame, UnitAny* pPlayer1, void* pArg);
//D2Game.0x6FC7B4C0
int32_t __fastcall PARTYSCREEN_CommandHandler(Game* pGame, UnitAny* pUnit, int32_t nCallbackId, int32_t nOtherPlayerGUID, int32_t nParam);
