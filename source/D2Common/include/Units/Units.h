#pragma once

#include "D2CommonDefinitions.h"
#include "Item.h"
#include "Missile.h"
#include "Monster.h"
#include "Object.h"
#include "Player.h"
#include "D2Chat.h"
#include <DataTbls/AnimTbls.h>

struct Game; // From D2Game
struct QuestChain; // From D2Game
struct TimerArg; // From D2Game
struct Act;
struct StatListEx;
struct UnitEvent;
struct SkillList;
struct Skill;

struct GfxData;
struct GfxLight;

#pragma pack(1)

enum UnitTypes
{
	UNIT_PLAYER,
	UNIT_MONSTER,
	UNIT_OBJECT,
	UNIT_MISSILE,
	UNIT_ITEM,
	UNIT_TILE,
	UNIT_TYPES_COUNT // Used as an invalid value
};

enum UnitFlags
{
	UNITFLAG_DOUPDATE = 0x00000001,				//tells to update the unit
	UNITFLAG_TARGETABLE = 0x00000002,			//whenever the unit can be selected or not
	UNITFLAG_CANBEATTACKED = 0x00000004,		//whenever the unit can be attacked
	UNITFLAG_ISVALIDTARGET = 0x00000008,		//used to check if unit is a valid target
	UNITFLAG_INITSEEDSET = 0x00000010,			//tells whenever the unit seed has been initialized
	UNITFLAG_DRAWSHADOW = 0x00000020,			//tells whenver to draw a shadow or not (client only)
	UNITFLAG_SKSRVDOFUNC = 0x00000040,			//set when skill srvdofunc is executed
	UNITFLAG_OBJPREOPERATE = 0x00000080,		//unknown, used by objects with pre-operate disabled
	UNITFLAG_HASTXTMSG = 0x00000100,			//whenever this unit has a text message attached to it
	UNITFLAG_ISMERC = 0x00000200,				//is mercenary unit
	UNITFLAG_HASEVENTSOUND = 0x00000400,		//does this unit have an event-sound attached to it (server)
	UNITFLAG_SUMMONER = 0x00000800,				//set for the summoner only
	UNITFLAG_SENDREFRESHMSG = 0x00001000,		//used by items to send a refresh message when it drops on ground
	UNITFLAG_ISLINKREFRESHMSG = 0x00002000,		//tells whenever this unit is linked to an update message chain
	UNITFLAG_SQGFXCHANGE = 0x00004000,			//tells whenever to load new anim for skill SQ
	UNITFLAG_UPGRLIFENHITCLASS = 0x00008000,	//updates life% and hitclass on client
	UNITFLAG_ISDEAD = 0x00010000,				//unit is dead
	UNITFLAG_NOTC = 0x00020000,					//disables treasureclass drops
	UNITFLAG_MONMODEISCHANGING = 0x00080000,	//set when monmode changes
	UNITFLAG_PREDRAW = 0x00100000,				//pre-draw this unit (like floor tiles, client only)
	UNITFLAG_ISASYNC = 0x00200000,				//is async unit (critters)
	UNITFLAG_ISCLIENTUNIT = 0x00400000,			//is client unit
	UNITFLAG_ISINIT = 0x01000000,				//set when unit has been initialized
	UNITFLAG_ISRESURRECT = 0x02000000,			//set for resurrected units and items on floor
	UNITFLAG_NOXP = 0x04000000,					//no xp gain from killing this unit
	UNITFLAG_AUTOMAP = 0x10000000,				//automap stuff
	UNITFLAG_AUTOMAP2 = 0x20000000,				//automap stuff
	UNITFLAG_PETIGNORE = 0x40000000,			//ignored by pets
	UNITFLAG_ISREVIVE = 0x80000000				//is revived monster
};

enum UnitFlagsEx
{
	UNITFLAGEX_HASINV = 0x00000001,				//unit has inventory attached to it
	UNITFLAGEX_UPDATEINV = 0x00000002,			//tells to update inventory content
	UNITFLAGEX_ISVENDORITEM = 0x00000004,		//set for vendor shop items
	UNITFLAGEX_ISSHAPESHIFTED = 0x00000008,		//unit is shapeshifted
	UNITFLAGEX_ITEMINIT = 0x00000010,			//set for items, related to init
	UNITFLAGEX_ISINLOS = 0x00000080,			//unit is in client's line of sight
	UNITFLAGEX_HASBEENDELETED = 0x00000100,		//unit has been deleted but not free'd yet
	UNITFLAGEX_STOREOWNERINFO = 0x00000400,		//unit stores info about owner
	UNITFLAGEX_ISCORPSE = 0x00001000,			//unit is a corpse (use UNITFLAG_ISDEAD instead)
	UNITFLAGEX_UNK_PATH_RELATED = 0x00002000,	//related to path
	UNITFLAGEX_TELEPORTED = 0x00010000,			//unit has been teleported, needs resync
	UNITFLAGEX_STORELASTATTACKER = 0x00020000,	//unit stores info about last attacker
	UNITFLAGEX_NODRAW = 0x00040000,				//don't draw this unit
	UNITFLAGEX_ISEXPANSION = 0x02000000,		//is expansion unit
	UNITFLAGEX_SERVERUNIT = 0x04000000			//is server-side unit
};

