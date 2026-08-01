#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeDllResources
// NEEDS GLOBAL: g_pPrevExceptionFilter
// NEEDS GLOBAL: g_pArenaPool
// NEEDS GLOBAL: g_szSBLT_CPP
// NEEDS GLOBAL: g_pAudioStreamHandler_94
// NEEDS GLOBAL: g_pAudioStreamHandler_A0

extern "C" void* __stdcall SetUnhandledExceptionFilter(void* pFilter);
extern "C" void exc_filter_SBLT_entry();
extern "C" void* __stdcall MEMORY_AllocateMemoryFromArena(uint32_t dwSize, const char* szFile, uint32_t dwLine, uint32_t dwFlags);
extern "C" void __stdcall InitializeAudioStreamHandler(int* pHandler);
extern "C" void __stdcall InitializePowerOfTenTable();
extern "C" void __stdcall ExceptionHandler_Wrapper();
extern "C" void __stdcall ShutdownGameAndCleanup();

extern "C" int __stdcall InitializeDllResources(void* pDllInstance, int nInitMode, int nSkipRandomTableGen)
{
	if (nInitMode == 1) {
		/* ram0x6fc45390 = pDllInstance */
		void* instAddr = D2MOO_Resolve("g_dwInstance");
		if (!instAddr) return 0;
		*(void**)instAddr = pDllInstance;

		/* _g_pPrevExceptionFilter = SetUnhandledExceptionFilter(exc_filter_SBLT_entry) */
		void* prevAddr = D2MOO_Resolve("g_pPrevExceptionFilter");
		if (!prevAddr) return 0;
		*(void**)prevAddr = SetUnhandledExceptionFilter((void*)exc_filter_SBLT_entry);

		/* if (_g_pArenaPool == NULL) */
		void* arenaAddr = D2MOO_Resolve("g_pArenaPool");
		if (!arenaAddr) return 0;
		if (*(void**)arenaAddr == 0) {
			/* _g_pArenaPool = MEMORY_AllocateMemoryFromArena(0xc00, &g_szSBLT_CPP, 0x7e, 8) */
			void* szAddr = D2MOO_Resolve("g_szSBLT_CPP");
			if (!szAddr) return 0;
			*(void**)arenaAddr = MEMORY_AllocateMemoryFromArena(0xc00, (const char*)szAddr, 0x7e, 8);

			/* InitializeAudioStreamHandler(&g_pAudioStreamHandler_94) */
			void* h94Addr = D2MOO_Resolve("g_pAudioStreamHandler_94");
			if (!h94Addr) return 0;
			InitializeAudioStreamHandler((int*)h94Addr);

			/* InitializeAudioStreamHandler(&g_pAudioStreamHandler_A0) */
			void* hA0Addr = D2MOO_Resolve("g_pAudioStreamHandler_A0");
			if (!hA0Addr) return 0;
			InitializeAudioStreamHandler((int*)hA0Addr);
		}

		/* InitializePowerOfTenTable() */
		InitializePowerOfTenTable();

		/* if (nSkipRandomTableGen == 0) { ExceptionHandler_Wrapper(); return 1; } */
		if (nSkipRandomTableGen == 0) {
			ExceptionHandler_Wrapper();
			return 1;
		}
	}
	else if (nInitMode == 0) {
		/* SetUnhandledExceptionFilter(_g_pPrevExceptionFilter) */
		void* prevAddr = D2MOO_Resolve("g_pPrevExceptionFilter");
		if (!prevAddr) return 0;
		SetUnhandledExceptionFilter(*(void**)prevAddr);

		/* ShutdownGameAndCleanup() */
		ShutdownGameAndCleanup();
	}
	return 1;
}
