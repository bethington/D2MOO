#pragma once

struct ObjectsTxt;
struct ShrinesTxt;

#pragma pack(1)

struct ObjectRoomCoord
{
	Room1* pRoom;				//0x00
	int32_t nX;								//0x04
	int32_t nY;								//0x08
	ObjectRoomCoord* pNext;			//0x0C
};

struct ObjectRegion
{
	uint8_t nAct;							//0x00
	uint8_t pad0x01[3];						//0x01
	int32_t field_4;						//0x04
	int32_t nPopulatedRooms;				//0x08
	int32_t unk0x0C;						//0x0C
	int32_t nHealingShrines;				//0x10
	int32_t nShrines;						//0x14
	int32_t nWells;							//0x18
	int32_t nTrapMonsterId;					//0x1C
	Coord wellCoordinates[4];			//0x20
	Coord shrineCoordinates[10];		//0x40
};

struct ShrineData
{
	int32_t nShrineSubTypes[8];				//0x00
	int32_t* pShrineSubTypeIds[8];			//0x20
};

struct ObjectControl
{
	Seed pSeed;						//0x00
	ShrineData shrineData;			//0x08
	ObjectRegion* pObjectRegion[1024];//0x48
	int field_1048[50];						//0x1048
	ObjectRoomCoord* pObjectRoomCoord;//0x1110
};

struct ObjectData
{
	ObjectsTxt* pObjectTxt;				//0x00
	uint8_t InteractType;					//0x04
	uint8_t nPortalFlags;					//0x05
	uint16_t unk0x06;						//0x06
	ShrinesTxt* pShrineTxt;				//0x08
	D2UnitGUID dwOperateGUID;					//0x0C
	BOOL bPermanent;						//0x10
	uint32_t unk0x014;							//0x14
	Coord DestRoomCooords;			//0x18
	Coord DestPortalCoords;			//0x20
	char szOwner[16];						//0x28
};

#pragma pack()
