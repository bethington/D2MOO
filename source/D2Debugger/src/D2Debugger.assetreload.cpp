// D2Debugger.assetreload.cpp -- ASSET OVERLAY ARCHIVE REGISTRATION (PD2 Asset Studio).
//
// Registers a SEPARATE, high-priority patch.mpq into the live game's Storm
// archive search list via SFileOpenArchive(path, priority, ...), so an edited
// DC6/asset inside that archive OVERRIDES the base-game file for every shared
// path -- WITHOUT touching any existing MPQ. This is the override channel for
// Asset Studio: Phase-0 proved the loose-`data\` DC6 overlay does NOT render in
// PD2 (Fog ignores loose DC6), and that modifying PD2's own patch_d2.mpq
// corrupts the loader. A separately-opened archive at priority > 5000 (the
// game's own patch_d2.mpq sits at 5000) sidesteps both. See doc/AssetStudioPlan.md.
//
// SFileOpenArchive is Storm.dll's real archive-open API (ordinal #266); the
// game's own ARCHIVE_LoadArchives calls it at boot. Marshalled onto the GAME
// THREAD (via the gtqueue), so it serializes against the game's own file I/O.
// Register at the menu (before entering a game); then a soft reload (exit-to-menu
// -> re-enter) makes freshly-loaded item art come from our archive.
//
// This TU only OPENS/closes archives (no file writes). Authoring the patch.mpq
// happens out-of-process in the Asset Studio Python tool (StormLib).
#include <Windows.h>
#include <detours.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>

// Game-thread call queue (D2Debugger.gtqueue.cpp).
extern "C" int  D2Gt_Call(void* fn, int cc, const uint32_t* args, int nargs,
                          int ret64, uint64_t* out, int timeoutMs);
// Menu-frame pump installer (D2Debugger.action.cpp) -- ensures the game-thread
// queue is drained while a menu screen is up (that's when we register).
extern "C" void D2Action_InstallPumpHook();

namespace
{
	// Storm.dll archive API -- exported by ORDINAL only (Storm exports are nameless).
	//   BOOL __stdcall SFileOpenArchive (const char* szName, int nPriority, int nFlags, HSARCHIVE* out)  #266
	//   BOOL __stdcall SFileCloseArchive(HSARCHIVE hArchive)                                             #252
	typedef int (__stdcall* OpenArchiveFn )(const char*, int, int, void**);
	typedef int (__stdcall* CloseArchiveFn)(void*);
	const uint16_t kOrdOpenArchive  = 266;
	const uint16_t kOrdCloseArchive = 252;

	void*    g_hArchive     = nullptr;   // our registered archive handle (null = none)
	char     g_lastPath[512] = {0};      // path of the currently-registered archive
	uint32_t g_lastPriority  = 0;

	// Game-thread-visible request slot (HTTP thread writes; game thread reads).
	// Actions are serialized by the caller's g_mcpMutex, so a single slot is safe.
	char     g_reqPath[512] = {0};
	int      g_reqPriority  = 0;

	uintptr_t ResolveStormBase()
	{
		static uintptr_t base = 0;
		if (!base) base = (uintptr_t)GetModuleHandleA("Storm.dll");
		return base;
	}
	OpenArchiveFn ResolveOpen()
	{
		HMODULE h = (HMODULE)ResolveStormBase();
		return h ? (OpenArchiveFn)GetProcAddress(h, MAKEINTRESOURCEA(kOrdOpenArchive)) : nullptr;
	}
	CloseArchiveFn ResolveClose()
	{
		HMODULE h = (HMODULE)ResolveStormBase();
		return h ? (CloseArchiveFn)GetProcAddress(h, MAKEINTRESOURCEA(kOrdCloseArchive)) : nullptr;
	}

	// GAME-THREAD body: close any previously-registered overlay, then open g_reqPath
	// at g_reqPriority. Returns 0 ok, -1 Storm/API not resolved, -2 open failed.
	int __cdecl RegisterArchiveImpl(void)
	{
		OpenArchiveFn  open  = ResolveOpen();
		CloseArchiveFn close = ResolveClose();
		if (!open) return -1;
		if (g_hArchive && close) { close(g_hArchive); g_hArchive = nullptr; }
		void* h = nullptr;
		// nFlags=0: default read-only search participation. High priority so this
		// archive wins every shared file path against the base/patch MPQs.
		int ok = open(g_reqPath, g_reqPriority, 0, &h);
		if (!ok || !h) return -2;
		g_hArchive     = h;
		memcpy(g_lastPath, g_reqPath, sizeof(g_lastPath));
		g_lastPriority = (uint32_t)g_reqPriority;
		return 0;
	}

	// GAME-THREAD body: close the registered overlay (revert to stock file resolution).
	int __cdecl CloseArchiveImpl(void)
	{
		CloseArchiveFn close = ResolveClose();
		if (!close) return -1;
		if (g_hArchive) { close(g_hArchive); g_hArchive = nullptr; g_lastPath[0] = 0; g_lastPriority = 0; }
		return 0;
	}

	// --- Early auto-registration: hook the PD2 all-tables loader (D2Common #10943) ---
	// Data tables load BEFORE any point the menu-time HTTP register can run -- so an
	// overlay archive registered from that route is too late to override
	// data\global\excel\*.bin (item art is lazy-loaded and doesn't care; excel bins
	// do). This detour fires exactly before the game loads the tables and registers
	// the archive named in <workspace>\autoload.txt -- so excel-bin edits (e.g. the
	// uniqueitems invfile sliver) land on a plain full reload with no timing games.
	//
	// ORDINAL GOTCHA (crashed live test #1): PD2 RENUMBERED D2Common's exports --
	// vanilla-1.13c #10576 (DATATBLS_LoadAllTxts) points at a 1-arg RET-4 missile
	// getter in PD2, and detouring that with this 3-arg RET-0xC signature smashed
	// the caller's stack (ACCESS_VIOLATION at world entry). The REAL PD2 loader is
	// DATATBLS_LoadAllDataTables @ 6fdb6160 = EXPORT ORDINAL #10943,
	// void __stdcall(void* pArchive, BOOL fLoadLevelFiles, int nLevelSubParam),
	// verified RET 0xC + Detours-safe 6-byte first instruction (SUB ESP,0x108).
	// A prologue byte-guard below refuses to attach if a future PD2 build moves it
	// again -- failure mode is "no early hook" (visible in /asset/status), never a
	// wrong-function detour.
	typedef void (__stdcall* LoadAllTxtsFn)(void* hArchive, int a2, int a3);
	void* g_loadAllTxtsTramp = nullptr;
	bool  g_earlyHooked = false;
	volatile LONG g_earlyFired  = 0;
	volatile int  g_earlyResult = -1; // 0 ok, 1 no config, 2 archive missing, 3 Storm unresolved, 4 open failed

	const uint16_t kOrdLoadAllTables = 10943; // PD2 D2Common: DATATBLS_LoadAllDataTables
	// First instruction of the PD2 loader (SUB ESP,0x108) -- attach guard.
	const uint8_t  kLoadAllTablesPrologue[6] = { 0x81, 0xEC, 0x08, 0x01, 0x00, 0x00 };

