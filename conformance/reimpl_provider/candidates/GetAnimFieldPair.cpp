#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetAnimFieldPair
extern "C" int __stdcall GetAnimFieldPair(int nMissileId)
{
	char* tbl = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
	if (!tbl) return -1; // resolver missing / name unknown -> obvious mismatch

	int count = *(int*)(tbl + 0xBC0);          // sgptDataTables->field_0xBC0 (nOverlayTxtRecordCount)
	uint32_t basePtr = *(uint32_t*)(tbl + 0xBBC); // sgptDataTables->field_0xBBC (pOverlayTxt)

	if (((-1 < nMissileId) && (nMissileId < count)) &&
	   ((char*)basePtr + nMissileId * 0x84 != (char*)0)) {
		return *(int*)((char*)basePtr + nMissileId * 0x84 + 0x58);
	}
	return 0;
}
