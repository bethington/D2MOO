#pragma once

#include "D2WinControlHeader.h"


#pragma pack(push, 1)
struct WinProgressBar
{
	Control controlHeader;					//0x00
	float fProgress;								//0x40
};
#pragma pack(pop)


//D2Win.0x6F8AEED0
WinProgressBar* __fastcall PROGRESSBAR_Create(int32_t nX, int32_t nY, int32_t nWidth, int32_t nHeight);
//D2Win.0x6F8B1BF0
int32_t __fastcall PROGRESSBAR_Destroy(Control* pControl);
//D2Win.0x6F8AEF70
int32_t __stdcall PROGRESSBAR_SetProgress(WinProgressBar* pProgressBar, float fProgress);
//D2Win.0x6F8AEFB0
int32_t __fastcall PROGRESSBAR_Draw(Control* pControl);
