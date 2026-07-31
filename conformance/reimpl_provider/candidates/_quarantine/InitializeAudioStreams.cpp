#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeAudioStreams
// NEEDS GLOBAL: g_pCritSecGameState
// NEEDS GLOBAL: g_pSmackFunctionTable

extern "C" int __stdcall SMACK_InitializeSmackLibrary(void);
extern "C" void* __stdcall MEMORY_AllocateMemoryFromArena(uint32_t size, const char* sourceFile, uint32_t sourceLine, uint32_t flags);
extern "C" int __stdcall SCODE_ProcessAudioStreamWithFormat(const char* primary, const char* secondary, void* param3, int samples, int flags, void** outStreamPtr);

extern "C" int __stdcall InitializeAudioStreams(uint32_t dwAudioContext)
{
	// Resolve g_pCritSecGameState (pointer variable: deref resolved addr once)
	void* critSecAddr = D2MOO_Resolve("g_pCritSecGameState");
	if (!critSecAddr)
		return -1;
	char* critSecBase = (char*)*(void**)critSecAddr;

	// Resolve g_pSmackFunctionTable (pointer variable: deref resolved addr once)
	void* smackAddr = D2MOO_Resolve("g_pSmackFunctionTable");
	if (!smackAddr)
		return -2;
	char* smackBase = (char*)*(void**)smackAddr;

	int smackResult = SMACK_InitializeSmackLibrary();
	if (smackResult == 0)
		return 0;

	if (critSecBase == (char*)0x0) {
		critSecBase = (char*)MEMORY_AllocateMemoryFromArena(0xC, "g_pSVIDSourceFile", 0x23A, 0);
		*(char**)critSecAddr = critSecBase;

		SCODE_ProcessAudioStreamWithFormat(
			"1 W2=S W1=W2 2 D=W",
			"1 W1=W2 W2=S W1=TW 2 D=W",
			(void*)0x0, 0x140, 0,
			(void**)(critSecBase + 0x0));
		SCODE_ProcessAudioStreamWithFormat(
			(char*)0x0,
			"1 W1=S W2=SC D=TW",
			(void*)0x0, 0x140, 0,
			(void**)(critSecBase + 0x4));
		SCODE_ProcessAudioStreamWithFormat(
			(char*)0x0,
			"1 W1=S W2=W1 2 D=W",
			(void*)0x0, 0x140, 0x4000000,
			(void**)(critSecBase + 0x8));
	}

	if (((*(void**)(critSecBase + 0x0) != (void*)0x0) &&
	     (*(void**)(critSecBase + 0x4) != (void*)0x0)) &&
	    (*(void**)(critSecBase + 0x8) != (void*)0x0)) {
		typedef void (*SmackCallback_t)(uint32_t);
		SmackCallback_t fnPtr = (SmackCallback_t)*(void**)(smackBase + 0x20);
		fnPtr(dwAudioContext);
		return 1;
	}

	return 0;
}
