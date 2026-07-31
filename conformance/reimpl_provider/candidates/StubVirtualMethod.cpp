// StubVirtualMethod -- a true no-op vtable filler. The decompiled body is
// literally `return;` -- no reads, no writes, no side effects. This stub may
// be replaced by an actual implementation in a derived class, but as
// analyzed here it does nothing.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: StubVirtualMethod
extern "C" void __fastcall StubVirtualMethod(void* pThis)
{
	(void)pThis; // explicitly unused -- mirror the decompile's empty body
}
