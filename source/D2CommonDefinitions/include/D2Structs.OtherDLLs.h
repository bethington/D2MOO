#pragma once

// Archive of types that are probably coming from other DLLs.


// ---------- D2Client ----------

struct AltDraw
{
	int32_t nLeft;								//0x000
	int32_t nTop;								//0x004
	int32_t nRight;								//0x008
	int32_t nBottom;							//0x00C
	UnitAny* pItem;							//0x010
	wchar_t wszName[128];						//0x014
	int32_t nRectColor;							//0x114
	int32_t nRectDrawMode;						//0x118
	int32_t nColor;								//0x11C
};

struct AnvilUIButton
{
	uint16_t nStringId;							//0x00
	int32_t nTextX;								//0x02
	int32_t nTextY;								//0x06
	int32_t nCellfileX;							//0x0A
	int32_t nCellfileY;							//0x0E
	int32_t nClickAreaXStart;					//0x12
	int32_t nClickAreaXEnd;						//0x16
	int32_t nClickAreaYStart;					//0x1A
	int32_t nClickAreaYEnd;						//0x1E
	int32_t nFrame;								//0x22
	uint8_t bButtonClicked;						//0x26
	uint8_t unk0x27;							//0x27
};

struct AutomapCell
{
	uint32_t fSaved;							//0x00
	uint16_t nCellNo;							//0x04
	uint16_t xPixel;							//0x06
	uint16_t yPixel;							//0x08
	uint16_t wWeight;							//0x0A
	AutomapCell* pPrev;				//0x0C
	AutomapCell* pNext;				//0x10
};

struct AutomapLayer
{
	uint32_t nLayerNo;							//0x00
	uint32_t fSaved;							//0x04
	AutomapCell* pFloors;				//0x08
	AutomapCell* pWalls;				//0x0C
	AutomapCell* pObjects;			//0x10
	AutomapCell* pExtras;				//0x14
	AutomapLayer* pNext;				//0x18
};


struct BuySellTab
{
	int32_t nX;									//0x00
	int32_t nY;									//0x04
	uint16_t nStringIndex;						//0x06
	BOOL bSelected;							//0x0A
	BOOL bActive;							//0x0E
};


struct KeyConfigOption
{
	uint16_t nStringIndex;					//0x00
	void* pCallback;						//0x02
	WORD word0x06;							//0x06
	int nTextBeginX;						//0x08
	int nTextEndX;							//0x0C
	int nTextBeginY;						//0x10
	int nTextEndY;							//0x14
	WORD word0x18;							//0x18
};

struct KeyConfig
{
	int32_t nConfig;							//0x00
	uint16_t nStringIndex;						//0x04
	int32_t unk0x06;							//0x06
};



typedef int32_t(__fastcall* MENUENABLED)();
typedef void(__fastcall* MENUSELECT)(MenuItem*, SMSGHANDLER_PARAMS*);
typedef void(__fastcall* MENUOPTION)(MenuItem*, int32_t);
typedef void(__fastcall* MENUUPDATE)(MenuItem*, int32_t);

struct MenuEntry
{
	int32_t nType;								//0x00
	int32_t nLeft;								//0x04
	int32_t nTop;								//0x08
	int32_t nWidth;								//0x0C
	int32_t nHeight;							//0x10
	int32_t unk0x014;								//0x14
	int32_t nStrIndex;							//0x18
	GfxData* pGfxData;				//0x1C
	int32_t(__stdcall* pBtnFunction)(void*);	//0x20
	uint32_t unk0x024[2];							//0x24
	int32_t nFont;								//0x2C
};

struct MenuInfo
{
	int32_t nItemCount;							//0x00
	int32_t unk0x04;							//0x04
	int32_t unk0x08;							//0x08
	int32_t unk0x0C;							//0x0C
	int32_t unk0x10;							//0x10
	int32_t unk0x14;							//0x14
};

struct MenuItem
{
	uint32_t dwType;							//0x00
	BOOL bExpansion;						//0x04
	int32_t nHeight;							//0x08
	char szImage[260];						//0x0C
	MENUENABLED pfEnabled;					//0x110
	MENUSELECT pfSelect;					//0x114			
	MENUOPTION pfOption;					//0x118
	MENUUPDATE pfUpdate;					//0x11C
	uint32_t dwCount;							//0x120
	uint32_t dwMoveCount;						//0x124
	uint32_t dwSliderType;						//0x128
	char szChoices[4][260];					//0x12C
	CellFile* pImage;					//0x53C
	CellFile* pChoices[4];			//0x540
};

struct MenuUIStateSave
{
	BOOL bCloseWhenOpen;					//0x00
	BOOL bSaveUIState;						//0x04
	int32_t nUIState;							//0x08
};


struct QuestDesc
{
	uint16_t wTblTitle;							//0x00
	uint16_t wTblInitDesc;						//0x02
};

struct QuestDescriptor
{
	uint8_t bActive;							//0x00
	uint8_t nQuestNo;							//0x01
	uint8_t nPosition;							//0x02
	uint8_t nTab;								//0x03
	uint16_t* pStringIds;						//0x04
	int32_t nQuestFlag;							//0x08
	int32_t nArrayId;							//0x0C
};

struct QuestUiButtonCoord
{
	int32_t nCellfileX;							//0x00
	int32_t nCellfileY;							//0x04
	int32_t nClickX;							//0x08
	int32_t nClickY;							//0x0C
};

struct QuestUi
{
	uint8_t field_0;							//0x00
	int32_t nQuest;								//0x01
	int16_t nQuestTitleStringId;				//0x05
	wchar_t wszQuestDesc[300];				//0x07
	int16_t field_25F;						//0x25F
	uint8_t nPosition;							//0x261
	uint8_t nQuestNo;							//0x262
	uint8_t field_263;							//0x263
	uint8_t field_264;							//0x264
	uint8_t field_265;							//0x265
	int32_t nQuestState;						//0x266
};

struct QuestUiTab
{
	int32_t nStartQuest;						//0x00
	int32_t nEndQuest;							//0x04
};


struct WindowPlacement
{
	HWND hWnd;								//0x00
	WINDOWPLACEMENT windowPlacement;		//0x04
};

struct SMSGHANDLER_PARAMS
{
	HWND hWnd;								//0x00
	uint32_t uMessage;							//0x04
	union									//0x08
	{
		WPARAM  wParam;
		int32_t	  nKey;
	};
	union									//0x0C
	{
		struct
		{
			uint16_t	nXpos;
			uint16_t	nYpos;
		};
		LPARAM   lParam;
	};
	uint32_t dwCommandSource;					//0x10 HIWORD(wParam) when uMessage == WM_COMMAND
	uint32_t dwArg;							//0x14
	BOOL bReturn;							//0x18
	LRESULT lResult;						//0x1C
};
