#pragma once

#include <cstdint>


constexpr uint32_t MAX_MSG_SIZE = 516;


#pragma pack(push, 1)
struct Packet
{
	uint8_t data[MAX_MSG_SIZE];
	uint32_t nPacketSize;
	uint32_t dwTickCount;
	Packet* pNext;
};

struct PacketBuffer
{
	uint8_t data[1976];
	uint32_t nUsedBytes;
};
#pragma pack(pop)
