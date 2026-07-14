#pragma once

#include "D2PacketDef.h"
#include <Units/Units.h>
#include "Game.h"
#include "D2Packet.h"

struct SaveHeader;

#pragma pack(1)
enum SystemError
{
	SYSERROR_SUCCESS = 0,
	SYSERROR_BAD_INPUT = 1,
	SYSERROR_UNK_1 = 1,
	SYSERROR_UNK_2 = 2,
	SYSERROR_BAD_WAYPOINT = 3,
	SYSERROR_BAD_STATS = 4,
	SYSERROR_BAD_SKILLS = 5,
	SYSERROR_UNK_6 = 6,
	SYSERROR_UNK_7 = 7,
	SYSERROR_CORPSES = 8,
	SYSERROR_UNK_9 = 9,
	SYSERROR_UNK_10 = 10,
	SYSERROR_UNK_11 = 11,
	SYSERROR_UNK_12 = 12,
	SYSERROR_INVENTORY = 13,
	SYSERROR_UNK_14 = 14,
	SYSERROR_NIGHTMARE_NOT_UNLOCKED = 17,
	SYSERROR_HELL_NOT_UNLOCKED = 18,
	SYSERROR_SOFTCOREJOINHARDCORE = 19,
	SYSERROR_HARDCOREJOINSOFTCORE = 20,
	SYSERROR_DEADHARDCORE = 21,
	SYSERROR_EXPANSIONGAME = 23,
	SYSERROR_NOTEXPANSIONGAME = 24,
	SYSERROR_LADDERGAME = 25,
	SYSERROR_NOTLADDERGAME = 26,
};

enum ClientState : uint32_t
{
	CLIENTSTATE_JUST_CREATED = 0,
	CLIENTSTATE_GAME_INIT_SENT = 1,
	CLIENTSTATE_ACT_INIT_SENT = 2,
	CLIENTSTATE_PLAYER_SPAWNED = 3,
	CLIENTSTATE_INGAME = 4,
	CLIENTSTATE_CHANGING_ACT = 5,
};


// Character information flags
enum ClientSaveFlags
{
	CLIENTSAVEFLAG_INIT = 0x1, // Newbie save
	CLIENTSAVEFLAG_0x2 = 0x2, // Set at character creation for realm characters.
	CLIENTSAVEFLAG_HARDCORE = 0x4,
	CLIENTSAVEFLAG_DEAD = 0x8,
	CLIENTSAVEFLAG_0x10 = 0x10,
	CLIENTSAVEFLAG_EXPANSION = 0x20,
	CLIENTSAVEFLAG_LADDER = 0x40,
	CLIENTSAVEFLAG_0x80 = 0x80,
	CLIENTSAVEFLAG_WEAPON_SWITCH = 0x2000,

	// Encodes completed acts 
	// => 0 No act completed
	// => Acts 1-5 Normal (values 1-5), then 1-5 NM (values 6-10), then 1-5 Hell (values 11-15) for Expansion
	// => Acts 1-4 Normal (values 1-4), then 1-4 NM (values 5-8), then 1-4 Hell  (values 9-12) for Classic
	CLIENTSAVEFLAG_CHARACTER_PROGRESSION_BIT = 8,
	CLIENTSAVEFLAG_CHARACTER_PROGRESSION_MASK = (0x1F) << CLIENTSAVEFLAG_CHARACTER_PROGRESSION_BIT,
};

