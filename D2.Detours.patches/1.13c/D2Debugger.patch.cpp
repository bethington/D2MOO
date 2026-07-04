#include <DetoursPatch.h>
#include <chrono>
#include <thread>

#include <D2Debugger.h>
#include <D2Debugger.patch.common.h>
#include <D2Game.h>

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmicrosoft-cast"
#endif

extern "C" {

static const int D2GameImageBase = 0x6FC20000;
static const int D2ClientImageBase = 0x6FAB0000; // TODO: Verify D2Client base address for 1.13c

static ExtraPatchAction D2GameExtraPatchActions[] = {
#ifdef D2_VERSION_113C
	// GAME_UpdateGamesProgress is at ordinal 10004, RVA 0x54400 (from dumpbin /exports)
	// Full address: 0x6FC20000 + 0x54400 = 0x6FC74400, offset = 0x54400
	{ 0x54400, &GAME_UpdateProgress_WithDebugger, PatchAction::FunctionReplaceOriginalByPatch, &GAME_UpdateProgress_Original},
#endif
    { 0, 0, PatchAction::Ignore}, // Here because we need at least one element in the array
};

static ExtraPatchAction D2ClientExtraPatchActions[] = {
	{ 0, 0, PatchAction::Ignore}, // Here because we need at least one element in the array
};

ExtraPatchAction* __cdecl D2ClientGetExtraPatchAction(int index)
{
	return &D2ClientExtraPatchActions[index];
}

__declspec(dllexport)
uint32_t __cdecl DllPreLoadHook(HookContext* ctx, const wchar_t* dllName)
{
	if (wcsicmp(dllName, L"D2Game.dll") == 0)
	{
		for (auto& p : D2GameExtraPatchActions)
		{
			ctx->ApplyPatchAction(ctx, p.originalDllOffset, p.patchData, p.action, (void**)p.detouredPatchedFunctionPointerStorageAddress);
		}
	}
	else if (wcsicmp(dllName, L"D2Client.dll") == 0)
	{
		for (auto& p : D2ClientExtraPatchActions)
		{
			ctx->ApplyPatchAction(ctx, p.originalDllOffset, p.patchData, p.action, (void**)p.detouredPatchedFunctionPointerStorageAddress);
		}
	}
	else
	{
		__debugbreak(); // Insert other dlls here if you added them in the .rc.
		return {};
	}
	return 0;
}

} // extern "C"

#include <type_traits>
static_assert(std::is_same<decltype(DllPreLoadHook)*, DllPreLoadHookType>::value, "Ensure calling convention doesn't change");
