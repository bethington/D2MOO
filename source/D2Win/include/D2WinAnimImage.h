#pragma once

#include "D2WinControlHeader.h"

#include <DrawMode.h>


#pragma pack(push, 1)
struct AnimatedImageDescriptor
{
	CellFile** ppCellFile1;							//0x00
	CellFile** ppCellFile2;							//0x04
	DrawMode eDrawMode;										//0x08
	uint32_t nAnimSpeed;									//0x0C
};

struct WinAnimImage
{
	Control controlHeader;							//0x00
	AnimatedImageDescriptor* pAnimatedImageDescriptor;	//0x40
	uint32_t nAnimSpeed;									//0x44
	uint32_t dwStartTickCount;								//0x48
	uint32_t nFrame;										//0x4C
	int32_t nAnimType;										//0x50
	DrawMode eDrawMode;										//0x54
	void(__stdcall* field_58)(SMSGHANDLER_PARAMS*);				//0x58
	int32_t bIsRunning;										//0x5C
};
#pragma pack(pop)


//D2Win.0x6F8A53B0
WinAnimImage* __fastcall ANIMIMAGE_Create(int nX, int nY, int nWidth, int nHeight, CellFile* pCellFile, uint32_t nAnimSpeed, int(__stdcall* a7)(SMSGHANDLER_PARAMS*), AnimatedImageDescriptor* pDescriptor, DrawMode eDrawMode, void(__stdcall* a10)(SMSGHANDLER_PARAMS*));
//D2Win.0x6F8B1BF0
int __fastcall ANIMIMAGE_Destroy(Control* pControl);
//D2Win.0x6F8A54A0
int __fastcall D2Win_10106(WinAnimImage* pAnimImage);
//D2Win.0x6F8A54D0
int __fastcall D2Win_10104(WinAnimImage* pAnimImage);
//D2Win.0x6F8A5530
int32_t __fastcall ANIMIMAGE_ShouldMouseInputBeHandled(Control* pControl);
//D2Win.0x6F8A5610
int32_t __fastcall ANIMIMAGE_Draw(Control* pControl);
//D2Win.0x6F8A5900
int __stdcall ANIMIMAGE_HandleMouseDown(SMSGHANDLER_PARAMS* pMsg);
//D2Win.0x6F8A5980
int32_t __stdcall ANIMIMAGE_SetIsRunning(WinAnimImage* pAnimImage, int32_t bIsRunning);
