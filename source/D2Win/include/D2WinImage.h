#pragma once

#include "D2WinControlHeader.h"

#include <DrawMode.h>


#pragma pack(push, 1)
struct WinImageClickRect
{
	int32_t nX;										//0x00
	int32_t nY;										//0x04
	int32_t nWidth;									//0x08
	int32_t nHeight;								//0x0C
	int32_t(__stdcall* pCallback)(SMSGHANDLER_PARAMS*);	//0x14
};

struct WinImage
{
	Control controlHeader;					//0x00
	int32_t nFrame;									//0x40
	DrawMode eDrawMode;								//0x44
	WinImageClickRect* pClickRect;			//0x48
	int32_t field_4C;								//0x4C
	int32_t field_50;								//0x50
};

struct WinImage2
{
	Control controlHeader;					//0x00
	int32_t nFrame;									//0x40
	DrawMode eDrawMode;								//0x44
	WinImageClickRect* pClickRect;			//0x48
};
#pragma pack(pop)


//D2Win.0x6F8ABA90
WinImage2* __fastcall IMAGE2_Create(int32_t nX, int32_t nY, int32_t nWidth, int32_t nHeight, CellFile* pCellFile, int32_t(__stdcall* a6)(SMSGHANDLER_PARAMS*), WinImageClickRect* pClickRect, int32_t(__stdcall* pfHandleVirtualKeyInput)(SMSGHANDLER_PARAMS*));
//D2Win.0x6F8B1BF0
int __fastcall IMAGE_Destroy(Control* pControl);
//D2Win.0x6F8ABB40
WinImage* __fastcall IMAGE_Create(int32_t nX, int32_t nY, int32_t nWidth, int32_t nHeight, CellFile* pCellFile, int32_t a6, int32_t a7);
//D2Win.0x6F8ABC00
int32_t __stdcall IMAGE_SetCellFile(WinImage* pImage, CellFile* pCellFile, WinImageClickRect* pClickRect);
//D2Win.0x6F8ABC40
int32_t __stdcall IMAGE_SetCoordinates(WinImage* pImage, int32_t nX, int32_t nY);
//D2Win.0x6F8ABC80
int32_t __stdcall IMAGE_SetFrame(WinImage* pImage, int32_t nFrame);
//D2Win.0x6F8ABCD0
int32_t __stdcall IMAGE_ForceNormalDrawMode(WinImage* pImage, int32_t nUnused);
//D2Win.0x6F8ABD10
int32_t __stdcall IMAGE_GetFrameCount(WinImage* pImage);
//D2Win.0x6F8ABD50
int32_t __fastcall IMAGE2_Draw(Control* pControl);
//D2Win.0x6F8ABEA0
int32_t __fastcall IMAGE_Draw(Control* pControl);
//D2Win.0x6F8AC170
int32_t __stdcall IMAGE2_HandleMouseDown(SMSGHANDLER_PARAMS* pMsg);
