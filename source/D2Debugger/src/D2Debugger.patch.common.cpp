#include <chrono>
#include <thread>
#include <type_traits>

#include <D2Debugger.h>
#include <GAME/Game.h>


decltype(&GAME_UpdateProgress) GAME_UpdateProgress_Original = nullptr;

// The standalone debugger window (D2Debugger.imgui.d3d9.cpp) runs on its own
// thread and owns the single global ImGui context. The legacy in-game
// GAME_UpdateProgress render path below must stand down so the two don't both
// CreateContext / NewFrame / Render.
bool D2Debugger_IsStandaloneActive();

bool CheckEnvVarTrue(const char* envVarName)
{
	char* envBuffer = nullptr;
	size_t bufferSize = 0;

	if (0 == _dupenv_s(&envBuffer, &bufferSize, envVarName))
	{
		const bool isSet = envBuffer && *envBuffer == '1';
		free(envBuffer);
		return isSet;
	}
	return false;
}

bool IsDebuggerEnabled()
{
	static bool bEnabledFromCommandLine = strstr(GetCommandLineA(), "-debug") || CheckEnvVarTrue("D2_DEBUGGER");
	if (bEnabledFromCommandLine)
	{
		return true;
	}
	// TODO: support toggling window based on chat command ?
	return false;
}

void __fastcall GAME_UpdateProgress_WithDebugger(D2GameStrc* pGame)
{
	// Defer entirely to the standalone debugger window (it owns the ImGui
	// context on its own thread). Guard BEFORE the static D2DebuggerInit() so this
	// path never creates a second window / device / context.
	if (IsDebuggerEnabled() && !D2Debugger_IsStandaloneActive())
	{
		static bool bDebuggerAvailable = D2DebuggerInit() == 0;
		if (bDebuggerAvailable && !D2Debugger_IsStandaloneActive())
		{
			static bool bFreezeGame = false;
			do {
				D2DebuggerNewFrame();
				if (!bFreezeGame)
				{
					GAME_UpdateProgress_Original(pGame);
				}
				bFreezeGame = D2DebugGame(pGame);
				D2DebugLiveDispatch();
				D2DebuggerEndFrame(bFreezeGame/*vsync ON if frozen*/);
			} while (bFreezeGame);
			return;
		}
	}
	GAME_UpdateProgress_Original(pGame);
}
static_assert(std::is_same_v<decltype(&GAME_UpdateProgress), decltype(&GAME_UpdateProgress_WithDebugger)>, "GAME_UpdateProgress_WithDebugger has a different type than previously known.");
