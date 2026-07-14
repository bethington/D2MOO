#pragma once

#include "D2WinControlHeader.h"


struct WinTextBox;

#pragma pack(push, 1)
struct WinScrollBar
{
	Control controlHeader;					//0x00
	int32_t bIsUpperArrowPressed;					//0x40
	int32_t bIsLowerArrowPressed;					//0x44
	int32_t nMaxSteps;								//0x48
	int32_t nCurrentStep;							//0x4C
	int32_t field_50;								//0x50
	WinTextBox* pTextBox;						//0x54
	int32_t bIsStepIndicatorDragged;				//0x58
	void(__stdcall* field_5C)(SMSGHANDLER_PARAMS*);		//0x5C
};
#pragma pack(pop)


//D2Win.0x6F8AF040
WinScrollBar* __fastcall SCROLLBAR_Create(int nX, int nY, int nWidth, int nHeight, CellFile* pCellFile, int(__stdcall* a6)(SMSGHANDLER_PARAMS*));
//D2Win.0x6F8B1BF0
int __fastcall SCROLLBAR_Destroy(Control* pControl);
//D2Win.0x6F8AF100
int __fastcall SCROLLBAR_GetMaxSteps(WinScrollBar* pScrollBar);
//D2Win.0x6F8AF130
int32_t __fastcall SCROLLBAR_SetMaxSteps(WinScrollBar* pScrollBar, int32_t nMaxSteps);
//D2Win.0x6F8AF170
int32_t __fastcall SCROLLBAR_SetCurrentStep(WinScrollBar* pScrollBar, int32_t nCurrentStep);
//D2Win.0x6F8AF1B0
int __fastcall SCROLLBAR_GetCurrentStep(WinScrollBar* pScrollBar);
//D2Win.0x6F8AF1E0
int __fastcall SCROLLBAR_SetTextBox(WinScrollBar* pScrollBar, WinTextBox* pTextBox);
//D2Win.0x6F8AF240
int __fastcall sub_6F8AF240(SMSGHANDLER_PARAMS* pMsg, WinScrollBar* pScrollBar, int a3);
//D2Win.0x6F8AF2D0
void __fastcall D2Win_10202(WinScrollBar* pScrollBar, int nPosition);
//D2Win.0x6F8AF300
int __fastcall D2Win_10201(WinScrollBar* pScrollBar);
//D2Win.0x6F8AF330
int32_t __fastcall SCROLLBAR_Draw(Control* pControl);
//D2Win.0x6F8AF4D0
int __stdcall SCROLLBAR_HandleMouseDown(SMSGHANDLER_PARAMS* pMsg);
//D2Win.0x6F8AF6B0
int __stdcall SCROLLBAR_HandleMouseUp(SMSGHANDLER_PARAMS* pMsg);
