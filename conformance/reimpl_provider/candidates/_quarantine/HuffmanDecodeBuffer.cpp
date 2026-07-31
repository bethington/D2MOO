#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: HuffmanDecodeBuffer

// Storm.dll subroutines invoked by the function (declared as stubs since
// they are not exposed via the D2MOO_Resolve global table).
extern "C" uint32_t __stdcall HuffmanDecodeSymbol(void* pDecoder);
extern "C" void __stdcall InitializeEntitySlotManagement(uint8_t allocationType);
extern "C" void __stdcall AllocateAndLinkEntitySlots(void* pDecoder, uint32_t pSlotMgr, void* ebx, void* edi, void* ebp);
extern "C" void __stdcall RebalanceEntityList(void* pDecoder);

extern "C" int __stdcall HuffmanDecodeBuffer(void* pDecoderContext, void* pOutputBuffer, int nByteCount)
{
	if (nByteCount == 0) {
		return 0;
	}

	// HuffmanDecodeCtx layout (offsets inferred from the decompile):
	//   +0x00: void*  dataPtr      (input stream cursor)
	//   +0x04: uint   accumulator  (bit buffer)
	//   +0x08: int    bitCount     (valid bits in accumulator)
	//   +0x0C: void*  validFlag    (1 if initial allocationType == 0, else 0)
	char*        ctx    = (char*)pDecoderContext;
	void**       ppData = (void**)(ctx + 0x00);
	uint32_t* pAcc   = (uint32_t*)(ctx + 0x04);
	int*         pBits  = (int*)(ctx + 0x08);
	void**       ppFlag = (void**)(ctx + 0x0C);

	// Step 2: initial refill if bitCount < 9
	int bits = *pBits;
	if ((uint32_t)bits < 9) {
		uint16_t w = *(uint16_t*)*ppData;
		*pBits = bits + 0x10;
		*pAcc  = *pAcc | ((uint32_t)w << ((uint32_t)bits & 0x1f));
		*ppData = (char*)*ppData + 2;
	}

	// Step 3: extract low 8 bits as allocationType
	uint32_t tmp = *pAcc;
	*pAcc  = tmp >> 8;
	*pBits = *pBits - 8;
	uint8_t allocationType = (uint8_t)(tmp & 0xff);

	// Step 4: setup entity slot management
	InitializeEntitySlotManagement(allocationType);

	// Step 5: store validation flag
	*ppFlag = (void*)(uintptr_t)(allocationType == 0);

	// Step 6: main decode loop
	uint8_t* outPtr = (uint8_t*)pOutputBuffer;

	while (1) {
		uint32_t sym = HuffmanDecodeSymbol(pDecoderContext);

		// 6b: refill marker 0x101 -> refill + extract new allocation type
		if (sym == 0x101u) {
			bits = *pBits;
			if ((uint32_t)bits < 9) {
				uint16_t w = *(uint16_t*)*ppData;
				*pBits = bits + 0x10;
				*pAcc  = *pAcc | ((uint32_t)w << ((uint32_t)bits & 0x1f));
				*ppData = (char*)*ppData + 2;
			}
			uint32_t tmp2 = *pAcc;
			*pAcc   = tmp2 >> 8;
			*pBits  = *pBits - 8;
			uint32_t newType = tmp2 & 0xff;

			AllocateAndLinkEntitySlots(pDecoderContext, newType, 0, 0, 0);

			if (*ppFlag == (void*)0) {
				RebalanceEntityList(pDecoderContext);
			}
			continue;
		}

		// 6c: end-of-stream marker 0x100
		if (sym == 0x100u) {
			break;
		}

		// 6d/6e: write decoded byte, advance, decrement remaining
		*outPtr = (uint8_t)sym;
		outPtr++;
		nByteCount--;

		// 6f: finished the requested count
		if (nByteCount == 0) break;

		// 6g: validation-flag-triggered rebalance after a write
		if (*ppFlag != (void*)0) {
			RebalanceEntityList(pDecoderContext);
		}
	}

	// Step 7: return bytes written
	return (int)(outPtr - (uint8_t*)pOutputBuffer);
}
