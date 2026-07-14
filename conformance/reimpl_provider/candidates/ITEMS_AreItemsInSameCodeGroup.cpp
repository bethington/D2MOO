#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: ITEMS_AreItemsInSameCodeGroup
extern "C" int __stdcall ITEMS_AreItemsInSameCodeGroup(void* pItem1, void* pItem2)
{
	if (pItem1 == (void*)0 || pItem2 == (void*)0) {
		return 0;
	}

	uint32_t uCode1 = *(uint32_t*)((char*)pItem1 + 4);
	uint32_t uCode2 = *(uint32_t*)((char*)pItem2 + 4);

	if (uCode1 == uCode2) {
		return 1;
	}

	typedef void* (__stdcall *GetItemDataRecord_t)(uint32_t);
	GetItemDataRecord_t pGetItemDataRecord = (GetItemDataRecord_t)D2MOO_Resolve("GetItemDataRecord");
	if (pGetItemDataRecord == 0) return 0;

	void* pItemData1 = pGetItemDataRecord(uCode1);
	void* pItemData2 = pGetItemDataRecord(uCode2);

	/* Code Group 1: g_dwCodeGroup1Count at 0x6fdef08c, g_abCodeGroup1Codes at 0x6fdef090 */
	uint32_t dwCodeGroup1Count = *(uint32_t*)0x6fdef08c;
	uint32_t* pCodeGroup1Codes = (uint32_t*)0x6fdef090;
	int fItem1Found = 0;
	int fItem2Found = 0;
	if (dwCodeGroup1Count != 0) {
		uint32_t dwIndex = 0;
		do {
			if ((!fItem1Found) && (*(uint32_t*)((char*)pItemData1 + 0x80) == pCodeGroup1Codes[dwIndex])) {
				fItem1Found = 1;
			}
			if ((!fItem2Found) && (*(uint32_t*)((char*)pItemData2 + 0x80) == pCodeGroup1Codes[dwIndex])) {
				fItem2Found = 1;
			}
			if ((fItem1Found) && (fItem2Found)) {
				return 1;
			}
			dwIndex = dwIndex + 1;
		} while (dwIndex < dwCodeGroup1Count);
	}

	/* Code Group 2: g_dwCodeGroup2Count at 0x6fdef074, g_dwCodeGroup2Values at 0x6fdef078 */
	uint32_t dwCodeGroup2Count = *(uint32_t*)0x6fdef074;
	int* pCodeGroup2Values = (int*)0x6fdef078;
	int bGroup2Found = 0;
	int bGroup3Found = 0;
	if (dwCodeGroup2Count != 0) {
		uint32_t dwGroupIndex = 0;
		do {
			if ((!bGroup2Found) && (*(int*)((char*)pItemData1 + 0x80) == *(int*)((char*)pCodeGroup2Values + dwGroupIndex * 4))) {
				bGroup2Found = 1;
			}
			if ((!bGroup3Found) && (*(int*)((char*)pItemData2 + 0x80) == *(int*)((char*)pCodeGroup2Values + dwGroupIndex * 4))) {
				bGroup3Found = 1;
			}
			if ((bGroup2Found) && (bGroup3Found)) {
				return 1;
			}
			dwGroupIndex = dwGroupIndex + 1;
		} while (dwGroupIndex < dwCodeGroup2Count);
	}

	/* Code Group 3: g_dwCodeGroup3Count at 0x6fdef068, g_abCodeGroup3Codes at 0x6fdef06c */
	uint32_t dwCodeGroup3Count = *(uint32_t*)0x6fdef068;
	int* pCodeGroup3Codes = (int*)0x6fdef06c;
	int fItem1Found_CodeGroup3 = 0;
	int fItem2Found_CodeGroup3 = 0;
	if (dwCodeGroup3Count != 0) {
		uint32_t uVar1 = 0;
		do {
			if ((!fItem1Found_CodeGroup3) && (*(int*)((char*)pItemData1 + 0x80) == *(int*)((char*)pCodeGroup3Codes + uVar1 * 4))) {
				fItem1Found_CodeGroup3 = 1;
			}
			if ((!fItem2Found_CodeGroup3) && (*(int*)((char*)pItemData2 + 0x80) == *(int*)((char*)pCodeGroup3Codes + uVar1 * 4))) {
				fItem2Found_CodeGroup3 = 1;
			}
			if ((fItem1Found_CodeGroup3) && (fItem2Found_CodeGroup3)) {
				return 1;
			}
			uVar1 = uVar1 + 1;
		} while (uVar1 < dwCodeGroup3Count);
	}

	return 0;
}
