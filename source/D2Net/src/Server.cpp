#include "Server.h"

#include <algorithm>

#include <Fog.h>
#include <Storm.h>

#include <D2PacketDef.h>

#include "Client.h"
#include "D2Net.h"


QServer* gpServer;
int32_t gnLocalClientGameGuid_6FC0B26C;


constexpr int32_t VARIABLE_PACKET_SIZE = -1;


//D2Net.0x6FC01B30 (#10024)
int32_t __stdcall SERVER_WSAGetLastError()
{
	return WSAGetLastError();
}

//D2Net.0x6FC01B60 (#10030)
int32_t __fastcall SERVER_GetServerPacketSize(PacketBuffer* pBuffer, uint32_t nBufferSize, int32_t* pSize)
{
	constexpr int32_t gServerPacketSizeTable[] =
	{
		sizeof(GSPacketSrv00),
		sizeof(GSPacketSrv01),
		sizeof(GSPacketSrv02),
		sizeof(GSPacketSrv03),
		sizeof(GSPacketSrv04),
		sizeof(GSPacketSrv05),
		sizeof(GSPacketSrv06),
		sizeof(GSPacketSrv07),
		sizeof(GSPacketSrv08),
		sizeof(GSPacketSrv09),
		sizeof(GSPacketSrv0A),
		sizeof(GSPacketSrv0B),
		sizeof(GSPacketSrv0C),
		sizeof(GSPacketSrv0D),
		sizeof(GSPacketSrv0E),
		sizeof(GSPacketSrv0F),
		sizeof(GSPacketSrv10),
		sizeof(GSPacketSrv11),
		sizeof(GSPacketSrv12),
		sizeof(GSPacketSrv13),
		sizeof(GSPacketSrv14),
		sizeof(GSPacketSrv15),
		VARIABLE_PACKET_SIZE,
		0,
		sizeof(GSPacketSrv18),
		sizeof(GSPacketSrv19),
		sizeof(GSPacketSrv1A),
		sizeof(GSPacketSrv1B),
		sizeof(GSPacketSrv1C),
		//sizeof(GSPacketSrv1D),
		3, // 0x1D
		//sizeof(GSPacketSrv1E),
		4, // 0x1E
		//sizeof(GSPacketSrv1F),
		6, // 0x1F
		//sizeof(GSPacketSrv20),
		10, // 0x20
		sizeof(GSPacketSrv21),
		sizeof(GSPacketSrv22),
		sizeof(GSPacketSrv23),
		sizeof(GSPacketSrv24),
		sizeof(GSPacketSrv25),
		VARIABLE_PACKET_SIZE,
		sizeof(GSPacketSrv27),
		sizeof(GSPacketSrv28),
		sizeof(GSPacketSrv29),
		sizeof(GSPacketSrv2A),
		0,
		sizeof(GSPacketSrv2C),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		VARIABLE_PACKET_SIZE,
		sizeof(GSPacketSrv3F),
		sizeof(GSPacketSrv40),
		0,
		sizeof(GSPacketSrv42),
		0,
		0,
		sizeof(GSPacketSrv45),
		0,
		sizeof(GSPacketSrv47),
		sizeof(GSPacketSrv48),
		0,
		0,
		0,
		sizeof(GSPacketSrv4C),
		sizeof(GSPacketSrv4D),
		//sizeof(GSPacketSrv4E),
		7, // 0x4E
		sizeof(GSPacketSrv4F),
		sizeof(GSPacketSrv50),
		sizeof(GSPacketSrv51),
		sizeof(GSPacketSrv52),
		sizeof(GSPacketSrv53),
		//sizeof(GSPacketSrv54),
		3, // 0x54
		0,
		0,
		//sizeof(GSPacketSrv57),
		14, // 0x57
		sizeof(GSPacketSrv58),
		sizeof(GSPacketSrv59),
		sizeof(GSPacketSrv5A),
		VARIABLE_PACKET_SIZE,
		sizeof(GSPacketSrv5C),
		sizeof(GSPacketSrv5D),
		sizeof(GSPacketSrv5E),
		sizeof(GSPacketSrv5F),
		sizeof(GSPacketSrv60),
		sizeof(GSPacketSrv61),
		sizeof(GSPacketSrv62),
		sizeof(GSPacketSrv63),
		0,
		sizeof(GSPacketSrv65),
		sizeof(GSPacketSrv66),
		sizeof(GSPacketSrv67),
		sizeof(GSPacketSrv68),
		sizeof(GSPacketSrv69),
		sizeof(GSPacketSrv6A),
		sizeof(GSPacketSrv6B),
		sizeof(GSPacketSrv6C),
		sizeof(GSPacketSrv6D),
		sizeof(GSPacketSrv6E),
		sizeof(GSPacketSrv6F),
		sizeof(GSPacketSrv70),
		sizeof(GSPacketSrv71),
		sizeof(GSPacketSrv72),
		sizeof(GSPacketSrv73),
		sizeof(GSPacketSrv74),
		sizeof(GSPacketSrv75),
		sizeof(GSPacketSrv76),
		sizeof(GSPacketSrv77),
		sizeof(GSPacketSrv78),
		sizeof(GSPacketSrv79),
		sizeof(GSPacketSrv7A),
		sizeof(GSPacketSrv7B),
		sizeof(GSPacketSrv7C),
		sizeof(GSPacketSrv7D),
		sizeof(GSPacketSrv7E),
		sizeof(GSPacketSrv7F),
		0,
		sizeof(GSPacketSrv81),
		sizeof(GSPacketSrv82),
		0,
		0,
		0,
		0,
		0,
		0,
		sizeof(GSPacketSrv89),
		sizeof(GSPacketSrv8A),
		sizeof(GSPacketSrv8B),
		sizeof(GSPacketSrv8C),
		sizeof(GSPacketSrv8D),
		sizeof(GSPacketSrv8E),
		sizeof(GSPacketSrv8F),
		sizeof(GSPacketSrv90),
		sizeof(GSPacketSrv91),
		sizeof(GSPacketSrv92),
		sizeof(GSPacketSrv93),
		VARIABLE_PACKET_SIZE,
		//sizeof(GSPacketSrv95),
		13,
		sizeof(GSPacketSrv96),
		sizeof(GSPacketSrv97),
		sizeof(GSPacketSrv98),
		sizeof(GSPacketSrv99),
		sizeof(GSPacketSrv9A),
		sizeof(GSPacketSrv9B),
		VARIABLE_PACKET_SIZE,
		VARIABLE_PACKET_SIZE,
		//sizeof(GSPacketSrv9E),
		7, // 0x9E
		//sizeof(GSPacketSrv9F),
		8, // 0x9F
		//sizeof(GSPacketSrvA0),
		10, // 0xA0
		//sizeof(GSPacketSrvA1),
		7, // 0xA1
		//sizeof(GSPacketSrvA2),
		8, // 0xA2
		sizeof(GSPacketSrvA3),
		sizeof(GSPacketSrvA4),
		sizeof(GSPacketSrvA5),
		VARIABLE_PACKET_SIZE,
		//sizeof(GSPacketSrvA7),
		7, // 0xA7
		VARIABLE_PACKET_SIZE,
		//sizeof(GSPacketSrvA9),
		7, // 0xA9
		VARIABLE_PACKET_SIZE,
		sizeof(GSPacketSrvAB),
		VARIABLE_PACKET_SIZE,
		sizeof(GSPacketSrvAD),
		VARIABLE_PACKET_SIZE,
		//sizeof(GSPacketSrvAF),
		1, // 0xAF
		0,
		sizeof(GSPacketSrvB1),
		VARIABLE_PACKET_SIZE,
		sizeof(GSPacketSrvB3),
	};

	if (nBufferSize == 0)
	{
		return 0;
	}

	const uint8_t nHeader = pBuffer->data[0];
	if (nHeader >= std::size(gServerPacketSizeTable))
	{
		return 0;
	}

	*pSize = gServerPacketSizeTable[nHeader];
	if (*pSize >= 0)
	{
		return *pSize;
	}

	switch (nHeader)
	{
	case 0x16u:
	{
		if (nBufferSize < 0xD)
		{
			return 0;
		}

		*pSize = *(uint16_t*)&pBuffer->data[1];
		return *pSize;
	}
	case 0x26u:
	{
		if (nBufferSize < 10)
		{
			return 0;
		}

		const char* v6 = (const char*)&pBuffer->data[10];
		const int32_t v7 = SStrLen(v6) + 11;
		if (nBufferSize < v7)
		{
			return 0;
		}

		const int32_t v8 = SStrLen(v6);
		const int32_t result = v7 + SStrLen(&v6[v8 + 1]) + 1;
		if (nBufferSize < result)
		{
			return 0;
		}

		*pSize = result;
		return *pSize;
	}
	case 0x3Eu:
	{
		if (nBufferSize < 2)
		{
			return 0;
		}

		*pSize = pBuffer->data[1];
		return *pSize;
	}
	case 0x5Bu:
	{
		if (nBufferSize < 0x22)
		{
			return 0;
		}

		*pSize = *(uint16_t*)&pBuffer->data[1];
		return *pSize;
	}
	case 0x94u:
	{
		if (nBufferSize < 9)
		{
			return 0;
		}

		*pSize = 3 * (pBuffer->data[1] + 2);
		return *pSize;
	}
	case 0x9Cu:
	case 0x9Du:
	{
		if (nBufferSize < 3)
		{
			return 0;
		}

		*pSize = pBuffer->data[2];
		return *pSize;
	}
	case 0xA6u:
	{
		if (nBufferSize < 4)
		{
			return 0;
		}

		*pSize = *(uint16_t*)&pBuffer->data[2];
		return *pSize;
	}
	case 0xA8u: // NOLINT error: switch has 2 consecutive identical branches [bugprone-branch-clone,-warnings-as-errors]
	{
		if (nBufferSize < 7)
		{
			return 0;
		}

		*pSize = pBuffer->data[6];
		return *pSize;
	}
	case 0xAAu:
	{
		if (nBufferSize < 7)
		{
			return 0;
		}

		*pSize = pBuffer->data[6];
		return *pSize;
	}
	case 0xACu:
	{
		if (nBufferSize < 0xD)
		{
			return 0;
		}

		*pSize = pBuffer->data[12];
		return *pSize;
	}
	case 0xAEu:
	{
		if (nBufferSize < 2)
		{
			return 0;
		}

		if (pBuffer->data[1])
		{
			*pSize = pBuffer->data[1] + 1;
		}
		else
		{
			*pSize = 2;
		}
		return *pSize;
	}
	case 0xB2u:
	{
		if (nBufferSize < 8)
		{
			return 0;
		}

		*pSize = pBuffer->data[1] + 7;
		return *pSize;
	}
	default:
	{
		*pSize = 0;
		return 0;
	}
	}
}

