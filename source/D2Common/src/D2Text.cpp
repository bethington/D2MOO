#include "D2Text.h"
#include <Fog.h>

//D2Common.0x6FDC36E0 (#10901)
TextHeader* __stdcall TEXT_AllocTextHeader(void* pMemPool)
{
	TextHeader* pTextHeader = D2_ALLOC_STRC_POOL(pMemPool, TextHeader);
	pTextHeader->pMemPool = pMemPool;
	pTextHeader->nCount = 0;
	pTextHeader->pNode = NULL;
	
	return pTextHeader;
}

//D2Common.0x6FDC3710 (#10902)
void __stdcall TEXT_FreeTextHeader(TextHeader* pTextHeader)
{
	TextNode* pNode = NULL;
	TextNode* pNext = NULL;

	pNode = pTextHeader->pNode;
	while (pNode)
	{
		pNext = pNode->pNext;
		D2_FREE_POOL(pTextHeader->pMemPool, pNode);
		pNode = pNext;
	}
	pTextHeader->nCount = 0;
	D2_FREE_POOL(pTextHeader->pMemPool, pTextHeader);
}

//D2Common.0x6FDC3760 (#10903)
void __stdcall TEXT_AddNodeToTextList(TextHeader* pTextHeader, short nStringId, int nMenu)
{
	TextNode* pNode = NULL;

	pNode = D2_ALLOC_STRC_POOL(pTextHeader->pMemPool, TextNode);
	pNode->pNext = pTextHeader->pNode;
	pNode->nStringId = nStringId;
	pNode->nMenu = nMenu;

	++pTextHeader->nCount;
	pTextHeader->pNode = pNode;
}

//D2Common.0x6FDC37A0 (#10904)
void __stdcall TEXT_RemoveNodeFromTextList(TextHeader* pTextHeader, short nStringId)
{
	TextNode* pPrevious = NULL;
	TextNode* pNode = NULL;

	pNode = pTextHeader->pNode;
	if (pNode)
	{
		while (pNode->nStringId != nStringId)
		{
			pPrevious = pNode;

			pNode = pNode->pNext;

			if (!pNode)
			{
				return;
			}
		}

		if (pPrevious)
		{
			pPrevious->pNext = pNode->pNext;
		}
		else
		{
			pTextHeader->pNode = pNode->pNext;
		}

		D2_FREE_POOL(pTextHeader->pMemPool, pNode);
		--pTextHeader->nCount;
	}
}

//D2Common.0x6FDC37F0 (#10905)
int __stdcall TEXT_GetMenuType2NodeCount(TextHeader* pTextHeader)
{
	int nNodes = 0;

	for (TextNode* pNode = pTextHeader->pNode; pNode; pNode = pNode->pNext)
	{
		if (pNode->nMenu == 2)
		{
			++nNodes;
		}
	}

	return nNodes;
}

//D2Common.0x6FDC33820 (#10906)
short __stdcall TEXT_GetStringIdOfMenuType2Node(TextHeader* pTextHeader, int nNodeId)
{
	TextNode* pNode = NULL;
	int nCounter = 0;

	pNode = pTextHeader->pNode;

	while (pNode)
	{
		if (pNode->nMenu == 2)
		{
			if (nCounter == nNodeId)
			{
				return pNode->nStringId;
			}

			++nCounter;
		}

		pNode = pNode->pNext;
	}

	return 3724;
}

//D2Common.0x6FDC3850 (#10907)
short __stdcall TEXT_GetStringIdOfMenuType1Or2Node(TextHeader* pTextHeader, int nNodeId)
{
	TextNode* pNode = NULL;
	int nCounter = 0;

	pNode = pTextHeader->pNode;

	while (pNode)
	{
		if (pNode->nMenu != 0)
		{
			if (nCounter == nNodeId)
			{
				return pNode->nStringId;
			}

			++nCounter;
		}

		pNode = pNode->pNext;
	}

	return -1;
}

//D2Common.0x6FDC3890 (#10908)
short __stdcall TEXT_GetStringIdOfMenuType0Or2Node(TextHeader* pTextHeader, int nNodeId)
{
	TextNode* pNode = NULL;
	int nCounter = 0;

	pNode = pTextHeader->pNode;

	while (pNode)
	{
		if (pNode->nMenu != 1)
		{
			if (nCounter == nNodeId)
			{
				return pNode->nStringId;
			}

			++nCounter;
		}

		pNode = pNode->pNext;
	}

	return -1;
}