union PackedClientSaveFlags
{
	uint16_t nPackedValue;			// ClientSaveFlags
	struct {
		uint16_t bInit : 1;			// BIT(0)
		uint16_t bUnkFlag0x02 : 1;	// BIT(1)
		uint16_t bHardcore : 1;		// BIT(2)
		uint16_t bDead : 1;			// BIT(3)
		uint16_t bUnkFlag0x10 : 1;	// BIT(4)
		uint16_t bExpansion : 1;	// BIT(5)
		uint16_t bLadder : 1;		// BIT(6)
		uint16_t bUnkFlag0x80 : 1;	// BIT(7)
		// Encodes completed acts
		// => 0 No act completed
		// => Acts 1-5 Normal (values 1-5), then 1-5 NM (values 6-10), then 1-5 Hell (values 11-15) for Expansion
		// => Acts 1-4 Normal (values 1-4), then 1-4 NM (values 5-8), then 1-4 Hell  (values 9-12) for Classic
		uint16_t nProgression : 5;	// BIT(8-12)
		uint16_t bWeaponSwitch : 1;	// BIT(13)
		uint16_t nDONOTUSEBITS : 2; // BIT(14-15) Will not be propagated to character preview info string due to encoding with FOG_Encode14BitsToString
	};
};

// Internal management flags
enum ClientFlagsEx
{
	CLIENTFLAGEX_PLAYER_UNIT_ALIVE	= 0x01,
	CLIENTFLAGEX_ARENA_RELATED		= 0x04,
	CLIENTFLAGEX_HAS_SAVE_CHECKSUM	= 0x08,
	CLIENTFLAGEX_SAVE_LOADED		= 0x10,
};

enum ClientsConstants
{
	CLIENTS_MAX_UPDATES = 55,
};

struct ClientInfo
{
	ClientInfo* pSelf;				//0x00
	int32_t dwClientId;							//0x04
	uint32_t nFlags;					//0x08
	const char* szKickMessage;				//0x0C
	int32_t unk0x10;							//0x10
	uint32_t dwLastPacketResetTick;		//0x14
	uint32_t nPacketsPerSecond;			//0x18
	uint32_t dwNewGameTick;				//0x1C
	uint32_t dwRemoveTick;				//0x20
	int32_t unk0x24;							//0x24
	int32_t unk0x28;							//0x28
	uint32_t dwHackDetectionPacketTick;	//0x2C
	uint32_t nACDataCount;				//0x30
	int32_t unk0x34;							//0x34
	int32_t unk0x38;							//0x38
	uint32_t unk0x3C;					//0x3C
};

struct ClientPlayerData
{
	uint32_t dwInactivityTime;					//0x00
	uint16_t nHitPoints;						//0x04
	uint16_t nManaPoints;						//0x06
	uint16_t nStaminaPoints;					//0x08
	uint8_t nPotionLifePercent;					//0x0A
	uint8_t nPotionManaPercent;					//0x0B
	uint16_t nPosX;								//0x0C
	uint16_t nPosY;								//0x0E
	uint16_t nTargetOffsetX;					//0x10
	uint16_t nTargetOffsetY;					//0x12
	uint32_t dwBeltGold;						//0x14
	uint32_t dwExperience;						//0x18
};

struct ClientUnitUpdateSort
{
	UnitAny* pUnit;							//0x00
	int32_t nDistance;							//0x04
	int32_t nNextIndex;							//0x08
};

struct ClientKeySkill
{
	int16_t nSkill;								//0x00
	int16_t nHand;								//0x02
	uint32_t nItemGUID;							//0x04
};

struct GuildInformation
{
	int16_t nGuildFlags;						//0x00
	uint32_t szGuildTag;						//0x02
	char szGuildName[28];						//0x06
	uint8_t nBackgroundColor;					//0x22
	uint8_t nForegroundColor;					//0x23
	uint8_t nEmblemType;						//0x24
};

