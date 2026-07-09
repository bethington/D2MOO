// D2Debugger.gtqueue.cpp -- GAME-THREAD CALL QUEUE (final stateful frontier).
//
// Some game functions must run on the GAME THREAD (thread-local state, or they
// race the game's own per-frame reads/writes of the objects they touch). The
// oracle runs on the HTTP thread, so it can't call them directly. This is a
// single-slot request queue: the HTTP thread POSTS a call and waits; a
// game-thread PUMP (driven from the live-object capture hook, which fires every
// frame in-world) executes it and signals back.
//
// SEH-guarded: a faulting game-thread call is caught here so it can NEVER take
// down the game -- the HTTP side gets a "faulted" result instead.
//
// Re-entrancy note: the executed function runs at the capture hook's call site
// (a normal, consistent game state mid-frame), before that hook's own body.
#include <Windows.h>
#include <cstdint>
#include <cstring>

extern "C" uint32_t D2Oracle_Call(void* fn, int cc, const uint32_t* a, int n);
extern "C" uint64_t D2Oracle_Call64(void* fn, int cc, const uint32_t* a, int n);

namespace
{
	struct GtReq
	{
		volatile LONG pending;   // 1 = HTTP posted, awaiting the pump
		volatile LONG done;      // 1 = pump executed (or faulted)
		volatile LONG faulted;   // 1 = the call access-violated (SEH caught)
		void* fn; void* fn2; int cc; int nargs; int ret64;  // fn2: optional 2nd fn, run atomically after fn
		uint32_t args[8];
		uint64_t ret; uint64_t ret2;
	};
	GtReq g_gt = {};
}

// GAME THREAD -- drain one pending call. Called from the capture hook; cheap when
// idle (a single flag read). SEH keeps a bad call from crashing the game thread.
extern "C" void D2Gt_Pump()
{
	if (!g_gt.pending) return;
	g_gt.pending = 0; // CLAIM immediately: if the executed call re-enters the
	                  // capture hook, its pump sees no pending work (no double-exec).
	uint64_t r = 0, r2 = 0; LONG faulted = 0;
	__try
	{
		r = g_gt.ret64 ? D2Oracle_Call64(g_gt.fn, g_gt.cc, g_gt.args, g_gt.nargs)
		               : D2Oracle_Call(g_gt.fn, g_gt.cc, g_gt.args, g_gt.nargs);
		// Optional SECOND call (the reimpl) BACK-TO-BACK in the SAME pump: no frame
		// runs between the two, so both observe IDENTICAL live-object state. This is
		// what makes a getter over a VOLATILE field (unit position/mode/anim, which
		// the game mutates every frame) comparable -- two separate pumps would let a
		// frame move the unit between original and reimpl -> false mismatch.
		if (g_gt.fn2)
			r2 = g_gt.ret64 ? D2Oracle_Call64(g_gt.fn2, g_gt.cc, g_gt.args, g_gt.nargs)
			                : D2Oracle_Call(g_gt.fn2, g_gt.cc, g_gt.args, g_gt.nargs);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { faulted = 1; }
	g_gt.ret = r; g_gt.ret2 = r2;
	g_gt.faulted = faulted;
	MemoryBarrier();
	g_gt.done = 1;
}

// HTTP THREAD -- post a call to the game thread and wait for the pump. Returns:
//   1  = executed (ret written), 0 = TIMED OUT (pump not running -> not in-world),
//   -1 = executed but the call FAULTED (SEH). Single-slot; callers are serial
//   (one oracle request at a time via the synchronous server).
extern "C" int D2Gt_Call(void* fn, int cc, const uint32_t* args, int nargs,
                         int ret64, uint64_t* out, int timeoutMs)
{
	if (nargs < 0) nargs = 0; if (nargs > 8) nargs = 8;
	g_gt.fn = fn; g_gt.fn2 = nullptr; g_gt.cc = cc; g_gt.nargs = nargs; g_gt.ret64 = ret64;
	memcpy((void*)g_gt.args, args, (size_t)nargs * 4);
	g_gt.done = 0; g_gt.faulted = 0;
	MemoryBarrier();
	g_gt.pending = 1;
	for (int i = 0; i < timeoutMs; ++i)
	{
		if (g_gt.done)
		{
			*out = g_gt.ret;
			return g_gt.faulted ? -1 : 1;
		}
		Sleep(1);
	}
	g_gt.pending = 0; // give up
	return 0;
}

// HTTP THREAD -- post TWO calls (same cc + args) to run ATOMICALLY in one game-
// thread pump, capturing both returns. Used by the oracle to call original+reimpl
// with no frame between them (identical live-object state -> volatile-field getters
// become comparable). Same timeout/fault contract as D2Gt_Call; *outA/*outB get the
// two returns. A single fault flag covers both (either faulting marks the pair).
extern "C" int D2Gt_Call2(void* fnA, void* fnB, int cc, const uint32_t* args, int nargs,
                          int ret64, uint64_t* outA, uint64_t* outB, int timeoutMs)
{
	if (nargs < 0) nargs = 0; if (nargs > 8) nargs = 8;
	g_gt.fn = fnA; g_gt.fn2 = fnB; g_gt.cc = cc; g_gt.nargs = nargs; g_gt.ret64 = ret64;
	memcpy((void*)g_gt.args, args, (size_t)nargs * 4);
	g_gt.done = 0; g_gt.faulted = 0;
	MemoryBarrier();
	g_gt.pending = 1;
	for (int i = 0; i < timeoutMs; ++i)
	{
		if (g_gt.done)
		{
			*outA = g_gt.ret; *outB = g_gt.ret2;
			return g_gt.faulted ? -1 : 1;
		}
		Sleep(1);
	}
	g_gt.pending = 0; // give up
	return 0;
}
