#include "D2WinMain.h"

#include <algorithm>
#include <cstdio>

#include <D2CMP.h>
#include <D2Gfx.h>
#include <D2Sound.h>
#include "JpegLibraryWrapper.h"
#include <Storm.h>

#include <Fog.h>
#include <D2BitManip.h>

#include <Texture.h>
#include <Window.h>

#include "D2WinAccountList.h"
#include "D2WinAnimImage.h"
#include "D2WinButton.h"
#include "D2WinComp.h"
#include "D2WinEditBox.h"
#include "D2WinFont.h"
#include "D2WinImage.h"
#include "D2WinList.h"
#include "D2WinPalette.h"
#include "D2WinPopUp.h"
#include "D2WinProgressBar.h"
#include "D2WinScrollBar.h"
#include "D2WinSmack.h"
#include "D2WinTextBox.h"
#include "D2WinTimer.h"
#include "D2WRegister.h"

#include "D2WinControlHeader.h"


#pragma warning (disable : 28159)
#pragma warning (disable : 6262)


#pragma pack(push, 1)
struct WinControlInit
{
	int nType;
	int nX;
	int nY;
	int nWidth;
	int nHeight;
	int field_14;
	int nStringId;
	CellFile** ppCellFile;
	int(__stdcall* field_20)(SMSGHANDLER_PARAMS*);
	void* field_24;
	void* field_28;
	void* field_2C;
};
#pragma pack(pop)


WNDPROC gpfWndProc_6F8FE268;
HWND ghMainWindow_6F8FE23C;
int32_t dword_6F8FE26C;
int32_t(*dword_6F8FE27C)();
int32_t gnResolution_6F8FE248;
CellFile* gpBackgroundCellFile;
CellFile* gpCursorCellFile;
int32_t dword_6F8BDF78;
Control* gpControlList_6F8FE24C;
int32_t dword_6F8FE280;
int32_t dword_6F8FE274;
int32_t dword_6F8BDE18;
void* dword_6F8FE260;
int32_t gbIsMouseButtonPressed;
int32_t dword_6F8FE288;
void* dword_6F8FE284;
Control* dword_6F8FE264;
int32_t dword_6F8BDE20;
int32_t dword_6F8FE278;
DWORD gdwScreenshotTickCount_6F8FE28C;
int32_t gnScreenshotCounter_6F8BDE1C;
WinEditBox* gpEditBox_6F8FE258;
WinEditBox* dword_6F8FE25C;
int32_t gnCursorFrame;
POINT gMousePosition_6F8FE234;
int32_t gbWindowed_6F8FE244;
Control* dword_6F8FE250;
//Name in original .dll: sgtWindowData.deadChildrenLocked
int32_t dword_6F8FE254;
int32_t dword_6F8FE228;
int32_t dword_6F8FE22C;



WinCommandRegister gpCommandRegister_6F8BDE28[28] =
{
	{ 0, WM_MOUSEMOVE, D2Win_COMMANDS_MouseMove_6F8AD670 },
	{ 0, WM_LBUTTONDOWN, D2Win_COMMANDS_MouseButton_6F8AE070 },
	{ 0, WM_LBUTTONUP, D2Win_COMMANDS_MouseButton_6F8AE070 },
	{ 0, WM_RBUTTONDOWN, D2Win_COMMANDS_MouseButton_6F8AE070 },
	{ 0, WM_RBUTTONUP, D2Win_COMMANDS_MouseButton_6F8AE070 },
	{ 0, WM_CHAR, D2Win_COMMANDS_Char_6F8ADA80 },
	{ 3, VK_BACK, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_LEFT, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_RIGHT, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_HOME, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_END, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_DELETE, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_UP, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_DOWN, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_PRIOR, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_NEXT, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_RETURN, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_ESCAPE, D2Win_COMMANDS_Escape_6F8ADB10 },
	{ 3, VK_TAB, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_SPACE, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, VK_CONTROL, D2Win_COMMANDS_Control_6F8ADBE0 },
	{ 3, VK_F1, D2Win_COMMANDS_VirtualKey_6F8ADB50 },
	{ 3, 0x4D, D2Win_COMMANDS_KeyF1_6F8AE020 },
	{ 0, WM_ACTIVATEAPP, D2Win_COMMANDS_ActivateApp_6F8ADC40 },
	{ 0, WM_SYSCOMMAND, D2Win_COMMANDS_SysCommand_6F8ADC00 },
	{ 2, VK_SNAPSHOT, D2Win_COMMANDS_Snapshot_6F8ADE70 },
	{ 2, VK_CONTROL, D2Win_COMMANDS_Control_6F8ADBF0 },
	{ 0, WM_MOUSEWHEEL, D2Win_COMMANDS_MouseWheel_6F8AE220 },
};


//D2Win.0x6F8ACC60 (#10000)
BOOL __stdcall D2Win_CreateWindow(HINSTANCE hInstance, DisplayType nRenderMode, BOOL bWindowed, BOOL bCompress)
{
	D2CMP_SetCompressedDataMode(bCompress);
	return D2GFX_Initialize(hInstance, D2Win_MAINWINDOW_WndProc_6F8AD9B0, nRenderMode, bWindowed);
}