	// Read <workspace>\autoload.txt (line 1: archive path, line 2: priority, default
	// 9000) and open that archive. Plain C (no C++ unwinding) so it can sit under SEH.
	int TryEarlyRegisterBody()
	{
		char ws[384] = {0};
		DWORD n = GetEnvironmentVariableA("ASSET_STUDIO_WS", ws, sizeof(ws) - 1);
		if (n == 0 || n >= sizeof(ws) - 1)
			lstrcpynA(ws, "C:\\Diablo2\\AssetStudio", sizeof(ws));
		char cfg[512];
		_snprintf_s(cfg, sizeof(cfg), _TRUNCATE, "%s\\autoload.txt", ws);
		FILE* f = nullptr;
		if (fopen_s(&f, cfg, "r") != 0 || !f) return 1;
		char path[512] = {0};
		char prio[64]  = {0};
		if (!fgets(path, sizeof(path), f)) { fclose(f); return 1; }
		fgets(prio, sizeof(prio), f);
		fclose(f);
		for (int i = 0; path[i]; ++i) if (path[i] == '\r' || path[i] == '\n') { path[i] = 0; break; }
		int priority = atoi(prio);
		if (priority <= 0) priority = 9000;
		if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) return 2;
		OpenArchiveFn open = ResolveOpen();
		if (!open) return 3;
		void* h = nullptr;
		if (!open(path, priority, 0, &h) || !h) return 4;
		g_hArchive = h;
		lstrcpynA(g_lastPath, path, sizeof(g_lastPath));
		g_lastPriority = (uint32_t)priority;
		return 0;
	}

	void __stdcall LoadAllTxtsDetour(void* hArchive, int a2, int a3)
	{
		if (!InterlockedCompareExchange(&g_earlyFired, 1, 0))
		{
			int rv = -1;
			__try { rv = TryEarlyRegisterBody(); }
			__except (EXCEPTION_EXECUTE_HANDLER) { rv = -2; } // never let a fault stop the boot
			g_earlyResult = rv;
		}
		((LoadAllTxtsFn)g_loadAllTxtsTramp)(hArchive, a2, a3);
	}

	// --- Showcase spawn verb: summon an item at the player's feet ----------
	// Recipe verified in Ghidra (see tools/asset-studio/GHIDRA_FINDINGS.md), matching
	// the game's own quest-drop code (ITEMS_FindItemByDataCode):
	//   classId = ITEMS_GetDataByCode(dwCode)                 // D2Common RVA 0x71940
	//   ITEMS_CreateAndDropItem(pGame, pPlayer, classId, drop) // D2Game  RVA 0x6b070
	// Item creation is SERVER-side, so it needs the SERVER Game* + SERVER player. The
	// server Game* is captured every server frame by hooking GAME_ProcessGameFrameTick
	// (D2Game RVA 0x2E050) -- the per-frame server tick, which receives the game in EAX
	// (caller-set register-passing convention). From it we walk to the player:
	//   Game.pClientList (+0x88) -> GameClient.pPlayer (+0x174).
	// (The D2Common capture hook only sees the CLIENT player in single-player, whose
	// union at +0x80 is a tick count, not pGame -- so it is NOT usable here. And the
	// D2Debugger GAME_UpdateProgress patch @ 0x54400 is misaligned on PD2 -- that offset
	// is PLAYER_HandleDisconnect there, so it never fires in normal play.)
	const uint32_t kD2CommonBase           = 0x6fd50000u;
	const uint32_t kD2GameBase             = 0x6fc20000u;
	const uint32_t kItemsGetDataByCodeRva  = 0x6fdc1940u - kD2CommonBase; // int __stdcall(uint dwCode) -> classId/-1
	const uint32_t kItemsCreateAndDropRva  = 0x6fc8b070u - kD2GameBase;   // int __stdcall(Game*,UnitAny*,int,BOOL)
	const uint32_t kFrameTickRva           = 0x6fc4e050u - kD2GameBase;   // GAME_ProcessGameFrameTick (game in EAX)
	const uint32_t kGameClientListOff      = 0x88u;                       // Game.pClientList
	const uint32_t kClientPlayerOff        = 0x174u;                      // GameClient.pPlayer

	// Inventory pickup via the NATIVE packet path (§27). We drop the item (client-synced ground
	// copy), locate it in the server item hash for its GUID, then replay the client->server pickup
	// packet 0x16 -- in single-player it loops back through D2Net and the server runs the full,
	// synced ground->inventory pickup, exactly as a real mouse click would.
	const uint32_t kGameItemHashOff        = 0x1720u;                     // Game -> item-GUID hash (128 buckets)
	const uint32_t kItemHashNextOff        = 0xE4u;                       // UnitAny -> next-in-hash chain link
	const uint32_t kUnitTypeOff            = 0x00u;                       // UnitAny.dwType (4 == ITEM)
	const uint32_t kUnitTxtFileNoOff       = 0x04u;                       // UnitAny.dwTxtFileNo (== classId)
	const uint32_t kUnitIdOff              = 0x0Cu;                       // UnitAny.dwUnitId (GUID)
	// D2Client generic C->S packet sender (SendChatMessageThrottled @6fac43e0): ECX=buffer, EBX=len.
	const uint32_t kD2ClientBase           = 0x6fab0000u;
	const uint32_t kSendPacketRva          = 0x6fac43e0u - kD2ClientBase; // __fastcall(ECX=buf) + EBX=len
	// Inventory panel (§28, CORRECTED session 2 2026-07-18): 0x6fb0c3d0 is the PARTY-panel key
	// handler (UI panel 0x23) -- NOT the inventory; that is why the old toggle "worked" but nothing
	// rendered. The in-game inventory is UI panel 1. Render gate: g_adwUIPanelActive[1] @6fbaad84
	// (CLIENT_ProcessCompleteGameUIRenderingPipeline calls RenderInventoryPanelUI @6fb49440 when
	// flags[1]|flags[0xC]|flags[0xE]|flags[0x19]|flags[0x1A]|flags[0x1C]|flags[0x1D] is set).
	// Open/close through the coordinated UI state machine (sets the flag + viewport arrange + sound):
	//   CLIENT_ProcessUIStateChange @6fb72790  __fastcall(ECX=panelId, EDX=action, [esp]=bUpdateCursor)
	//   action: 0=open, 1=close, 2=toggle. Panel ids: 1=inventory, 2=char, 4=skilltree, 0xC=stash...
	const uint32_t kProcUIStateChangeRva   = 0x6fb72790u - kD2ClientBase; // the real open/close lever
	const uint32_t kUiPanelActiveArrOff    = 0x6fbaad80u - kD2ClientBase; // dword[0x26]; [1] = inventory
	const uint32_t kInvStateOff            = 0x6fbcc284u - kD2ClientBase; // PARTY panel state (kept: diagnostic)
	// Item hover-text (§28 session 2): the item NAME/description string builder -- callable directly.
	//   CLIENT_BuildItemDescriptionTooltip @6fb414f0  __stdcall(UnitAny* pItem, wchar_t* out, int maxLen)
	// Client player unit g_pCurrentUnit @6fbcbbfc; UnitAny.pInventory +0x60; Inventory.pFirstItem +0x0C;
	// UnitAny.pItemData +0x14; ItemData+0x64 = next-item-in-inventory (ItemExtraData.pNextItem).
	// Mouse-position globals (hover simulation: poke these over a grid cell while the panel is open):
	//   MouseX @6fbcb828 (rva 0x11B828), MouseY @6fbcb824 (rva 0x11B824).
	const uint32_t kItemDescBuilderRva     = 0x6fb414f0u - kD2ClientBase;
	const uint32_t kPlayerUnitOff          = 0x6fbcbbfcu - kD2ClientBase; // g_pCurrentUnit (client player)

	typedef int (__stdcall* GetDataByCodeFn)(uint32_t);
	typedef int (__stdcall* CreateAndDropFn)(void*, void*, int, int);

	uint32_t g_reqItemCode = 0; // 4-char code packed little-endian ('uap ' = 0x20706175)
	int      g_reqDrop     = 1;
	int      g_reqDest     = 0; // 0 = drop at feet; 1 = place in inventory (create+drop, then pickup)
	int      g_reqQuality  = 0; // ITEMQUAL_* to force (0 = normal roll). 5=set, 7=unique, etc.
	int      g_reqQualRow  = -1; // >=0 = force this setitems/uniqueitems row for the quality; -1 = random
	int      g_reqIdentify = 0; // 1 = set IFLAG_IDENTIFIED on the dropped item (so unique own-invfile art shows)

	bool LooksLikePtr(void* p); // defined below

	// --- Forced-quality create hook (spawn a SPECIFIC set item) --------------
	// A set item's exact row is forced by the creation desc: sub_6FC542C0 (ItemsMagic.cpp:842)
	// selects setitems row i when pItemDrop->nItemIndex-1 == i. The convenient wrappers don't
	// expose nItemIndex, so we DETOUR ITEMS_CreateItemUnit @ 6fc31490 -- the single create+assign
	// entry both ITEMS_CreateAndDropItem and CreateItemWithParams call -- and flip nQuality/nItemIndex
	// in the desc just-in-time, reusing the whole proven create->drop->0x16-pickup path unchanged.
	// The desc layout matches D2MOO's ItemDrop EXACTLY (confirmed against 6fc31490's disassembly):
	//   nItemLvl @0x0C, nId @0x14, nSpawnType @0x18, wUnitInitFlags @0x28, wItemFormat @0x2A,
	//   nQuality @0x30, nItemIndex @0x40, dwFlags2 @0x80.  __stdcall(Game*, ItemDrop*, int) RET 0xC.
	typedef int (__stdcall* CreateItemUnitFn)(void* pGame, void* pDesc, int arg2);
	const uint32_t kCreateItemUnitRva = 0x6fc31490u - kD2GameBase;
	const uint8_t  kCreateItemUnitPrologue[5] = { 0x55, 0x8B, 0x6C, 0x24, 0x0C }; // PUSH EBP; MOV EBP,[ESP+0xC]
	void* g_createItemUnitTramp   = nullptr;
	bool  g_createItemUnitHooked  = false;
	volatile LONG g_forceQualPending = 0; // one-shot: consumed by the next CreateItemUnit
	volatile int  g_forceQualValue   = 5; // ITEMQUAL_* to force (0 = don't force quality -> normal roll)
	volatile int  g_forceQualIndex   = 0; // row+1 hint: setitems/uniqueitems row (0 = random of that quality)
	volatile int  g_forceIlvlMin     = 30; // stamp ilvl to at least this so the forced row is eligible
	                                        // (unique/set rows gate on wLvl <= ilvl; 99 makes all eligible)

	int __stdcall CreateItemUnitDetour(void* pGame, void* pDesc, int arg2)
	{
		if (InterlockedCompareExchange(&g_forceQualPending, 0, 1) && LooksLikePtr(pDesc))
		{
			char* d = (char*)pDesc;
			if (g_forceQualValue)
				*(volatile int*)(d + 0x30) = g_forceQualValue;                   // nQuality (set/unique/magic/...)
			*(volatile int*)(d + 0x40) = g_forceQualIndex;                       // nItemIndex = row+1 (set OR unique)
			if (*(volatile int*)(d + 0x0C) < g_forceIlvlMin)
				*(volatile int*)(d + 0x0C) = g_forceIlvlMin;                     // ilvl >= forced row's lvl
			*(volatile uint32_t*)(d + 0x80) |= 1u;                               // dwFlags2: allow restricted sets
		}
		return ((CreateItemUnitFn)g_createItemUnitTramp)(pGame, pDesc, arg2);
	}

	// Find a server item by GUID in the game item hash (read-only, guarded). Returns node or null.
	void* FindServerItemByGuid(void* pGame, uint32_t guid)
	{
		if (!LooksLikePtr(pGame) || !guid) return nullptr;
		uint32_t* buckets = (uint32_t*)((char*)pGame + kGameItemHashOff);
		void* node = (void*)buckets[guid & 0x7f];
		for (int g = 0; LooksLikePtr(node) && g < 4096; ++g)
		{
			if (*(volatile uint32_t*)((char*)node + kUnitIdOff) == guid) return node;
			node = (void*)*(volatile uint32_t*)((char*)node + kItemHashNextOff);
		}
		return nullptr;
	}

	// --- server Game* capture hook (GAME_ProcessGameFrameTick) --------------
	void* g_frameTickTramp = nullptr;      // trampoline to the real frame tick
	bool  g_frameTickHooked = false;
	volatile void* g_serverGame = nullptr; // EAX (Game*) grabbed each server frame

	// Spawn request/completion, consumed INSIDE the frame-tick handler so the item is
	// created in the SERVER frame context (game lock held) -- calling it from the
	// client-side capture pump faults (server item-creation needs that context).
	volatile LONG g_spawnPending = 0;      // HTTP thread sets; frame-tick handler consumes
	volatile LONG g_spawnDone    = 0;      // frame-tick handler sets when finished
	volatile int  g_spawnResult  = 0;      // result code (see SpawnItemCore)

	// Diagnostics for the last spawn attempt (surfaced via /asset/status).
	volatile int   g_dbgStage    = 0;
	volatile void* g_dbgGame      = nullptr;
	volatile void* g_dbgClient    = nullptr;
	volatile void* g_dbgPlayer    = nullptr;
	volatile int   g_dbgClassId   = -999;
	volatile uint32_t g_dbgPlayerCls  = 0;  // player+0x04 dwClassId
	volatile uint32_t g_dbgPlayerId   = 0;  // player+0x0C dwUnitId (GUID)
	volatile uint32_t g_dbgPlayerPath = 0;  // player+0x2C pPath (should look like a ptr)
	volatile uint32_t g_dbgGuid       = 0;  // GUID of the item located for inventory pickup
	volatile uint32_t g_dbgMode       = 0xFFFFFFFFu; // item field +0x10 as found (pickup gates on ==4)

	volatile uint32_t g_lastDroppedGuid = 0; // GUID of the item dropped by the last spawn (for pickup)

	// Plausible in-process heap pointer? Guards every dereference below so a wrong
	// struct offset / not-yet-populated capture fails cleanly instead of crashing.
	bool LooksLikePtr(void* p)
	{
		uintptr_t v = (uintptr_t)p;
		return v >= 0x00100000u && v < 0x7FFF0000u && (v & 3u) == 0u;
	}

	// READ-ONLY: walk the server game's item hash (Game+0x1720, 128 buckets, chained via UnitAny+0xE4)
	// and return the newest (highest dwUnitId) on-ground ITEM whose dwTxtFileNo == classId -- i.e. the
	// item we just dropped. Guarded derefs; returns 0 if not found (never crashes).
	uint32_t ScanForDroppedGuid(void* pGame, int classId)
	{
		uint32_t* buckets = (uint32_t*)((char*)pGame + kGameItemHashOff);
		uint32_t best = 0; bool found = false;
		for (int b = 0; b < 128; ++b)
		{
			void* node = (void*)buckets[b];
			for (int guard = 0; LooksLikePtr(node) && guard < 4096; ++guard)
			{
				uint32_t type = *(volatile uint32_t*)((char*)node + kUnitTypeOff);
				uint32_t cls  = *(volatile uint32_t*)((char*)node + kUnitTxtFileNoOff);
				uint32_t guid = *(volatile uint32_t*)((char*)node + kUnitIdOff);
				if (type == 4 && (int)cls == classId && (!found || guid > best)) { best = guid; found = true; }
				node = (void*)*(volatile uint32_t*)((char*)node + kItemHashNextOff);
			}
		}
		return found ? best : 0;
	}

	// Create+drop under its own SEH. In single-player the drop path lands the item on the ground
	// (created, positioned, hash-registered, and synced to the client) and THEN faults on a harmless
	// post-drop client-notify step -- swallowing that fault leaves a clean, client-visible ground
	// item. Returns createDrop's status, or 1 if it faulted post-drop (item is on the ground by then).
	int CreateAndDropSwallow(CreateAndDropFn fn, void* pGame, void* pPlayer, int classId, int bDrop)
	{
		int rv = 0;
		__try { rv = fn(pGame, pPlayer, classId, bDrop); }
		__except (EXCEPTION_EXECUTE_HANDLER) { rv = 1; } // post-drop notify fault; item already dropped
		return rv;
	}

	// The actual spawn, run on the SERVER thread inside the frame-tick handler with the
	// live current-frame game. Returns 0 ok, -8 no client, -1 no player, -7 not a
	// player, -4 module, -5 unknown code, -6 create failed.
	int SpawnItemCore(void* pGame)
	{
		g_dbgStage = 1; g_dbgClient = g_dbgPlayer = nullptr; g_dbgClassId = -999;
		g_dbgGame = pGame;
		void* pClient = *(void* volatile*)((char*)pGame + kGameClientListOff); // Game.pClientList
		g_dbgClient = pClient;
		if (!LooksLikePtr(pClient)) return -8;
		g_dbgStage = 3;
		void* pPlayer = *(void* volatile*)((char*)pClient + kClientPlayerOff); // GameClient.pPlayer
		g_dbgPlayer = pPlayer;
		if (!LooksLikePtr(pPlayer)) return -1;
		g_dbgStage = 4;
		if (*(volatile uint32_t*)pPlayer != 0) return -7; // dwType at +0 must be 0 (PLAYER)
		// Record + validate the player is a real UnitAny before the risky create call.
		g_dbgPlayerCls  = *(volatile uint32_t*)((char*)pPlayer + 0x04); // dwClassId
		g_dbgPlayerId   = *(volatile uint32_t*)((char*)pPlayer + 0x0C); // dwUnitId
		g_dbgPlayerPath = *(volatile uint32_t*)((char*)pPlayer + 0x2C); // pPath (should be a ptr)
		if (!LooksLikePtr((void*)g_dbgPlayerPath)) return -9; // player has no valid path -> wrong walk
		g_dbgStage = 5;

		uintptr_t d2common = (uintptr_t)GetModuleHandleA("D2Common.dll");
		uintptr_t d2game   = (uintptr_t)GetModuleHandleA("D2Game.dll");
		if (!d2common || !d2game) return -4;
		GetDataByCodeFn getByCode = (GetDataByCodeFn)(d2common + kItemsGetDataByCodeRva);
		CreateAndDropFn createDrop = (CreateAndDropFn)(d2game + kItemsCreateAndDropRva);

		g_dbgStage = 6;
		int classId = getByCode(g_reqItemCode);
		g_dbgClassId = classId;
		if (classId < 0) return -5;
		// Both dests DROP the item at the player's feet -- the drop is the only runtime path that
		// syncs a full item copy to the SP client (dropped items render), so the workflow is
		// drop-then-click: the item lands on the ground and the player clicks it to pick it up into
		// the inventory (which shows the overridden art). A fully-automated inventory placement is a
		// multi-stage re-implementation of D2's pickup pipeline -- see doc/AssetStudioPlan.md §26.
		// The drop's harmless post-notify fault is swallowed so this returns a clean success.
		g_dbgStage = 7;
		g_lastDroppedGuid = 0;
		// Arm the one-shot create hook so the item created below is forced to the requested quality
		// (and specific setitems/uniqueitems row when given). Consumed inside ITEMS_CreateAndDropItem's
		// create call. The unique roller (ItemMode.cpp:6596) and set roller (ItemsMagic.cpp:842) both
		// select row i when the desc's nItemIndex-1 == i, gated by wLvl <= ilvl -- so we stamp a high
		// ilvl for those. quality 0 = leave the normal roll alone (plain base item).
		if (g_reqQuality || g_reqQualRow >= 0)
		{
			g_forceQualValue = g_reqQuality;                                  // ITEMQUAL_* (0 = don't force)
			g_forceQualIndex = (g_reqQualRow >= 0) ? g_reqQualRow + 1 : 0;    // row+1 hint (0 = random)
			// unique/set specific rows can require a high item level to be eligible; stamp 99 so any
			// forced row for the base qualifies. Other qualities keep the modest default floor.
			g_forceIlvlMin = (g_reqQuality == 5 || g_reqQuality == 7) ? 99 : 30;
			MemoryBarrier();
			g_forceQualPending = 1;
		}
		int rv = CreateAndDropSwallow(createDrop, pGame, pPlayer, classId, 1);
		g_forceQualPending = 0; // clear even if the create path didn't consume it
		if (rv == 0) { g_dbgStage = 8; return -6; }
		// Locate the just-dropped item so the caller can replay the 0x16 pickup packet for dest=inventory.
		g_dbgStage = 9;
		g_lastDroppedGuid = ScanForDroppedGuid(pGame, classId);
		g_dbgGuid = g_lastDroppedGuid;
		// Uniques/sets are created UNIDENTIFIED by the game (the rollers clear IFLAG_IDENTIFIED). But a
		// unique's own inventory art only renders once IDENTIFIED, so on request we set the flag on the
		// SERVER ground item now, before the player picks it up -- the natural pickup then re-serializes
		// full state to the client, showing the unique's art and tooltip. We set it on the ground unit
		// only (no force-pickup, no client poke): the 2026-07-18 crash was IDENTIFIED + a FORCE-pickup
		// REPLAY on a set amulet, which desynced the set/partial-bonus recompute. The caller withholds
		// identify for set jewelry as a belt-and-braces guard (see api_drop).
		if (g_reqIdentify && g_lastDroppedGuid)
		{
			void* item = FindServerItemByGuid(pGame, g_lastDroppedGuid);
			if (LooksLikePtr(item))
			{
				char* idata = *(char* volatile*)((char*)item + 0x14); // UnitAny.pItemData
				if (LooksLikePtr(idata))
					*(volatile uint32_t*)(idata + 0x18) |= 0x10u;     // ItemData.dwItemFlags |= IFLAG_IDENTIFIED
			}
		}
		g_dbgStage = 8;
		return 0;
	}
}

