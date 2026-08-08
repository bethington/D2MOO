/* Link-only shim. The reconstructed launcher's entry (0x408540) IS WinMain;
 * our Main.c names it GameEntryPoint to keep the byte-match manifest stable, so
 * this provides the _WinMain@16 the CRT startup expects and forwards to it.
 * Not part of the reconstruction -- a linking artifact, one tail call. */
#include <windows.h>
int __stdcall GameEntryPoint(HINSTANCE, HINSTANCE, char *, int);
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    return GameEntryPoint(hInst, hPrev, (char *)lpCmd, nShow);
}
