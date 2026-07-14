#pragma once 

#include <cstdint>

#pragma pack(1)

struct PacketData					//sizeof 0x208
{
	int32_t nPacketSize;				//0x00
	uint8_t packetData[512];			//0x04
	PacketData* pNext;			//0x204
};

struct PacketList
{
	int32_t nTotal;						//0x00
	int32_t nUsed;						//0x04
	int32_t unk0x08;					//0x08
	void* unk0x0C;						//0x0C
};

struct PacketTable
{
	void* pCallback1;					//0x00
	int32_t nSize;						//0x04
	void* pCallback2;					//0x08
};

#pragma pack()