// C handler called from the naked frame-tick stub EVERY server frame with the live
// server Game* (EAX). Caches it, and if a spawn is pending, runs it HERE (server
// frame context) under SEH so a fault can't take down the server thread. extern "C"
// so the naked stub can call it; no C++ object unwinding in this function.
extern "C" void FrameTickHandler(void* pGame)
{
	g_serverGame = pGame;
	if (!InterlockedCompareExchange(&g_spawnPending, 0, 1)) return; // claim if pending
	int rv;
	__try { rv = LooksLikePtr(pGame) ? SpawnItemCore(pGame) : -3; }
	__except (EXCEPTION_EXECUTE_HANDLER) { rv = -99; } // keep g_dbgStage = the faulting stage
	g_spawnResult = rv;
	MemoryBarrier();
	g_spawnDone = 1;
}

namespace
{
	// The frame tick receives Game* in EAX and is void(void) RET 0. We run the real
	// tick body FIRST (call the trampoline with EAX=game), THEN run the pending spawn --
	// so item creation/drop happens at frame-tick EXIT, after the tick's unit/room/
	// command processing has set up the per-frame context (the drop's command-write path
	// faults if run at entry). The original preserves ESI/EBX/EDI; we touch only EAX +
	// the stack, so the caller's callee-saved regs survive.
	__declspec(naked) void FrameTickHookStub()
	{
		__asm {
			mov  g_serverGame, eax       // cache game for /status + the post-tick spawn
			call [g_frameTickTramp]        // run the real GAME_ProcessGameFrameTick (EAX=game)
			push dword ptr g_serverGame   // arg0 = game (EAX is scratch after the void tick)
			call FrameTickHandler          // run the pending spawn now (post-tick context)
			add  esp, 4
			ret                            // return to the tick's caller (RET 0)
		}
	}
}

