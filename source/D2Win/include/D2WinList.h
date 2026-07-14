#pragma once

#include "D2WinControlHeader.h"

#include <D2Unicode.h>
#include "Font.h"


#pragma pack(push, 1)
struct WinListData
{
	Unicode wszText[256];							//0x00
	int32_t field_200;								//0x200
	int32_t(__stdcall* field_204)(SMSGHANDLER_PARAMS*);	//0x204
	int32_t field_208;								//0x208
	int32_t field_20C;								//0x20C
	int32_t field_210;								//0x210
	int32_t field_214;								//0x214
	char field_218;									//0x218
	char field_219;									//0x219
	char field_21A;									//0x21A
	char field_21B;									//0x21B
	int32_t field_21C;								//0x21C
	WinListData* pNext;						//0x220
};

struct WinList
{
	Control controlHeader;					//0x00
	Font eFont;										//0x40
	int32_t field_44;								//0x44
	int32_t field_48;								//0x48
	int32_t field_4C;								//0x4C
	int32_t field_50;								//0x50
	int32_t field_54;								//0x54
	int32_t field_58;								//0x58
	int32_t field_5C;								//0x5C
	int32_t field_60;								//0x60
	WinListData* pDataList;					//0x64
	WinListData* pSelectedDataEntry;			//0x68
};
#pragma pack(pop)


//D2Win.0x6F8AC270
WinList* __fastcall LIST_Create(int nX, int nY, int nWidth, int nHeight, void* a5, Font* pFont);
//D2Win.0x6F8AC490
int __fastcall LIST_Destroy(WinList* pList);
//D2Win.0x6F8AC4F0
void __fastcall D2Win_10138(WinList* pList, const char* szText, int(__stdcall* a3)(SMSGHANDLER_PARAMS*), char a4, int a5, int a6, int a7);
//D2Win.0x6F8AC570
void __fastcall D2Win_10137(WinList* pList, Unicode* wszText, int(__stdcall* a3)(SMSGHANDLER_PARAMS*), char a4, int a5, int a6, int a7);
//D2Win.0x6F8AC6D0
int __fastcall LIST_GetSelectedDataIndex(WinList* pList);
//D2Win.0x6F8AC720
WinListData* __fastcall LIST_GetDataFromIndex(WinList* pList, int32_t nIndex);
//D2Win.0x6F8AC770
int __fastcall D2Win_10141(WinList* pList);
//D2Win.0x6F8AC7A0
void __stdcall LIST_LoadPentspinCellfile();
//D2Win.0x6F8AC800
void __stdcall LIST_UnloadPentspinCellfile();
//D2Win.0x6F8AC820
int32_t __fastcall LIST_Draw(Control* pControl);
//D2Win.0x6F8AC9B0
int __stdcall LIST_HandleMouseDown(SMSGHANDLER_PARAMS* pMsg);
//D2Win.0x6F8ACA70
int __stdcall LIST_HandleCharInput(SMSGHANDLER_PARAMS* pMsg);
//D2Win.0x6F8ACAE0
int __stdcall LIST_HandleVirtualKeyInput(SMSGHANDLER_PARAMS* pMsg);