//D2Common.0x6FDC38C0 (#10909)
void __stdcall TEXT_CreateMessageListFromTextHeader(TextHeader* pTextHeader, MessageList* pMsgList)
{
	D2_ASSERT(pTextHeader != nullptr);
	D2_ASSERT(pMsgList != nullptr);

	memset(pMsgList, 0x00, sizeof(MessageList));

	pMsgList->nCount = LOBYTE(pTextHeader->nCount);
	
	int i = 0;
	for (TextNode* pNode = pTextHeader->pNode; pNode != nullptr; pNode = pNode->pNext)
	{
		D2_ASSERT(i < MAX_TEXT_LIST_NODES);

		pMsgList->pMessages[i].nMenu = LOBYTE(pNode->nMenu);
		pMsgList->pMessages[i].nStringId = pNode->nStringId;
		++i;
	}
}

//D2Common.0x6FDC3970 (#10910)
void __stdcall TEXT_CreateTextHeaderFromMessageList(TextHeader* pTextHeader, MessageList* pMsgList)
{
	D2_ASSERT(pTextHeader != nullptr);
	D2_ASSERT(pMsgList != nullptr);

	TextNode* pNext;
	for (TextNode* pCurrent = pTextHeader->pNode; pCurrent != nullptr; pCurrent = pNext)
	{
		pNext = pCurrent->pNext;
		D2_FREE_POOL(pTextHeader->pMemPool, pCurrent);
	}

	D2_ASSERT(pMsgList->nCount < MAX_TEXT_LIST_NODES);

	for (pTextHeader->nCount = 0; pTextHeader->nCount < pMsgList->nCount; ++pTextHeader->nCount)
	{
		TextNode* pNode = D2_ALLOC_STRC_POOL(pTextHeader->pMemPool, TextNode);
		pNode->nMenu = pMsgList->pMessages[pTextHeader->nCount].nMenu;
		pNode->nStringId = pMsgList->pMessages[pTextHeader->nCount].nStringId;
		pNode->pNext = pTextHeader->pNode;
		pTextHeader->pNode = pNode;
	}
}

//D2Common.0x6FDC3A70 (#10911)
void __stdcall TEXT_SortTextNodeListByStringId(TextHeader* pTextHeader)
{
	D2_ASSERT(pTextHeader != nullptr);

	// The list is already sorted if it is empty, or has one element.
	if (pTextHeader->nCount <= 1)
	{
		return;
	}

	// Copy all of the input into a separate node list for sorting.
	// TODO: Replace this with array version of D2_ALLOC_STRC_POOL.
	TextNode* pNodeList = (TextNode*)D2_ALLOC_POOL(pTextHeader->pMemPool, sizeof(TextNode) * pTextHeader->nCount);

	int nodeIndex = 0;
	for (TextNode* pNode = pTextHeader->pNode; pNode != nullptr; pNode = pNode->pNext)
	{
		pNodeList[nodeIndex].nStringId = pNode->nStringId;
		pNodeList[nodeIndex].nMenu = pNode->nMenu;

		++nodeIndex;
	}

	// Sort the node list via insertion sort.
	short nPreviousStringId = pNodeList[0].nStringId;
	int nPreviousMenu = pNodeList[0].nMenu;

	for (size_t i = 0; i + 1 < pTextHeader->nCount; ++i)
	{
		short nStringId = pNodeList[i + 1].nStringId;
		int nMenu = pNodeList[i + 1].nMenu;

		// If the current node is sorted, move to the next node.
		if (nPreviousStringId <= nStringId)
		{
			nPreviousMenu = nMenu;
			nPreviousStringId = nStringId;
			continue;
		}

		// Current node is not sorted, shift bigger elements forward by one index.
		pNodeList[i + 1].nStringId = nPreviousStringId;
		pNodeList[i + 1].nMenu = nPreviousMenu;

		int nId;
		for (nId = i; nId != 0; --nId)
		{
			if (pNodeList[nId - 1].nStringId <= nStringId)
			{
				break;
			}

			pNodeList[nId].nMenu = pNodeList[nId - 1].nMenu;
			pNodeList[nId].nStringId = pNodeList[nId - 1].nStringId;
		}

		pNodeList[nId].nMenu = nMenu;
		pNodeList[nId].nStringId = nStringId;
	}

	// Copy the sorted node list to the input node list.
	TextNode* pNode = pTextHeader->pNode;
	for (int i = 0; i < pTextHeader->nCount; ++i)
	{
		pNode->nStringId = pNodeList[i].nStringId;
		pNode->nMenu = pNodeList[i].nMenu;

		pNode = pNode->pNext;
	}

	D2_FREE_POOL(pTextHeader->pMemPool, pNodeList);
}