// Install the DATATBLS_LoadAllTxts pre-hook (idempotent, retryable). Called from
// D2Debugger_StartStandalone (DllMain context -- Detours supports attach there,
// and D2Common is already loaded as a dependency of the DLL whose load triggered
// us) so it is in place before the game's startup sequence reaches table load.
// The hook body only reads a config file + calls SFileOpenArchive, both on the
// game's own data-load thread -- nothing heavy happens at DllMain time.
extern "C" void D2Asset_InstallEarlyRegHook()
{
	if (g_earlyHooked) return;
	HMODULE h = GetModuleHandleA("D2Common.dll");
	if (!h) return;
	void* fn = GetProcAddress(h, MAKEINTRESOURCEA(kOrdLoadAllTables));
	if (!fn) return;
	// Attach guard: only hook if this is really the PD2 all-tables loader we
	// verified (RET 0xC / 3 stdcall args). A moved/renumbered export fails safe.
	if (memcmp(fn, kLoadAllTablesPrologue, sizeof(kLoadAllTablesPrologue)) != 0)
	{
		g_earlyResult = -3; // prologue mismatch -- refused to attach
		return;
	}
	g_loadAllTxtsTramp = fn;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&g_loadAllTxtsTramp, (PVOID)LoadAllTxtsDetour);
	g_earlyHooked = (DetourTransactionCommit() == NO_ERROR);
}

