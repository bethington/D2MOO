#pragma once

#include <Units/Units.h>
#include <OBJECTS/ObjMode.h>
#include <D2DataTbls.h>

#pragma pack(1)

struct CubeItem
{
	UnitAny* pItem;						//0x00
	int32_t nClassId;						//0x04
	int32_t nItemLevel;						//0x08
};

typedef BOOL(__fastcall* SPECIALCUBEFN)(Game*, UnitAny*);

struct CubeTable
{
	SPECIALCUBEFN pFunc;					//0x00
};

#pragma pack()

//D2Game.0x6FC8F3D0
int32_t __fastcall OBJECTS_OperateFunction32_Bank(ObjOperateFn* pOp, int32_t nOperate);
//D2Game.0x6FC8F450
int32_t __fastcall PLRTRADE_CheckCubeInput(Game* pGame, Inventory* pInventory, CubeMainTxt* pCubeMainTxt, int32_t nInputIndex, CubeItem* pCubeItem, int32_t* a6);
//D2Game.0x6FC8FE40
int16_t __fastcall PLRTRADE_RollRandomItemClassOfSameType(Game* pGame, int32_t nItemLevel, int32_t nItemId);
//D2Game.0x6FC90010
void __fastcall PLRTRADE_CreateCubeOutputs(Game* pGame, UnitAny* pUnit, CubeMainTxt* pCubeMainTxt, CubeItem* pCubeItem);
//D2Game.0x6FC90A60
void __fastcall PLRTRADE_Free(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC90AE0
void __fastcall sub_6FC90AE0(Game* pGame, UnitAny* pUnit, int32_t a3);
//D2Game.0x6FC90BE0
void __fastcall sub_6FC90BE0(Game* pGame, UnitAny* pPlayer, void* pArg);
//D2Game.0x6FC90C20
void __fastcall sub_6FC90C20(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC90D70
void __fastcall PLRTRADE_SendEventPacketToPlayer(UnitAny* pPlayer, SRV2CLT5A_TYPES nType, char* szSource);
//D2Game.0x6FC90DE0
void __fastcall PLRTRADE_TryToTrade(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2);
//D2Game.0x6FC91050
void __fastcall sub_6FC91050(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, PlayerData* pPlayerData1, PlayerData* pPlayerData2, int32_t nTradeState);
//D2Game.0x6FC91250
int32_t __fastcall sub_6FC91250(Game* pGame, UnitAny* pPlayer, uint16_t nButton, int32_t nGoldAmount);
//D2Game.0x6FC92130
void __fastcall PLRTRADE_HandleCubeInteraction(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC927D0
int32_t __fastcall PLRTRADE_AllocPlayerTrade(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC92890
void __fastcall PLRTRADE_AddGold(UnitAny* pUnit, int32_t nStat, int32_t nValue);
//D2Game.0x6FC92920
void __fastcall sub_6FC92920(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2, PlayerData* pPlayerData1, PlayerData* pPlayerData2);
//D2Game.0x6FC92A90
int32_t __fastcall sub_6FC92A90(Game* pGame, UnitAny* pPlayer1, UnitAny* pPlayer2);
//D2Game.0x6FC92CF0
void __fastcall PLRTRADE_StopAllPlayerInteractions(Game* pGame, UnitAny* pPlayer);
//D2Game.0x6FC92EE0
void __fastcall sub_6FC92EE0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC92EF0
void __fastcall sub_6FC92EF0(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC92F10
void __fastcall sub_6FC92F10(Game* pGame, UnitAny* pPlayer, UnitAny* pItem);
//D2Game.0x6FC931D0
void __fastcall sub_6FC931D0(Game* pGame, UnitAny* pPlayer, UnitAny* pItem);
//D2Game.0x6FC933F0
int32_t __fastcall PLRTRADE_CopyTradeSaveDataToBuffer(UnitAny* pUnit, uint8_t* pBuffer, int32_t nBufferSize);
//D2Game.0x6FC93430
int32_t __fastcall sub_6FC93430(Game* pGame, UnitAny* pPlayer, UnitAny* pItem);
//D2Game.0x6FC93740
int32_t __fastcall sub_6FC93740(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC937A0
int32_t __fastcall D2GAME_PLRTRADE_IsInteractingWithPlayer(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC937F0
void __fastcall D2GAME_PLRTRADE_Last_6FC937F0(Game* pGame, UnitAny* pPlayer, int32_t a3, int32_t a4);
