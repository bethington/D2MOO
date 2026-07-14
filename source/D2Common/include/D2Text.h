#pragma once

#include "D2CommonDefinitions.h"
#include "D2PacketDef.h"

#pragma pack(1)

#define MAX_TEXT_LIST_NODES 8

struct TextNode
{
	int16_t nStringId;			//0x00
	uint16_t pad0x02;			//0x02
	int32_t nMenu;				//0x04
	TextNode* pNext;		//0x08
};

struct TextHeader
{
	void* pMemPool;				//0x00
	uint16_t nCount;			//0x04
	uint16_t pad0x06;			//0x06
	TextNode* pNode;		//0x08
};

struct TextMessage
{
	wchar_t* pStringLines[6];	//0x00
	uint16_t nLines;			//0x18
	int32_t nColor;				//0x1A
	uint32_t nEndTick;			//0x1E
	TextMessage* pNext;	//0x22
};
#pragma pack()

//D2Common.0x6FDC36E0 (#10901)
D2COMMON_DLL_DECL TextHeader* __stdcall TEXT_AllocTextHeader(void* pMemPool);
//D2Common.0x6FDC3710 (#10902)
D2COMMON_DLL_DECL void __stdcall TEXT_FreeTextHeader(TextHeader* pTextHeader);
//D2Common.0x6FDC3760 (#10903)
D2COMMON_DLL_DECL void __stdcall TEXT_AddNodeToTextList(TextHeader* pTextHeader, short nStringId, int nMenu);
//D2Common.0x6FDC37A0 (#10904)
D2COMMON_DLL_DECL void __stdcall TEXT_RemoveNodeFromTextList(TextHeader* pTextHeader, short nStringId);
//D2Common.0x6FDC37F0 (#10905)
D2COMMON_DLL_DECL int __stdcall TEXT_GetMenuType2NodeCount(TextHeader* pTextHeader);
//D2Common.0x6FDC33820 (#10906)
D2COMMON_DLL_DECL short __stdcall TEXT_GetStringIdOfMenuType2Node(TextHeader* pTextHeader, int nNodeId);
//D2Common.0x6FDC3850 (#10907)
D2COMMON_DLL_DECL short __stdcall TEXT_GetStringIdOfMenuType1Or2Node(TextHeader* pTextHeader, int nNodeId);
//D2Common.0x6FDC3890 (#10908)
D2COMMON_DLL_DECL short __stdcall TEXT_GetStringIdOfMenuType0Or2Node(TextHeader* pTextHeader, int nNodeId);
//D2Common.0x6FDC38C0 (#10909)
D2COMMON_DLL_DECL void __stdcall TEXT_CreateMessageListFromTextHeader(TextHeader* pTextHeader, MessageList* pMsgList);
//D2Common.0x6FDC3970 (#10910)
D2COMMON_DLL_DECL void __stdcall TEXT_CreateTextHeaderFromMessageList(TextHeader* pTextHeader, MessageList* pMsgList);
//D2Common.0x6FDC3A70 (#10911)
D2COMMON_DLL_DECL void __stdcall TEXT_SortTextNodeListByStringId(TextHeader* pTextHeader);