// Install the forced-quality create hook on ITEMS_CreateItemUnit (idempotent). Guarded by
// a prologue byte-check so a renumbered/moved PD2 build fails safe (no hook) rather than
// detouring the wrong function. Only active while g_forceQualPending is armed for one call.
extern "C" void D2Asset_InstallCreateItemHook()
{
	if (g_createItemUnitHooked) return;
	uintptr_t base = (uintptr_t)GetModuleHandleA("D2Game.dll");
	if (!base) return;
	void* fn = (void*)(base + kCreateItemUnitRva);
	if (memcmp(fn, kCreateItemUnitPrologue, sizeof(kCreateItemUnitPrologue)) != 0) return; // fail-safe
	g_createItemUnitTramp = fn;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&g_createItemUnitTramp, (PVOID)CreateItemUnitDetour);
	g_createItemUnitHooked = (DetourTransactionCommit() == NO_ERROR);
}

// Install the server-Game* capture hook on GAME_ProcessGameFrameTick (idempotent,
// retryable). Called at D2Debugger startup and lazily before each spawn; the first
// successful attach means the next server frame populates g_serverGame. Safe if it
// fails -- spawn just reports "server game not captured".
extern "C" void D2Asset_InstallServerGameHook()
{
	if (g_frameTickHooked) return;
	uintptr_t base = (uintptr_t)GetModuleHandleA("D2Game.dll");
	if (!base) return;
	g_frameTickTramp = (void*)(base + kFrameTickRva);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&g_frameTickTramp, (PVOID)FrameTickHookStub);
	g_frameTickHooked = (DetourTransactionCommit() == NO_ERROR);
	D2Asset_InstallCreateItemHook(); // also arm the forced-quality (set-item) create hook
}

// HTTP entry: register (or re-register) a high-priority overlay archive. Copies
// path into the game-thread slot and marshals SFileOpenArchive onto the game
// thread. Returns 1 ok, 0 timeout (no game-thread pump firing), -1 faulted
// (SEH), -2 Storm/API not resolved, -3 SFileOpenArchive failed (bad path?).
extern "C" int D2Asset_RegisterArchive(const char* path, int priority, int timeoutMs)
{
	if (!path || !*path) return -2;
	D2Action_InstallPumpHook(); // ensure the menu pump drains our queued call
	int i = 0; for (; path[i] && i < 511; ++i) g_reqPath[i] = path[i]; g_reqPath[i] = 0;
	g_reqPriority = priority;
	uint64_t out = 0;
	int gs = D2Gt_Call((void*)RegisterArchiveImpl, /*cc=cdecl*/0, nullptr, 0, /*ret64=*/0, &out, timeoutMs);
	if (gs == 0)  return 0;   // timeout
	if (gs == -1) return -1;  // faulted
	int rv = (int)(int32_t)out;
	return (rv == 0) ? 1 : (rv == -1 ? -2 : -3);
}

// HTTP entry: close the registered overlay archive. Returns 1 ok, 0 timeout,
// -1 faulted, -2 close API not resolved.
extern "C" int D2Asset_CloseArchive(int timeoutMs)
{
	D2Action_InstallPumpHook();
	uint64_t out = 0;
	int gs = D2Gt_Call((void*)CloseArchiveImpl, /*cc=cdecl*/0, nullptr, 0, /*ret64=*/0, &out, timeoutMs);
	if (gs <= 0) return gs;
	return ((int)(int32_t)out == 0) ? 1 : -2;
}

// HTTP entry: summon an item (showcase verb). `code` is the 1-4 char item code (e.g. "uap"
// for the Shako base); it is space-padded to 4 and packed little-endian. `dest` picks where it
// lands: 0 = drop at the player's feet (flippy + inv art on pickup); 1 = place directly in the
// player's inventory (create+drop, then run the game's own pickup so the inventory art shows with
// full client sync). `drop` is honoured only for dest=0. Must be IN a game. Posts a spawn request
// that the SERVER frame-tick handler runs in the server context (game lock held), then waits.
// Returns 1 ok, 0 timeout (no server frame ran -- not in a game, or the frame-tick hook not
// installed), -1 faulted (SEH), -2 server game not captured, -3 no client / no player, -4 module
// not resolved, -5 unknown item code, -6 create/drop failed, -7 located but pickup rejected,
// -8 dropped but not locatable to pick up.
extern "C" void D2Asset_InstallServerGameHook();
extern "C" void D2Asset_InstallCreateItemHook();