//D2Win.0x6F8ACC90 (#10001)
BOOL __stdcall D2Win_InitializeSpriteCache(BOOL bWindowed, GameResolutionMode nResolution)
{
	dword_6F8FE26C = 1;

	if (!WINDOW_Create(bWindowed, nResolution))
	{
		return 0;
	}

	D2CMP_InitSpriteCache(0, 0x800000u, 0x7D000u, 0);
	gpBackgroundCellFile = 0;
	gpControlList_6F8FE24C = 0;
	dword_6F8FE250 = 0;
	dword_6F8FE254 = 0;
	dword_6F8FE25C = 0;
	dword_6F8FE260 = 0;
	gpEditBox_6F8FE258 = 0;
	gpfWndProc_6F8FE268 = 0;
	dword_6F8FE264 = 0;
	dword_6F8FE278 = 0;
	gnCursorFrame = 0;
	dword_6F8FE27C = 0;
	gnResolution_6F8FE248 = nResolution;
	ghMainWindow_6F8FE23C = WINDOW_GetWindow();
	gbWindowed_6F8FE244 = bWindowed;

	return 1;
}

//D2Win.0x6F8ACD40 (#10014)
int __stdcall D2Win_10014_SetWndProc(WNDPROC wndProc)
{
	gpfWndProc_6F8FE268 = wndProc;
	return 1;
}

//D2Win.0x6F8ACD60 (#10015)
int __fastcall D2Win_10015()
{
	D2Win_10115_FONT();
	D2Win_10127_SetFont(D2FONT_FONT16);
	LIST_LoadPentspinCellfile();
	D2Win_10157();
	gpfWndProc_6F8FE268 = nullptr;
	WREGISTER_RegisterCommands(WINDOW_GetWindow(), gpCommandRegister_6F8BDE28, std::size(gpCommandRegister_6F8BDE28));
	D2Win_10177(0);
	dword_6F8FE27C = nullptr;
	return 1;
}

//D2Win.0x6F8ACDB0 (#10002)
int __stdcall D2Win_CloseSpriteCache()
{
	dword_6F8FE26C = 0;
	ghMainWindow_6F8FE23C = 0;
	D2CMP_FlushSpriteCache(1);
	WINDOW_Destroy();
	return 1;
}

//D2Win.0x6F8ACDD0 (#10003)
void __stdcall D2Win_10003()
{
	gnResolution_6F8FE248 = 0;
	WINDOW_PlayCutscene();
}

//D2Win.0x6F8ACDE0 (#10004)
void __stdcall D2Win_10004(GameResolutionMode bForceResize)
{
	gnResolution_6F8FE248 = bForceResize;

	if (bForceResize == 1)
	{
		D2CMP_FlushSpriteCache(1);
	}

	WINDOW_EndCutScene(bForceResize);

	if (bForceResize == 1)
	{
		D2CMP_InitSpriteCache(0, 0x800000u, 0x7D000u, 0);
	}
}

// Helper function
//TODO: Name
void __stdcall D2Win_Helper()
{
	Control* pNext = nullptr;

	Control* pCurrent = dword_6F8FE250;
	while (pCurrent)
	{
		if (pCurrent->dwFlags & gdwBitMasks[0])
		{
			break;
		}

		pNext = pCurrent->pNext;
		D2_FREE(pCurrent);
		pCurrent = pNext;
		dword_6F8FE250 = pNext;
	}

	while (pCurrent)
	{
		if (pCurrent->dwFlags & gdwBitMasks[1])
		{
			break;
		}

		pNext = pCurrent->pNext;
		D2_FREE(pCurrent);
		pCurrent = pNext;
		dword_6F8FE250 = pNext;
	}
}

//D2Win.0x6F8ACE20 (#10016)
int __stdcall D2Win_10016()
{
	dword_6F8FE254 = 0;

	D2Win_Helper();

	LIST_UnloadPentspinCellfile();
	D2Win_10116();
	WREGISTER_UnregisterCommands(WINDOW_GetWindow(), gpCommandRegister_6F8BDE28, std::size(gpCommandRegister_6F8BDE28));
	return 1;
}

//D2Win.0x6F8ACEC0 (#10005)
int __stdcall D2Win_10005(GameResolutionMode nResolution)
{
	gnResolution_6F8FE248 = nResolution;

	D2CMP_FlushSpriteCache(1);

	if (!WINDOW_Resize(nResolution, 0))
	{
		FOG_Trace("Failed to resize window... this is fatal!");
		exit(-1);
	}

	D2Win_10176(0);
	D2CMP_InitSpriteCache(0, 0x800000u, 0x7D000u, 0);
	return 1;
}

//D2Win.0x6F8ACF20 (#10006)
void __stdcall D2Win_10006_ClearDrawCaches()
{
	WINDOW_ClearCaches();
}

//D2Win.0x6F8ACF30 (#10007)
int __stdcall BACKGROUND_SetCellFile(CellFile* pCellFile)
{
	gpBackgroundCellFile = pCellFile;
	return 1;
}

//D2Win.0x6F8ACF50 (#10011)
int __stdcall CURSOR_SetCellFile(CellFile* pCellFile)
{
	gpCursorCellFile = pCellFile;
	return 1;
}

//D2Win.0x6F8ACF70 (#10010)
void __stdcall D2Win_10010(int(*a1)())
{
	dword_6F8FE27C = a1;
}

//D2Win.0x6F8ACF80 (#10009)
int __stdcall D2Win_10009()
{
	dword_6F8BDF78 = 0;
	return 1;
}

