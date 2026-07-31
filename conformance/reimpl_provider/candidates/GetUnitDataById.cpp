#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetUnitDataById
extern "C" int __stdcall GetUnitDataById(int nUnitId, void* pOutputBuffer)
{
	// Resolve globals by NAME (drop leading underscore from Ghidra's _g_Foo)
	char* pCritSec          = (char*)D2MOO_Resolve("g_pCritSec");
	char* pGameStateVtbl    = (char*)D2MOO_Resolve("g_pGameStateVtbl");
	char* pSNetProvider     = (char*)D2MOO_Resolve("g_pCurrentSNetProvider");
	char* pLocalDisconnected= (char*)D2MOO_Resolve("g_bLocalDisconnected");
	char* pUnitBaseOffset   = (char*)D2MOO_Resolve("g_dwUnitBaseOffset");
	char* pLastError        = (char*)D2MOO_Resolve("g_dwLastError");

	if (!pCritSec || !pGameStateVtbl || !pSNetProvider || !pLocalDisconnected || !pUnitBaseOffset || !pLastError)
		return 0;

	// Step 1: validate pOutputBuffer != NULL AND dwMagicMarker == 0x24
	// Decompile: ((-(uint)(pOutputBuffer!=NULL) & ((magic!=0x24)-1)) == 0) -> error
	if (pOutputBuffer == (void*)0x0 || *(uint32_t*)pOutputBuffer != 0x24) {
		*(uint32_t*)pLastError = 0x57;
		return 0;
	}

	// EnterCritSec(&g_pCritSec) -- skipped: function-pointer call to DAT_0004d708 (not resolvable)

	// Step 3: clear 8 data fields (offsets +4 through +0x20)
	uint32_t* pClear = (uint32_t*)((char*)pOutputBuffer + 0x04);
	pClear[0] = 0; // +0x04
	pClear[1] = 0; // +0x08
	pClear[2] = 0; // +0x0C
	pClear[3] = 0; // +0x10
	pClear[4] = 0; // +0x14
	pClear[5] = 0; // +0x18
	pClear[6] = 0; // +0x1C
	pClear[7] = 0; // +0x20

	// nDataPtr = *_g_pCurrentSNetProvider (g_p prefix -> pointer var -> single deref)
	char* nDataPtr = *(char**)pSNetProvider;

	// Step 4: state check -- g_pGameStateVtbl and g_pCurrentSNetProvider both non-NULL
	char* gameStateVtbl = *(char**)pGameStateVtbl;
	if (gameStateVtbl == (char*)0x0 || nDataPtr == (char*)0x0) {
		*(uint32_t*)pLastError = 0x4B4;
		return 0;
	}

	// g_bLocalDisconnected == -1 (signed-byte comparison)
	if (*(int8_t*)pLocalDisconnected == (int8_t)-1) {
		*(uint32_t*)pLastError = 0x85100070;
		return 0;
	}

	// Step 6: FindUnitById(nUnitId - (signed char)g_dwUnitBaseOffset)
	// NEEDS GLOBAL: FindUnitById
	int baseOffsetSigned = (int)(int8_t)*(uint32_t*)pUnitBaseOffset;
	int adjustedId = nUnitId - baseOffsetSigned;

	typedef void* (__stdcall *FindUnitByIdFn)(int);
	void* pFindUnitById = D2MOO_Resolve("FindUnitById");
	void* pUnit = (void*)0x0;
	if (pFindUnitById) {
		pUnit = ((FindUnitByIdFn)pFindUnitById)(adjustedId);
	}

	if (pUnit == (void*)0x0) {
		*(uint32_t*)pLastError = 0x8510006A;
		return 0;
	}

	// Step 7: copy 9 dwords from (nDataPtr + 0x214) to pOutputBuffer (offsets 0x00..0x20)
	uint32_t* src = (uint32_t*)(nDataPtr + 0x214);
	uint32_t* dst = (uint32_t*)pOutputBuffer;
	for (int i = 0; i < 9; i++) {
		dst[i] = src[i];
	}

	// Step 8: conditional copy of pUnit[3].dwAct (= pUnit + 0x128) to output[5] (= +0x18) if non-zero
	uint32_t extraField = *(uint32_t*)((char*)pUnit + 0x128);
	if (extraField != 0) {
		dst[6] = extraField; // +0x18 (dwAdwData5)
	}

	// LeaveCritSec(&g_pCritSec) -- skipped: function-pointer call to DAT_0004d6f0 (not resolvable)
	return 1;
}
