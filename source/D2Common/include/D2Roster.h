#pragma once

#pragma pack(1)

enum RosterInfoFlags
{
	ROSTERINFOFLAG_LOOT = 0x01,
	ROSTERINFOFLAG_IGNORE = 0x02,
	ROSTERINFOFLAG_SQUELCH = 0x04,
	ROSTERINFOFLAG_HOSTILE = 0x08,
};

enum RosterControlFlags
{
	ROSTERCONTROLFLAG_INVITE = 0,
	ROSTERCONTROLFLAG_INPARTY = 1,
	ROSTERCONTROLFLAG_ACCEPT = 2,
	ROSTERCONTROLFLAG_LEAVE = 3,
	ROSTERCONTROLFLAG_CANCEL = 4,
};

enum RosterButtonTypes
{
	ROSTERBUTTONTYPE_HOSTILE = 1,
	ROSTERBUTTONTYPE_LOOT = 2,
	ROSTERBUTTONTYPE_IGNORE = 3,
	ROSTERBUTTONTYPE_SQUELCH = 4,
};


struct RosterButton
{
	int32_t unk0x00;						//0x00
	int32_t nX;								//0x04
	int32_t nY;								//0x08
	CellFile* pCellFile;				//0x0C
	int32_t nFrame;							//0x10
	int32_t nButtonType;					//0x14
	int32_t nPartyFlags;					//0x18
	int32_t unk0x1C;						//0x1C
};

struct RosterControl
{
	RosterButton pButtons[5];			//0x00
	char szNameEx[16];						//0xA0
	uint32_t unk0xB0;						//0xB0
	uint32_t unk0xB4;						//0xB4
	uint8_t unk0xB8;						//0xB8
	uint32_t dwClassId;						//0xB9
	uint32_t dwLevel;						//0xBD
	uint32_t dwLevelId;						//0xC1
	uint32_t dwUnitId;						//0xC5
	uint32_t unk0xC9;						//0xC9
	RosterControl* pNext;				//0xCD
};

struct RosterCorpse
{
	uint32_t dwCorpseId;					//0x00
	RosterCorpse* pPrev;				//0x04
};

struct RosterInfo
{
	uint32_t dwUnitId;						//0x00
	uint32_t dwPartyFlags;					//0x04
	RosterInfo* pNext;				//0x08
};

struct RosterPet
{
	int32_t nMonStatsId;					//0x00
	int32_t nPetTypeId;						//0x04
	uint32_t dwUnitId;						//0x08
	uint32_t dwOwnerId;						//0x0C
	uint32_t unk0x10;						//0x10
	uint32_t unk0x14;						//0x14
	uint32_t unk0x18;						//0x18
	int32_t nLifePercent;					//0x1C
	BOOL bDrawIcon;							//0x20
	void* unk0x24;							//0x24
	uint32_t unk0x28;						//0x28
	uint32_t unk0x2C;						//0x2C
	RosterPet* pNext;					//0x30
};

struct RosterUnit
{
	char szName[16];						//0x00
	uint32_t dwUnitId;						//0x10
	uint32_t dwPartyLife;					//0x14
	uint32_t dwKillCount;					//0x18
	uint32_t dwClassId;						//0x1C
	uint16_t wLevel;						//0x20
	uint16_t wPartyId;						//0x22
	uint32_t dwLevelId;						//0x24
	uint32_t dwPosX;						//0x28
	uint32_t dwPosY;						//0x2C
	uint32_t dwPartyFlags;					//0x30
	RosterInfo** pRosterInfo;			//0x34
	RosterCorpse* pCorpse;			//0x38
	D2UnitGUID dwSrcPortalGUID;				//0x3C
	D2UnitGUID dwDstPortalGUID;				//0x40
	uint16_t unk0x44;						//0x44
	uint8_t unk0x46[32];					//0x46
	char szNameEx[26];						//0x66 - name with clan tag
	RosterUnit* pNext;				//0x80
};

#pragma pack()

