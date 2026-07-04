#include <Windows.h>
#include <cstdio>

#include <D2Dll.h>

// Custom implementations for Display functions (to replace MessageBox popups)
// These must be defined BEFORE the stub macro creates them
FOG_DLL_DECL void __cdecl FOG_DisplayAssert(const char* szMsg, const char* szFile, int nLine)
{
    fprintf(stderr, "[FOG ASSERT] %s at %s:%d\n", szMsg, szFile, nLine);
}

FOG_DLL_DECL void __cdecl FOG_DisplayHalt(const char* szMsg, const char* szFile, int nLine)
{
    fprintf(stderr, "[FOG HALT] %s at %s:%d\n", szMsg, szFile, nLine);
}

FOG_DLL_DECL void __cdecl FOG_DisplayWarning(const char* szMsg, const char* szFile, int nLine)
{
    fprintf(stderr, "[FOG WARNING] %s at %s:%d\n", szMsg, szFile, nLine);
}

FOG_DLL_DECL void __cdecl FOG_DisplayError(int nCategory, const char* szMsg, const char* szFile, int nLine)
{
    fprintf(stderr, "[FOG ERROR] Category %d: %s at %s:%d\n", nCategory, szMsg, szFile, nLine);
}

// Skip generating stubs for Display functions since we defined them above
#define FOG_SKIP_DISPLAY_STUBS

#undef D2FUNC_DLL
#define D2FUNC_DLL D2FUNC_DLL_STUB

#include <Fog.h>