enum UnitAlignment {

	UNIT_ALIGNMENT_EVIL = 0,
	UNIT_ALIGNMENT_NEUTRAL = 1,
	UNIT_ALIGNMENT_GOOD = 2,
	UNIT_NUM_ALIGNMENT = 3,
	UNIT_ALIGNMENT_UNASSIGNED = 4,
};

//TODO: Redo Header defs when .cpp is done


struct EventTimer;

struct UnitPacketList
{
	UnitPacketList* pNext;
	uint8_t nHeader;
	//uint8_t nData[255];
};

struct UnitAny
{
	uint32_t dwUnitType;						//0x00 UnitTypes
	int32_t dwClassId;							//0x04
	void* pMemoryPool;							//0x08
	D2UnitGUID dwUnitId;						//0x0C
	union										//0x10
	{
		uint32_t dwAnimMode;					//Player, Monster, Object
		uint32_t dwItemMode;					//Items, see ItemModes
		uint32_t dwMissileMode;					// see MissileModes
	};
	union										//0x14
	{
		PlayerData* pPlayerData;
		ItemData* pItemData;
		MonsterData* pMonsterData;
		ObjectData* pObjectData;
		MissileData* pMissileData;
	};
	uint8_t nAct;								//0x18
	uint8_t unk0x19[3];							//0x19
	Act* pDrlgAct;					//0x1C
	Seed pSeed;							//0x20
	uint32_t dwInitSeed;						//0x28
	union										//0x2C
	{
		Path* pDynamicPath;
		StaticPath* pStaticPath;
	};
	struct AnimSeqTxt* pAnimSeq;				//0x30
	uint32_t dwSeqFrameCount;					//0x34
	int32_t dwSeqFrame;							//0x38
	uint32_t dwSeqSpeed;						//0x3C
	uint32_t dwSeqMode;							//0x40
	int32_t nSeqCurrentFramePrecise;			//0x44 8-bits fixed point. Actually reused as stop frame when not using a seq. For example for missiles / inferno.
	int32_t dwFrameCountPrecise;				//0x48 8-bits fixed point.
	int16_t wAnimSpeed;							//0x4C
	uint8_t nActionFrame;						//0x4E
	uint8_t unk0x4F;							//0x4F
	const AnimDataRecord* pAnimData;		//0x50
	GfxData* pGfxData;					//0x54
	GfxData* pGfxDataCopy;				//0x58
	StatListEx* pStatListEx;				//0x5C
	Inventory* pInventory;				//0x60
	union
	{
		struct									//Server Unit
		{
			D2UnitGUID dwInteractGUID;			//0x064
			uint32_t dwInteractType;			//0x068
			uint16_t nInteract;					//0x06C
			uint16_t nUpdateType;				//0x06E
			UnitAny* pUpdateUnit;			//0x070
			QuestChain* pQuestEventList;	//0x074
			BOOL bSparkChest;					//0x078
			void* pTimerParams;					//0x07C
			Game* pGame;					//0x080
			uint32_t unk0x084[3];				//0x084
			UnitEvent* pSrvUnitEventsList;//0x090
		};

		struct									//Client Unit
		{
			GfxLight* pLight;				//0x064
			uint32_t dwStartLight;				//0x068
			int32_t nPaletteIndex;				//0x06C
			BOOL bUnitSfx;						//0x070
			uint32_t dwSfxMode;					//0x074
			void* pUnitSfxData;					//0x078
			uint32_t dwSfxTicks;				//0x07C
			uint32_t dwSfxAsyncTicks;			//0x080
			uint32_t dwSfxStepTicks;			//0x084
			BOOL bHasActiveSound;				//0x088
			uint16_t nLastClickX;				//0x08C
			uint16_t nLastClickY;				//0x08E
			UnitEvent* pCltTimerList;		//0x090
		};
	};
	uint32_t dwOwnerType;						//0x94
	D2UnitGUID dwOwnerGUID;						//0x98
	uint32_t dwKillerType;						//0x09C
	D2UnitGUID dwKillerGUID;						//0x0A0
	HoverText* pHoverText;				//0xA4
	SkillList* pSkills;					//0xA8
	struct Combat* pCombat;				//0xAC
	uint32_t dwLastHitClass;					//0xB0
	uint32_t unk0xB4;							//0xB4
	uint32_t dwDropItemCode;					//0xB8
	uint32_t unk0xBC[2];						//0xBC
	uint32_t dwFlags;							//0xC4
	uint32_t dwFlagEx;							//0xC8
	void* pUnused0xCC;							//0xCC may be quest related ?

