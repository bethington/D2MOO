// D2Debugger.action.cpp -- STATE-MUTATING "drive the game" capability.
//
// Everything else in D2Debugger (dispatchers, /oracle, /call, /capture) either
// reads state or calls a chosen function with chosen inputs. This file instead
// drives the game's OWN menu/state machine by calling its real functions -- no
// OS input (no SendInput/PostMessage/window automation), matching D2MOO's
// existing philosophy of in-process function/data access only.
//
// BACKGROUND (this session's live dbgeng trace against a running PD2-S12):
// single-player game entry from the menu is owned by D2Launch.dll, NOT the
// D2Client state machine (D2Client's RunGameMainLoop loop is dormant while
// D2Launch runs the menu). The real launch path, live-confirmed:
//   difficulty-button click -> D2Win button dispatch -> LaunchCharacterDefaultMode
//   -> SelectCharacterByIndex(0) @ D2Launch 0x6FA4DE20 -> LAUNCH_GameWithCharacter
//   @ 0x6FA5AAB0 -> StopMessageLoop() -> hands control to D2Client, which then
//   runs its state machine (3->5->6->2) to actually create/enter the game.
// So the correct automation is to call SelectCharacterByIndex(0) for the
// currently-highlighted character -- exactly what clicking "Normal" does.
//
// THREADING: SelectCharacterByIndex manipulates live UI state (releases
// controls, stops the message loop) so it MUST run on the GAME THREAD. The
// game-thread queue (D2Debugger.gtqueue.cpp, D2Gt_Call/D2Gt_Pump) is drained
// from a hook on a function the game thread calls every frame. The existing
// D2Capture hook only fires IN-WORLD; for the MENU we hook D2Win's
// RenderMainFrame @ 0x6F8F87E0 (called every frame ~25fps by
// D2Win!RunMainMessageLoop while any menu screen is up). Passthrough only --
// same pattern as D2Debugger.capture.cpp's CaptureStub, no behavior change.
#include <Windows.h>
#include <detours.h>
#include <cstdint>
#include <cstdio>

extern "C" int D2Gt_Call(void* fn, int cc, const uint32_t* args, int nargs,
                          int ret64, uint64_t* out, int timeoutMs);
extern "C" void D2Gt_Pump();

namespace
{
	// --- D2Win menu-frame pump site ---------------------------------------
	// RenderMainFrame @ 0x6F8F87E0 (D2Win preferred base 0x6F8E0000, RVA 0x187E0);
	// void __stdcall, no args -> EAX/ECX/EDX are scratch at entry, so a
	// pushad/popad-wrapped pump call is fully safe. Fires every frame on the
	// GAME THREAD while any D2Win menu screen (title/char-select/difficulty) is
	// active -- exactly the pre-game game-thread pump the queue needs.
	const uint32_t kD2WinRenderMainFrameRva = 0x6F8F87E0u - 0x6F8E0000u;
	void* g_pumpTramp = nullptr;
	bool  g_pumpHooked = false;

	__declspec(naked) void PumpHookStub()
	{
		__asm {
			pushad                  // preserve everything (RenderMainFrame is __stdcall void)
			call D2Gt_Pump          // drain the game-thread call queue (cheap when idle)
			popad
			jmp  [g_pumpTramp]
		}
	}

	// --- D2Launch single-player launch path (all @ preferred base 0x6FA40000,
	// unrelocated -> runtime == Ghidra; verified live this session). Addresses
	// from SelectCharacterByIndex disassembly. ------------------------------
	const uint32_t kD2LaunchPreferredBase   = 0x6FA40000u;
	const uint32_t kSelectedCharIndexRva    = 0x6FA64DB0u - kD2LaunchPreferredBase; // dword, 0xFFFFFFFF = none
	const uint32_t kCharListHeadRva         = 0x6FA65EC8u - kD2LaunchPreferredBase; // ptr, 0 = list not loaded
	const uint32_t kSelectCharByIndexRva    = 0x6FA4DE20u - kD2LaunchPreferredBase; // __stdcall(byte), RET 4
	const uint32_t kStartSinglePlayerRva    = 0x6FA5B030u - kD2LaunchPreferredBase; // StartSinglePlayerMode, int(void)

