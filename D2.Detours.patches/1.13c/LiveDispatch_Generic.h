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

	// --- distinct-input sampling ---------------------------------------------
	// SHIPPING_PROMOTION_PLAN.md defines the battletest bar as volume x input
	// diversity x state diversity, and warns "1M calls with 3 inputs is not
	// coverage" -- but only volume was ever measured. battletest_promoter now
	// REFUSES to promote without a diversity number, so this counter is what
	// unblocks promotion for both D2Common and D2Client.
	//
	// A lock-free, fixed-capacity set of argument-tuple hashes. This runs on the
	// shadow path of every real game call, so it must never allocate, lock, or
	// grow: kSlots is the saturation point and a saturated sampler simply stops
	// counting (already far above any realistic diversity floor). Approximate by
	// construction -- hash collisions undercount, which is the safe direction:
	// it can only delay a promotion, never manufacture one.
	struct DistinctSampler {
		static const uint32_t kSlots = 128;
		std::atomic<uint32_t> slots[kSlots];   // 0 == empty (static storage zero-inits)
		std::atomic<uint32_t> count;
		// The ACTUAL argument values behind each accepted slot (added
		// 2026-07-30). The hash alone can say HOW MANY distinct inputs a
		// function has seen but never WHICH, so coverage guidance could only
		// ever be "cast a variety of skills" -- a guess. With the values, the
		// dashboard can say "you have hit skill ids 0,1,5,7,12; try others",
		// which is what makes the 46 called-but-short dispatchers targetable.
		// Two dwords is enough: arg0 is the whole input for the 1-arg majority,
		// and for the rest arg1 is usually the id (arg0 being a unit pointer).
		std::atomic<uint32_t> v0[kSlots];
		std::atomic<uint32_t> v1[kSlots];
	};

	inline uint32_t HashArgs(const uint32_t* args, int nargs) {
		uint32_t h = 2166136261u;              // FNV-1a
		for (int i = 0; i < nargs; ++i) { h ^= args[i]; h *= 16777619u; }
		return h ? h : 1u;                     // 0 is the empty-slot sentinel
	}

	// A 0-arg function has exactly ONE possible input, so it hashes to a single
	// constant and saturates at count==1. That is correct, and the promoter is
	// told the arg count so it can skip the diversity floor rather than block
	// such a function forever.
	inline void NoteInputs(DistinctSampler& s, const uint32_t* args, int nargs) {
		if (s.count.load(std::memory_order_relaxed) >= DistinctSampler::kSlots) return;
		const uint32_t h = HashArgs(args, nargs);
		uint32_t i = h % DistinctSampler::kSlots;
		for (uint32_t probe = 0; probe < DistinctSampler::kSlots; ++probe) {
			const uint32_t cur = s.slots[i].load(std::memory_order_relaxed);
			if (cur == h) return;                       // already seen
			if (cur == 0) {
				uint32_t expected = 0;
				if (s.slots[i].compare_exchange_strong(expected, h,
						std::memory_order_relaxed, std::memory_order_relaxed)) {
					// Record the values BEFORE publishing the count, so a reader
					// that sees count==N never reads an unwritten value slot.
					s.v0[i].store(nargs > 0 ? args[0] : 0u, std::memory_order_relaxed);
					s.v1[i].store(nargs > 1 ? args[1] : 0u, std::memory_order_relaxed);
					s.count.fetch_add(1, std::memory_order_release);
					return;
				}
				// Lost the race: if the winner stored OUR hash it is already
				// counted; otherwise fall through and keep probing.
				if (s.slots[i].load(std::memory_order_relaxed) == h) return;
			}
			i = (i + 1) % DistinctSampler::kSlots;
		}
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
		DistinctSampler* distinct;       // distinct argument tuples seen in shadow
		int argc;                        // 0-arg functions have no diversity axis
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
	unsigned long long DistinctInputs(int i);
	int                ArgCount(int i);
	// Recorded argument value for one sampler slot. `which` selects arg0/arg1.
	// Returns 0 for an out-of-range dispatcher, slot or arg index -- callers
	// bound the loop with DistinctInputs(i).
	uint32_t           SampleValue(int i, int slot, int which);
	void*              Trampoline(int i);
	void*              Reimpl(int i);
	void               SetReimpl(int i, void* fn);
	void               QuiesceModes();   // force every gen dispatcher to Original
	int                InFlight();        // g_inFlight snapshot (drain wait)
}