//D2Win.0x6F8ACF90 (#10008)
int __stdcall D2Win_10008(void(__stdcall* pCallback)(DWORD))
{
	int nSleepMillis = 0;
	DWORD dwCounter = 0;
	
	DWORD dwTickCount = GetTickCount();

	while (dword_6F8BDF78)
	{
		struct tagMSG Msg = {};

		if (PeekMessageA(&Msg, 0, 0, 0, 0))
		{
			dword_6F8BDF78 = GetMessageA(&Msg, 0, 0, 0);
			if (!dword_6F8BDF78 && dword_6F8FE27C)
			{
				dword_6F8FE27C();
			}

			TranslateMessage(&Msg);
			DispatchMessageA(&Msg);
		}
		else
		{
			DWORD dwTickDiff = GetTickCount() - dwTickCount;

			dwTickCount = GetTickCount();
			if (dwTickDiff > 1000)
			{
				dwTickDiff = 1000;
			}

			nSleepMillis -= dwTickDiff;
			if (nSleepMillis <= 0)
			{
				nSleepMillis += 40;
				if (nSleepMillis < -1000)
				{
					nSleepMillis = 0;
				}

				if (pCallback)
				{
					pCallback(dwCounter);
					++dwCounter;
				}

				D2Win_10019();
			}

			dword_6F8FE25C = gpEditBox_6F8FE258;

			DWORD dwSleepMillis = std::min(nSleepMillis, 20);

			if (dword_6F8BDE20)
			{
				dwSleepMillis = 0;
			}

			Sleep(dwSleepMillis);
		}
	}

	D2Win_10019();

	if (!dword_6F8FE280)
	{
		dword_6F8BDF78 = 1;
	}

	return 0;
}

//D2Win.0x6F8AD0B0 (#10013)
int __stdcall CONTROL_SetCellFile(Control* pControl, CellFile* pCellFile)
{
	pControl->pCellFile = pCellFile;
	return 1;
}

//D2Win.0x6F8AD0D0 (#10012)
int __stdcall D2Win_10012(WinEditBox* pEditBox)
{
	if (gpEditBox_6F8FE258)
	{
		if (pEditBox != gpEditBox_6F8FE258)
		{
			D2Win_10083(gpEditBox_6F8FE258);
		}
	}

	gpEditBox_6F8FE258 = pEditBox;


	return 1;
}

//D2Win.0x6F8AD110 (#10023)
WinEditBox* __stdcall D2Win_10023()
{
	return dword_6F8FE25C;
}

//D2Win.0x6F8AD120
int __stdcall CONTROL_AppendToList(Control* pControl)
{
	if (gpControlList_6F8FE24C)
	{
		Control* pLast = gpControlList_6F8FE24C;
		for (Control* i = gpControlList_6F8FE24C->pNext; i; i = i->pNext)
		{
			pLast = i;
		}
		pLast->pNext = pControl;
	}
	else
	{
		gpControlList_6F8FE24C = pControl;
	}

	return 1;
}

//D2Win.0x6F8AD160
int __stdcall CONTROL_RemoveFromList(Control* pControl)
{
	if (dword_6F8FE25C == (WinEditBox*)pControl)
	{
		dword_6F8FE25C = nullptr;
	}

	if (gpEditBox_6F8FE258 == (WinEditBox*)pControl)
	{
		gpEditBox_6F8FE258 = nullptr;
	}

	if (dword_6F8FE264 == pControl)
	{
		dword_6F8FE264 = nullptr;
		dword_6F8FE278 = 0;
	}

	if (dword_6F8FE260 == pControl)
	{
		dword_6F8FE260 = nullptr;
	}

	if (!gpControlList_6F8FE24C)
	{
		return 0;
	}

	Control* pPrevious = nullptr;
	for (Control* pCurrent = gpControlList_6F8FE24C; pCurrent; pCurrent = pCurrent->pNext)
	{
		if (pCurrent == pControl)
		{
			if (pPrevious)
			{
				pPrevious->pNext = pCurrent->pNext;
			}
			else
			{
				gpControlList_6F8FE24C = pControl->pNext;
			}
			pControl->pNext = nullptr;

			if (dword_6F8FE250)
			{
				Control* pLast = dword_6F8FE250;
				for (Control* i = dword_6F8FE250->pNext; i; i = i->pNext)
				{
					pLast = i;
				}

				pLast->pNext = pControl;
			}
			else
			{
				dword_6F8FE250 = pControl;
			}

			return 1;
		}

		pPrevious = pCurrent;
	}

	return 1;
}

