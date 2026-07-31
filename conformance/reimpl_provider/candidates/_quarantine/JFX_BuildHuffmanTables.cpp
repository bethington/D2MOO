#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: JFX_BuildHuffmanTables
// Calling convention: register_explicit (EBX=pHuffmanCtx, ESI=pInBuf, EDI=pUnk), 0 stack params, RET 0

extern "C" int JFX_GetBufferSize(void* pBuf, int* pOutSize);
extern "C" int JFX_AppendBufferData(void* pCtx, uint32_t nSize, void** ppAllocBuf, uint8_t** ppOutBuf, uint32_t* pOutBufSize);
extern "C" void HUFFMAN_BuildTableEntry(void* pEntry, uint32_t dwType, uint32_t dwIndex, uint8_t* pData, uint8_t* pEnd);
extern "C" void ROOM_DeleteTileState(void* pBuf);

extern "C" int JFX_BuildHuffmanTables(void* pHuffmanCtx, void* pInBufParam, void* pUnk)
{
	uint8_t* pCtx = (uint8_t*)pInBufParam;
	uint8_t* pInBuf = (uint8_t*)pInBufParam;
	uint8_t* pOutBuf = (uint8_t*)0;
	void* pAllocBuf = (void*)0;
	uint32_t dwOutBufSize = 0;
	int nResult = JFX_GetBufferSize(pInBuf, (int*)&pInBuf);

	if (nResult == 0) {
		if ((uint32_t)pInBuf > 1u) {
			pInBuf = (uint8_t*)((uint32_t)pInBuf - 2u);
			if ((uint32_t)pInBuf < 0x444u) {
				nResult = JFX_AppendBufferData(pCtx, (uint32_t)pInBuf, &pAllocBuf, &pOutBuf, &dwOutBufSize);
				if (nResult == 0) {
					int nAccum = 0;
					for (; pInBuf != (uint8_t*)0; pInBuf = pInBuf + (-0x11 - nAccum)) {
						uint32_t dwIndex = *pOutBuf & 0xfu;
						uint32_t bNibble = (uint32_t)(*pOutBuf >> 4);
						uint32_t dwType = bNibble;

						if ((dwType != 1u && bNibble != 0u) || 3u < dwIndex) {
							nResult = -6;
							break;
						}

						uint32_t* pField90 = (uint32_t*)((char*)pHuffmanCtx + 0x90);
						if (*pField90 <= dwIndex) {
							*pField90 = dwIndex + 1u;
						}

						if (dwType == 0u) {
							void* pTableEntry = (char*)pHuffmanCtx + 0x1d24 + dwIndex * 0x19c;
							HUFFMAN_BuildTableEntry(pTableEntry, 0u, dwIndex, pOutBuf + 1, pOutBuf + 0x11);

							uint32_t* pField8C = (uint32_t*)((char*)pHuffmanCtx + 0x8C);
							if (*pField8C < 4u) {
								*pField8C = *pField8C + 1u;
							}
						}
						else {
							void* pTableEntry = (char*)pHuffmanCtx + 0x364 + dwIndex * 0x670;
							HUFFMAN_BuildTableEntry(pTableEntry, dwType, dwIndex, pOutBuf + 1, pOutBuf + 0x11);

							uint32_t* pField88 = (uint32_t*)((char*)pHuffmanCtx + 0x88);
							if (*pField88 < 4u) {
								*pField88 = *pField88 + 1u;
							}
						}

						int nRemaining = 0x10;
						uint8_t* pBuf = pOutBuf + 1;
						nAccum = 0;
						do {
							nAccum = nAccum + (uint32_t)*pBuf;
							pBuf = pBuf + 1;
							nRemaining = nRemaining + -1;
						} while (nRemaining != 0);

						pOutBuf = pBuf + nAccum;
					}
				}
			}
			else {
				nResult = -0x15;
			}
		}
		else {
			nResult = -0x15;
		}
	}

	if ((dwOutBufSize != 0u) && (pAllocBuf != (void*)0)) {
		ROOM_DeleteTileState(pAllocBuf);
	}
	return nResult;
}