//D2Net.0x6FC01E60 (#10031)
int32_t __fastcall SERVER_GetClientPacketSize(PacketBuffer* pBuffer, uint32_t nBufferSize, int32_t* pSize)
{
	constexpr int32_t gClientPacketSizeTable[] =
	{
		0,
		sizeof(GSPacketClt01),
		sizeof(GSPacketClt02),
		sizeof(GSPacketClt03),
		sizeof(GSPacketClt04),
		sizeof(GSPacketClt05),
		sizeof(GSPacketClt06),
		sizeof(GSPacketClt07),
		sizeof(GSPacketClt08),
		sizeof(GSPacketClt09),
		sizeof(GSPacketClt0A),
		sizeof(GSPacketClt0B),
		sizeof(GSPacketClt0C),
		sizeof(GSPacketClt0D),
		sizeof(GSPacketClt0E),
		sizeof(GSPacketClt0F),
		sizeof(GSPacketClt10),
		sizeof(GSPacketClt11),
		sizeof(GSPacketClt12),
		sizeof(GSPacketClt13),
		VARIABLE_PACKET_SIZE,
		VARIABLE_PACKET_SIZE,
		sizeof(GSPacketClt16),
		sizeof(GSPacketClt17),
		sizeof(GSPacketClt18),
		sizeof(GSPacketClt19),
		//sizeof(GSPacketClt1A),
		9, // 0x1A
		//sizeof(GSPacketClt1B),
		9, // 0x1B
		//sizeof(GSPacketClt1C),
		3, // 0x1C
		//sizeof(GSPacketClt1D),
		9, // 0x1D
		//sizeof(GSPacketClt1E),
		9, // 0x1E
		//sizeof(GSPacketClt1F),
		17, // 0x1F
		sizeof(GSPacketClt20),
		//sizeof(GSPacketClt21),
		9, // 0x21
		//sizeof(GSPacketClt22),
		5, // 0x22
		sizeof(GSPacketClt23),
		//sizeof(GSPacketClt24),
		5, // 0x24
		//sizeof(GSPacketClt25),
		9, // 0x25
		sizeof(GSPacketClt26),
		sizeof(GSPacketClt27),
		//sizeof(GSPacketClt28),
		9, // 0x28
		sizeof(GSPacketClt29),
		sizeof(GSPacketClt2A),
		0,
		0,
		//sizeof(GSPacketClt2D),
		1, // 0x2D
		//sizeof(GSPacketClt2E),
		3, // 0x2E
		//sizeof(GSPacketClt2F),
		9, // 0x2F
		//sizeof(GSPacketClt30),
		9, // 0x30
		sizeof(GSPacketClt31),
		sizeof(GSPacketClt32),
		sizeof(GSPacketClt33),
		//sizeof(GSPacketClt34),
		5, // 0x34
		//sizeof(GSPacketClt35),
		17, // 0x35
		//sizeof(GSPacketClt36),
		9, // 0x36
		//sizeof(GSPacketClt37),
		5, // 0x37
		sizeof(GSPacketClt38),
		//sizeof(GSPacketClt39),
		5, // 0x39
		sizeof(GSPacketClt3A),
		sizeof(GSPacketClt3B),
		sizeof(GSPacketClt3C),
		sizeof(GSPacketClt3D),
		//sizeof(GSPacketClt3E),
		5, // 0x3E
		//sizeof(GSPacketClt3F),
		3, // 0x3F
		//sizeof(GSPacketClt40),
		1, // 0x40
		sizeof(GSPacketClt41),
		sizeof(GSPacketClt42),
		//sizeof(GSPacketClt43),
		1, // 0x43
		sizeof(GSPacketClt44),
		sizeof(GSPacketClt45),
		//sizeof(GSPacketClt46),
		13, // 0x46
		//sizeof(GSPacketClt47),
		13, // 0x47
		//sizeof(GSPacketClt48),
		1, // 0x48
		sizeof(GSPacketClt49),
		0,
		//sizeof(GSPacketClt4B),
		9, // 0x4B
		//sizeof(GSPacketClt4C),
		5, // 0x4C
		//sizeof(GSPacketClt4D),
		3, // 0x4D
		0,
		sizeof(GSPacketClt4F),
		//sizeof(GSPacketClt50),
		9, // 0x50
		sizeof(GSPacketClt51),
		//sizeof(GSPacketClt52),
		5, // 0x52
		//sizeof(GSPacketClt53),
		1, // 0x53
		//sizeof(GSPacketClt54),
		1, // 0x54
		0,
		0,
		0,
		sizeof(GSPacketClt58),
		sizeof(GSPacketClt59),
		0,
		0,
		0,
		sizeof(GSPacketClt5D),
		sizeof(GSPacketClt5E),
		//sizeof(GSPacketClt5F),
		5, // 0x5F
		//sizeof(GSPacketClt60),
		1, // 0x60
		//sizeof(GSPacketClt61),
		3, // 0x61
		//sizeof(GSPacketClt62),
		5, // 0x62
		sizeof(GSPacketClt63),
		sizeof(GSPacketClt64),
		sizeof(GSPacketClt65),
		sizeof(GSPacketClt66),
		sizeof(GSPacketClt67),
		sizeof(GSPacketClt68),
		sizeof(GSPacketClt69),
		sizeof(GSPacketClt6A),
		VARIABLE_PACKET_SIZE,
		sizeof(GSPacketClt6C),
		sizeof(GSPacketClt6D),
		0,
		sizeof(GSPacketClt6F),
	};

	if (nBufferSize == 0)
	{
		return 0;
	}

	const uint8_t nHeader = pBuffer->data[0];
	if (nHeader == 0xFF)
	{
		*pSize = 16;
		return 1;
	}

	if (nHeader >= std::size(gClientPacketSizeTable))
	{
		return 0;
	}

	*pSize = gClientPacketSizeTable[nHeader];
	if (*pSize >= 0)
	{
		return *pSize;
	}

	switch (nHeader)
	{
	case 0x14:
	case 0x15:
	{
		if (nBufferSize < 3)
		{
			*pSize = 0;
			return *pSize;
		}

		const char* v7 = (const char*)&pBuffer->data[3];
		const int32_t v8 = SStrLen(v7) + 4;
		if (nBufferSize < v8)
		{
			*pSize = 0;
			return *pSize;
		}

		const char* v9 = &v7[SStrLen(v7) + 1];
		const size_t v9Len = SStrLen(v9);
		const int32_t v10 = v8 + v9Len + 1;
		if (nBufferSize < v10)
		{
			*pSize = 0;
			return *pSize;
		}

		const int32_t result = v10 + v9[v9Len + 1] + 1;
		if (nBufferSize < result)
		{
			return 0;
		}

		*pSize = result;
		return *pSize;
	}
	case 0x6B:
	{
		if (nBufferSize < 6)
		{
			return 0;
		}

		*pSize = *(uint8_t*)(pBuffer + 1) + 7;
		return *pSize;
	}
	default:
	{
		*pSize = 0;
		return *pSize;
	}
	}
}