	// --- D2Win message-loop quit levers (from RunMainMessageLoop disassembly,
	// D2Win preferred base 0x6F8E0000). The menu loop runs `while
	// (g_dwIsRunning != 0)`; on exit it restarts UNLESS g_fQuitPosted != 0. So a
	// clean "exit Diablo" = set g_fQuitPosted=1 then g_dwIsRunning=0 (the game's
	// OWN quit mechanism -- what the title EXIT button ultimately drives). ------
	const uint32_t kD2WinPreferredBase = 0x6F8E0000u;
	const uint32_t kIsRunningRva  = 0x6F8FFD30u - kD2WinPreferredBase;
	const uint32_t kQuitPostedRva = 0x6F9A9CF4u - kD2WinPreferredBase;

	uintptr_t ResolveD2LaunchBase()
	{
		static uintptr_t base = 0;
		if (!base) base = (uintptr_t)GetModuleHandleA("D2Launch.dll");
		return base;
	}
	uintptr_t ResolveD2WinBase()
	{
		static uintptr_t base = 0;
		if (!base) base = (uintptr_t)GetModuleHandleA("D2Win.dll");
		return base;
	}
	uintptr_t ResolveD2ClientBase()
	{
		static uintptr_t base = 0;
		if (!base) base = (uintptr_t)GetModuleHandleA("D2Client.dll");
		return base;
	}

	// --- D2Client in-game "Save and Exit Game" -----------------------------
	// HandleSaveAndExitDialogConfirm @ 0x6FB09980 (D2Client preferred base
	// 0x6FAB0000) -- the ESC-menu "SAVE AND EXIT GAME" confirm handler:
	// ProcessGameEndCleanup + sends the save-and-exit packet (0x4F 0x03) so the
	// server saves the character and tears the game down, returning to
	// character-select. dword(void), no args. Runs IN-WORLD, so it's drained by
	// the D2Capture in-world pump (not the menu RenderMainFrame pump).
	const uint32_t kD2ClientPreferredBase   = 0x6FAB0000u;
	const uint32_t kSaveAndExitRva          = 0x6FB09980u - kD2ClientPreferredBase; // HandleSaveAndExitDialogConfirm
	const uint32_t kCreateSaveExitDlgRva    = 0x6FB09AC0u - kD2ClientPreferredBase; // CreateSaveExitDialog
	// Dialog-context refcount (0x6FBCBC08): incremented by CLIENT_Create*DialogWindow,
	// decremented by ProcessGameEndCleanup (which ABORTS -- "internal error
	// 6fb53e1e" -- unless it reaches 0, i.e. must be 1). So the save-exit cleanup
	// can ONLY run when the save-exit DIALOG is actually open (refcount==1). That's
	// why calling the confirm handler cold crashed (refcount was 0 -> -1 -> abort).
	const uint32_t kDlgRefcountRva          = 0x6FBCBC08u - kD2ClientPreferredBase;
	const uint32_t kSaveExitDlgPtrRva       = 0x6FBCC994u - kD2ClientPreferredBase; // g_pSaveExitDialog (0=none)

	// GAME-THREAD body: SAFE in-game save-and-exit. Replicates EXACTLY what the
	// user does (ESC -> Save and Exit Game): first OPEN the save-exit dialog via
	// CreateSaveExitDialog (which properly sets up the dialog context AND bumps the
	// refcount 0->1), THEN confirm it via HandleSaveAndExitDialogConfirm (refcount
	// 1->0, so ProcessGameEndCleanup runs its cleanup on a VALID, properly-set-up
	// context instead of aborting). Runs on the game thread (in-world capture pump).
	// Returns 0 ok, -1 D2Client not resolved, -2 a dialog was already open (skip).
	int __cdecl SaveExitViaDialogImpl(void)
	{
		uintptr_t base = ResolveD2ClientBase();
		if (!base) return -1;
		// Don't touch anything if a dialog is already open (would confuse the refcount).
		if (*(volatile uint32_t*)(base + kSaveExitDlgPtrRva) != 0) return -2;

		typedef unsigned(__stdcall* DlgFn)(void);
		DlgFn createDlg = (DlgFn)(base + kCreateSaveExitDlgRva);
		DlgFn confirm   = (DlgFn)(base + kSaveAndExitRva);

		unsigned created = createDlg();       // opens dialog: refcount 0->1, context set up
		if (!created) return -2;              // 0 = dialog already existed; don't confirm
		confirm();                             // confirms: refcount 1->0, clean cleanup + state + packet
		return 0;
	}

