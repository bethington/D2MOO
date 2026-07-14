#pragma once

#include <Units/Units.h>

#include <GAME/Game.h>

//D2Game.0x6FC7F000
void __fastcall PLRINTRO_SetQuestIntroFlag(UnitAny* pUnit, Game* pGame, int32_t nNpcId);
//D2Game.0x6FC7F060
int32_t __fastcall PLRINTRO_GetQuestIntroFlag(UnitAny* pUnit, Game* pGame, int32_t nNpcId);
//D2Game.0x6FC7F0C0
void __fastcall PLRINTRO_SetNpcIntroFlag(PlrIntro* pPlayerIntro, int32_t nNpcId);
//D2Game.0x6FC7F100
int32_t __fastcall PLRINTRO_GetNpcIntroFlag(PlrIntro* pPlayerIntro, int32_t nNpcId);
//D2Game.0x6FC7F140
void __fastcall PLRINTRO_MaskNpcIntroFlag(Game* pGame, UnitAny* pPlayer, int32_t nNpcId);
//D2Game.0x6FC7F1A0
void __fastcall PLRINTRO_CopyQuestIntroFlagsToBuffer(PlrIntro* pPlayerIntro, uint8_t* pBuffer);
//D2Game.0x6FC7F1B0
void __fastcall PLRINTRO_CopyNpcIntroFlagsToBuffer(PlrIntro* pPlayerIntro, uint8_t* pBuffer);
//D2Game.0x6FC7F1C0
void __fastcall PLRINTRO_CopyBufferToQuestIntroFlags(PlrIntro* pPlayerIntro, uint8_t* pBuffer);
//D2Game.0x6FC7F1D0
void __fastcall PLRINTRO_CopyBufferToNpcIntroFlags(PlrIntro* pPlayerIntro, uint8_t* pBuffer);
//D2Game.0x6FC7F1E0
PlrIntro* __fastcall PLRINTRO_Create(Game* pGame);
//D2Game.0x6FC7F2A0
void __fastcall PLRINTRO_Destroy(Game* pGame, PlrIntro* pPlayerIntro);