//D2Net.0x6FC01FA0 (#10039)
D2NET_CLIENT_SendFunctionType __stdcall D2NET_10039()
{
	return CLIENT_Send;
}

//D2Net.0x6FC01FB0 (#10040)
D2NET_SERVER_GetClientGameGUIDFunctionType __stdcall D2NET_10040()
{
	return SERVER_GetClientGameGUID;
}

//D2Net.0x6FC01FC0
int32_t __fastcall SERVER_ReadPacketFromBufferCallback(QServer* nUnused, PacketBuffer* pPacketBuffer, int32_t nBufferSize)
{
	D2_MAYBE_UNUSED(nUnused);
	CLIENT_ReadPacketFromBuffer(pPacketBuffer, nBufferSize);
	return 1;
}

//D2Net.0x6FC01FE0
int32_t __fastcall SERVER_ValidateClientPacket(PacketBuffer* pPacketBuffer, uint32_t nBufferSize, int32_t* a3, int32_t* a4, int32_t* a5, int32_t* a6, int32_t nUnused1, int32_t nUnused2)
{
	int32_t nSize = 0;
	if (nBufferSize == 0 || !SERVER_GetClientPacketSize(pPacketBuffer, nBufferSize, &nSize))
	{
		return 3;
	}

	const uint8_t nHeader = pPacketBuffer->data[0];
	if ((nHeader >= 0x70u && nHeader != 0xFF) || nSize < 0 || nSize > MAX_MSG_SIZE)
	{
		return 4;
	}

	if (nSize > nBufferSize)
	{
		return 3;
	}

	*a4 = 0;
	*a3 = nSize;

	if (nHeader < 0x66u)
	{
		*a5 = 1;
	}
	else if (nHeader < 0x70u)
	{
		*a5 = 0;
	}
	else
	{
		*a5 = 2;
	}

	*a6 = 100;

	if (*a5 == 1 || *a5 == 2)
	{
		return 1;
	}

	return 2;
}

