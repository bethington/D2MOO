#pragma once
// LiveDispatch_Generic.h -- generalized shadow-dispatcher primitives for the
// full-D2Common 1:1 conformance goal (conformance/D2COMMON_FULL_SHADOW_PLAN.md).
//
// The coord family (LiveDispatch_CoordFamily.h) predates this and stays as-is: a
// SPECIALIZED "void __stdcall(int*,int*)" out-param dispatcher. This header gives
// the GENERATED dispatchers (D2Common_ShadowDispatch.gen.h, emitted by
// conformance/tools/gen_shadow_dispatch.py from conformance/shadow_manifest.json)
// their shared machinery, and declares the accessor functions the coord header's
// C bridge exports delegate to -- so coord + generated dispatchers share ONE
// contiguous index space and D2Debugger sees them all with no code change.
//
// DECLARATIONS only. The generated header provides the DEFINITIONS (it is the one
// translation-unit home, same single-TU model as the coord header).
#include <Windows.h>
#include <atomic>
#include <cstdint>

namespace LiveDispatchGen
{
	enum class Mode : int32_t { Original = 0, Reimpl = 1, Shadow = 2 };

	// Own reentrancy guard + drain counter (parallel to LiveDispatch's). Class A
	// (pure / read-only-through-pointer) dispatchers don't need cross-family
	// nesting protection; when Class C (state mutation) lands, unify these with
	// LiveDispatch's shared guard. Defined in the generated header.
	extern thread_local bool tl_inDispatch;
	extern std::atomic<int> g_inFlight;

	// RAII: mark this thread in-dispatch + bump the drain counter around a reimpl
	// call, so a hot-reload can quiesce then wait for g_inFlight==0 before unmapping
	// the provider DLL (never unmap code mid-call).
	struct Guard {
		Guard()  { tl_inDispatch = true;  ++g_inFlight; }
		~Guard() { --g_inFlight; tl_inDispatch = false; }
	};

	// Masked comparison width for sub-dword returns (u8->0xFF, u16->0xFFFF, else full).
	inline uint32_t RetMask(int retBits) {
		return retBits >= 32 ? 0xFFFFFFFFu : ((1u << retBits) - 1u);
	}

	// Divergence sink: newline-delimited vectors-schema JSON (same convention as the
	// coord family) -- a live divergence becomes an offline regression case for free.
	// Class A (return value):
	void LogDivergence(const char* fn, const uint32_t* args, int nargs,
		uint32_t origRet, uint32_t reimplRet);
	// Class B (out-param buffer): the original's output bytes vs the reimpl's.
	void LogDivergenceBuf(const char* fn, const uint32_t* args, int nargs,
		const unsigned char* origOut, const unsigned char* reimplOut, int nbytes);
	// A reimpl access-violated and was caught by the shadow thunk's SEH -- logged as a
	// fault (counted like a divergence); the original's result is used, game unharmed.
	void LogFault(const char* fn);

	struct GenEntry {
		const char* name;
		uint32_t offset;                 // D2Common+off (verified, corrected_maps)
		std::atomic<int32_t>* mode;
		uint64_t* hits;
		uint64_t* divergences;
		void** reimplSlot;               // swappable reimpl ptr (bound from provider by name)
		void** trampolineSlot;           // Detours trampoline (raw original)
	};

	// --- accessors DEFINED in D2Common_ShadowDispatch.gen.h. The coord header's C
	// bridge exports delegate here for indices >= its own kCount, yielding one
	// contiguous index space over coord + generated dispatchers. ---
	int                Count();
	const char*        Name(int i);
	uint32_t           Offset(int i);
	int                GetMode(int i);
	void               SetMode(int i, int m);
	unsigned long long Hits(int i);
	unsigned long long Divergences(int i);
	void*              Trampoline(int i);
	void*              Reimpl(int i);
	void               SetReimpl(int i, void* fn);
	void               QuiesceModes();   // force every gen dispatcher to Original
	int                InFlight();        // g_inFlight snapshot (drain wait)
}
