#pragma once

#include "D2WinControlHeader.h"


#pragma pack(push, 1)
struct WinTimer
{
	Control controlHeader;					//0x00
	uint32_t nTimeout;								//0x40
	uint32_t dwTickCount;							//0x44
};
#pragma pack(pop)


//D2Win.0x6F8B1B70
WinTimer* __fastcall TIMER_Create(uint32_t nTimeout, int32_t(__stdcall* a2)(SMSGHANDLER_PARAMS*));
//D2Win.0x6F8B1BF0
int32_t __fastcall TIMER_Destroy(Control* pControl);
//D2Win.0x6F8B1C00
void __fastcall D2Win_10180(WinTimer* pTimer);
//D2Win.0x6F8B1C30
void __fastcall D2Win_10182(WinTimer* pTimer);
//D2Win.0x6F8B1C60
int32_t __fastcall sub_6F8B1C60(Control* pControl);
//D2Win.0x6F8B1CF0
int32_t __fastcall sub_6F8B1CF0(Control* pControl);
//D2Win.0x6F8B1D70
int32_t __stdcall TIMER_HandleVirtualKeyInput(SMSGHANDLER_PARAMS* pMsg);
