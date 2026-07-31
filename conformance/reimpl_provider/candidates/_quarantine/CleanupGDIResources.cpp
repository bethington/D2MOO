#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CleanupGDIResources
// NEEDS GLOBAL: g_pExceptionFramePool
// NEEDS GLOBAL: g_pActiveModule
// NEEDS GLOBAL: g_pResourceBuffer
// NEEDS GLOBAL: g_dwResourceBufferSize
// NEEDS GLOBAL: g_pSGD_ArrayBuffer
// NEEDS GLOBAL: g_dwSGD_ElementCount
// NEEDS GLOBAL: g_dwSGD_ArrayCapacity
// NEEDS GLOBAL: g_szAUSGDIOBJ_DbgName

extern "C" void __stdcall SErrReportNamedResourceLeak(uint32_t pName, char* arg2);
extern "C" void __stdcall SMemFree(void* pMemory, uint32_t* szFileName, uint32_t dwLine);
extern "C" void __stdcall UnwindExceptionContext(void* pCtx);

extern "C" uint32_t __stdcall CleanupGDIResources()
{
	void** ppExceptionFramePool = (void**)D2MOO_Resolve("g_pExceptionFramePool");
	if (!ppExceptionFramePool) return 0;

	void** ppActiveModule = (void**)D2MOO_Resolve("g_pActiveModule");
	void** ppResourceBuffer = (void**)D2MOO_Resolve("g_pResourceBuffer");
	uint32_t** ppResourceBufferSize = (uint32_t**)D2MOO_Resolve("g_dwResourceBufferSize");
	void** ppSGDArrayBuffer = (void**)D2MOO_Resolve("g_pSGD_ArrayBuffer");
	uint32_t** ppSGDArrayCapacity = (uint32_t**)D2MOO_Resolve("g_dwSGD_ArrayCapacity");
	uint32_t** ppSGDElementCount = (uint32_t**)D2MOO_Resolve("g_dwSGD_ElementCount");
	const char** ppAUSGDIOBJ_DbgName = (const char**)D2MOO_Resolve("g_szAUSGDIOBJ_DbgName");

	void* pGDIResourceNode = *ppExceptionFramePool;
	while ((int)(uintptr_t)pGDIResourceNode > 0) {
		uint16_t* pwResourceType = (uint16_t*)((char*)pGDIResourceNode + 10);

		const char* szResourceType;
		if ((short)*pwResourceType == 0) {
			szResourceType = "HSGDIFONT";
		} else {
			szResourceType = "HSGDIOBJ";
		}
		SErrReportNamedResourceLeak((uint32_t)(uintptr_t)szResourceType, (char*)0);

		if ((*(short*)((char*)pGDIResourceNode + 8) == (short)0x4F4D) &&
			((short)*pwResourceType == 0)) {
			if (ppActiveModule && *ppActiveModule == pGDIResourceNode) {
				*ppActiveModule = (void*)0;
			}
			void** ppResourceData = (void**)((char*)pGDIResourceNode + 0x0c);
			if (*ppResourceData != (void*)0) {
				SMemFree(*ppResourceData, (uint32_t*)"..\\3rdParty\\STORM\\SOURCE\\SGDI.CPP", 0x117);
				*ppResourceData = (void*)0;
			}
			UnwindExceptionContext(pGDIResourceNode);
			SMemFree(pGDIResourceNode,
				(uint32_t*)(ppAUSGDIOBJ_DbgName ? *ppAUSGDIOBJ_DbgName : (const char*)"..\\3rdParty\\STORM\\SOURCE\\SGDI.CPP"),
				(uint32_t)-2);
		}

		pGDIResourceNode = *ppExceptionFramePool;
	}

	if (ppResourceBuffer && *ppResourceBuffer != (void*)0) {
		SMemFree(*ppResourceBuffer, (uint32_t*)"..\\3rdParty\\STORM\\SOURCE\\SGDI.CPP", 0x4b);
		*ppResourceBuffer = (void*)0;
		if (ppResourceBufferSize) *ppResourceBufferSize = (uint32_t)0;
	}

	if (ppSGDArrayBuffer && *ppSGDArrayBuffer != (void*)0) {
		SMemFree(*ppSGDArrayBuffer, (uint32_t*)"..\\3rdParty\\STORM\\SOURCE\\SGDI.CPP", 0x135);
		*ppSGDArrayBuffer = (void*)0;
		if (ppSGDElementCount) *ppSGDElementCount = (uint32_t)0;
		if (ppSGDArrayCapacity) *ppSGDArrayCapacity = (uint32_t)0;
	}

	return 1;
}