//D2Win.0x6F8AD260 (#10017)
Control* __stdcall CONTROL_Create(WinControlInit* pControlInit)
{
	CellFile* pCellFile = nullptr;
	if (pControlInit->ppCellFile)
	{
		pCellFile = *pControlInit->ppCellFile;
	}

	switch (pControlInit->nType)
	{
	case D2WIN_TEXTBOX:
		return (Control*)D2Win_10042_TEXTBOX_Create(
			pControlInit->nX,
			pControlInit->nY,
			pControlInit->nWidth,
			pControlInit->nHeight,
			pControlInit->field_14,
			pControlInit->nStringId,
			pCellFile,
			pControlInit->field_20,
			(int)pControlInit->field_2C,
			(int)pControlInit->field_28,
			(int)pControlInit->field_24);

	case D2WIN_EDITBOX:
		return (Control*)D2Win_10070_EDITBOX_Create(
			pControlInit->nX,
			pControlInit->nY,
			pControlInit->nWidth,
			pControlInit->nHeight,
			pControlInit->field_14,
			pControlInit->nStringId,
			pCellFile,
			(int)pControlInit->field_24,
			pControlInit->field_20,
			(int)pControlInit->field_2C,
			(int)pControlInit->field_28);

	case D2WIN_IMAGE:
		return (Control*)IMAGE2_Create(
			pControlInit->nX,
			pControlInit->nY,
			pControlInit->nWidth,
			pControlInit->nHeight,
			pCellFile,
			(int(__stdcall*)(SMSGHANDLER_PARAMS*))pControlInit->field_20,
			(WinImageClickRect*)pControlInit->field_24,
			(int(__stdcall*)(SMSGHANDLER_PARAMS*))pControlInit->field_28);

	case D2WIN_ANIMIMAGE:
		return (Control*)ANIMIMAGE_Create(
			pControlInit->nX,
			pControlInit->nY,
			pControlInit->nWidth,
			pControlInit->nHeight,
			pCellFile,
			pControlInit->field_14,
			pControlInit->field_20,
			(AnimatedImageDescriptor*)pControlInit->field_24,
			(DrawMode)pControlInit->nStringId,
			(void(__stdcall*)(SMSGHANDLER_PARAMS*))pControlInit->field_28);

	case D2WIN_IMAGE2:
		return (Control*)IMAGE_Create(pControlInit->nX, pControlInit->nY, pControlInit->nWidth, pControlInit->nHeight, pCellFile, pControlInit->field_14, pControlInit->nStringId);

	case D2WIN_SCROLLBAR:
		return (Control*)SCROLLBAR_Create(pControlInit->nX, pControlInit->nY, pControlInit->nWidth, pControlInit->nHeight, pCellFile, pControlInit->field_20);

	case D2WIN_BUTTON:
		return (Control*)BUTTON_Create(
			pControlInit->nX,
			pControlInit->nY,
			pControlInit->nWidth,
			pControlInit->nHeight,
			pCellFile,
			pControlInit->field_20,
			pControlInit->field_14,
			(int)pControlInit->field_24,
			(int)pControlInit->field_2C,
			pControlInit->nStringId,
			(int(__stdcall*)(SMSGHANDLER_PARAMS*))pControlInit->field_28);

	case D2WIN_LIST:
		return (Control*)LIST_Create(pControlInit->nX, pControlInit->nY, pControlInit->nWidth, pControlInit->nHeight, pControlInit->field_24, (Font*)pControlInit->field_28);

	case D2WIN_TIMER:
		return (Control*)TIMER_Create(pControlInit->nX, pControlInit->field_20);

	case D2WIN_SMACK:
		return (Control*)SMACK_Create(pControlInit->nX, pControlInit->nY, pControlInit->nWidth, pControlInit->nHeight, pControlInit->field_20);

	case D2WIN_PROGRESSBAR:
		return (Control*)PROGRESSBAR_Create(pControlInit->nX, pControlInit->nY, pControlInit->nWidth, pControlInit->nHeight);

	case D2WIN_ACCOUNTLIST:
		return (Control*)ACCOUNTLIST_Create(pControlInit->nX, pControlInit->nY, pControlInit->nWidth, pControlInit->nHeight, (int)pControlInit->field_2C, pControlInit->field_28);

	default:
		break;
	}

	return nullptr;
}

