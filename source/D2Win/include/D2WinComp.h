#pragma once

#include <Windows.h>
#include <cstdint>

#include <D2Gfx.h>
#include <D2Unicode.h>

#pragma pack(push, 1)
struct CofData
{
	uint8_t unk0x00;
	uint8_t unk0x01;
	uint8_t unk0x02;
	uint8_t unk0x03;
	uint32_t unk0x04;
	uint32_t unk0x08;
	uint32_t unk0x0C;
	uint32_t unk0x10;
	uint32_t unk0x14;
	uint32_t unk0x18;
};

struct CofNode
{
	uint32_t unk0x00;
	uint32_t nMode;
	uint32_t unk0x08;
	uint32_t unk0x0C;
	uint32_t unk0x10;
	CofData* pCofData;
	CofNode* unk0x18;
	CofNode* unk0x1C;
};

struct CofInfo
{
	CofNode* pCofNode;
	CofInfo* pNext;
};

struct GfxInfo
{
	CofInfo* pCofInfo;
};

struct CompositeUnit
{
	uint32_t dwFlags;
	uint8_t unk0x04;
	uint8_t unk0x05;
	uint8_t unk0x06;
	uint8_t unk0x07;
	uint32_t unk0x08;
	uint32_t unk0x0C;
	uint32_t unk0x10;
	int32_t nX;
	int32_t nY;
	int32_t nUnitIndex;
	int32_t nMode;
	int32_t nWClassTokenIndex;
	uint8_t unk0x28[16];
	uint32_t dwArmorType[16];
	uint32_t unk0x78;
	uint32_t unk0x7C;
	uint32_t unk0x80;
	uint32_t unk0x84;
	uint32_t unk0x88;
	uint32_t unk0x8C;
	uint32_t unk0x90;
	uint32_t unk0x94;
	uint32_t unk0x98;
	uint32_t unk0x9C;
	uint32_t unk0xA0;
	uint32_t unk0xA4;
	uint32_t unk0xA8;
	uint32_t unk0xAC;
	uint32_t unk0xB0;
	uint32_t unk0xB4;
	uint32_t unk0xB8;
	GfxInfo* pGfxInfo;
	CellFile* pEmblemCellFile;
	uint8_t unk0xC0;
	uint8_t unk0xC1;
	uint8_t unk0xC2;
	uint8_t unk0xC3;
	CompositeUnit* pNext;
	CompositeUnit* unk0xC8;
	Unicode unk0xCC[64];
	Unicode unk0x14C[64];
	Unicode unk0x1CC[16];
	Unicode unk0x1EC[32];
	uint32_t unk0x22C;
	uint32_t unk0x230;
	uint32_t unk0x234;
};
#pragma pack(pop)


void __stdcall sub_6F8A4C10();
void __stdcall D2Win_10157();
CompositeUnit* __stdcall D2Win_10147();
inline void* D2Win_10148(void*)
{
	return nullptr;
}
inline void sub_6F8A15E0(void*)
{
}
inline void sub_6F8A1890(void*, int)
{
}
inline void sub_6F8A1D10(void*)
{
}
inline void sub_6F8A2070(void*)
{
}
void __stdcall D2Win_10142_CompUnitDestroy(CompositeUnit* pCompositeUnit, int a2);
const char* __stdcall D2Win_10160_GetPlayerTitle(int a1, int a2, BOOL bSoftCore);