struct GameClient
{
	uint32_t dwClientId;						//0x000
	ClientState dwClientState;				//0x004
	uint8_t nClassId;							//0x008
	uint8_t unk0x09;							//0x009
	PackedClientSaveFlags tSaveFlags;			//0x00A
	uint8_t nCharTemplate;						//0x00C
	char szName[16];							//0x00D
	char szAccount[16];							//0x01D
	uint8_t unk0x2D[51];						//0x02D
	int32_t nCharSaveTransactionToken;			//0x060
	int32_t unk0x64;							//0x064
	ClientInfo* pClientInfo;				//0x068
	CharacterPreviewInfo tCharacterInfo;	//0x06C
	uint8_t unk0x92[218];						//0x092
	uint32_t dwUnitType;						//0x16C
	D2UnitGUID dwUnitGUID;						//0x170
	UnitAny* pPlayer;						//0x174
	uint32_t bUnlockCharacter;					//0x178
	SaveHeader* pSaveHeader;				//0x17C
	int32_t nSaveHeaderSize;					//0x180
	uint32_t unk0x184;							//0x184
	uint32_t nSaveHeaderDataSentBytes;			//0x188
	DWORD nSaveChecksum;						//0x18C
	FILETIME nSaveCreationTimestamp;			//0x190
	DWORD unk0x198[4];							//0x198
	Game* pGame;							//0x1A8
	uint8_t nAct;								//0x1AC
	uint8_t pad0x1AD[3];						//0x1AD
	uint32_t unk0x1B0;							//0x1B0
	Room1* pRoom;					//0x1B4
	struct PacketList {
		PacketData* pHead;				//0x1B8
		PacketData* pTail;				//0x1BC
		PacketData* pPacketDataPool;		//0x1C0
	} tPacketDataList;
	uint32_t nSaveHeaderSendFailures;			//0x1C4
	uint32_t unk0x1C8;							//0x1C8
	ClientUnitUpdate unitUpdate[CLIENTS_MAX_UPDATES]; //0x1CC
	uint8_t pad0x3BB;							//0x3BB
	uint32_t nUnitUpdateIndex;					//0x3BC
	int32_t aLastWarpAttemptsFrame[5];			//0x3C0
	uint32_t dwFlags;							//0x3D4 ClientFlagsEx
	uint32_t dwLastPacketTick;					//0x3D8
	ClientKeySkill HotkeySkills[16];		//0x3DC
	uint8_t bSwitchWeapon;						//0x45C
	uint8_t padding0x45D;						//0x45D
	GuildInformation tGuildInfo;			//0x45E
	uint8_t padding0x483;						//0x483
	int32_t nIronGolemItemGUID;					//0x484
	uint32_t nCreateTime;						//0x488
	ClientPlayerData clientPlayerData;	//0x48C
	GameClient* pNext;						//0x4A8
	GameClient* pServerNext;					//0x4AC
	GameClient* pServerNextByName;			//0x4B0
	uint32_t aPingHistory[17];					//0x4B4
	uint64_t nPingMovingAverage;				//0x4F8
	uint32_t dwPingsCount;						//0x500
	uint32_t unk0x504;							//0x504
	int32_t nExpLoss;							//0x508
	uint32_t dwLocale;							//0x50C
	uint32_t dwLangId;							//0x510
	uint32_t pad0x514;							//0x514
};

#pragma pack()

