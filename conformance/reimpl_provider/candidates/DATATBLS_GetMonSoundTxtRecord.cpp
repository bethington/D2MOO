#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetMonSoundTxtRecord
extern "C" void* __fastcall DATATBLS_GetMonSoundTxtRecord(int nMonClassId)
{
	void* _g = D2MOO_Resolve("g_pDataTables");
	if (!_g) return (void*)0xDEADBEEF; // resolver missing -> obvious mismatch
	char* base = (char*)*(void**)_g;
	if (!base) return (void*)0xDEADBEEF; // deref null -> obvious mismatch

	// Step 1: Validate monster ID in [0, nMonStatsCount)
	if (nMonClassId < 0) return nullptr;
	int nMonStatsCount = *(int*)(base + 0xa80);
	if (nMonClassId >= nMonStatsCount) return nullptr;

	// Step 2: Look up MonStats record, read wMonSoundId at offset +0x18 (sign-extended short, MOVSX)
	char* pMonStatsTxt = *(char**)(base + 0xa78);
	int soundId = (int)*(short*)(pMonStatsTxt + nMonClassId * 0x1a8 + 0x18);

	// Step 3: Validate sound ID in [0, nMonSoundCount)
	if (soundId < 0) return nullptr;
	int nMonSoundCount = *(int*)(base + 0xa98);
	if (soundId >= nMonSoundCount) return nullptr;

	// Step 4: Return pMonSoundTxt + soundId * 0x134
	char* pMonSoundTxt = *(char**)(base + 0xa90);
	return (void*)(pMonSoundTxt + soundId * 0x134);
}