	// GAME-THREAD body (queued via D2Gt_Call, drained by the RenderMainFrame
	// hook). __stdcall byte arg to match SelectCharacterByIndex's real ABI.
	// D2Oracle_Call marshals cc=1 (stdcall) with one 32-bit slot (low byte used).
	typedef void(__stdcall* SelectCharFn)(uint8_t);

	// Character-list entry layout: +0x00 = ASCII character name (live-confirmed
	// 2026-07-07 via a raw entry dump: "frozen-orb2" sits at +0x00, NOT the wide
	// field at +0x100 which reads empty), +0x320 = class byte, +0x34C = next
	// pointer. List head is g_pCharListHead; a character's "index" is its 0-based
	// position in that list (== g_dwSelectedCharIndex).
	const uint32_t kCharEntryNameRva = 0x0u;
	const uint32_t kCharEntryClassRva = 0x320u;
	const uint32_t kCharEntryNextRva = 0x34Cu;

	// Requested character name for LoadCharacterByNameImpl (HTTP thread writes it,
	// game thread reads it). Single-slot; actions are serialized by g_mcpMutex.
	char g_reqCharName[64] = {0};

	// Requested difficulty (0=Normal, 1=Nightmare, 2=Hell) -- passed as
	// SelectCharacterByIndex's bLaunchMode, which IS the difficulty index
	// (live-confirmed 2026-07-07: Normal click -> 0, Nightmare click -> 1).
	// The character must have that difficulty UNLOCKED (the real difficulty popup
	// only offers unlocked ones); requesting a locked difficulty may be rejected
	// or fall back to what's available -- caller's responsibility for now.
	uint32_t g_reqDifficulty = 0;

	// GAME-THREAD body: find the character whose name matches g_reqCharName in the
	// live char-select list, set g_dwSelectedCharIndex to its position, and launch
	// it via SelectCharacterByIndex(0). Names at +0x100 are wide (UTF-16); D2 names
	// are ASCII-range so we compare low bytes case-insensitively. Returns the
	// matched index (>=0), or -1 (no match / list empty / D2Launch not resolved).
	int __cdecl LoadCharacterByNameImpl(void)
	{
		uintptr_t base = ResolveD2LaunchBase();
		if (!base) return -1;
		uint32_t head = *(volatile uint32_t*)(base + kCharListHeadRva);
		if (!head) return -1;

		auto ieq = [](char a, char b) {
			if (a >= 'A' && a <= 'Z') a += 32;
			if (b >= 'A' && b <= 'Z') b += 32;
			return a == b;
		};

		uint32_t entry = head;
		for (int idx = 0; entry != 0; ++idx)
		{
			const char* aname = (const char*)(entry + kCharEntryNameRva);
			// Compare ASCII name against g_reqCharName, case-insensitive.
			bool match = true;
			int i = 0;
			for (; i < 63; ++i)
			{
				char c = aname[i];
				char r = g_reqCharName[i];
				if (r == 0 && c == 0) break;      // both ended -> match
				if (!ieq(c, r)) { match = false; break; }
				if (r == 0 || c == 0) { match = false; break; }
			}
			if (match)
			{
				*(volatile uint32_t*)(base + kSelectedCharIndexRva) = (uint32_t)idx;
				void* fn = (void*)(base + kSelectCharByIndexRva);
				uint8_t diff = (uint8_t)(g_reqDifficulty <= 2 ? g_reqDifficulty : 0);
				((SelectCharFn)fn)(diff); // bLaunchMode == difficulty (0/1/2); already on the game thread
				return idx;
			}
			entry = *(volatile uint32_t*)(entry + kCharEntryNextRva);
		}
		return -1; // no character with that name
	}

	// GAME-THREAD body: set D2Win's quit levers so the menu loop exits and does
	// NOT restart -> the game shuts down cleanly through its own path. cdecl,
	// no args (called via D2Gt_Call with cc=cdecl, 0 args).
	int __cdecl ExitGameImpl(void)
	{
		uintptr_t base = ResolveD2WinBase();
		if (!base) return -1;
		*(volatile uint32_t*)(base + kQuitPostedRva) = 1; // don't restart the loop
		*(volatile uint32_t*)(base + kIsRunningRva)  = 0; // break the current loop
		return 0;
	}
}

extern "C" bool D2Action_IsPumpHookInstalled() { return g_pumpHooked; }