//D2Game.0x6FC31CD0
void __fastcall CLIENTS_Initialize();
//D2Game.0x6FC31D50
void __fastcall CLIENTS_Release();
//D2Game.0x6FC31D80
int32_t __stdcall CLIENTS_GetExpansionClientCount();
//D2Game.0x6FC31DE0
GameClient* __fastcall CLIENTS_GetClientFromClientId(Game* pGame, int32_t nClientId);
//D2Game.0x6FC31E20
UnitAny* __stdcall CLIENTS_GetPlayerFromClient(GameClient* pClient, BOOL bIgnoreDeath);
//D2Game.0x6FC31EA0
void __fastcall CLIENTS_SetPlayerInClient(GameClient* pClient, UnitAny* pUnit);
//D2Game.0x6FC31EF0
void __fastcall sub_6FC31EF0(GameClient* pClient, UnitAny* pPlayer, Game* pGame, Room1* pRoomArg, int32_t nXArg, int32_t nYArg);
//D2Game.0x6FC32220
void __fastcall sub_6FC32220(GameClient* pClient);
//D2Game.0x6FC32260
int32_t __fastcall CLIENTS_AddPlayerToGame(GameClient* pClient, Game* pGame, int32_t a3, Room1* pRoomArg, int32_t nXArg, int32_t nYArg);
//1.10f: D2Game.0x6FC325E0
//1.13c: D2Game.0x6FC6A9B0
GameClient* __fastcall CLIENTS_AddToGame(Game* pGame, int32_t nClientId, uint8_t nClassIdOrCharTemplate, const char* szClientName, const char* szAccount, int32_t nCharSaveTransactionToken, uint32_t nLocale, int32_t a8, int32_t a9);
//D2Game.0x6FC327E0
void __fastcall CLIENTS_SetGameData(Game* pGame);
//D2Game.0x6FC32810
void __fastcall CLIENTS_FillCharacterPreviewInfo(GameClient* pClient, CharacterPreviewInfo* pCharacterPreviewInfo);
//D2Game.0x6FC32A30
void __fastcall CLIENTS_RemoveClientFromGame(Game* pGame, int32_t nClientIdToRemove, BOOL bTriggerSave);
//D2Game.0x6FC32FE0
void __fastcall CLIENTS_FreeClientsFromGame(Game* pGame);
//D2Game.0x6FC33020
void __fastcall sub_6FC33020(GameClient* pClient, Room1* pRoom);
//D2Game.0x6FC33210
void __fastcall CLIENTS_RefreshUnitsUpdateList(GameClient* pClient, uint32_t nUpdateSize);
//D2Game.0x6FC33510
int32_t __fastcall CLIENTS_GetUnitX(UnitAny* pUnit);
//D2Game.0x6FC33540
int32_t __fastcall CLIENTS_GetUnitY(UnitAny* pUnit);
//D2Game.0x6FC33570
void __fastcall CLIENTS_SetUnitsUpdateList(GameClient* pClient, ClientUnitUpdateSort* pSort);
//D2Game.0x6FC33670
void __fastcall sub_6FC33670(Game* pGame, GameClient* pClient);
//D2Game.0x6FC337B0
int32_t __fastcall CLIENTS_IsInGame(Game* pGame, int32_t nClientId);
//D2Game.0x6FC337E0
void __fastcall CLIENTS_SetRoomInClient(GameClient* pClient, Room1* pRoom);
//D2Game.0x6FC33830
void __fastcall D2GAME_SetClientDead_6FC33830(GameClient* pClient, void* pAlwaysNull);
//D2Game.0x6FC33890
uint8_t __fastcall CLIENTS_GetActNo(GameClient* pClient);
//D2Game.0x6FC338C0
void __fastcall CLIENTS_SetActNo(GameClient* pClient, uint8_t nAct);
//D2Game.0x6FC33910
void __fastcall sub_6FC33910(GameClient* pClient);
//D2Game.0x6FC33940
uint32_t __fastcall D2GAME_GetSaveChecksumFromClient_6FC33940(GameClient* pClient);
//D2Game.0x6FC33970
void __fastcall D2GAME_SetSaveFileChecksum_6FC33970(GameClient* pClient, uint32_t a2);
//D2Game.0x6FC339A0
int32_t __fastcall CLIENTS_GetClientId(GameClient* pClient);
//D2Game.0x6FC339E0
int32_t __fastcall CLIENTS_GetClassId(GameClient* pClient);
//D2Game.0x6FC33A10
void __fastcall CLIENTS_SetClassId(GameClient* pClient, int32_t nClass);
//D2Game.0x6FC33A20
void __fastcall CLIENTS_SetFlags(GameClient* pClient, int32_t nFlags);
//D2Game.0x6FC33A30
int32_t __fastcall CLIENTS_GetFlags(GameClient* pClient);
//D2Game.0x6FC33A40
void __fastcall CLIENTS_ToggleFlag(GameClient* pClient, uint16_t nFlag, int32_t bSet);
//D2Game.0x6FC33A60
int32_t __fastcall CLIENTS_CheckFlag(GameClient* pClient, uint16_t nFlag);
//D2Game.0x6FC33A70
void __fastcall CLIENTS_UpdateCharacterProgression(GameClient* pClient, uint16_t nAct, uint16_t nDifficulty);
//D2Game.0x6FC33AC0
void __fastcall CLIENTS_SetClientState(GameClient* pClient, ClientState nClientState);
//D2Game.0x6FC33AF0
void __fastcall CLIENTS_SetIronGolemItemGUID(GameClient* pClient, int32_t nIronGolemItemGUID);
//D2Game.0x6FC33B20
void __fastcall CLIENTS_SetCreateTime(GameClient* pClient, DWORD dwCreateTime);
//D2Game.0x6FC33B50
int32_t __fastcall CLIENTS_GetCreateTime(GameClient* pClient);
//D2Game.0x6FC33B80
Game* __fastcall CLIENTS_GetGame(GameClient* pClient);
//D2Game.0x6FC33BB0
int32_t __fastcall CLIENTS_IsInUnitsRoom(UnitAny* pUnit, GameClient* pClient);
//D2Game.0x6FC33BE0
char* __fastcall CLIENTS_GetName(GameClient* pClient);
//D2Game.0x6FC33C10
uint32_t __fastcall sub_6FC33C10(GameClient* pClient);
//D2Game.0x6FC33C50
BOOL __fastcall CLIENTS_CheckState(int32_t nClientId, ClientState nExpectedClientState);
//D2Game.0x6FC33CD0
void __fastcall CLIENTS_UpdatePing(int32_t nClientId, int32_t a2, int32_t arg_0);
//D2Game.0x6FC33EA0
int32_t __fastcall sub_6FC33EA0(int32_t nClientId, char* szName);
//D2Game.0x6FC33F20
int32_t __fastcall CLIENTS_GetClientIdByName(const char* szName);
//D2Game.0x6FC33F90
int32_t __fastcall sub_6FC33F90(const char* szName, char* szGameName);
//D2Game.0x6FC34020
int32_t __fastcall CLIENTS_AttachSaveFile(int32_t nClientId, const void* pSaveData, int32_t nSize, int32_t nTotalSize, BOOL bUnlockCharacter, int32_t a6, int32_t a7);
//D2Game.0x6FC34170
SaveHeader* __fastcall CLIENTS_GetSaveHeader(GameClient* pClient);
//D2Game.0x6FC341A0
uint32_t __fastcall CLIENTS_GetSaveHeaderSize(GameClient* pClient);
//D2Game.0x6FC341D0
void __fastcall CLIENTS_CopySaveDataToClient(GameClient* pClient, const void* pData, uint32_t nSize);
//D2Game.0x6FC34280
void __fastcall CLIENTS_FreeSaveHeader(GameClient* pClient);
//D2Game.0x6FC34300
void __fastcall D2GAME_SetSaveLoadComplete_6FC34300(GameClient* pClient);
//D2Game.0x6FC34350
int32_t __fastcall CLIENT_GetSaveHeader_6FC34350(GameClient* pClient);
//D2Game.0x6FC34390
void __fastcall CLIENT_SetSaveLoaded_6FC34390(GameClient* pClient);
//D2Game.0x6FC343D0
uint32_t __fastcall CLIENT_IsSaveLoaded(GameClient* pClient);
//D2Game.0x6FC34420
GameClient* __fastcall CLIENTS_GetNext(GameClient* pClient);
//D2Game.0x6FC34430
void __fastcall CLIENTS_SetSkillHotKey(GameClient* pClient, int32_t nHotkeyId, int16_t nSkillId, uint8_t nHand, int32_t nItemGUID);
//D2Game.0x6FC34460
void __fastcall CLIENTS_GetSkillHotKey(GameClient* pClient, int32_t nId, int32_t* pSkillId, int32_t* nHand, int32_t* nItemGUID);
//D2Game.0x6FC344A0
int16_t __fastcall CLIENTS_GetGuildFlags(GameClient* pClient);
//D2Game.0x6FC344B0
void __fastcall CLIENTS_SetGuildFlags(GameClient* pClient, int16_t nFlags);
//D2Game.0x6FC344C0
void __fastcall CLIENTS_GetGuildTag(GameClient* pClient, int32_t* pTag);
//D2Game.0x6FC344D0
void __fastcall CLIENTS_SetGuildTag(GameClient* pClient, int32_t* a2);
//D2Game.0x6FC344E0
void __fastcall CLIENTS_GetGuildName(GameClient* pClient, char* szName);
//D2Game.0x6FC34500
void __fastcall CLIENTS_SetGuildName(GameClient* pClient, char* szName);
//D2Game.0x6FC34520
void __fastcall CLIENTS_GetGuildEmblem(GameClient* pClient, uint8_t* pBackgroundColor, uint8_t* pForegroundColor, uint8_t* pEmblemType);
//D2Game.0x6FC34550
void __fastcall CLIENTS_SetGuildEmblem(GameClient* pClient, uint8_t nBackgroundColor, uint8_t nForegroundColor, uint8_t nEmblemType);
//D2Game.0x6FC34570
void __fastcall CLIENTS_SetExperienceLoss(GameClient* pClient, int32_t nExpLoss);
//D2Game.0x6FC345A0
int32_t __fastcall CLIENTS_GetExperienceLoss(GameClient* pClient);
//D2Game.0x6FC345B0
uint8_t __fastcall CLIENTS_GetWeaponSwitch(GameClient* pClient);
//D2Game.0x6FC345C0
void __fastcall CLIENTS_SetWeaponSwitch(GameClient* pClient, uint8_t bSwitchWeapon);
//D2Game.0x6FC345D0
void __fastcall CLIENTS_PacketDataList_Append(GameClient* pClient, PacketData* pPacketData);
//D2Game.0x6FC34600
void __fastcall CLIENTS_PacketDataList_Reset(GameClient* pClient, PacketData* pSinglePacketData);
//D2Game.0x6FC34630
PacketData* __fastcall CLIENTS_PacketDataList_PopHead(GameClient* pClient);
//D2Game.0x6FC34670
PacketData* __fastcall CLIENTS_PacketDataList_GetTail(GameClient* pClient);
//D2Game.0x6FC34680
PacketData* __fastcall CLIENTS_PacketDataList_GetHead(GameClient* pClient);
//D2Game.0x6FC34690
void __fastcall CLIENTS_CopyAccountNameToBuffer(GameClient* pClient, char* szAccount);
//D2Game.0x6FC346A0
void __fastcall D2GAME_GetCharSaveTransactionToken_6FC346A0(GameClient* pClient, int32_t* pCharSaveTransactionToken);
//D2Game.0x6FC346B0
void __fastcall D2GAME_GetRealmIdFromClient_6FC346B0(GameClient* pClient, ClientInfo** ppClientInfo);
//D2Game.0x6FC346C0
void __fastcall D2GAME_SetClientsRealmId_6FC346C0(GameClient* pClient, ClientInfo* pClientInfo);
//D2Game.0x6FC346D0
ClientPlayerData* __fastcall CLIENTS_GetClientPlayerData(GameClient* pClient);
//D2Game.0x6FC34700
void __fastcall CLIENTS_NotifyWarpAttempt(Game* pGame, UnitAny* pUnit);
//D2Game.0x6FC347A0
BOOL __fastcall CLIENTS_ShouldDelayWarpAttempt(Game* pGame, UnitAny* pUnit);
