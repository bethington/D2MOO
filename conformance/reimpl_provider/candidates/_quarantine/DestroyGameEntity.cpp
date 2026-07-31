#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DestroyGameEntity

// External Storm.dll functions referenced by the decompile
extern "C" uint32_t __stdcall IsHandleInCodecList(void* pEntity);
extern "C" void __stdcall SMemFree(void* pMem, uint32_t* pSourceFile, uint32_t dwLine);
extern "C" void __stdcall GetNextListNode(void* pListNode);

// SmackVideoEntity layout (per plate comment):
//   +0x00  pCodecNode       (void*)
//   +0x04  dwFileHandle     (unsigned int; 0xFFFFFFFF = closed)
//   +0x08  pAllocatedBuffer (void*)
//   +0x0C  pDistanceData    (void*)
//   +0x10  pCodecBufferBlock(void*)
// CodecNode layout (inferred from decompile):
//   +0x00  nHandler         (int)
//   +0x04  pNext            (void*)
extern "C" int __stdcall DestroyGameEntity(void* pEntity)
{
	if (pEntity == 0) {
		// _g_dwLastError = 0x57;
		// NEEDS GLOBAL: g_dwLastError
		uint32_t* pLastError = (uint32_t*)D2MOO_Resolve("g_dwLastError");
		if (pLastError) *pLastError = 0x57;
		// (*(code *)&DAT_0004d892)(0x57);
		// NEEDS GLOBAL: DAT_0004d892 (code pointer; side-effect call skipped)
		return 0;
	}

	uint32_t dwIsInCodecList = IsHandleInCodecList(pEntity);

	if (dwIsInCodecList != 0) {
		// pEntity->pCodecNode at +0x00
		void* pCodecNode = *(void**)pEntity;
		if (pCodecNode != 0) {
			// nCodecHandler = *(int*)pCodecNode (first dword of CodecNode)
			int nCodecHandler = *(int*)pCodecNode;
			if (nCodecHandler != 0) {
				// (*(code *)*_g_pSmackFunctionTable)(nCodecHandler);
				// NEEDS GLOBAL: g_pSmackFunctionTable
				void** pFuncTable = (void**)*(void**)D2MOO_Resolve("g_pSmackFunctionTable");
				if (pFuncTable) {
					((void(*)(int))pFuncTable[0])(nCodecHandler);
				}
			}

			// pCodecNode->pNext at +0x04
			void* pNextNode = *(void**)((char*)pCodecNode + 4);
			if (pNextNode != 0) {
				// (*(code *)_g_pSmackFunctionTable[3])(pNextNode);
				void** pFuncTable = (void**)*(void**)D2MOO_Resolve("g_pSmackFunctionTable");
				if (pFuncTable) {
					((void(*)(void*))pFuncTable[3])(pNextNode);
				}
			}

			// SMemFree(pCodecNode, &g_pSVIDSourceFile, 0x406);
			// NEEDS GLOBAL: g_pSVIDSourceFile
			uint32_t* pSourceFile = (uint32_t*)D2MOO_Resolve("g_pSVIDSourceFile");
			SMemFree(pCodecNode, pSourceFile, 0x406);
			*(void**)pEntity = 0; // pEntity->pCodecNode = NULL

			// pEntity->dwFileHandle at +0x04
			uint32_t dwFileHandle = *(uint32_t*)((char*)pEntity + 4);
			if (dwFileHandle != 0xFFFFFFFFu) {
				// (*(code *)&DAT_0004d7ea)(dwFileHandle);
				// NEEDS GLOBAL: DAT_0004d7ea (code pointer; side-effect call skipped)
				*(uint32_t*)((char*)pEntity + 4) = 0xFFFFFFFFu;
			}

			// pEntity->pAllocatedBuffer at +0x08
			void* pAllocatedBuffer = *(void**)((char*)pEntity + 8);
			if (pAllocatedBuffer != 0) {
				SMemFree(pAllocatedBuffer, pSourceFile, 0x40F);
				*(void**)((char*)pEntity + 8) = 0;  // pAllocatedBuffer = NULL
				*(void**)((char*)pEntity + 12) = 0; // pDistanceData    = NULL
			}

			// pEntity->pCodecBufferBlock at +0x10
			void* pCodecBufferBlock = *(void**)((char*)pEntity + 16);
			if (pCodecBufferBlock != 0) {
				SMemFree(pCodecBufferBlock, pSourceFile, 0x415);
				// (field NOT cleared per spec)
			}

			// GetNextListNode((ListNode*)&g_pfnSmackVideoVtable);
			// NEEDS GLOBAL: g_pfnSmackVideoVtable
			void* pVtable = D2MOO_Resolve("g_pfnSmackVideoVtable");
			GetNextListNode(pVtable);

			return 1;
		}
	}

	return 0;
}
