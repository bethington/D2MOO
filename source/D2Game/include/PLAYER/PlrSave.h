#pragma once

#include <Units/Units.h>
#include "PlrSave2.h"

#pragma pack(push, 1)
struct SavedItem
{
    uint8_t nBodyLoc;
    uint8_t unk0x01;
    int32_t nItemLevel;
    int32_t nItemId;
    uint8_t nSpawnType;
    uint8_t nItemQuality;
    int32_t nQuantity;
    int32_t nMinDurability;
    int32_t nMaxDurability;
    int32_t nX;
    int32_t nY;
    int32_t nItemIndex;
    int32_t nItemFlags;
    int32_t nSeed;
    int32_t nItemSeed;
    uint8_t nInvPage;
    uint8_t nEarLevel;
    char szName[16];
    uint16_t nItemFormat;
    uint32_t nItemCode;
};
#pragma pack(pop)


//D2Game.0x6FC895D0
void __fastcall D2GAME_10036_PLRSAVE_EnableSaveFileWriting(int32_t bWriteSaveFile);
//D2Game.0x6FC895E0
int32_t __fastcall sub_6FC895E0(Game* pGame, UnitAny* pPlayer, SavedItem* pSavedItem, UnitAny** ppItem, int32_t* a5);
//D2Game.0x6FC897F0
UnitAny* __fastcall sub_6FC897F0(Game* pGame, SavedItem* pSavedItem);
//D2Game.0x6FC898F0
int32_t __fastcall sub_6FC898F0(Game* pGame, UnitAny* pPlayer, UnitAny* pItem, UnitAny* a4);
//D2Game.0x6FC89AB0
int32_t __stdcall D2GAME_10029_SAVE_WriteFileInterface(Game* pGame, UnitAny* pPlayer, const char* szName);
//D2Game.0x6FC89AD0
int32_t __stdcall D2GAME_SAVE_SerializeItem_6FC89AD0(UnitAny* pItem, uint8_t* pBitstream, uint32_t nBitstreamSize, int32_t n0, int32_t a5);
//D2Game.0x6FC89B50
int32_t __stdcall PLRSAVE_WriteItems_6FC89B50(Inventory* pInventory, uint8_t* pBitstream, uint32_t nBitstreamSize, int32_t a4, int32_t a5);
//D2Game.0x6FC8A0F0
int32_t __fastcall sub_6FC8A0F0(Game* pGame, UnitAny* pUnit, uint8_t* pSection, int32_t nSize, int32_t a5, int32_t a6);
//D2Game.0x6FC8A140
int32_t __fastcall D2GAME_SAVE_CalculateChecksum_6FC8A140(SaveHeader* pSaveHeader, int32_t nSize);
//D2Game.0x6FC8A1B0
int32_t __fastcall D2GAME_SAVE_WriteFileOnRealm_6FC8A1B0(Game* pGame, UnitAny* pPlayer, const char* szCharName, char* szAccountName, int32_t bInteractsWithPlayer, int32_t a6, int32_t a7, int32_t nRealmId);
//D2Game.0x6FC8A500
int32_t __fastcall D2GAME_SAVE_WriteFile_6FC8A500(Game* pGame, UnitAny* pPlayer, const char* szName, DWORD dwArg);
//D2Game.0x6FC8A780
int32_t __fastcall sub_6FC8A780(Game* pGame, GameClient* pClient, uint8_t* pSavefile, UnitAny** ppPlayer, int32_t* pValid, int32_t* pVersion, int32_t* a7, int32_t* a8, int32_t* a9, int32_t* a10, int32_t* a11);
//D2Game.0x6FC8AD50
int32_t __fastcall D2GAME_SAVE_ReadWaypointData_6FC8AD50(UnitAny* pUnit, uint8_t* pSection, int32_t nUnused, int32_t* pSize);
//D2Game.0x6FC8ADE0
int32_t __fastcall sub_6FC8ADE0(Game* pGame, UnitAny* pUnit, uint8_t* pSection, int32_t a4, int32_t nUnused, int32_t* pSize);
//D2Game.0x6FC8AEC0
int32_t __fastcall sub_6FC8AEC0(Game* pGame, GameClient* pClient, UnitAny* pUnit, BYTE* pSection, int32_t a5, int32_t a6, int32_t* pSkillCount);
//D2Game.0x6FC8AF70
size_t __fastcall sub_6FC8AF70(void* a1, BYTE* a2);
//D2Game.0x6FC8B3D0
int32_t __fastcall sub_6FC8B3D0(Game* pGame, UnitAny* pPlayer, BYTE* pSection, uint32_t dwVersion, int32_t nSize, int32_t a6, int32_t* pSize);
//D2Game.0x6FC8B680
int32_t __fastcall sub_6FC8B680(Game* pGame, UnitAny* pUnit, SavedItem* pSavedItem, UnitAny** ppItem, int32_t* a5);
//D2Game.0x6FC8B8A0
int32_t __fastcall PLRSAVE_ReadItems_6FC8B8A0(Game* pGame, UnitAny* pPlayer, uint8_t* pSection, uint32_t dwVersion, int32_t nSize, int32_t a6, int32_t* pSize);
//D2Game.0x6FC8BAA0
int32_t __fastcall sub_6FC8BAA0(Game* pGame, UnitAny* pPlayer, UnitAny* pItem, UnitAny* a4);
//D2Game.0x6FC8BC70
int32_t __fastcall sub_6FC8BC70(Game* pGame, UnitAny* pUnit, uint8_t* pSection, uint32_t dwVersion, int32_t nSize, int32_t a6, int32_t* pSize);
//D2Game.0x6FC8BCC0
int32_t __fastcall sub_6FC8BCC0(Game* pGame, UnitAny* pPlayer, uint8_t* pSection, uint32_t dwVersion, int32_t nRemainingSize, int32_t* pSize);
//D2Game.0x6FC8BEE0
int32_t __fastcall sub_6FC8BEE0(int16_t nHirelingId, int32_t nLevel);
//D2Game.0x6FC8C050
uint32_t __fastcall sub_6FC8C050(Game* pGame, int16_t nHirelingId, uint32_t a3);
//D2Game.0x6FC8C0C0
int32_t __fastcall D2GAME_SAVE_ProcessSaveFile_6FC8C0C0(Game* pGame, GameClient* pClient, uint8_t* pSaveFile, int32_t nSize, UnitAny** ppPlayer, int32_t nUnused1, int32_t nUnused2, int32_t nUnused3);
//D2Game.0x6FC8C890
int32_t __fastcall sub_6FC8C890(Game* pGame, GameClient* pClient, UnitAny** ppPlayer, int32_t a4, int32_t a5, int32_t a6);
//D2Game.0x6FC8C9D0
int32_t __fastcall D2GAME_SAVE_ReadFile_6FC8C9D0(Game* pGame, GameClient* pClient, const char* szName, UnitAny** ppPlayer, DWORD dw1, DWORD dw2, DWORD dw3);
//D2Game.0x6FC8CB40
int32_t __fastcall D2GAME_SAVE_GetUnitDataFromFile_6FC8CB40(Game* pGame, GameClient* pClient, const char* szName, int32_t a4, UnitAny** ppPlayer, Room1* pRoomArg, int32_t nXArg, int32_t nYArg);