// `quality` (ITEMQUAL_*, 0 = normal roll) forces the item's quality; `qualRow` >= 0 additionally
// forces a specific setitems/uniqueitems row for that quality (row is the game's compiled index,
// Expansion-separator excluded). For set/unique the base `code` MUST be that row's base item (e.g.
// "uap" for the Shako base of Harlequin Crest) -- the game's roller only matches a row whose base
// code equals the item's, and a mismatch silently falls back. `identify` sets IFLAG_IDENTIFIED on
// the dropped item (needed for a unique's own inventory art to render). qualRow<0 + quality 5/7 =
// a RANDOM set/unique of the base. quality 0 = plain base item (unchanged legacy behavior).
extern "C" int D2Asset_SpawnItem(const char* code, int drop, int dest,
                                 int quality, int qualRow, int identify, int timeoutMs)
{
	if (!code || !*code) return -5;
	D2Asset_InstallServerGameHook(); // lazy retry (idempotent) in case startup attach raced
	D2Asset_InstallCreateItemHook(); // ensure the forced-quality create hook is in place
	// Pack up to 4 chars little-endian, space-padded (matches how D2 stores codes).
	uint32_t packed = 0x20202020u; // four spaces
	for (int i = 0; i < 4 && code[i]; ++i)
		packed = (packed & ~(0xFFu << (i * 8))) | ((uint32_t)(uint8_t)code[i] << (i * 8));
	g_reqItemCode = packed;
	g_reqDrop = drop ? 1 : 0;
	g_reqDest = (dest == 1) ? 1 : 0;
	g_reqQuality = quality;
	g_reqQualRow = qualRow;
	g_reqIdentify = identify ? 1 : 0;

	// Post the request to the server frame-tick handler and wait for it to run.
	g_spawnDone = 0; g_spawnResult = 0;
	MemoryBarrier();
	g_spawnPending = 1;
	for (int i = 0; i < timeoutMs; ++i)
	{
		if (g_spawnDone)
		{
			int rv = g_spawnResult;
			if (rv == 0)   return 1;   // ok
			if (rv == -99) return -1;  // faulted inside create/pickup (SEH)
			if (rv == -3)  return -2;  // server game bad/absent
			if (rv == -8 || rv == -1 || rv == -7) return -3; // no client / player / not player
			if (rv == -4)  return -4;
			if (rv == -5)  return -5;
			if (rv == -11) return -7;  // pickup rejected
			if (rv == -10) return -8;  // dropped but not located
			return -6;
		}
		Sleep(1);
	}
	g_spawnPending = 0; // give up (no server frame ran)
	return 0;
}

// GUID of the item dropped by the last D2Asset_SpawnItem (for the pickup replay).
extern "C" uint32_t D2Asset_LastDroppedGuid() { return g_lastDroppedGuid; }

namespace
{
	// --- Native 0x16 "pick up ground item" packet replay (see doc/AssetStudioPlan.md §27) ---------
	char  g_pickupPkt[16];          // the 13-byte 0x16 packet buffer (persists across the async call)
	void* g_sendPacketFn = nullptr; // D2Client SendChatMessageThrottled (@6fac43e0)

	// GAME-THREAD body: replay the packet exactly as the client would. From the disassembly,
	// SendChatMessageThrottled takes the packet BUFFER as a single stack arg ([EBP+8]) and the LENGTH
	// in EBX, and cleans its own arg (RET 4). So: EBX=len, push buffer, call.
	int __cdecl SendPickupPacketImpl(void)
	{
		void* fn = g_sendPacketFn; char* buf = g_pickupPkt;
		if (!fn) return -1;
		__asm {
			push ebx            // preserve EBX
			mov  ebx, 13        // packet length
			push buf            // arg1 = packet buffer (callee RET 4 pops it)
			call fn
			pop  ebx            // restore EBX
		}
		return 0;
	}
}

// HTTP entry: replay the client->server pickup packet 0x16 for `guid` with dest=0 (straight to the
// inventory). In single-player D2Net loops it back to the local server, which runs the full, synced
// ground->inventory pickup -- exactly as a real mouse click. Marshalled onto the game thread.
// Returns 1 ok, 0 timeout, -1 faulted (SEH), -2 D2Client not resolved, -5 bad guid.
extern "C" int D2Asset_PickupDropped(uint32_t guid, int timeoutMs)
{
	if (guid == 0) return -5;
	uintptr_t d2client = (uintptr_t)GetModuleHandleA("D2Client.dll");
	if (!d2client) return -2;
	g_sendPacketFn = (void*)(d2client + kSendPacketRva);
	// 0x16 packet: 16 04 00 00 00 <guid:4 LE> <dest:4 = 0 -> inventory>
	memset(g_pickupPkt, 0, sizeof(g_pickupPkt));
	g_pickupPkt[0] = 0x16; g_pickupPkt[1] = 0x04;
	*(uint32_t*)(g_pickupPkt + 5) = guid;
	D2Action_InstallPumpHook();
	uint64_t out = 0;
	int gs = D2Gt_Call((void*)SendPickupPacketImpl, /*cc=cdecl*/0, nullptr, 0, /*ret64*/0, &out, timeoutMs);
	if (gs == 0)  return 0;   // timeout
	if (gs == -1) return -1;  // faulted
	return 1;
}

namespace
{
	// GAME-THREAD body: drive the coordinated UI state machine for panel 1 (inventory).
	// g_uiPanelAction: 0=open, 1=close. Runs CLIENT_ProcessUIStateChange(1, action, 0), which sets
	// g_adwUIPanelActive[1], arranges the viewport, and plays the panel sound -- the same path the
	// game's own minipanel button uses.
	volatile uint32_t g_uiPanelAction = 0;
	int __cdecl DriveInventoryPanelImpl(void)
	{
		uintptr_t d2client = (uintptr_t)GetModuleHandleA("D2Client.dll");
		if (!d2client) return -1;
		typedef int (__fastcall* ProcUIStateChangeFn)(uint32_t dwPanelId, uint32_t dwAction, int bUpdateCursor);
		((ProcUIStateChangeFn)(d2client + kProcUIStateChangeRva))(1u, g_uiPanelAction, 0);
		return 0;
	}
}

// HTTP entry: open (nClose=0) or close (nClose=1) the inventory panel (idempotent) through
// CLIENT_ProcessUIStateChange(panel=1, action) on the game thread. Verifies the render-gate flag
// g_adwUIPanelActive[1] after the call. Returns 1 ok, 0 timeout, -1 faulted, -2 D2Client not
// resolved, -3 state machine refused (flag unchanged).
extern "C" int D2Asset_DriveInventory(int nClose, int timeoutMs);
extern "C" int D2Asset_OpenInventory(int timeoutMs) { return D2Asset_DriveInventory(0, timeoutMs); }
extern "C" int D2Asset_DriveInventory(int nClose, int timeoutMs)
{
	uintptr_t d2client = (uintptr_t)GetModuleHandleA("D2Client.dll");
	if (!d2client) return -2;
	volatile uint32_t* pFlag = (volatile uint32_t*)(d2client + kUiPanelActiveArrOff + 4); // [1] = inventory
	const uint32_t want = nClose ? 0u : 1u;
	if (*pFlag == want) return 1; // already in the requested state
	g_uiPanelAction = nClose ? 1u : 0u;
	D2Action_InstallPumpHook();
	uint64_t out = 0;
	int gs = D2Gt_Call((void*)DriveInventoryPanelImpl, /*cc=cdecl*/0, nullptr, 0, /*ret64*/0, &out, timeoutMs);
	if (gs == 0)  return 0;
	if (gs == -1) return -1;
	return (*pFlag == want) ? 1 : -3;
}