// Enumerate the single-player character-select list (read-only, HTTP thread --
// the list is stable while sitting at char-select). Writes a JSON array of
// {index,name,class} into buf. Also the diagnostic that shows the real stored
// name format at entry+0x100. Returns the character count, or -1 if D2Launch
// isn't resolved / list not loaded.
extern "C" int D2Action_ListCharactersJson(char* buf, int bufSize)
{
	uintptr_t base = ResolveD2LaunchBase();
	if (!base) { if (bufSize) buf[0] = 0; return -1; }
	uint32_t head = *(volatile uint32_t*)(base + kCharListHeadRva);

	int n = 0, off = 0;
	off += _snprintf_s(buf + off, bufSize - off, _TRUNCATE, "[");
	uint32_t entry = head;
	for (int idx = 0; entry != 0 && idx < 64; ++idx)
	{
		const char* aname = (const char*)(entry + kCharEntryNameRva);
		char name[24]; int j = 0;
		for (; j < 23; ++j) { char c = aname[j]; if (!c) break; name[j] = (c >= 32 && c < 127) ? c : '?'; }
		name[j] = 0;
		uint8_t cls = *(volatile uint8_t*)(entry + kCharEntryClassRva);
		if (off < bufSize - 64)
			off += _snprintf_s(buf + off, bufSize - off, _TRUNCATE,
				"%s{\"index\":%d,\"name\":\"%s\",\"class\":%u}", idx ? "," : "", idx, name, cls);
		++n;
		entry = *(volatile uint32_t*)(entry + kCharEntryNextRva);
	}
	_snprintf_s(buf + off, bufSize - off, _TRUNCATE, "]");
	return n;
}

// Read the menu-launch preconditions from the HTTP thread (cheap plain reads;
// no game-thread hop needed). Returns true if we're at a character-select-like
// screen with a character highlighted (safe to launch).
//   outCharIndex: g_dwSelectedCharIndex (0xFFFFFFFF = none highlighted)
//   outListLoaded: g_pCharListHead != 0 (character list populated)
extern "C" bool D2Action_ReadCharSelectState(uint32_t* outCharIndex, uint32_t* outListLoaded)
{
	uintptr_t base = ResolveD2LaunchBase();
	if (!base) return false;
	uint32_t idx  = *(volatile uint32_t*)(base + kSelectedCharIndexRva);
	uint32_t head = *(volatile uint32_t*)(base + kCharListHeadRva);
	if (outCharIndex)  *outCharIndex = idx;
	if (outListLoaded) *outListLoaded = (head != 0) ? 1u : 0u;
	return (idx != 0xFFFFFFFFu) && (head != 0);
}

// Attach the menu-frame pump hook (idempotent). Safe to call repeatedly; a
// failed attach just means /action routes time out (reported) rather than crash.
extern "C" void D2Action_InstallPumpHook()
{
	if (g_pumpHooked) return;
	uintptr_t base = ResolveD2WinBase();
	if (!base) return; // D2Win not loaded yet; retried lazily before each action

	g_pumpTramp = (void*)(base + kD2WinRenderMainFrameRva);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&g_pumpTramp, (PVOID)PumpHookStub);
	g_pumpHooked = (DetourTransactionCommit() == NO_ERROR);
}

// HTTP-thread entry point: launch the currently-highlighted single-player
// character (replicates clicking the "Normal" difficulty button) by calling
// D2Launch's SelectCharacterByIndex(0) on the GAME THREAD via the queue.
// Returns: 1 ok (queued+executed), 0 timeout (menu pump not firing -- not at a
// menu?), -1 faulted (SEH-caught), -2 D2Launch not resolved, -3 precondition
// (no character highlighted / list not loaded).
extern "C" int D2Action_LaunchSelectedCharacter(int difficulty, int timeoutMs)
{
	D2Action_InstallPumpHook(); // lazy retry (D2Win may load after startup)

	uintptr_t base = ResolveD2LaunchBase();
	if (!base) return -2;

	uint32_t idx = 0, listLoaded = 0;
	if (!D2Action_ReadCharSelectState(&idx, &listLoaded))
		return -3; // no character highlighted, or not at char-select

	void* fn = (void*)(base + kSelectCharByIndexRva);
	uint32_t diff = (difficulty >= 0 && difficulty <= 2) ? (uint32_t)difficulty : 0;
	uint32_t args[1] = { diff }; // bLaunchMode == difficulty (0=Normal,1=NM,2=Hell)
	uint64_t out = 0;
	int gs = D2Gt_Call(fn, /*cc=stdcall*/1, args, 1, /*ret64=*/0, &out, timeoutMs);
	return gs; // 1 ok, 0 timeout, -1 faulted
}