	uint32_t dwNodeIndex;						//0xD0
	uint32_t dwTickCount;						//0xD4
	union										//0xD8
	{
		uint32_t dwSrvTickCount;				//Server pUnit
		struct PacketList* pPacketList;	//Client pUnit
	};
	EventTimer* pTimer;					//0xDC
	UnitAny* pChangeNextUnit;				//0xE0
	UnitAny* pListNext;						//0xE4
	UnitAny* pRoomNext;						//0xE8
	UnitPacketList* pMsgFirst;			//0xEC
	UnitPacketList* pMsgLast;				//0xF0
};

#pragma pack()

// Helper function
inline AnimSeqTxt* UNITS_GetAnimSeq(UnitAny* pUnit) { 
	return (pUnit->dwUnitType == UNIT_PLAYER || pUnit->dwUnitType == UNIT_MONSTER) ? pUnit->pAnimSeq : nullptr;
}

//D2Common.0x6FDBD520 (#10457)
D2COMMON_DLL_DECL uint8_t __stdcall UNITS_GetDirection(UnitAny* pUnit);
//D2Common.0x6FDBD570 (#10320)
D2COMMON_DLL_DECL Skill* __stdcall UNITS_GetStartSkill(UnitAny* pUnit);
//D2Common.0x6FDBD5B0 (#10321)
D2COMMON_DLL_DECL Skill* __stdcall UNITS_GetLeftSkill(UnitAny* pUnit);
//D2Common.0x6FDBD5F0 (#10322)
D2COMMON_DLL_DECL Skill* __stdcall UNITS_GetRightSkill(UnitAny* pUnit);
//D2Common.0x6FDBD630 (#10324)
D2COMMON_DLL_DECL void __stdcall UNITS_SetUsedSkill(UnitAny* pUnit, Skill* pUsedSkill);
//D2Common.0x6FDBD670 (#10323)
D2COMMON_DLL_DECL Skill* __stdcall UNITS_GetUsedSkill(UnitAny* pUnit);
//D2Common.0x6FDBD6B0 (#11259)
D2COMMON_DLL_DECL UnitAny* __stdcall UNITS_AllocUnit(void* pMemPool, int nUnitType);
//D2Common.0x6FDBD720 (#11260)
D2COMMON_DLL_DECL void __stdcall UNITS_FreeUnit(UnitAny* pUnit);
//D2Common.0x6FDBD780 (#10327)
D2COMMON_DLL_DECL int __stdcall UNITS_GetPrecisionX(UnitAny* pUnit);
//D2Common.0x6FDBD7D0 (#10330)
D2COMMON_DLL_DECL int __stdcall UNITS_GetPrecisionY(UnitAny* pUnit);
//D2Common.0x6FDBD820 (#10328)
D2COMMON_DLL_DECL void __stdcall UNITS_SetXForStaticUnit(UnitAny* pUnit, int nX);
//D2Common.0x6FDBD890 (#10331)
D2COMMON_DLL_DECL void __stdcall UNITS_SetYForStaticUnit(UnitAny* pUnit, int nY);
//D2Common.0x6FDBD900 (#10336)
D2COMMON_DLL_DECL int __stdcall UNITS_GetUnitSizeX(UnitAny* pUnit);
//D2Common.0x6FDBDA00 (#10337)
D2COMMON_DLL_DECL int __stdcall UNITS_GetUnitSizeY(UnitAny* pUnit);
//D2Common.0x6FDBDB10 (#10333)
D2COMMON_DLL_DECL int __stdcall UNITS_GetClientCoordX(UnitAny* pUnit);
//D2Common.0x6FDBDB60 (#10334)
D2COMMON_DLL_DECL int __stdcall UNITS_GetClientCoordY(UnitAny* pUnit);
//D2Common.0x6FDBDBB0 (#10411)
D2COMMON_DLL_DECL int __stdcall UNITS_GetAbsoluteXDistance(UnitAny* pUnit1, UnitAny* pUnit2);
//D2Common.0x6FDBDC20 (#10412)
D2COMMON_DLL_DECL int __stdcall UNITS_GetAbsoluteYDistance(UnitAny* pUnit1, UnitAny* pUnit2);
//D2Common.0x6FDBDC90 (#10340)
D2COMMON_DLL_DECL void __stdcall UNITS_SetTargetX(UnitAny* pUnit, int nTargetX);
//D2Common.0x6FDBDCD0 (#10341)
D2COMMON_DLL_DECL void __stdcall UNITS_SetTargetY(UnitAny* pUnit, int nTargetY);
//D2Common.0x6FDBDD10 (#10332)
D2COMMON_DLL_DECL void __stdcall UNITS_GetCoords(UnitAny* pUnit, Coord* pCoord);
//D2Common.0x6FDBDDA0 (#10335)
D2COMMON_DLL_DECL void __stdcall UNITS_GetClientCoords(UnitAny* pUnit, Coord* pClientCoords);
//D2Common.0x6FDBDE10 (#10338)
D2COMMON_DLL_DECL int __fastcall UNITS_GetCollisionMask(UnitAny* pUnit);
//D2Common.0x6FDBDEC0 (#10352)
D2COMMON_DLL_DECL void __stdcall UNITS_FreeCollisionPath(UnitAny* pUnit);
//D2Common.0x6FDBE060 (#10351)
D2COMMON_DLL_DECL void __stdcall UNITS_BlockCollisionPath(UnitAny* pUnit, Room1* pRoom, int nX, int nY);
//D2Common.0x6FDBE1A0 (#10350)
D2COMMON_DLL_DECL void __stdcall UNITS_InitializeStaticPath(UnitAny* pUnit, Room1* pRoom, int nX, int nY);
//D2Common.0x6FDBE210 (#10343)
D2COMMON_DLL_DECL void __stdcall UNITS_ResetRoom(UnitAny* pUnit);
//D2Common.0x6FDBE270 (#10342)
D2COMMON_DLL_DECL Room1* __stdcall UNITS_GetRoom(UnitAny* pUnit);
//D2Common.0x6FDBE2D0 (#10344)
D2COMMON_DLL_DECL void __stdcall UNITS_SetTargetUnitForDynamicUnit(UnitAny* pUnit, UnitAny* pTargetUnit);
//D2Common.0x6FDBE330 (#10345)
D2COMMON_DLL_DECL int __stdcall UNITS_GetTargetTypeFromDynamicUnit(UnitAny* pUnit);
//D2Common.0x6FDBE3A0 (#10346)
D2COMMON_DLL_DECL D2UnitGUID __stdcall UNITS_GetTargetGUIDFromDynamicUnit(UnitAny* pUnit);
//D2Common.0x6FDBE410 (#10347)
D2COMMON_DLL_DECL void __stdcall UNITS_SetTargetUnitForPlayerOrMonster(UnitAny* pUnit, UnitAny* pTargetUnit);
//D2Common.0x6FDBE470 (#10354)
D2COMMON_DLL_DECL void __stdcall UNITS_GetRunAndWalkSpeedForPlayer(int nUnused, int nCharId, int* pWalkSpeed, int* pRunSpeed);
//D2Common.0x6FDBE4C0 (#10325)
D2COMMON_DLL_DECL void __stdcall UNITS_SetAnimData(UnitAny* pUnit, int nUnitType, int nClassId, int nMode);
//D2Common.0x6FDBE510
void __stdcall UNITS_SetAnimStartFrame(UnitAny* pUnit);
//D2Common.0x6FDBEA60 (#10348)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_ChangeAnimMode(UnitAny* pUnit, int nMode);
//D2Common.0x6FDBEAD0 (#10355)
D2COMMON_DLL_DECL int __stdcall UNITS_IsCurrentRoomInvalid(UnitAny* pUnit);
//D2Common.0x6FDBEB20 (#10356)
D2COMMON_DLL_DECL void __stdcall UNITS_SetCurrentRoomInvalid(UnitAny* pUnit, int a2);
//D2Common.0x6FDBEB80 (#10357)
D2COMMON_DLL_DECL void __stdcall UNITS_RefreshInventory(UnitAny* pUnit, BOOL bSetFlag);
//D2Common.0x6FDBEBE0 (#10409)
D2COMMON_DLL_DECL int __stdcall UNITS_GetInventoryRecordId(UnitAny* pUnit, int nInvPage, BOOL bLoD);
//D2Common.0x6FDBECD0 (#10383)
D2COMMON_DLL_DECL GfxLight* __stdcall UNITS_ResetLightMap(UnitAny* pUnit);
//D2Common.0x6FDBED10 (#10369)
D2COMMON_DLL_DECL int __stdcall UNITS_GetAnimOrSeqMode(UnitAny* pUnit);
//D2Common.0x6FDBED40 (#10370)
D2COMMON_DLL_DECL void __stdcall UNITS_SetAnimOrSeqMode(UnitAny* pUnit, int nAnimMode);
//D2Common.0x6FDBED90 (#10371)
D2COMMON_DLL_DECL void __stdcall UNITS_InitializeSequence(UnitAny* pUnit);
//D2Common.0x6FDBEE20 (#10372)
D2COMMON_DLL_DECL void __stdcall UNITS_SetAnimationFrame(UnitAny* pUnit, int nFrame);
//D2Common.0x6FDBEE60 (#10373)
D2COMMON_DLL_DECL void __stdcall UNITS_StopSequence(UnitAny* pUnit);
//D2Common.0x6FDBEFF0 (#10374)
D2COMMON_DLL_DECL void __stdcall UNITS_UpdateFrame(UnitAny* pUnit);
//D2Common.0x6FDBF020 (#10375)
D2COMMON_DLL_DECL void __stdcall D2COMMON_10375_UNITS_SetFrameNonRate(UnitAny* pUnit, int nRate, int nFailRate);
//1.10f: D2Common.0x6FDBF050 (#10376)
//1.13c: D2Common.0x6FD83110 (#10819)
D2COMMON_DLL_DECL void __stdcall UNITS_UpdateAnimRateAndVelocity(UnitAny* pUnit, const char* szFile, int nLine);
//D2Common.0x6FDBF8D0 (#10377)
D2COMMON_DLL_DECL void __stdcall UNITS_SetAnimationSpeed(UnitAny* pUnit, int nSpeed);
//D2Common.0x6FDBF910 (#10378)
D2COMMON_DLL_DECL int __stdcall UNITS_IsAtEndOfFrameCycle(UnitAny* pUnit);
//D2Common.0x6FDBF970 (#10379)
D2COMMON_DLL_DECL void __stdcall UNITS_GetShiftedFrameMetrics(UnitAny* pUnit, int* pFrameNo, int* pFrameCount);
//D2Common.0x6FDBF9E0 (#10380)
D2COMMON_DLL_DECL void __stdcall UNITS_GetFrameMetrics(UnitAny* pUnit, int* pFrame, int* pFrameCount);
//D2Common.0x6FDBFA40 (#10381)
D2COMMON_DLL_DECL void __stdcall UNITS_SetAnimActionFrame(UnitAny* pUnit, int nFrame);
//D2Common.0x6FDBFA90 (#10382)
D2COMMON_DLL_DECL int __stdcall UNITS_GetEventFrameInfo(UnitAny* pUnit, int nFrame);
//D2Common.0x6FDBFB40 (#10410)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_HasCollision(UnitAny* pUnit);
//D2Common.0x6FDBFB70 (#10358)
D2COMMON_DLL_DECL Skill* __stdcall UNITS_GetSkillFromSkillId(UnitAny* pUnit, int nSkillId);
//D2Common.0x6FDBFC10 (#10392)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsDoor(UnitAny* pUnit);
//D2Common.0x6FDBFC50
bool __fastcall UNITS_CheckIfObjectOrientationIs1(UnitAny* pUnit);
//D2Common.0x6FDBFC90 (#10393)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsShrine(UnitAny* pUnit);
//D2Common.0x6FDBFCB0 (#10394)
D2COMMON_DLL_DECL ObjectsTxt* __stdcall UNITS_GetObjectTxtRecordFromObject(UnitAny* pUnit);
//D2Common.0x6FDBFD00 (#10395)
D2COMMON_DLL_DECL ShrinesTxt* __stdcall UNITS_GetShrineTxtRecordFromObject(UnitAny* pUnit);
//D2Common.0x6FDBFD50 (#10396)
D2COMMON_DLL_DECL void __stdcall UNITS_SetShrineTxtRecordInObjectData(UnitAny* pUnit, ShrinesTxt* pShrinesTxtRecord);
//D2Common.0x6FDBFDB0 (#10413)
D2COMMON_DLL_DECL void __stdcall UNITS_UpdateDirectionAndSpeed(UnitAny* pUnit, int nX, int nY);
//D2Common.0x6FDBFDD0 (#10414)
D2COMMON_DLL_DECL int __stdcall UNITS_GetNewDirection(UnitAny* pUnit);
//D2Common.0x6FDBFF20 (#10416)
D2COMMON_DLL_DECL void __stdcall UNITS_StoreOwnerTypeAndGUID(UnitAny* pUnit, int nOwnerType, D2UnitGUID nOwnerId);
//D2Common.0x6FDBFF40
void __fastcall UNITS_StoreOwnerInfo(UnitAny* pUnit, int nOwnerType, int nOwnerId);
//D2Common.0x6FDBFFE0 (#10415)
D2COMMON_DLL_DECL void __stdcall UNITS_StoreOwner(UnitAny* pUnit, UnitAny* pOwner);
//D2Common.0x6FDC0060 (#10417)
D2COMMON_DLL_DECL void __stdcall UNITS_StoreLastAttacker(UnitAny* pUnit, UnitAny* pKiller);
//D2Common.0x6FDC00E0 (#10418)
D2COMMON_DLL_DECL int __stdcall UNITS_GetDirectionToCoords(UnitAny* pUnit, int nNewX, int nNewY);
//D2Common.0x6FDC0160 (#10437)
D2COMMON_DLL_DECL void __stdcall UNITS_SetOverlay(UnitAny* pUnit, int nOverlay, int nUnused);
//D2Common.0x6FDC01F0 (#10367)
D2COMMON_DLL_DECL int __stdcall UNITS_GetBeltType(UnitAny* pUnit);
//D2Common.0x6FDC0260 (#10368)
D2COMMON_DLL_DECL int __stdcall UNITS_GetCurrentLifePercentage(UnitAny* pUnit);
//D2Common.0x6FDC02A0 (#10359)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsSoftMonster(UnitAny* pUnit);
//D2Common.0x6FDC0320 (#10420)
D2COMMON_DLL_DECL void __stdcall UNITS_AllocPlayerData(UnitAny* pUnit);
//D2Common.0x6FDC03F0 (#10421)
D2COMMON_DLL_DECL void __stdcall UNITS_FreePlayerData(void* pMemPool, UnitAny* pPlayer);
//D2Common.0x6FDC04A0 (#10422)
D2COMMON_DLL_DECL void __stdcall UNITS_SetNameInPlayerData(UnitAny* pUnit, char* szName);
//D2Common.0x6FDC0530 (#10423)
D2COMMON_DLL_DECL char* __stdcall UNITS_GetPlayerName(UnitAny* pUnit);
//D2Common.0x6FDC05B0 (#10424)
D2COMMON_DLL_DECL PlayerData* __stdcall UNITS_GetPlayerData(UnitAny* pUnit);
//D2Common.0x6FDC0600 (#10425)
D2COMMON_DLL_DECL void __stdcall UNITS_SetPlayerPortalFlags(UnitAny* pUnit, int nPortalFlags);
//D2Common.0x6FDC0660 (#10426)
D2COMMON_DLL_DECL int __stdcall UNITS_GetPlayerPortalFlags(UnitAny* pUnit);
//D2Common.0x6FDC06C0 (#10353)
D2COMMON_DLL_DECL uint32_t __stdcall UNITS_GetNameOffsetFromObject(UnitAny* pUnit);
//D2Common.0x6FDC0700 (#10427)
D2COMMON_DLL_DECL uint8_t __stdcall UNITS_GetObjectPortalFlags(UnitAny* pUnit);
//D2Common.0x6FDC0760 (#10428)
D2COMMON_DLL_DECL void __stdcall UNITS_SetObjectPortalFlags(UnitAny* pUnit, uint8_t nPortalFlag);
//D2Common.0x6FDC07C0 (#10429)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_CheckObjectPortalFlag(UnitAny* pUnit, uint8_t nFlag);
//D2Common.0x6FDC0820 (#10430)
D2COMMON_DLL_DECL int __stdcall UNITS_GetOverlayHeight(UnitAny* pUnit);
//D2Common.0x6FDC08B0 (#10431)
D2COMMON_DLL_DECL int __stdcall UNITS_GetDefense(UnitAny* pUnit);
//D2Common.0x6FDC0AC0 (#10432)
D2COMMON_DLL_DECL int __stdcall UNITS_GetAttackRate(UnitAny* pAttacker);
//D2Common.0x6FDC0B60 (#10433)
D2COMMON_DLL_DECL int __stdcall UNITS_GetBlockRate(UnitAny* pUnit, BOOL bExpansion);
//D2Common.0x6FDC0DA0 (#10434)
D2COMMON_DLL_DECL UnitAny* __stdcall D2Common_10434(UnitAny* pUnit, BOOL a2);
//D2Common.0x6FDC0F70 (#10435)
D2COMMON_DLL_DECL UnitAny* __stdcall UNITS_GetEquippedWeaponFromMonster(UnitAny* pMonster);
//D2Common.0x6FDC0FC0 (#10436)
D2COMMON_DLL_DECL int __stdcall UNITS_GetFrameBonus(UnitAny* pUnit);
//D2Common.0x6FDC1120 (#10360)
D2COMMON_DLL_DECL int __stdcall UNITS_GetMeleeRange(UnitAny* pUnit);
//D2Common.0x6FDC1230 (#10364)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_TestCollisionByCoordinates(UnitAny* pUnit, int nX, int nY, int nFlags);
//D2Common.0x6FDC13D0
BOOL __fastcall UNITS_TestCollision(int nX1, int nY1, int nSize1, int nX2, int nY2, int nSize2, Room1* pRoom, int nCollisionMask);
//D2Common.0x6FDC14C0 (#10362)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_TestCollisionWithUnit(UnitAny* pUnit1, UnitAny* pUnit2, int nCollisionMask);
//D2Common.0x6FDC1760
void __fastcall UNITS_ToggleUnitFlag(UnitAny* pUnit, int nFlag, BOOL bSet);
//D2Common.0x6FDC1790 (#10363)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_TestCollisionBetweenInteractingUnits(UnitAny* pUnit1, UnitAny* pUnit2, int nCollisionMask);
//D2Common.0x6FDC1A70 (#10361)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsInMeleeRange(UnitAny* pUnit1, UnitAny* pUnit2, int nRangeBonus);
//D2Common.0x6FDC1B40 (#10318)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsInMovingMode(UnitAny* pUnit);
//D2Common.0x6FDC1C30 (#10319)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsInMovingModeEx(UnitAny* pUnit);
//D2Common.0x6FDC1C50 (#10365)
D2COMMON_DLL_DECL int __fastcall UNITS_GetHitClass(UnitAny* pUnit);
//D2Common.0x6FDC1CE0 (#10366)
D2COMMON_DLL_DECL int __fastcall UNITS_GetWeaponClass(UnitAny* pUnit);
//D2Common.0x6FDC1D00 (#10438)
D2COMMON_DLL_DECL unsigned int __stdcall UNITS_GetHealingCost(UnitAny* pUnit);
//D2Common.0x6FDC1D90 (#10439)
D2COMMON_DLL_DECL unsigned int __stdcall UNITS_GetInventoryGoldLimit(UnitAny* pUnit);
//D2Common.0x6FDC1DB0 (#10440)
D2COMMON_DLL_DECL void __stdcall UNITS_MergeDualWieldWeaponStatLists(UnitAny* pUnit, int a2);
//D2Common.0x6FDC1EE0
MonStats2Txt* __fastcall UNITS_GetMonStats2TxtRecord(int nRecordId);
//D2Common.0x6FDC1F10 (#10442)
D2COMMON_DLL_DECL uint8_t __stdcall UNITS_GetItemComponentId(UnitAny* pUnit, UnitAny* pItem);
//D2Common.0x6FDC1FE0
MonStats2Txt* __fastcall UNITS_GetMonStats2TxtRecordFromMonsterId(int nMonsterId);
//D2Common.0x6FDC2030 (#10443)
D2COMMON_DLL_DECL void __stdcall UNITS_InitRightSkill(UnitAny* pUnit);
//D2Common.0x6FDC20A0 (#10444)
D2COMMON_DLL_DECL void __stdcall UNITS_InitLeftSkill(UnitAny* pUnit);
//D2Common.0x6FDC2110 (#10445)
D2COMMON_DLL_DECL void __stdcall UNITS_InitSwitchRightSkill(UnitAny* pUnit);
//D2Common.0x6FDC2180 (#10446)
D2COMMON_DLL_DECL void __stdcall UNITS_InitSwitchLeftSkill(UnitAny* pUnit);
//D2Common.0x6FDC21F0 (#10447)
D2COMMON_DLL_DECL void __stdcall UNITS_GetRightSkillData(UnitAny* pUnit, int* pRightSkillId, int* pRightSkillFlags);
//D2Common.0x6FDC2250 (#10448)
D2COMMON_DLL_DECL void __stdcall UNITS_GetLeftSkillData(UnitAny* pUnit, int* pLeftSkillId, int* pLeftSkillFlags);
//D2Common.0x6FDC22B0 (#10449)
D2COMMON_DLL_DECL void __stdcall UNITS_GetSwitchRightSkillDataResetRightSkill(UnitAny* pUnit, int* pSwitchRightSkillId, int* pSwitchRightSkillFlags);
//D2Common.0x6FDC2330 (#10450)
D2COMMON_DLL_DECL void __stdcall UNITS_GetSwitchLeftSkillDataResetLeftSkill(UnitAny* pUnit, int* pSwitchLeftSkillId, int* pSwitchLeftSkillFlags);
//D2Common.0x6FDC23B0 (#10451)
D2COMMON_DLL_DECL void __stdcall UNITS_GetSwitchLeftSkillData(UnitAny* pUnit, int* pSwitchLeftSkillId, int* pSwitchLeftSkillFlags);
//D2Common.0x6FDC2420 (#10452)
D2COMMON_DLL_DECL void __stdcall UNITS_GetSwitchRightSkillData(UnitAny* pUnit, int* pSwitchRightSkillId, int* pSwitchRightSkillFlags);
//D2Common.0x6FDC2490 (#10453)
D2COMMON_DLL_DECL void __stdcall UNITS_SetSwitchLeftSkill(UnitAny* pUnit, int nSwitchLeftSkillId, int nSwitchLeftSkillFlags);
//D2Common.0x6FDC24E0 (#10454)
D2COMMON_DLL_DECL void __stdcall UNITS_SetSwitchRightSkill(UnitAny* pUnit, int nSwitchRightSkillId, int nSwitchRightSkillFlags);
//D2Common.0x6FDC2530 (#10455)
D2COMMON_DLL_DECL void __stdcall UNITS_SetWeaponGUID(UnitAny* pUnit, UnitAny* pWeapon);
//D2Common.0x6FDC25B0 (#10456)
D2COMMON_DLL_DECL D2UnitGUID __stdcall UNITS_GetWeaponGUID(UnitAny* pUnit);
//D2Common.0x6FDC2630 (#10339)
D2COMMON_DLL_DECL unsigned int __stdcall UNITS_GetStashGoldLimit(UnitAny* pUnit);
//D2Common.0x6FDC2680 (#10317)
D2COMMON_DLL_DECL BOOL __fastcall UNITS_CanSwitchAI(int nMonsterId);
//D2Common.0x6FDC2720 (#10458)
D2COMMON_DLL_DECL void __fastcall UNITS_SetTimerArg(UnitAny* pUnit, TimerArg* pTimerArg);
//D2Common.0x6FDC2750 (#10459)
D2COMMON_DLL_DECL TimerArg* __fastcall UNITS_GetTimerArg(UnitAny* pUnit);
//D2Common.0x6FDC2780 (#10460)
D2COMMON_DLL_DECL void __stdcall UNITS_AllocStaticPath(UnitAny* pUnit);
//D2Common.0x6FDC27C0 (#10461)
D2COMMON_DLL_DECL void __stdcall UNITS_FreeStaticPath(UnitAny* pUnit);
//D2Common.0x6FDC27F0 (#10462)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_CanDualWield(UnitAny* pUnit);
//D2Common.0x6FDC2860 (#11238)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsCorpseUseable(UnitAny* pUnit);
//D2Common.0x6FDC2910 (#11307)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsObjectInInteractRange(UnitAny* pUnit, UnitAny* pObject);
//D2Common.0x6FDC2C80
struct CharStatsTxt* __fastcall UNITS_GetCharStatsTxtRecord(int nRecordId);
//1.10f: D2Common.0x6FDC2CB0 (#10399)
//1.13c: D2Common.0x6FDCFCD0 (#10407)
D2COMMON_DLL_DECL int __stdcall D2Common_10399(UnitAny* pUnit1, UnitAny* pUnit2);
//D2Common.0x6FDC2E40 (#10397)
D2COMMON_DLL_DECL int __stdcall UNITS_GetDistanceToOtherUnit(UnitAny* pUnit1, UnitAny* pUnit2);
//D2Common.0x6FDC2F50 (#10398)
D2COMMON_DLL_DECL unsigned int __stdcall UNITS_GetDistanceToCoordinates(UnitAny* pUnit, int nX, int nY);
//D2Common.0x6FDC2FF0 (#10400)
D2COMMON_DLL_DECL BOOL __stdcall UNITS_IsInRange(UnitAny* pUnit, Coord* pCoord, int nDistance);
//D2Common.0x6FDC3090 (#10406)
D2COMMON_DLL_DECL UnitAny* __stdcall D2Common_10406(UnitAny* pUnit, int(__fastcall* pCallback)(UnitAny*, void*), void* a3);
//D2Common.0x6FDC33C0 (#10407)
D2COMMON_DLL_DECL UnitAny* __stdcall D2Common_10407(Room1* pRoom, int nX, int nY, int(__fastcall* pCallback)(UnitAny*, void*), void* a5, int a6);
//D2Common.0x6FDC3680 (#10419)
D2COMMON_DLL_DECL void __fastcall UNITS_SetInteractData(UnitAny* pUnit, int nSkillId, int nUnitType, D2UnitGUID nUnitGUID);