namespace
{
	// --- Item hover-text extraction (§28 session 2) --------------------------------------------
	// Finds the CLIENT-side item unit by GUID (walks the client player's inventory chain) and runs
	// the game's own name/description builder into a static wide buffer -- the exact string the
	// hover tooltip's name section shows (localized, quality-colored). Runs on the game thread.
	volatile uint32_t g_itemTextGuid = 0;   // in: GUID to look up (0 = first item in inventory)
	wchar_t           g_itemTextBuf[512];   // out: wide tooltip name text
	volatile int      g_itemTextRc = 0;     // out: 1 ok, -1 no player, -2 no inventory, -3 not found

	int __cdecl BuildItemTextImpl(void)
	{
		g_itemTextRc = 0; g_itemTextBuf[0] = 0;
		uintptr_t d2client = (uintptr_t)GetModuleHandleA("D2Client.dll");
		if (!d2client) { g_itemTextRc = -1; return -1; }
		char* player = *(char* volatile*)(d2client + kPlayerUnitOff);      // g_pCurrentUnit
		if (!LooksLikePtr(player)) { g_itemTextRc = -1; return -1; }
		char* inv = *(char* volatile*)(player + 0x60);                     // UnitAny.pInventory
		if (!LooksLikePtr(inv)) { g_itemTextRc = -2; return -2; }
		char* item = *(char* volatile*)(inv + 0x0C);                       // Inventory.pFirstItem
		char* found = nullptr;
		for (int g = 0; LooksLikePtr(item) && g < 4096; ++g)
		{
			if (g_itemTextGuid == 0 || *(volatile uint32_t*)(item + kUnitIdOff) == g_itemTextGuid) { found = item; break; }
			char* itemData = *(char* volatile*)(item + 0x14);              // UnitAny.pItemData
			if (!LooksLikePtr(itemData)) break;
			item = *(char* volatile*)(itemData + 0x64);                    // ItemExtraData.pNextItem
		}
		if (!found) { g_itemTextRc = -3; return -3; }
		typedef void (__stdcall* BuildItemDescFn)(void* pItem, wchar_t* wszOut, int nMaxLen);
		((BuildItemDescFn)(d2client + kItemDescBuilderRva))(found, g_itemTextBuf, 500);
		g_itemTextBuf[511] = 0;
		g_itemTextRc = 1;
		return 0;
	}
}

// HTTP entry: park the REAL OS cursor so the GAME sees its mouse at game-space (gameX, gameY) --
// deterministic hover driving. The game polls the physical cursor and writes its own view into
// g_nMouseX/Y (@6fbcb828/@6fbcb824), so we close the loop against those globals: move, wait a
// frame, read back, correct (handles any window position/DPI/scale without constants). Runs on the
// HTTP thread (no game-thread call needed). Returns 1 converged, -1 no movement registered (game
// paused / minimized?), -2 D2Client not resolved, -3 did not converge (target off-window?).
// On return *outX/*outY hold the game's final mouse coords.
extern "C" int D2Asset_HoverXY(int gameX, int gameY, int* outX, int* outY)
{
	uintptr_t d2client = (uintptr_t)GetModuleHandleA("D2Client.dll");
	if (!d2client) return -2;
	volatile int* pMX = (volatile int*)(d2client + (0x6fbcb828u - kD2ClientBase));
	volatile int* pMY = (volatile int*)(d2client + (0x6fbcb824u - kD2ClientBase));
	POINT cur; if (!GetCursorPos(&cur)) return -1;
	// The game only refreshes g_nMouseX/Y while the physical cursor is INSIDE its render window --
	// seed the loop from the window center (and re-seed if the readback ever freezes) so the
	// feedback always has signal. Same-process FindWindow works regardless of elevation.
	HWND hGame = FindWindowA(nullptr, "Diablo II");
	RECT rcGame = { 0, 0, 0, 0 };
	if (hGame) GetWindowRect(hGame, &rcGame);
	if (hGame && (cur.x < rcGame.left || cur.x > rcGame.right || cur.y < rcGame.top || cur.y > rcGame.bottom))
	{
		cur.x = (rcGame.left + rcGame.right) / 2; cur.y = (rcGame.top + rcGame.bottom) / 2;
		SetCursorPos(cur.x, cur.y);
		Sleep(80);
	}
	double scale = 0.8; // screen-px per game-px first guess; re-estimated from the first observed move
	int lastGX = *pMX, lastGY = *pMY; POINT lastCur = cur;
	int frozen = 0;
	for (int it = 0; it < 12; ++it)
	{
		int gx = *pMX, gy = *pMY;
		int dx = gameX - gx, dy = gameY - gy;
		if (dx >= -2 && dx <= 2 && dy >= -2 && dy <= 2)
		{
			if (outX) *outX = gx; if (outY) *outY = gy;
			return 1;
		}
		if (it > 0)
		{
			if (gx != lastGX || gy != lastGY)
			{
				frozen = 0;
				// re-estimate the screen/game scale from what the last move actually did
				int mvS = (cur.x - lastCur.x) + (cur.y - lastCur.y);
				int mvG = (gx - lastGX) + (gy - lastGY);
				if (mvG != 0 && mvS != 0) { double s = (double)mvS / (double)mvG; if (s > 0.2 && s < 5.0) scale = s; }
			}
			else if (++frozen >= 2 && hGame)
			{
				// readback frozen: cursor drifted out of the window -- recover from its center
				GetWindowRect(hGame, &rcGame);
				cur.x = (rcGame.left + rcGame.right) / 2; cur.y = (rcGame.top + rcGame.bottom) / 2;
				SetCursorPos(cur.x, cur.y);
				Sleep(80);
				frozen = 0; lastGX = *pMX; lastGY = *pMY; lastCur = cur;
				continue;
			}
		}
		lastGX = gx; lastGY = gy; lastCur = cur;
		int stepX = (int)(dx * scale), stepY = (int)(dy * scale);
		if (stepX > 400) stepX = 400; if (stepX < -400) stepX = -400;
		if (stepY > 400) stepY = 400; if (stepY < -400) stepY = -400;
		cur.x += stepX; cur.y += stepY;
		SetCursorPos(cur.x, cur.y);
		Sleep(60); // let the game frame poll the new position
	}
	if (outX) *outX = *pMX; if (outY) *outY = *pMY;
	int fx = *pMX - gameX, fy = *pMY - gameY;
	if (fx >= -6 && fx <= 6 && fy >= -6 && fy <= 6) return 1; // near enough for a 29px cell
	return -3;
}

