#pragma once

D2DEBUGGER_DLL_DECL
int D2DebuggerInit();

D2DEBUGGER_DLL_DECL
bool D2DebuggerNewFrame();

D2DEBUGGER_DLL_DECL
void D2DebuggerEndFrame(bool VSyncNextFrame);

D2DEBUGGER_DLL_DECL
void D2DebuggerDestroy();

// return true if we should freeze the game update
D2DEBUGGER_DLL_DECL
bool D2DebugGame(struct Game* pGame);

// The unified function browser (D2Debugger.LiveDispatch.cpp): renders the
// dynamic-profiler function tree (DYNAMIC_PROFILER_PLAN.md) and overlays
// live-dispatch control (LIVE_DISPATCH_FRAMEWORK_PLAN.md Phase 4) on the subset
// that has dispatchers -- one panel for everything.
D2DEBUGGER_DLL_DECL
void D2DebugLiveDispatch();