//D2Net.0x6FC020B0
int32_t __fastcall sub_6FC020B0(int32_t a1, int32_t nClientId, int32_t a3, int32_t a4)
{
	uint8_t data[2] = { 0xAE, 1 };

	D2NET_10006(0, nClientId, data, sizeof(data));
	return 1;
}

//D2Net.0x6FC020E0
int32_t __fastcall sub_6FC020E0(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{
	const uint8_t data[1] = { 0x6F };

	FOG_10175(gpServer, data, sizeof(data), a2);
	return 1;
}

//D2Net.0x6FC02110
int32_t __fastcall sub_6FC02110()
{
	uint8_t data[2] = { 0xAE, 0 };

	return D2NET_10006(0, 0, data, sizeof(data));
}

//D2Net.0x6FC02130 (#10002)
int32_t __stdcall SERVER_WaitForSingleObject(uint32_t dwMilliseconds)
{
	return FOG_WaitForSingleObject(gpServer, dwMilliseconds);
}

//D2Net.0x6FC02150 (#10003)
void __stdcall SERVER_Initialize(int32_t a1, int32_t a2)
{
	gpServer = FOG_InitializeServer(a1, 3, GAME_PORT, a2, SERVER_ValidateClientPacket, sub_6FC020B0, sub_6FC020E0, SERVER_ReadPacketFromBufferCallback);
}

//D2Net.0x6FC02190 (#10035)
int32_t __stdcall D2NET_10035(int32_t nIndex, int32_t nValue)
{
	return FOG_10186(gpServer, nIndex, nValue);
}

//D2Net.0x6FC021B0 (#10036)
void __stdcall D2NET_10036(int32_t a1, int32_t a2)
{
	FOG_10187(gpServer, a2, a1);
}

//D2Net.0x6FC021D0 (#10026)
void __stdcall SERVER_SetMaxClientsPerGame(int32_t nMaxClients)
{
	FOG_SetMaxClientsPerGame(gpServer, nMaxClients);
}

//D2Net.0x6FC021F0 (#10027)
int32_t __stdcall D2NET_10027()
{
	return FOG_10180(gpServer);
}

//D2Net.0x6FC02200 (#10023)
void __stdcall SERVER_SetHackListEnabled(BOOL bEnabled)
{
	FOG_SetHackListEnabled(gpServer, bEnabled);
}

//D2Net.0x6FC02220 (#10004)
void __stdcall SERVER_Release()
{
	const uint8_t data[1] = { 0xAF };
	FOG_10152(gpServer, data, sizeof(data));

	gpServer = nullptr;
}

// TODO: Better name
//D2Net.0x6FC02250 (#10010)
int32_t __stdcall SERVER_ReadFromMessageList1(uint8_t* pBuffer, int32_t nBufferSize)
{
	return FOG_10156(gpServer, 1, pBuffer, nBufferSize);
}

// TODO: Better name
//D2Net.0x6FC02270 (#10011)
int32_t __stdcall SERVER_ReadFromMessageList0(uint8_t* pBuffer, int32_t nBufferSize)
{
	return FOG_10156(gpServer, 0, pBuffer, nBufferSize);
}

// TODO: Better name
//D2Net.0x6FC02290 (#10012)
int32_t __stdcall SERVER_ReadFromMessageList2(uint8_t* pBuffer, int32_t nBufferSize)
{
	return FOG_10156(gpServer, 2, pBuffer, nBufferSize);
}

//D2Net.0x6FC022B0 (#10006)
uint32_t __stdcall D2NET_10006(int8_t a1, int32_t nClientId, void* pBufferArg, uint32_t nBufferSize)
{
	const uint8_t* pBuffer = (const uint8_t*)pBufferArg;

	D2_ASSERT(nBufferSize <= MAX_MSG_SIZE);

	if (sub_6FC01A00())
	{
		CLIENT_ReadPacketFromBuffer((PacketBuffer*)pBuffer, nBufferSize);
		return nBufferSize;
	}

	if (!a1 && *pBuffer == 0xAE)
	{
		return FOG_10157(gpServer, nClientId, pBuffer, nBufferSize) != 0 ? nBufferSize : 0;
	}

	FOG_10222(pBuffer, nBufferSize);
	if (a1 == 2)
	{
		return FOG_10157(gpServer, nClientId, pBuffer, nBufferSize) != 0 ? nBufferSize : 0;
	}

	uint8_t data[1036] = {};
	const uint32_t nSize = FOG_10223(&data[2], 1032, pBuffer, nBufferSize);
	D2_ASSERT(nSize);

	if (nSize + 1 < 0xF0)
	{
		data[1] = nSize + 1;
		return FOG_10157(gpServer, nClientId, &data[1], nSize + 1) != 0 ? nSize + 1 : 0;
	}

	const uint32_t v6 = nSize + 2;
	data[0] = BYTE1(v6) | 0xF0;
	data[1] = nSize + 2;
	return FOG_10157(gpServer, nClientId, data, v6) != 0 ? v6 : 0;
}

//D2Net.0x6FC02410 (#10014)
void __stdcall SERVER_GetIpAddressStringFromClientId(int32_t nClientId, char* szBuffer, int32_t nBufferSize)
{
	FOG_10159(gpServer, nClientId, szBuffer, nBufferSize);
}

//D2Net.0x6FC02430 (#10038)
int32_t __stdcall SERVER_GetIpAddressFromClientId(int32_t nClientId)
{
	return FOG_10158(gpServer, nClientId);
}

//D2Net.0x6FC02450 (#10037)
SOCKET __stdcall SERVER_GetSocketFromClientId(int32_t nClientId)
{
	return FOG_10161(gpServer, nClientId);
}

//D2Net.0x6FC02470 (#10015)
void __stdcall D2NET_10015(int32_t nClientId, const char* szFile, int32_t nLine)
{
	FOG_10162(gpServer, nClientId, szFile, nLine);
}

//D2Net.0x6FC02490 (#10032)
void __stdcall D2NET_10032(int32_t nClientId, const char* szFile, int32_t nLine)
{
	FOG_10163(gpServer, nClientId, szFile, nLine);
}

//D2Net.0x6FC024B0 (#10033)
int32_t __stdcall D2NET_10033(int32_t a1, int32_t a2, int32_t a3)
{
	return FOG_10164(gpServer, a1, a2, a3);
}

//D2Net.0x6FC024D0 (#10034)
int32_t __stdcall D2NET_10034(int32_t nClientId, int32_t a2, int32_t a3)
{
	return FOG_10166(gpServer, nClientId, a2, a3);
}

//D2Net.0x6FC024F0 (#10016)
void __stdcall D2NET_10016(int32_t nClientId)
{
	FOG_10165(gpServer, nClientId, __FILE__, __LINE__);
}

//D2Net.0x6FC02510 (#10018)
void __stdcall D2NET_10018(int32_t a1)
{
	if (!gpServer)
	{
		return;
	}

	FOG_10170(gpServer, a1);
}

//D2Net.0x6FC02530 (#10019)
int32_t __stdcall D2NET_10019(D2NET_Unk_Callback pfCallback)
{
	return FOG_10171(gpServer, pfCallback);
}

//D2Net.0x6FC02550 (#10020)
int32_t __stdcall SERVER_SetClientGameGUID(int32_t nClientId, int32_t dwGameGuid)
{
	if (nClientId)
	{
		return FOG_10172(gpServer, nClientId, dwGameGuid);
	}

	gnLocalClientGameGuid_6FC0B26C = dwGameGuid;

	return dwGameGuid;
}

//D2Net.0x6FC02580 (#10021)
int32_t __stdcall SERVER_GetClientGameGUID(int32_t nClientId)
{
	if (nClientId)
	{
		return FOG_10173(gpServer, nClientId);
	}

	return gnLocalClientGameGuid_6FC0B26C;
}

//D2Net.0x6FC025A0
int32_t __fastcall SERVER_EnqueuePacketToMessageList(const uint8_t* pBuffer, int32_t nBufferSize)
{
	D2_ASSERT(nBufferSize <= MAX_MSG_SIZE);

	return FOG_10175(gpServer, pBuffer, nBufferSize, 0) != 0;
}

//D2Net.0x6FC025F0 (#10022)
int32_t __stdcall D2NET_10022(uint32_t dwMilliseconds)
{
	return FOG_10177(gpServer, dwMilliseconds);
}

//D2Net.0x6FC02610 (#10028)
int32_t __stdcall D2NET_10028_Return()
{
	return FOG_10182_Return(gpServer);
}

//D2Net.0x6FC02620 (#10029)
int32_t __stdcall D2NET_10029_Return(int32_t nUnused)
{
	return FOG_10183_Return(gpServer, nUnused);
}