//D2Win.0x6F8AD450 (#10018)
int __stdcall CONTROL_Destroy(void* a1)
{
	//TODO: a1, result
	int result = 0;

	if (a1)
	{
		Control* v2 = *(Control**)a1;
		if (*(DWORD*)a1)
		{
			switch (v2->nType)
			{
			case D2WIN_IMAGE:
			case D2WIN_IMAGE2:
				result = IMAGE_Destroy(v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_ANIMIMAGE:
				result = ANIMIMAGE_Destroy(v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_SCROLLBAR:
				result = SCROLLBAR_Destroy(v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_TIMER:
				result = TIMER_Destroy(v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_PROGRESSBAR:
				result = PROGRESSBAR_Destroy(v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_TEXTBOX:
				result = D2Win_10043_TEXTBOX_Destroy((WinTextBox*)v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_EDITBOX:
				result = D2Win_10071_EDITBOX_Destroy((WinEditBox*)v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_BUTTON:
				result = BUTTON_Destroy((WinButton*)v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_LIST:
				result = LIST_Destroy((WinList*)v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_SMACK:
				result = SMACK_Destroy((WinSmack*)v2);
				*(DWORD*)a1 = 0;
				break;

			case D2WIN_ACCOUNTLIST:
				result = ACCOUNTLIST_Destroy((WinAccountList*)v2);
				*(DWORD*)a1 = 0;
				break;

			default:
				result = 0;
				*(DWORD*)a1 = 0;
				break;
			}
		}
	}

	return 0;
}

//D2Win.0x6F8AD570 (#10025)
void __fastcall D2Win_10025_CONTROL_ToggleFirstFlag(Control* pControl, int bSet)
{
	if (!pControl)
	{
		return;
	}

	if (bSet)
	{
		pControl->dwFlags |= gdwBitMasks[0];
	}
	else
	{
		pControl->dwFlags &= gdwInvBitMasks[0];
	}
}

//D2Win.0x6F8AD5A0 (#10024)
int __fastcall D2Win_10024_CONTROL_CheckFirstFlag(Control* pControl)
{
	D2_ASSERT(pControl);

	return pControl->dwFlags & gdwBitMasks[0];
}

//D2Win.0x6F8AD5D0 (#10027)
void __fastcall D2Win_10027_CONTROL_ToggleThirdFlag(Control* pControl, int bSet)
{
	D2_ASSERT(pControl);

	if (bSet)
	{
		pControl->dwFlags |= gdwBitMasks[2];
	}
	else
	{
		pControl->dwFlags &= gdwInvBitMasks[2];
	}
}

//D2Win.0x6F8AD620 (#10026)
int __fastcall D2Win_10026_CONTROL_CheckThirdFlag(Control* pControl)
{
	D2_ASSERT(pControl);

	return pControl->dwFlags & gdwBitMasks[2];
}

//D2Win.0x6F8AD650 (#10170)
BOOL __stdcall CONTROL_MouseInsideRect(const RECT* pRect)
{
	return PtInRect(pRect, gMousePosition_6F8FE234);
}

//D2Win.0x6F8AD670
void __stdcall D2Win_COMMANDS_MouseMove_6F8AD670(SMSGHANDLER_PARAMS* pMsg)
{
	int nX = (int)(int16_t)LOWORD(pMsg->lParam); // GET_X_LPARAM
	int nY = (int)(int16_t)HIWORD(pMsg->lParam); // GET_Y_LPARAM

	int nScreenWidth = 0;
	int nScreenHeight = 0;
	WINDOW_GetDimensions(&nScreenWidth, &nScreenHeight);
	--nScreenWidth;
	--nScreenHeight;

	if (D2GFX_HardwareAcceleratedRenderMode())
	{
		bool bSetCursorPos = false;
		if (nX > nScreenWidth)
		{
			nX = nScreenWidth;
			bSetCursorPos = true;
		}

		if (nY > nScreenHeight)
		{
			nY = nScreenHeight;
			bSetCursorPos = true;
		}

		if (bSetCursorPos)
		{
			SetCursorPos(nX, nY);
		}
	}

	gMousePosition_6F8FE234.x = nX;
	gMousePosition_6F8FE234.y = nY;
}

//D2Win.0x6F8AD6F0 (#10019)
void __stdcall D2Win_10019()
{
	if (WINDOW_GetState())
	{
		return;
	}

	for (Control* i = gpControlList_6F8FE24C; i; i = i->pNext)
	{
		if (i->pfInitialize)
		{
			i->pfInitialize(i);
		}
	}

	if (gpfWndProc_6F8FE268)
	{
		return;
	}

	int nScreenWidth = 0;
	int nScreenHeight = 0;
	WINDOW_GetDimensions(&nScreenWidth, &nScreenHeight);

	if (D2GFX_StartDraw(0, 0, 0, 0))
	{
		D2GFX_ClearScreen(0);

		if (gpBackgroundCellFile)
		{
			GfxData gfxData = {};
			gfxData.pCellFile = gpBackgroundCellFile;
			gfxData.nDirection = 0;
			gfxData.nFrame = 0;

			for (int y = 256; y < 768; y += 256)
			{
				for (int x = 0; x < 1024; x += 256)
				{
					TEXTURE_CelDraw(&gfxData, x, y, -1, DRAWMODE_NORMAL, 0);
					++gfxData.nFrame;
				}
			}

			for (int x = 0; x < 1024; x += 256)
			{
				TEXTURE_CelDraw(&gfxData, x, nScreenHeight - 1, -1, DRAWMODE_NORMAL, 0);
				++gfxData.nFrame;
			}
		}

		for (Control* i = gpControlList_6F8FE24C; i; i = i->pNext)
		{
			if (i->pfDraw && !(i->dwFlags & gdwBitMasks[3]))
			{
				i->pfDraw(i);
			}
		}

		for (void* i = D2Win_10147(); i; i = D2Win_10148(i))
		{
			sub_6F8A15E0(i);
		}

		for (void* i = D2Win_10147(); i; i = D2Win_10148(i))
		{
			sub_6F8A1890(i, -1);
		}

		for (void* i = D2Win_10147(); i; i = D2Win_10148(i))
		{
			sub_6F8A1D10(i);
			sub_6F8A2070(i);
		}

		if (dword_6F8FE278)
		{
			D2GFX_DrawBoxAlpha(0, 0, nScreenWidth, nScreenHeight, 0, 127u);
		}

		for (Control* i = gpControlList_6F8FE24C; i; i = i->pNext)
		{
			if (i->pfDraw && i->dwFlags & gdwBitMasks[3])
			{
				i->pfDraw(i);
			}
		}

		if (gpCursorCellFile && dword_6F8BDE18)
		{
			int nX = gMousePosition_6F8FE234.x;
			int nY = gMousePosition_6F8FE234.y;
			dword_6F8FE228 = gMousePosition_6F8FE234.x;
			dword_6F8FE22C = gMousePosition_6F8FE234.y;

			if (gMousePosition_6F8FE234.x <= 0)
			{
				nX = 0;
			}

			if (gMousePosition_6F8FE234.y <= 0)
			{
				nY = 0;
			}

			GfxData gfxData = {};
			gfxData.nDirection = 0;
			gfxData.pCellFile = gpCursorCellFile;
			gfxData.nFrame = gnCursorFrame;
			TEXTURE_CelDraw(&gfxData, nX, nY - 1, -1, DRAWMODE_NORMAL, 0);
		}

		D2GFX_EndScene();
	}

	if (dword_6F8FE254 <= 0)
	{
		D2Win_Helper();
	}
}

//D2Win.0x6F8AD9B0
LRESULT __stdcall D2Win_MAINWINDOW_WndProc_6F8AD9B0(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (!dword_6F8FE26C)
	{
		return 0;
	}

	if (gpfWndProc_6F8FE268)
	{
		return gpfWndProc_6F8FE268(hWnd, Msg, wParam, lParam);
	}

	if (Msg == WM_CLOSE)
	{
		dword_6F8FE280 = 1;
		PostQuitMessage(0);
		return 0;
	}

	int v5 = 0;
	LRESULT v6 = 0;

	if (SMsgDispatchMessage(hWnd, Msg, wParam, lParam, &v5, &v6) && v5)
	{
		return v6;
	}

	return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

//D2Win.0x6F8ADA80
void __stdcall D2Win_COMMANDS_Char_6F8ADA80(SMSGHANDLER_PARAMS* pMsg)
{
	//TODO: v4
	Control* pControl = dword_6F8FE264;
	if (dword_6F8FE25C)
	{
		if (dword_6F8FE264)
		{
			for (Control* i = dword_6F8FE264; i; i = i->pNext)
			{
				if ((WinEditBox*)i == dword_6F8FE25C)
				{
					if (D2Win_10074(dword_6F8FE25C, pMsg->wParam))
					{
						SMsgBreakHandlerChain(pMsg);
						// Should we return SMsgBreakHandlerChain's result?
						return;

					}

					break;
				}
			}
		}
		else
		{
			if (D2Win_10074(dword_6F8FE25C, pMsg->wParam))
			{
				SMsgBreakHandlerChain(pMsg);
				// Should we return SMsgBreakHandlerChain's result?
				return;
			}
		}
	}

	if (!pControl)
	{
		pControl = gpControlList_6F8FE24C;
	}

	while (pControl)
	{
		if (pControl->pfHandleCharInput)
		{
			pMsg->hWindow = (HWND)pControl;

			++dword_6F8FE254;
			const int v4 = pControl->pfHandleCharInput(pMsg);
			--dword_6F8FE254;

			if (v4)
			{
				break;
			}
		}

		pControl = pControl->pNext;
	}

	SMsgBreakHandlerChain(pMsg);
}

//D2Win.0x6F8ADB10
void __stdcall D2Win_COMMANDS_Escape_6F8ADB10(SMSGHANDLER_PARAMS* pMsg)
{
	if (dword_6F8FE260)
	{
		for (Control* pControl = dword_6F8FE264; pControl; pControl = pControl->pNext)
		{
			if (pControl == dword_6F8FE260)
			{
				pMsg->hWindow = (HWND)dword_6F8FE260;
				BUTTON_SimulateClick(pMsg);
				SMsgBreakHandlerChain(pMsg);
				// Should we return SMsgBreakHandlerChain's result?
				return;
			}
		}
	}

	D2Win_COMMANDS_VirtualKey_6F8ADB50(pMsg);
	// Should we return D2Win_COMMANDS_VirtualKey_6F8ADB50's result?
	return;
}

//D2Win.0x6F8ADB50
void __stdcall D2Win_COMMANDS_VirtualKey_6F8ADB50(SMSGHANDLER_PARAMS* pMsg)
{
	//TODO: v4
	Control* pControl = dword_6F8FE264;
	if (dword_6F8FE25C)
	{
		if (dword_6F8FE264)
		{
			for (Control* i = dword_6F8FE264; i; i = i->pNext)
			{
				if ((WinEditBox*)i == dword_6F8FE25C)
				{
					if (sub_6F8A7AB0(dword_6F8FE25C, pMsg->wParam))
					{
						SMsgBreakHandlerChain(pMsg);
						// Should we return SMsgBreakHandlerChain's result?$
						return;
					}

					break;
				}
			}
		}
		else
		{
			if (sub_6F8A7AB0(dword_6F8FE25C, pMsg->wParam))
			{
				SMsgBreakHandlerChain(pMsg);
				// Should we return SMsgBreakHandlerChain's result?
				return;
			}
		}
	}

	if (!pControl)
	{
		pControl = gpControlList_6F8FE24C;
	}

	while (pControl)
	{
		if (pControl->pfHandleVirtualKeyInput)
		{
			pMsg->hWindow = (HWND)pControl;
			++dword_6F8FE254;
			const int v4 = pControl->pfHandleVirtualKeyInput(pMsg);
			--dword_6F8FE254;
			if (v4)
			{
				SMsgBreakHandlerChain(pMsg);
				// Should we return SMsgBreakHandlerChain's result?
				return;
			}
		}

		pControl = pControl->pNext;
	}

	SMsgBreakHandlerChain(pMsg);
	// Should we return SMsgBreakHandlerChain's result?
	return;
}

//D2Win.0x6F8ADBE0
void __stdcall D2Win_COMMANDS_Control_6F8ADBE0(SMSGHANDLER_PARAMS* pMsg)
{
	dword_6F8FE274 = 1;
	SMsgBreakHandlerChain(pMsg);
	// Should we return SMsgBreakHandlerChain's result?
}

//D2Win.0x6F8ADBF0
void __stdcall D2Win_COMMANDS_Control_6F8ADBF0(SMSGHANDLER_PARAMS* pMsg)
{
	dword_6F8FE274 = 0;
	SMsgBreakHandlerChain(pMsg);
	// Should we return SMsgBreakHandlerChain's result?
}

//D2Win.0x6F8ADC00
void __stdcall D2Win_COMMANDS_SysCommand_6F8ADC00(SMSGHANDLER_PARAMS* pMsg)
{
	uint16_t v1 = pMsg->wParam & 0xFFF0;
	if (v1 == 0xF130 || v1 == 0xF100 || v1 == 0xF010 && WINDOW_IsFullScreen())
	{
		pMsg->bUseResult = 1;
		pMsg->lResult = 0;
	}
}

//D2Win.0x6F8ADC40
void __stdcall D2Win_COMMANDS_ActivateApp_6F8ADC40(SMSGHANDLER_PARAMS* pMsg)
{
	dword_6F8BDE20 = pMsg->wParam != 0;
	if (pMsg->wParam)
	{
		if (!FOG_IsHandlingError())
		{
			WINDOW_SetPaused(0);
			D2Win_10176(1);
		}

		D2SOUND_10049(dword_6F8FE284);
		dword_6F8FE288 = 0;
	}
	else
	{
		if (!dword_6F8FE288)
		{
			dword_6F8FE284 = D2SOUND_10048();
		}

		dword_6F8FE288 = 1;
		D2SOUND_10049(0);
		WINDOW_SetPaused(1);
	}

	if (WINDOW_CheckCutScene())
	{
		WINDOW_GetState();
	}

	pMsg->bUseResult = 0;
}

//D2Win.0x6F8ADCD0 (#10168)
void __stdcall D2Win_10168_WINMAIN_CreateScreenshot()
{
	const uint32_t dwTickCount = GetTickCount();

	if (dwTickCount - gdwScreenshotTickCount_6F8FE28C < 1000)
	{
		return;
	}

	int32_t nWidth = 0;
	int32_t nHeight = 0;
	WINDOW_GetDimensions(&nWidth, &nHeight);

	uint8_t* pBuffer = (uint8_t*)D2_ALLOC(3 * nWidth * nHeight);

	if (D2GFX_GetBackBuffer(pBuffer))
	{
		char szFilename[256] = {};
		while (1)
		{
			sprintf_s(szFilename, "Screenshot%03d.jpg", gnScreenshotCounter_6F8BDE1C++);
			const HANDLE v3 = FOG_CreateFileA(szFilename, 0x80000000u, 0, 0, 3, 128, 0);
			if (v3 == INVALID_HANDLE_VALUE)
			{
				break;
			}
			FOG_CloseFile(v3);
		}

		MooJpegProperties tJpegProperties; //  ijl's JPEG_CORE_PROPERTIESin in original code
		D2MooJpegLibInit(&tJpegProperties);
		tJpegProperties.nWidth = nWidth;
		tJpegProperties.nHeight = nHeight;
		tJpegProperties.pBuffer = pBuffer;
		tJpegProperties.szFileName = szFilename;
		tJpegProperties.nJQuality = 100;
		D2MooJpegLibWrite(&tJpegProperties);
		D2MooJpegLibFree(&tJpegProperties);
	}

	gdwScreenshotTickCount_6F8FE28C = dwTickCount;
	D2_FREE(pBuffer);
}

//D2Win.0x6F8ADE70
void __stdcall D2Win_COMMANDS_Snapshot_6F8ADE70(SMSGHANDLER_PARAMS* pMsg)
{
	D2Win_10168_WINMAIN_CreateScreenshot();

	pMsg->bUseResult = 1;
	pMsg->lResult = 0;
}

//D2Win.0x6F8AE020
void __stdcall D2Win_COMMANDS_KeyF1_6F8AE020(SMSGHANDLER_PARAMS* pMsg)
{
	if (!dword_6F8FE274 || !D2SOUND_10066())
	{
		return;
	}

	if (D2SOUND_10069())
	{
		D2SOUND_10065();
		D2SOUND_10070(0);
	}
	else
	{
		D2SOUND_10034(1);
		D2SOUND_10070(1);
	}

	pMsg->bUseResult = 1;
	pMsg->lResult = 0;
}

//D2Win.0x6F8AE070
void __stdcall D2Win_COMMANDS_MouseButton_6F8AE070(SMSGHANDLER_PARAMS* pMsg)
{
	//TODO: v10
	D2Win_COMMANDS_MouseMove_6F8AD670(pMsg);

	if (POPUP_IsActive())
	{
		const int32_t bInputHandled = POPUP_HandleMouseButtonInput(pMsg);
		POPUP_Destroy();
		if (bInputHandled)
		{
			return;
		}
	}

	Control* pControl = dword_6F8FE264;
	if (!dword_6F8FE264)
	{
		pControl = gpControlList_6F8FE24C;
	}

	switch (pMsg->nMessage)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		gbIsMouseButtonPressed = 1;

		while (pControl)
		{
			if ((pControl->pfShouldMouseInputBeHandled && pControl->pfShouldMouseInputBeHandled(pControl))
				|| (gMousePosition_6F8FE234.x >= pControl->nImageX && gMousePosition_6F8FE234.y >= pControl->nImageY - pControl->nHeight
					&& gMousePosition_6F8FE234.x < pControl->nImageX + pControl->nWidth && gMousePosition_6F8FE234.y < pControl->nImageY))
			{
				pMsg->hWindow = (HWND)pControl;
				if (pControl->pfHandleMouseDown)
				{
					++dword_6F8FE254;
					const int v10 = pControl->pfHandleMouseDown(pMsg);
					--dword_6F8FE254;
					if (v10)
					{
						SMsgBreakHandlerChain(pMsg);
						// Should we return SMsgBreakHandlerChain's result?
						return;
					}
				}
			}

			pControl = pControl->pNext;
		}

		SMsgBreakHandlerChain(pMsg);
		// Should we return SMsgBreakHandlerChain's result?
		return;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		gbIsMouseButtonPressed = 0;

		while (pControl)
		{
			pMsg->hWindow = (HWND)pControl;
			if (pControl->pfHandleMouseUp)
			{
				++dword_6F8FE254;
				pControl->pfHandleMouseUp(pMsg);
				--dword_6F8FE254;
			}
			pControl = pControl->pNext;
		}

		SMsgBreakHandlerChain(pMsg);
		// Should we return SMsgBreakHandlerChain's result?
		return;

	default:
		SMsgBreakHandlerChain(pMsg);
		// Should we return SMsgBreakHandlerChain's result?
		return;
	}
}

//D2Win.0x6F8AE220
void __stdcall D2Win_COMMANDS_MouseWheel_6F8AE220(SMSGHANDLER_PARAMS* pMsg)
{
	//TODO: Names
	const int nIncrement = (int)(int16_t)HIWORD(pMsg->wParam) / 120; // GET_WHEEL_DELTA_WPARAM

	D2Win_COMMANDS_MouseMove_6F8AD670(pMsg);

	if (POPUP_IsActive())
	{
		return;
	}

	Control* pFirst = dword_6F8FE264;
	Control* pScrollBar = nullptr;

	if (!dword_6F8FE264)
	{
		pFirst = gpControlList_6F8FE24C;
	}

	Control* pControl = pFirst;

	if (pFirst)
	{
		do
		{
			if (pControl->nType == D2WIN_SCROLLBAR && pControl->dwFlags & gdwBitMasks[2] && pControl->dwFlags & gdwBitMasks[0] && SCROLLBAR_GetMaxSteps((WinScrollBar*)pControl) > 0)
			{
				if (pScrollBar)
				{
					int nScrollBarX = 0;
					if (gMousePosition_6F8FE234.x < pScrollBar->nImageX)
					{
						nScrollBarX = gMousePosition_6F8FE234.x - pScrollBar->nImageX;
					}
					else if (gMousePosition_6F8FE234.x > pScrollBar->nWidth + pScrollBar->nImageX)
					{
						nScrollBarX = gMousePosition_6F8FE234.x - pScrollBar->nWidth - pScrollBar->nImageX;
					}

					int nScrollBarY = 0;
					if (gMousePosition_6F8FE234.y < pScrollBar->nImageY - pScrollBar->nHeight)
					{
						nScrollBarY = gMousePosition_6F8FE234.y + pScrollBar->nHeight - pScrollBar->nImageY;
					}
					else if (gMousePosition_6F8FE234.y > pScrollBar->nImageY)
					{
						nScrollBarY = gMousePosition_6F8FE234.y - pScrollBar->nImageY;
					}

					int nControlX = 0;
					if (gMousePosition_6F8FE234.x < pControl->nImageX)
					{
						nControlX = gMousePosition_6F8FE234.x - pControl->nImageX;
					}
					else if (gMousePosition_6F8FE234.x > pControl->nWidth + pControl->nImageX)
					{
						nControlX = gMousePosition_6F8FE234.x - pControl->nWidth - pControl->nImageX;
					}

					int nControlY = 0;
					if (gMousePosition_6F8FE234.y < pControl->nImageY - pControl->nHeight)
					{
						nControlY = gMousePosition_6F8FE234.y + pControl->nHeight - pControl->nImageY;
					}
					else if (gMousePosition_6F8FE234.y > pControl->nImageY)
					{
						nControlY = gMousePosition_6F8FE234.y - pControl->nImageY;
					}

					if (!nScrollBarY && nScrollBarX <= 0)
					{
						if (nControlY == 0 && nControlX <= 0 && nScrollBarX <= nControlX)
						{
							pScrollBar = pControl;
						}
					}
					else
					{
						if ((nControlY == 0 && nControlX <= 0)
							|| (nScrollBarX * nScrollBarX + nScrollBarY * nScrollBarY >= nControlX * nControlX + nControlY * nControlY))
						{
							pScrollBar = pControl;
						}
					}
				}
				else
				{
					pScrollBar = pControl;
				}
			}

			pControl = pControl->pNext;
		}
		while (pControl);

		if (pScrollBar && sub_6F8AF240(pMsg, (WinScrollBar*)pScrollBar, nIncrement))
		{
			SMsgBreakHandlerChain(pMsg);
			// Should we return SMsgBreakHandlerChain's result?
			return;
		}
	}

	for (Control* i = pFirst; i; i = i->pNext)
	{
		if (i->nType == D2WIN_BUTTON && BUTTON_OnMouseWheelScrolled((WinButton*)i, nIncrement))
		{
			break;
		}
	}

	SMsgBreakHandlerChain(pMsg);
	// Should we return SMsgBreakHandlerChain's result?
	return;
}

//D2Win.0x6F8AE4A0 (#10029)
void __stdcall D2Win_10029(int a1)
{
	dword_6F8BDE18 = a1;
}

//D2Win.0x6F8AE4B0 (#10030)
void __stdcall D2Win_10030_CONTROL_ToggleFourthFlag(Control* pControl, int bSet)
{
	if (bSet)
	{
		pControl->dwFlags |= gdwBitMasks[3];
	}
	else
	{
		pControl->dwFlags &= gdwInvBitMasks[3];
	}
}

//D2Win.0x6F8AE4F0 (#10031)
void __stdcall D2Win_10031(void* a1)
{
	dword_6F8FE260 = a1;
}

//D2Win.0x6F8AE500 (#10032)
int __stdcall D2Win_IsMouseButtonPressed()
{
	return gbIsMouseButtonPressed;
}

//D2Win.0x6F8AE510 (#10033)
int __stdcall D2Win_10033(Control* a1, int a2)
{
	dword_6F8FE264 = a1;
	dword_6F8FE278 = a2;
	return 1;
}

//D2Win.0x6F8AE530 (#10035)
int __stdcall CONTROL_GetType(Control* pControl)
{
	if (pControl)
	{
		return pControl->nType;
	}

	return 0;
}

//D2Win.0x6F8AE540
Control* __stdcall sub_6F8AE540()
{
	return dword_6F8FE264;
}
