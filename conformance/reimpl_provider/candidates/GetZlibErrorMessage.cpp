#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetZlibErrorMessage
extern "C" void* __fastcall GetZlibErrorMessage(int nErrorCode)
{
	// g_pZlibErrorStrings is a pointer variable (declared as g_p*).
	// The decompile reads (char*)(&g_pZlibErrorStrings)[-in_EAX],
	// i.e. a char* located at &g_pZlibErrorStrings + (-in_EAX)*sizeof(char*).
	// The resolver returns &g_pZlibErrorStrings (the address of the symbol).
	char** symAddr = (char**)D2MOO_Resolve("g_pZlibErrorStrings");
	if (!symAddr)
		return (void*)0xDEADBEEFu; // sentinel: resolver missing -> obvious mismatch

	// Translate the decompile literally:
	//   in_EAX * -4  =>  offset = -nErrorCode
	//   (&g_pZlibErrorStrings)[-in_EAX]  =>  symAddr[-nErrorCode]
	int offset = -nErrorCode;
	char* result = symAddr[offset];
	return (void*)result;
}