// HTTP-thread entry point: load a single-player character BY NAME -- finds the
// named character in the char-select list, highlights it, and launches it
// (game-thread; drained by the menu pump). Must be at the character-select
// screen. Returns >=0 (matched index, launched), -1 no such character, -2
// D2Launch not resolved, -3 timeout (menu pump not firing), -4 faulted.
extern "C" int D2Action_LoadCharacterByName(const char* name, int difficulty, int timeoutMs)
{
	D2Action_InstallPumpHook();
	if (!ResolveD2LaunchBase()) return -2;

	// Copy the requested name into the game-thread-visible slot.
	int i = 0;
	for (; name && name[i] && i < 63; ++i) g_reqCharName[i] = name[i];
	g_reqCharName[i] = 0;
	g_reqDifficulty = (difficulty >= 0 && difficulty <= 2) ? (uint32_t)difficulty : 0;

	uint64_t out = 0;
	int gs = D2Gt_Call((void*)LoadCharacterByNameImpl, /*cc=cdecl*/0, nullptr, 0, /*ret64=*/0, &out, timeoutMs);
	if (gs == 0)  return -3; // timeout
	if (gs == -1) return -4; // faulted
	return (int)(int32_t)out; // >=0 matched index, or -1 no match
}

// HTTP-thread entry point: click "Single Player" on the main menu -- calls
// D2Launch's StartSinglePlayerMode() on the game thread (sets connection mode 0
// and transitions the title screen to character-select). Returns 1 ok, 0
// timeout (menu pump not firing), -1 faulted, -2 D2Launch not resolved.
extern "C" int D2Action_MainMenuSinglePlayer(int timeoutMs)
{
	D2Action_InstallPumpHook();
	uintptr_t base = ResolveD2LaunchBase();
	if (!base) return -2;
	void* fn = (void*)(base + kStartSinglePlayerRva);
	uint64_t out = 0;
	// StartSinglePlayerMode is int(void); cc=cdecl, 0 args.
	return D2Gt_Call(fn, /*cc=cdecl*/0, nullptr, 0, /*ret64=*/0, &out, timeoutMs);
}

// HTTP-thread entry point: exit Diablo -- sets D2Win's quit levers on the game
// thread so the message loop exits and doesn't restart (the game's own clean
// quit). Returns 1 ok, 0 timeout (menu pump not firing -- must be at a menu),
// -1 faulted, -2 D2Win not resolved.
extern "C" int D2Action_ExitGame(int timeoutMs)
{
	D2Action_InstallPumpHook();
	if (!ResolveD2WinBase()) return -2;
	uint64_t out = 0;
	int gs = D2Gt_Call((void*)ExitGameImpl, /*cc=cdecl*/0, nullptr, 0, /*ret64=*/0, &out, timeoutMs);
	if (gs <= 0) return gs;
	return (int)(int32_t)out == 0 ? 1 : -2;
}

// HTTP-thread entry point: in-game "Save and Exit Game" -> back to the
// character-select menu. Calls D2Client's HandleSaveAndExitDialogConfirm() on
// the game thread; this one runs IN-WORLD so it's drained by the D2Capture
// in-world pump (be in a game, not at a menu). Returns 1 ok, 0 timeout
// (in-world pump not firing -- not in a game?), -1 faulted, -2 D2Client not
// resolved.
extern "C" int D2Action_SaveAndExitToMenu(int timeoutMs)
{
	// SAFE save-and-exit: open the save-exit dialog then confirm it (see
	// SaveExitViaDialogImpl) on the game thread -- the refcount-correct flow that
	// mirrors the manual ESC -> Save and Exit. Drained by the in-world capture pump.
	uintptr_t base = ResolveD2ClientBase();
	if (!base) return -2;
	uint64_t out = 0;
	int gs = D2Gt_Call((void*)SaveExitViaDialogImpl, /*cc=cdecl*/0, nullptr, 0, /*ret64=*/0, &out, timeoutMs);
	if (gs <= 0) return gs;                       // 0 timeout, -1 faulted
	int rv = (int)(int32_t)out;
	return (rv == 0) ? 1 : (rv == -2 ? -6 : -2);  // -6 = a dialog was already open
}