// HTTP entry: build the localized hover-name text for the client item with `guid` (0 = first item
// in the player's inventory) into utf8Buf. Returns 1 ok, 0 timeout, -1 faulted/no player, -2 no
// inventory / D2Client missing, -3 item not in client inventory. SEH lives in the gt-queue.
extern "C" int D2Asset_ItemText(uint32_t guid, char* utf8Buf, int bufSize, int timeoutMs)
{
	if (utf8Buf && bufSize > 0) utf8Buf[0] = 0;
	uintptr_t d2client = (uintptr_t)GetModuleHandleA("D2Client.dll");
	if (!d2client || !utf8Buf) return -2;
	g_itemTextGuid = guid;
	D2Action_InstallPumpHook();
	uint64_t out = 0;
	int gs = D2Gt_Call((void*)BuildItemTextImpl, /*cc=cdecl*/0, nullptr, 0, /*ret64*/0, &out, timeoutMs);
	if (gs == 0)  return 0;
	if (gs == -1) return -1;
	if (g_itemTextRc != 1) return g_itemTextRc;
	// wide -> UTF-8; keep D2 color-code marker (0xFF 'c' <digit>) readable as "\xC3\xBFc<digit>".
	WideCharToMultiByte(CP_UTF8, 0, g_itemTextBuf, -1, utf8Buf, bufSize, nullptr, nullptr);
	utf8Buf[bufSize - 1] = 0;
	return 1;
}

// HTTP entry: find the server item by GUID in the game item hash and dump its StatList as JSON
// [{id,sub,val},...] -- the ground-truth stat values for verifying .txt edits. Read-only, all
// dereferences pointer-guarded + SEH-wrapped. Returns stat count, or -1 fault, -2 no game/guid,
// -3 item not found. UnitAny.pStats @+0x5C; StatList: pStat @+0x24, count @+0x28; Stat = 8 bytes
// (sub @0, id @2, val @4). Writes the full JSON object into buf.
extern "C" int D2Asset_DumpItemStats(uint32_t guid, char* buf, int bufSize)
{
	if (buf && bufSize > 0) buf[0] = 0;
	void* pGame = (void*)g_serverGame;
	if (!LooksLikePtr(pGame) || guid == 0 || !buf) return -2;
	void* item = nullptr;
	int n = 0;
	__try
	{
		uint32_t* buckets = (uint32_t*)((char*)pGame + kGameItemHashOff);
		void* node = (void*)buckets[guid & 0x7f];
		for (int g = 0; LooksLikePtr(node) && g < 4096; ++g)
		{
			if (*(volatile uint32_t*)((char*)node + kUnitIdOff) == guid) { item = node; break; }
			node = (void*)*(volatile uint32_t*)((char*)node + kItemHashNextOff);
		}
		if (!LooksLikePtr(item)) return -3;
		uint32_t classId = *(volatile uint32_t*)((char*)item + kUnitTxtFileNoOff);
		void*    pStats  = *(void* volatile*)((char*)item + 0x5C); // UnitAny.pStats
		int j = _snprintf_s(buf, bufSize, _TRUNCATE,
			"{\"ok\":true,\"guid\":\"0x%08x\",\"classId\":%u,\"stats\":[", (unsigned)guid, classId);
		if (LooksLikePtr(pStats))
		{
			void* pStat = *(void* volatile*)((char*)pStats + 0x24); // StatList.pStat
			int count = *(volatile uint16_t*)((char*)pStats + 0x28); // StatList.wStatCount1
			if (count > 512) count = 512;
			if (LooksLikePtr(pStat))
			{
				for (int i = 0; i < count && j < bufSize - 64; ++i)
				{
					uint16_t sub = *(volatile uint16_t*)((char*)pStat + i * 8 + 0);
					uint16_t id  = *(volatile uint16_t*)((char*)pStat + i * 8 + 2);
					int32_t  val = *(volatile int32_t*)((char*)pStat + i * 8 + 4);
					j += _snprintf_s(buf + j, bufSize - j, _TRUNCATE,
						"%s{\"id\":%u,\"sub\":%u,\"val\":%d}", n ? "," : "", id, sub, val);
					++n;
				}
			}
		}
		_snprintf_s(buf + j, bufSize - j, _TRUNCATE, "]}");
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
	return n;
}

// HTTP entry: read-only peek of up to 8 dwords at <module>+rva (diagnostic; SEH-guarded).
// Returns count read (0 on bad module / fault). Used to probe client UI-state globals live.
extern "C" int D2Asset_PeekDwords(const char* module, uint32_t rva, int count, uint32_t* out)
{
	uintptr_t base = (uintptr_t)GetModuleHandleA(module);
	if (!base) return 0;
	if (count < 1) count = 1; if (count > 8) count = 8;
	int got = 0;
	__try {
		for (int i = 0; i < count; ++i) { out[i] = *(volatile uint32_t*)(base + rva + i * 4); got++; }
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	return got;
}

// HTTP entry: write a single dword at <module>+rva (diagnostic; SEH-guarded). Returns 1 ok, 0 bad
// module, -1 faulted. Used to probe/drive client UI-state globals live.
extern "C" int D2Asset_PokeDword(const char* module, uint32_t rva, uint32_t value)
{
	uintptr_t base = (uintptr_t)GetModuleHandleA(module);
	if (!base) return 0;
	__try { *(volatile uint32_t*)(base + rva) = value; return 1; }
	__except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
}

// HTTP entry: JSON status (read-only, HTTP thread). Emits the registered path
// with backslashes escaped so the result is valid JSON.
extern "C" int D2Asset_StatusJson(char* buf, int bufSize)
{
	char esc[512]; int j = 0;
	for (int i = 0; g_lastPath[i] && j < (int)sizeof(esc) - 2; ++i)
	{
		char c = g_lastPath[i];
		if (c == '\\' || c == '"') esc[j++] = '\\';
		esc[j++] = c;
	}
	esc[j] = 0;
	return _snprintf_s(buf, bufSize, _TRUNCATE,
		"{\"ok\":true,\"registered\":%s,\"handle\":\"0x%08x\",\"path\":\"%s\",\"priority\":%u,"
		"\"stormResolved\":%s,\"frameHook\":%s,\"serverGame\":\"0x%08x\","
		"\"earlyReg\":{\"hooked\":%s,\"fired\":%s,\"result\":%d},\"createItemHook\":%s,"
		"\"spawnDbg\":{\"stage\":%d,\"game\":\"0x%08x\",\"client\":\"0x%08x\",\"player\":\"0x%08x\","
		"\"classId\":%d,\"playerCls\":%u,\"playerId\":\"0x%08x\",\"playerPath\":\"0x%08x\","
		"\"guid\":\"0x%08x\",\"mode\":%d}}",
		g_hArchive ? "true" : "false", (unsigned)(uintptr_t)g_hArchive,
		esc, g_lastPriority, ResolveStormBase() ? "true" : "false",
		g_frameTickHooked ? "true" : "false", (unsigned)(uintptr_t)g_serverGame,
		g_earlyHooked ? "true" : "false", g_earlyFired ? "true" : "false", (int)g_earlyResult,
		g_createItemUnitHooked ? "true" : "false",
		g_dbgStage, (unsigned)(uintptr_t)g_dbgGame, (unsigned)(uintptr_t)g_dbgClient,
		(unsigned)(uintptr_t)g_dbgPlayer, g_dbgClassId,
		g_dbgPlayerCls, g_dbgPlayerId, g_dbgPlayerPath, g_dbgGuid, (int)g_dbgMode);
}
