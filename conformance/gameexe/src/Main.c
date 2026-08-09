/* Game.exe launcher -- the one translation unit at 0x407550..0x4085af.
 *
 * All 22 authored launcher functions live in this single object in the
 * original binary (contiguous in .text, one Main.c). Reconstructing them
 * TOGETHER is what lets MSVC /O2 reproduce the private register conventions
 * it chose for the `static` helpers -- see ../README.md. Verify with
 * verify_tu.py: compile this TU, byte-compare each function to the original.
 *
 * Names match ../launcher_manifest.json exactly so the scoreboard auto-maps.
 * Function ORDER here follows the original's .text order, which is MSVC's
 * default source order -- it does not affect per-function byte-match (call
 * displacements are relocations, masked) but keeping it aligned makes a full
 * link closer later.
 *
 * STATUS: 9/24 byte-exact. Reconstruct bottom-up (a static callee's private
 * convention only settles once its real body exists), so the order is
 * leaves -> mid-layer (registry/service/render) -> parsers -> GameStart ->
 * GameInit -> WinMain. The interconnected-core thesis is now PROVEN, not just
 * measured: writing a faithful GameInit body (even with an opaque Config frame
 * and stubbed callees) pinned its private convention -- argc in EAX, argv in
 * ECX -- and both its callers, D2ServerServiceMain and GameEntryPoint/WinMain,
 * snapped from DIFF/unwritten to MATCH in the same compile. GameInit and
 * GameStart themselves are the deep hard tail (huge Config frame, ~30 external
 * signatures, a video-registry jump table) and stay DIFF/CONF_TRACE until the
 * full Config layout lands. Functions not yet written show "--"; ones in
 * flight show LARGER/DIFF with the cause recorded at their definition.
 */

#include <windows.h>
#include <string.h>
#pragma intrinsic(strcmp, strcpy, strlen)

#define MAX_REG_KEY 1024

/* --- globals the launcher owns (resolved by address at link/patch time) --- */
void *ghCurrentProcess;
int   gnCmdShow;
BOOL  gbServiceRunning;   /* 0x40cf34 -- set around the service main body */

/* Windows service state (Game.exe can run headless as a D2 server). The
 * disassembly puts dwCurrentState at gD2ServerServiceStatus+4, which is where
 * SERVICE_STATUS.dwCurrentState sits -- so the real struct is the right one. */
SERVICE_STATUS        gD2ServerServiceStatus;
SERVICE_STATUS_HANDLE ghD2ServerServiceStatus;
BOOL                  gbD2ServerStopEvent;

/* 0x0040a3f9 -- the shared empty string. GameInit points lpArgvCmd here when
 * there is no command line, and SaveCmdLine dereferences it. */
char lpZero = 0;

/* Command-argument table. 57 entries -> loop bound 57*0x3c = 0xd5c, exactly
 * what DATATBLS_FindConfigOptionIndex tests against. The three char[16]
 * fields plus three dwords give the 0x3c stride the disassembly shows. Only
 * the LAYOUT and the szCommand strings matter for byte-matching the
 * functions; dwType/dwIndex/dwDefault are data, not code, so they are left 0
 * for now and will be filled when the parsers that read them are written. */
typedef struct CmdArg {
    char  szSection[16];
    char  szKey[16];
    char  szCommand[16];
    DWORD dwType;
    DWORD dwIndex;
    DWORD dwDefault;
} CmdArg;

/* dwType/dwIndex/dwDefault read verbatim from the original binary's
 * gaCmdArguments at 0x0040BC08 -- the authoritative 1.13c Config offsets, so
 * GAME_LoadConfigFromIniFile writes each option to the exact field the game's
 * DLLs read. */
#define CMD_BOOLEAN 0
#define CMD_INTEGER 1
#define CMD_STRING  2
CmdArg gaCmdArguments[57] = {
    { "VIDEO", "WINDOW", "w", CMD_BOOLEAN, 0x4, 0 },
    { "VIDEO", "WINDOW", "window", CMD_BOOLEAN, 0x4, 0 },
    { "VIDEO", "WINDOW", "windowed", CMD_BOOLEAN, 0x4, 0 },
    { "VIDEO", "ASPECT", "nofixaspect", CMD_BOOLEAN, 0x5, 0 },
    { "VIDEO", "3DFX", "3dfx", CMD_BOOLEAN, 0x6, 0 },
    { "VIDEO", "OPENGL", "opengl", CMD_BOOLEAN, 0x7, 0 },
    { "VIDEO", "D3D", "d3d", CMD_BOOLEAN, 0x9, 0 },
    { "VIDEO", "RAVE", "rave", CMD_BOOLEAN, 0x8, 0 },
    { "VIDEO", "PERSPECTIVE", "per", CMD_BOOLEAN, 0xa, 0 },
    { "VIDEO", "QUALITY", "lq", CMD_BOOLEAN, 0xb, 0 },
    { "VIDEO", "GAMMA", "gamma", CMD_INTEGER, 0xc, 0 },
    { "VIDEO", "VSYNC", "vsync", CMD_BOOLEAN, 0x10, 0 },
    { "VIDEO", "FRAMERATE", "fr", CMD_INTEGER, 0x11, 0 },
    { "NETWORK", "SERVERIP", "s", CMD_STRING, 0x33, 0 },
    { "NETWORK", "GAMETYPE", "gametype", CMD_INTEGER, 0x15, 0 },
    { "NETWORK", "ARENA", "arena", CMD_INTEGER, 0x203, 0 },
    { "NETWORK", "JOINID", "joinid", CMD_INTEGER, 0x19, 0 },
    { "NETWORK", "GAMENAME", "gamename", CMD_STRING, 0x1b, 0 },
    { "NETWORK", "BATTLENETIP", "bn", CMD_STRING, 0x4b, 0 },
    { "NETWORK", "MCPIP", "mcpip", CMD_STRING, 0x63, 0 },
    { "CHARACTER", "AMAZON", "ama", CMD_BOOLEAN, 0x81, 1 },
    { "CHARACTER", "PALADIN", "pal", CMD_BOOLEAN, 0x82, 0 },
    { "CHARACTER", "SORCERESS", "sor", CMD_BOOLEAN, 0x83, 0 },
    { "CHARACTER", "NECROMANCER", "nec", CMD_BOOLEAN, 0x84, 0 },
    { "CHARACTER", "BARBARIAN", "bar", CMD_BOOLEAN, 0x85, 0 },
    { "CHARACTER", "INVINCIBLE", "i", CMD_BOOLEAN, 0x88, 0 },
    { "CHARACTER", "NAME", "name", CMD_STRING, 0xb9, 0 },
    { "CHARACTER", "REALM", "realm", CMD_STRING, 0xd1, 0 },
    { "CHARACTER", "CTEMP", "ctemp", CMD_INTEGER, 0x1e9, 0 },
    { "MONSTER", "NOMONSTERS", "nm", CMD_BOOLEAN, 0x1ed, 0 },
    { "MONSTER", "MONSTERCLASS", "m", CMD_INTEGER, 0x1ee, 0 },
    { "MONSTER", "MONSTERINFO", "minfo", CMD_BOOLEAN, 0x1f2, 0 },
    { "MONSTER", "MONSTERDEBUG", "md", CMD_INTEGER, 0x1f3, 0 },
    { "ITEM", "RARE", "rare", CMD_BOOLEAN, 0x1f7, 0 },
    { "ITEM", "UNIQUE", "unique", CMD_BOOLEAN, 0x1f8, 0 },
    { "INTERFACE", "ACT", "act", CMD_INTEGER, 0x1fb, 1 },
    { "DEBUG", "LOG", "log", CMD_BOOLEAN, 0x212, 0 },
    { "DEBUG", "MSGLOG", "msglog", CMD_BOOLEAN, 0x213, 0 },
    { "DEBUG", "SAFEMODE", "safe", CMD_BOOLEAN, 0x214, 0 },
    { "DEBUG", "NOSAVE", "nosave", CMD_BOOLEAN, 0x215, 0 },
    { "DEBUG", "SEED", "seed", CMD_INTEGER, 0x216, 0 },
    { "NETWORK", "NOPK", "nopk", CMD_BOOLEAN, 0x7f, 0 },
    { "DEBUG", "CHEATS", "cheats", CMD_BOOLEAN, 0x21a, 0 },
    { "DEBUG", "TEEN", "teen", CMD_BOOLEAN, 0x21b, 0 },
    { "DEBUG", "NOSOUND", "ns", CMD_BOOLEAN, 0x21c, 0 },
    { "FILEIO", "NOPREDLOAD", "npl", CMD_BOOLEAN, 0x1ff, 0 },
    { "FILEIO", "DIRECT", "direct", CMD_BOOLEAN, 0x200, 0 },
    { "FILEIO", "LOWEND", "lem", CMD_BOOLEAN, 0x201, 0 },
    { "DEBUG", "QuEsTs", "questall", CMD_BOOLEAN, 0x21d, 0 },
    { "NETWORK", "COMINT", "comint", CMD_INTEGER, 0x221, 0 },
    { "NETWORK", "SKIPTOBNET", "skiptobnet", CMD_BOOLEAN, 0x359, 0 },
    { "NETWORK", "OPENC", "openc", CMD_BOOLEAN, 0x80, 0 },
    { "FILEIO", "NOCOMPRESS", "nocompress", CMD_BOOLEAN, 0x202, 0 },
    { "TXT", "TXT", "txt", CMD_BOOLEAN, 0x211, 0 },
    { "BUILD", "BUILD", "build", CMD_BOOLEAN, 0x21f, 0 },
    { "DEBUG", "NOSOUND", "nosound", CMD_BOOLEAN, 0x21c, 0 },
    { "DEBUG", "SOUNDBKG", "sndbkg", CMD_BOOLEAN, 0x220, 0 },
};

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

/* 0x004078b0 -- the separator predicate. `static`, so /O2 gives it a private
 * convention (arg in ECX) and inlines it into TrimWhitespaceDelimiters while
 * keeping this out-of-line copy for its other callers. The 4-element char
 * array (not a string literal, which would carry a NUL and scan 5) is the
 * shape the disassembly shows. */
static int IsWhitespaceOrColon(char c)
{
    char sep[4];
    int k;
    sep[0] = ' '; sep[1] = '\t'; sep[2] = '\n'; sep[3] = ':';
    for (k = 0; k < 4; k++) if (sep[k] == c) return 1;
    return 0;
}

/* 0x00407960 -- take the current directory and walk it down to the install
 * root by keeping everything up to (and including) the second backslash.
 * `static`, buffer in ESI. Counts backslashes (capped at 2), then advances
 * past that many and truncates. */
static int GetInstallRootDirectory(char *buffer)
{
    int len, i, n;
    if (!GetCurrentDirectoryA(0x100, buffer))
        return 0;
    len = (int)strlen(buffer);
    n = 0;
    for (i = 0; i < len && i < 0x100; i++)
        if (buffer[i] == '\\') n++;
    if (n > 2) n = 2;
    for (i = 0; i < len && n; i++)
        if (buffer[i] == '\\') n--;
    buffer[i] = 0;
    return 1;
}

/* 0x00407ec0 -- GetProcAddress wrapper. `static`, hModule in ECX, name in
 * EAX, out-pointer on the stack. Stores the resolved address and returns 1,
 * or returns 0 without touching *out. */
static int ResolveProcAddress(HMODULE hMod, const char *name, void **out)
{
    void *p = GetProcAddress(hMod, name);
    if (!p)
        return 0;
    *out = p;
    return 1;
}

/* 0x004078e0 -- 92 bytes, EXACT (proven). Loop bound must be unsigned. */
int __stdcall DATATBLS_FindConfigOptionIndex(const char *s)
{
    unsigned i;
    for (i = 0; i < ARRAY_SIZE(gaCmdArguments); i++)
        if (0 == strcmp(gaCmdArguments[i].szCommand, s))
            return i;
    return -1;
}

/* 0x00407940 -- stoLower. `static`, arg in ECX. Uppercase -> lowercase in
 * place; the do/while tests s[1] so the terminator is written too. */
static void ConvertStringToLowercase(char *s)
{
    if (*s) {
        do {
            if (*s >= 'A' && *s <= 'Z')
                *s += ('a' - 'A');
        } while ((s++)[1]);
    }
}

/* 0x004079d0 -- 170 bytes, EXACT (proven). i++ before the store; the two
 * scans read different buffers. */
void __stdcall TrimWhitespaceDelimiters(char *s)
{
    char szBuf[24];
    int len, i, j;

    strcpy(szBuf, s);
    len = (int)strlen(s);

    for (i = 0; i < len; i++)
        if (!IsWhitespaceOrColon(s[i])) break;

    j = 0;
    while (i < len) {
        char c = szBuf[i];
        if (IsWhitespaceOrColon(c)) break;
        i++;
        s[j++] = c;
    }
    s[j] = 0;
}

/* 0x00408110 -- AllowExpansion. `return TRUE` in 6 bytes: mov eax,1 / ret. */
BOOL __stdcall ValidateEntityOperationAlwaysTrue(void)
{
    return 1;
}

/* 0x00407db0 -- the service control callback (D2ServerServiceHandlerProc).
 * A standard WINAPI callback: Windows calls it, so no private convention.
 * STOP and SHUTDOWN share a handler; INTERROGATE just re-reports; anything
 * else is ignored. The compiler tests the codes in value order (1, 4, 5)
 * via dec/sub/dec, which is why STOP(1) and SHUTDOWN(5) land on one block. */
VOID WINAPI ServiceControlHandler(DWORD dwCtrlCode)
{
    switch (dwCtrlCode) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        gD2ServerServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(ghD2ServerServiceStatus, &gD2ServerServiceStatus);
        gbD2ServerStopEvent = TRUE;
        return;
    case SERVICE_CONTROL_INTERROGATE:
        SetServiceStatus(ghD2ServerServiceStatus, &gD2ServerServiceStatus);
        return;
    default:
        break;
    }
}

/* Module-type names, indexed 0..5 (D2_MODULES_COUNT). Referenced by address
 * (a relocation), so only the count and the symbol matter for byte-match. */
#define D2_MODULES_COUNT 6
#define MODULE_CLIENT    1
const char *lpszModuleType[D2_MODULES_COUNT] = {
    "modstate0", "client", "server", "multiplayer", "launcher", "expand"
};

/* Fog memory + string imports, in the fastcall/cdecl shapes the call sites
 * show: FOG_Alloc/FOG_Free take size/ptr in ECX and the file tag in EDX
 * (fastcall) with line and a trailing 0 on the stack; SStrCopy is cdecl. */
void *__fastcall FOG_Alloc(unsigned size, const char *file, unsigned line, void *z);
void  __fastcall FOG_Free(void *p, const char *file, unsigned line, void *z);
void __stdcall SStrCopy(char *dst, const char *src, unsigned len);
char *__cdecl strtok(char *, const char *);
int   __cdecl strncmp(const char *, const char *, unsigned);
#pragma intrinsic(strncmp)

/* 0x00407e00 -- scan the command line for a module keyword (server, launcher,
 * ...) and record the last match in *pnChosenModule, skipping CLIENT. Takes
 * argv in EAX; the out-pointer is on the stack and is pre-initialised by the
 * caller (this only overwrites on a hit). Dupes argv, strtok's on '-'.
 *
 * STATUS: body byte-EXACT (187 vs 189). The only difference is the tail --
 * ours `RET`, original `RET 4`. The original receives pnChosenModule as a
 * STACK argument (callee-cleaned), while /O2, free to choose for a `static`
 * function called only from the keepalive, passed it in a register and
 * spilled it to the identical [esp+0x14] slot. Both then generate the same
 * code; only the entry convention (and thus the RET immediate) differs. This
 * is the caller-pins-the-convention case the plan anticipated: it snaps to
 * MATCH once GameInit calls it pushing &nChosenModule on the stack. Also
 * settled here: SStrCopy is __stdcall (the single `add esp,8` after the
 * strtok proves the copy cleaned its own three args). */
static int GAME_ParseModStateFromCommandLine(const char *argv, int *pnChosenModule)
{
    unsigned argvLen = strlen(argv) + 1;
    char *lpArgvDupe = (char *)FOG_Alloc(argvLen, "Game.cpp", 0x1d8, 0);
    char *pCurrentParam;
    int i;

    SStrCopy(lpArgvDupe, argv, argvLen);
    for (pCurrentParam = strtok(lpArgvDupe, "-"); pCurrentParam;
         pCurrentParam = strtok(0, "-")) {
        for (i = 0; i < D2_MODULES_COUNT; i++) {
            if (0 == strncmp(pCurrentParam, lpszModuleType[i],
                             strlen(lpszModuleType[i])) && i != MODULE_CLIENT)
                *pnChosenModule = i;
        }
    }
    FOG_Free(lpArgvDupe, "Game.cpp", 0x1fe, 0);
    return 1;
}

int __cdecl atoi(const char *);
#pragma intrinsic(atoi)

/* 0x00407b70 -- parse one "-option[value]" token. `static`, cmd in EDX with
 * two output buffers on the stack. Extracts the option name up to '-'/NUL,
 * lowercases it, then shortens it one char at a time looking it up in the
 * command table (so "windowed" falls back to "window" ... to "w"); the
 * leftover tail is the value, passed through TrimWhitespaceDelimiters.
 * Returns the table index or -1. Calls only already-matched helpers. */
static int ParseCommandLineOption(const char *cmd, char *szName, char *szValue)
{
    char szCommand[48];
    int nCommandIndex, len, i;
    unsigned j, off;

    len = (int)strlen(cmd);
    i = 0;
    while (i < len && cmd[i] && cmd[i] != '-') {
        szCommand[i] = cmd[i];
        i++;
    }
    szCommand[i] = 0;
    szCommand[i + 1] = 0;

    strcpy(szName, szCommand);
    ConvertStringToLowercase(szName);

    nCommandIndex = -1;
    for (j = strlen(szName); j != 0; j--) {
        szName[j] = 0;
        nCommandIndex = DATATBLS_FindConfigOptionIndex(szName);
        if (nCommandIndex != -1)
            break;
    }

    off = strlen(szName);
    for (i = 0; szCommand[off + i]; i++)
        szValue[i] = szCommand[off + i];
    szValue[i] = 0;
    TrimWhitespaceDelimiters(szValue);
    return nCommandIndex;
}

/* 0x00407c90 -- walk the command line; for each "-opt" run
 * ParseCommandLineOption and apply the result to the Config by the option's
 * dwIndex/dwType (BOOLEAN=0 -> set 1, INTEGER=1 -> atoi, STRING=2 -> strcpy).
 * `static`, two stack args (argv, pCfg). dwType is read as a byte -- the
 * original casts (char)dwType. */
static int __stdcall ParseAllCommandLineOptions(char *pCfg, const char *argv)
{
    char szValue[24];
    char szName[24];
    int len, i, idx;

    len = (int)strlen(argv);
    for (i = 0; i < len; i++) {
        if (argv[i] != '-')
            continue;
        i++;
        if (i >= len)
            break;
        idx = ParseCommandLineOption(&argv[i], szName, szValue);
        if (idx == -1)
            continue;
        {
            char *pMember = pCfg + gaCmdArguments[idx].dwIndex;
            switch ((unsigned char)gaCmdArguments[idx].dwType) {  /* movzx, not movsx */
            case 0: *(unsigned char *)pMember = 1; break;
            case 1: *(int *)pMember = atoi(szValue); break;
            case 2: strcpy(pMember, szValue); break;
            }
        }
    }
    return 0;
}

char REG_PATH_BETA[] = "SOFTWARE\\Blizzard Entertainment\\Diablo II Beta";
char REG_PATH_HOME[] = "SOFTWARE\\Blizzard Entertainment\\Diablo II";
void __fastcall FOG_GetInstallPath(char *buf, DWORD len);   /* Fog.dll import */

/* 0x00407ee0 -- migrate the old beta registry keys to the release location,
 * then resolve the install path and (in service mode) chdir there. In PD2's
 * 1.13c this is one function; D2MOO has the pieces as separate statements in
 * GameInit. Self-contained: every call is a Win32 registry API or the Fog
 * import, all relocations. On a missing beta key the whole migration is
 * skipped and control joins the install-path block, which runs either way.
 *
 * STATUS: in flight, 334 vs 279 (+55). Structure, prologue and the `and
 * esp,-8` alignment all reproduce; the gap is REGISTER PRESSURE. The original
 * repurposes the frame-pointer EBP as a general register (a second `push ebp`
 * after `mov ebp,esp`) so it can hoist BOTH IAT pointers -- RegEnumValueA in
 * EDI and RegSetValueExA in EBP -- out of the loop and call them as `call
 * edi` / `call ebp` (2 bytes each). Ours frees only EDI, so RegSetValueExA is
 * reloaded and called as `call [import]` (6 bytes) every iteration. Forcing
 * MSVC to spill the frame pointer into a GPR is the open problem -- likely a
 * loop-structure or SP1-vs-RTM codegen question, to be chased like TryStart's
 * two iterations were. */
void GAME_MigrateBetaRegistryKeys(void)
{
    HKEY  hBeta, hHome;
    DWORD i, dwType, cchValue, cbData;
    char  szValue[MAX_PATH];
    BYTE  bData[MAX_REG_KEY];

    if (RegOpenKeyA(HKEY_LOCAL_MACHINE, REG_PATH_BETA, &hBeta) == ERROR_SUCCESS) {
        RegCreateKeyA(HKEY_LOCAL_MACHINE, REG_PATH_HOME, &hHome);
        for (i = 0; ; i++) {
            cchValue = MAX_PATH;
            cbData   = MAX_REG_KEY;
            if (RegEnumValueA(hBeta, i, szValue, &cchValue, 0,
                              &dwType, bData, &cbData) != ERROR_SUCCESS)
                break;
            if (RegSetValueExA(hHome, szValue, 0, dwType, bData, cbData)
                    != ERROR_SUCCESS)
                break;
        }
        RegCloseKey(hHome);
        RegCloseKey(hBeta);
        RegDeleteKeyA(HKEY_LOCAL_MACHINE, REG_PATH_BETA);
    }

    {
        char szPath[MAX_PATH] = {0};
        FOG_GetInstallPath(szPath, MAX_PATH);
        if (gbServiceRunning)
            SetCurrentDirectoryA(szPath);
    }
}

/* One global string, referenced by both OpenServiceA and the dispatch table
 * at the same address in the original. */
char SVC_NAME[] = "Diablo II Server";

/* ===================================================================
 *  Deep core -- GameStart and its module loop. GameStart is CONF_TRACE, not
 *  byte-exact: ~35 external calls and register allocation over 683 bytes put
 *  it past the stopping rule. So this is a SEMANTIC reconstruction (the D2MOO
 *  logic, adapted to 1.13c) whose equivalence is proven by the behavioural
 *  gate, and a Config laid out to the exact edi-offsets the disassembly reads.
 * =================================================================== */

/* External DLL entry points, in the conventions D2MOO's headers declare. */
void __fastcall FOG_SetLogPrefix(const char *pfx);
void __fastcall FOG_InitErrorMgr(const char *name, void *cb, const char *ver, int flag);
int  __cdecl    SStrPrintf(char *dst, int cch, const char *fmt, ...);
int  __fastcall FOG_MPQSetConfig(int dwDirectFlags, int bSeekOpt);
void __fastcall FOG_AsyncDataInitialize(BOOL bAsync);
void __fastcall FOG_10082_Noop(void);
void __fastcall FOG_10218(void);    /* pre-archive Fog init step (result unused) */
int  __fastcall FOG_IsExpansion(void);
void __fastcall FOG_AsyncDataDestroy(void);
void __cdecl    FOG_DestroyMemoryPoolSystem(void *pPool);
BOOL __stdcall  SRegLoadValue(const char *key, const char *val, unsigned flags, DWORD *out);
BOOL __stdcall  SRegSaveValue(const char *key, const char *val, BYTE flags, DWORD v);
BOOL __stdcall  SRegSaveString(const char *key, const char *val, BYTE flags, const char *s);
BOOL __stdcall  SRegLoadString(const char *key, const char *val, unsigned flags, char *buf, unsigned cb);
int  __cdecl    sprintf(char *dst, const char *fmt, ...);
char szSRegReadBuf[MAX_REG_KEY];   /* shared read scratch for the cmdline regs */
BOOL __fastcall ARCHIVE_LoadArchives(void);
BOOL __fastcall ARCHIVE_LoadExpansionArchives(void *pf1, void *pf2, HANDLE hFile, void *pCfg);
void __fastcall ARCHIVE_FreeArchives(void);
BOOL __stdcall  ARCHIVE_ShowInsertPlayDiscMessage(void);
BOOL __stdcall  ARCHIVE_ShowInsertExpansionDiscMessage(void);
void __stdcall  D2GFX_SetPerspective(int b);
int  __stdcall  D2GFX_ToggleLowQuality(void);
void __stdcall  D2GFX_SetFixedAspectRatio(void);
int  __stdcall  D2GFX_SetGamma(unsigned g);
void __stdcall  D2GFX_EnableVSync(void);
int  __stdcall  D2GFX_Release(void);
BOOL __stdcall  D2Win_CreateWindow(HINSTANCE h, int nRenderMode, BOOL bWindowed, BOOL bCompress);
BOOL __stdcall  D2Win_InitializeSpriteCache(BOOL bWindowed, int nRes);
int  __stdcall  D2Win_CloseSpriteCache(void);
HWND __stdcall  WINDOW_GetWindow(void);
void __fastcall D2SOUND_OpenSoundSystem(BOOL bExp, BOOL bBkg);
void __fastcall D2SOUND_CloseSoundSystem(void);
void __cdecl    D2MCPClientCloseMCP(void);

/* Config, laid out to the disassembly's edi-offsets (bDirect@0x200, etc.):
 * pack(1) + explicit pads so every field is where 1.13c reads it. Size is 969
 * (0xF2 dwords + 1), the memset width GameInit and the wrapper both emit. */
#pragma pack(push, 1)
typedef struct Config {
    BOOL  bIsExpansion;            /* 0x000 */
    BYTE  bWindow;                 /* 0x004 */
    BYTE  bNoFixedAspect;          /* 0x005 */
    BYTE  b3DFX;                   /* 0x006 */
    BYTE  bOpenGL;                 /* 0x007 */
    BYTE  bRave;                   /* 0x008 */
    BYTE  bD3D;                    /* 0x009 */
    BYTE  bPerspective;            /* 0x00A */
    BYTE  bQuality;                /* 0x00B */
    DWORD dwGamma;                 /* 0x00C */
    BYTE  bVSync;                  /* 0x010 */
    BYTE  _pad011[0x200 - 0x011];
    BYTE  bDirect;                 /* 0x200 */
    BYTE  bLowEnd;                 /* 0x201 */
    BYTE  bNoCompress;             /* 0x202 */
    BYTE  _pad203[0x20D - 0x203];
    /* 0x20D: a validation callback the launcher installs and D2Win CALLS.
     * ARCHIVE_LoadExpansionArchives (D2Win @10005) does, at its +0x32:
     *     mov  eax, [esp+0x10]        ; the Config we passed
     *     test eax, eax
     *     je   <skip>                 ; only guards a NULL *Config*...
     *     call dword ptr [eax+0x20D]  ; ...NOT a NULL callback
     * so leaving this zero is an immediate call to address 0. That was the
     * post-handoff crash: `eip=00000000`, returning into D2Win!Ordinal10005+0x38.
     * Read live off the original (breakpoint on @10005, dump [pCfg+0x20D]): it
     * holds 0x00408110, whose whole body is `mov eax,1 / ret` -- i.e.
     * ValidateEntityOperationAlwaysTrue. GameInit installs it right after
     * ParseAllCommandLineOptions with
     *     mov dword ptr [esp+0x265], offset 0x408110
     * and its Config local sits at esp+0x58, so 0x265-0x58 = 0x20D. */
    BOOL (__stdcall *pfnValidate)(void);   /* 0x20D */
    BYTE  _pad211[0x21C - 0x211];
    BYTE  bNoSound;                /* 0x21C (gaCmdArguments ns/nosound) */
    BYTE  _pad21D[0x220 - 0x21D];
    BYTE  bSoundBackground;        /* 0x220 (sndbkg) */
    void *pComInterface;           /* 0x221 (comint; unaligned, pack1) */
    BYTE  _pad225[969 - 0x225];
} Config;
#pragma pack(pop)

/* Module loop state. */
#define MODULE_NONE     0
#define MODULE_SERVER   2
#define MODULE_LAUNCHER 4
int     geModState = MODULE_NONE;
void   *gpCurrentModuleInterface = 0;
HMODULE ghModKeyhook = 0;
BOOL    gbUseKeyhook = 0;
const char *lpszD2Module[D2_MODULES_COUNT] = {
    "none.dll", "D2Client.dll", "D2Server.dll", "D2Multi.dll",
    "D2Launch.dll", "D2EClient.dll"
};

/* 0x00407550 -- load the selected module DLL, fetch its QueryInterface, call it
 * for the module's interface, then invoke that interface's first slot. Returns
 * the next module to load -- the module drives the transition.
 *
 * Reconstructed from the original's disassembly, which pins two details that
 * are NOT guessable and were both wrong on the first pass:
 *   1. `QueryInterface` is a bare __cdecl thunk (`mov eax,<static struct>; ret`)
 *      -- NOT __fastcall. It takes no arguments and returns the interface.
 *   2. The original then does `mov ecx,[esp+0x68]` + `call dword ptr [eax]`:
 *      it calls the function POINTED TO BY the interface's first slot, passing
 *      the Config in ECX (__fastcall). Reading the slot as the function itself
 *      (rather than a pointer to it) jumps into the middle of D2Launch's data
 *      and null-derefs deep inside D2Lang's locale init -- the crash this
 *      replaces. D2Launch's own slot-0 body confirms the ECX convention: its
 *      first instruction is `mov esi,ecx`.
 * The module name comes from the table at 0x40c964 indexed by module id, which
 * lpszD2Module reproduces. */
typedef int (__fastcall *ModuleInitPointer)(Config *);
static int LoadCurrentlySelectedModule(Config *pCfg)
{
    if (geModState >= MODULE_NONE && geModState < D2_MODULES_COUNT) {
        HMODULE hModule = LoadLibraryA(lpszD2Module[geModState]);
        if (hModule) {
            FARPROC pQI = GetProcAddress(hModule, "QueryInterface");
            if (pQI) {
                ModuleInitPointer *pInterface =
                    (ModuleInitPointer *)((void *(__cdecl *)(void))pQI)();
                gpCurrentModuleInterface = (void *)pInterface;
                if (pInterface && *pInterface)
                    return (*pInterface)(pCfg);
                return MODULE_NONE;
            }
            GetLastError();
        }
    }
    return MODULE_NONE;
}

/* 0x00407600 -- GameStart. Boot subsystems, load archives, make the window,
 * open sound, run the module loop until a module returns MODULE_NONE, tear
 * down. CONF_TRACE (semantic; behavioural gate proves equivalence). */
static int GAME_RunMainLoop(void *hInstance, Config *pCfg, int nModType)
{
    BOOL bSoundStarted = FALSE;
    BOOL bGfxStarted = FALSE;
    int  dwRenderMode;

    geModState = nModType;

    /* No MODULE_LAUNCHER shortcut here. An earlier pass skipped straight to the
     * module loop for the launcher, on the reading that the baseline trace
     * showed "zero Fog/D2Win calls before the handoff" -- that reading was
     * WRONG. The harness only hooks Game.exe's *named* Win32 imports; every D2
     * DLL export is imported BY ORDINAL and is invisible to it, so the absence
     * of Fog/D2Win events is a property of the instrument, not of the run. The
     * positive evidence that GameStart really does run first: the baseline's
     * FindWindowA (seq 246, immediately before the hand-off) is called from
     * inside D2gfx.dll with D2gfx's own string -- i.e. window creation, which
     * only happens further down this function, had already executed. */
    FOG_MPQSetConfig(pCfg->bDirect, FALSE);
    FOG_AsyncDataInitialize(TRUE);
    FOG_10082_Noop();
    FOG_10218();

    if (geModState != MODULE_SERVER) {
        if (!ARCHIVE_LoadArchives()
            || !ARCHIVE_LoadExpansionArchives(ARCHIVE_ShowInsertPlayDiscMessage,
                                              ARCHIVE_ShowInsertExpansionDiscMessage,
                                              0, pCfg)) {
            ARCHIVE_FreeArchives();
            return 0;
        }
        pCfg->bIsExpansion = FOG_IsExpansion();
    }

    if      (pCfg->b3DFX)   dwRenderMode = 4;   /* GLIDE */
    else if (pCfg->bWindow) dwRenderMode = 1;   /* GDI */
    else if (pCfg->bD3D)    dwRenderMode = 6;   /* DIRECT3D */
    else                    dwRenderMode = 3;   /* DDRAW */

    if (geModState != MODULE_SERVER) {
        if (!D2Win_CreateWindow((HINSTANCE)hInstance, dwRenderMode,
                                pCfg->bWindow, !pCfg->bNoCompress))
            return 0;
        if (pCfg->bPerspective && dwRenderMode >= 4)
            D2GFX_SetPerspective(TRUE);
        if (!D2Win_InitializeSpriteCache(pCfg->bWindow != 0, 0 /*640x480*/)) {
            D2GFX_Release();   /* same D2gfx @10084 the teardown path below uses */
            return 0;
        }
        if (gbUseKeyhook)
            ghModKeyhook = LoadLibraryA("Keyhook.dll");
        if (ghModKeyhook) {
            void *pFunc;
            if (ResolveProcAddress(ghModKeyhook, "InstallKeyboardHook", &pFunc))
                ((void (__stdcall *)(HWND))pFunc)(WINDOW_GetWindow());
        }
        bGfxStarted = TRUE;
    }

    if (pCfg->bQuality) D2GFX_ToggleLowQuality();
    if (pCfg->dwGamma)  D2GFX_SetGamma(pCfg->dwGamma);
    if (pCfg->bVSync)   D2GFX_EnableVSync();

    {
        DWORD bFixedAspect = 1;
        SRegLoadValue("Diablo II", "Fixed Aspect Ratio", 0, &bFixedAspect);
        if (pCfg->bNoFixedAspect || bFixedAspect != 1)
            D2GFX_SetFixedAspectRatio();
    }
    if (!pCfg->bIsExpansion)
        SRegSaveValue("Diablo II", "Resolution", 0, 0);

    if (!pCfg->bNoSound && geModState != MODULE_SERVER) {
        D2SOUND_OpenSoundSystem(pCfg->bIsExpansion, pCfg->bSoundBackground);
        bSoundStarted = TRUE;
    }

    while (geModState != MODULE_NONE) {
        if (geModState == MODULE_SERVER) {
            if (bSoundStarted) { D2SOUND_CloseSoundSystem(); bSoundStarted = FALSE; }
            if (bGfxStarted)   { D2Win_CloseSpriteCache(); D2GFX_Release(); bGfxStarted = FALSE; }
        }
        geModState = LoadCurrentlySelectedModule(pCfg);
    }

    if (bSoundStarted) D2SOUND_CloseSoundSystem();
    if (bGfxStarted)   { D2Win_CloseSpriteCache(); D2GFX_Release(); }
    ARCHIVE_FreeArchives();

    if (ghModKeyhook) {
        void *pFunc;
        if (ResolveProcAddress(ghModKeyhook, "UninstallKeyboardHook", &pFunc))
            ((void (__stdcall *)(void))pFunc)();
        FreeLibrary(ghModKeyhook);
    }

    FOG_AsyncDataDestroy();
    D2MCPClientCloseMCP();
    if (pCfg->pComInterface)
        (*(void (**)(void))((char *)pCfg->pComInterface + 12))();
    FOG_DestroyMemoryPoolSystem(0);
    return 0;
}

/* 0x00408000 -- SaveCmdLine. Persists / restores the launch command line in the
 * registry: with a real argv, save "<argv> -skiptobnet"; otherwise honour
 * UseCmdLine and load the saved line, or seed "-skiptobnet". In service mode,
 * override with the service command line. &argv comes in ESI. CONF_TRACE. */
static void GAME_InitializeCommandLineFromRegistry(const char **pargv)
{
    char szWrite[MAX_REG_KEY * 2];
    DWORD bUseCmdLine = FALSE;

    if (*pargv && strlen(*pargv)) {
        sprintf(szWrite, "%s -skiptobnet", *pargv);
        SRegSaveString("Diablo II", "CmdLine", 0, szWrite);
    } else {
        SRegLoadValue("Diablo II", "UseCmdLine", 0, &bUseCmdLine);
        if (bUseCmdLine) {
            SRegLoadString("Diablo II", "CmdLine", 0, szSRegReadBuf, MAX_REG_KEY);
            *pargv = szSRegReadBuf;
        } else {
            strcpy(szWrite, "-skiptobnet");
            SRegSaveString("Diablo II", "CmdLine", 0, szWrite);
        }
    }

    bUseCmdLine = FALSE;
    SRegSaveValue("Diablo II", "UseCmdLine", 0, 0);

    if (gbD2ServerStopEvent) {
        SRegLoadString("Diablo II", "SvcCmdLine", 0, szSRegReadBuf, MAX_REG_KEY);
        *pargv = szSRegReadBuf;
    }
}

/* 0x00407a80 -- the ini half of ParseCmdLine. Build the D2.ini path from the
 * install root, then read every option out of the ini into the Config at its
 * dwIndex (INT via GetPrivateProfileInt, STRING via GetPrivateProfileString
 * into a 16-byte field, BOOL as nonzero-int). This is what fills the render/
 * sound/etc. fields GameStart reads -- with it stubbed the Config was all
 * zeros. GetInstallRootDirectory leaves the trailing backslash, so append the
 * bare filename. */
static void GAME_LoadConfigFromIniFile(Config *pCfg)
{
    char szPath[MAX_PATH];
    unsigned i;

    if (!GetInstallRootDirectory(szPath))
        return;
    strcat(szPath, "D2.ini");

    for (i = 0; i < ARRAY_SIZE(gaCmdArguments); i++) {
        char *pMember = (char *)pCfg + gaCmdArguments[i].dwIndex;
        switch (gaCmdArguments[i].dwType) {
        case CMD_INTEGER:
            *(DWORD *)pMember = GetPrivateProfileIntA(
                gaCmdArguments[i].szSection, gaCmdArguments[i].szKey,
                gaCmdArguments[i].dwDefault, szPath);
            break;
        case CMD_STRING:
            GetPrivateProfileStringA(gaCmdArguments[i].szSection,
                gaCmdArguments[i].szKey, &lpZero, pMember, 16, szPath);
            break;
        case CMD_BOOLEAN:
            *(BYTE *)pMember = (BYTE)(0 != GetPrivateProfileIntA(
                gaCmdArguments[i].szSection, gaCmdArguments[i].szKey,
                gaCmdArguments[i].dwDefault, szPath));
            break;
        }
    }
}

/* GAME_InitializeAndStartGame @ 0x408250 -- GameInit, the launcher's linchpin.
 * argc in EAX, argv in ECX (the private convention WinMain and
 * D2ServerServiceMain both call with -- see their `mov eax,<argc>; ...ecx=argv;
 * call`). Reconstructed faithfully to the disassembly's call sequence so /O2
 * pins that convention; the body itself is the deep hard tail (huge Config
 * frame, a video-registry jump table, ~15 externals) and stays DIFF/CONF_TRACE
 * until the full Config layout lands. tCfg is an opaque frame here on purpose:
 * its size shifts only GameInit's OWN bytes, never the convention. */
static int GAME_InitializeAndStartGame(int argc, char **argv)
{
    const char *lpArgvCmd = &lpZero;
    int nMod = 4;                              /* MODULE_LAUNCHER default */
    char szVersion[MAX_PATH];
    Config tCfg;

    if (argc > 1)
        lpArgvCmd = argv[argc - 1];

    SStrPrintf(szVersion, MAX_PATH, "v%d.%02d", 1, 13);
    FOG_SetLogPrefix("D2");
    FOG_InitErrorMgr("Diablo II", 0, szVersion, 1);

    {
        HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, TRUE, "DIABLO_II_OK");
        if (hEvent) {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
    }

    GAME_InitializeCommandLineFromRegistry(&lpArgvCmd);   /* SaveCmdLine */
    GAME_MigrateBetaRegistryKeys();
    GAME_ParseModStateFromCommandLine(lpArgvCmd, &nMod);

    /* ParseCmdLine, as 1.13c factors it: zero the Config here (the inlined
     * rep-stos), the ini-file half in GAME_LoadConfigFromIniFile, the
     * command-line half in ParseAllCommandLineOptions(&tCfg, argv). */
    memset(&tCfg, 0, sizeof(tCfg));
    GAME_LoadConfigFromIniFile(&tCfg);
    ParseAllCommandLineOptions((char *)&tCfg, lpArgvCmd);

    /* Install the validation callback D2Win calls unconditionally through the
     * Config (see the pfnValidate comment on the struct). The original emits
     * this store here, interleaved into the render-flag tests that follow. */
    tCfg.pfnValidate = ValidateEntityOperationAlwaysTrue;

    /* No renderer chosen on the command line? Take it from the video registry
     * (1 D3D, 2 OpenGL, 3 Glide, 4 windowed). Matches D2MOO GameInit. */
    if (!tCfg.b3DFX && !tCfg.bWindow && !tCfg.bOpenGL && !tCfg.bD3D) {
        HKEY hKey;
        const char *szVid = "SOFTWARE\\Blizzard Entertainment\\Diablo II\\VideoConfig";
        if (RegOpenKeyExA(HKEY_CURRENT_USER, szVid, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS ||
            RegOpenKeyExA(HKEY_LOCAL_MACHINE, szVid, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
            DWORD dwType = REG_DWORD, dwValue = 0, dwCb = sizeof(dwValue);
            if (RegQueryValueExA(hKey, "Render", 0, &dwType,
                                 (LPBYTE)&dwValue, &dwCb) == ERROR_SUCCESS) {
                switch (dwValue) {
                case 1: tCfg.bD3D = TRUE; break;
                case 2: tCfg.bOpenGL = TRUE; break;
                case 3: tCfg.b3DFX = TRUE; break;
                case 4: tCfg.bWindow = TRUE; break;
                }
                RegCloseKey(hKey);
            }
        }
    }

    /* GameStart. The hand-off to D2Launch is NOT a separate step bolted on
     * here -- it is GAME_RunMainLoop's own module loop reaching
     * LoadCurrentlySelectedModule, exactly as the original does at 0x40842a
     * (GameInit's tail calls 0x407600 directly, and the ONLY reference to the
     * "QueryInterface" string in the whole binary is inside 0x407550, which
     * only GameStart's loop calls). Game.exe never calls FindWindowA itself:
     * the single-instance window probe seen on the wire comes from INSIDE
     * D2gfx.dll during window creation -- the baseline's FindWindowA passes
     * the string at 0x6fa90cfc, which lies in D2gfx.dll's image, not ours. */
    return GAME_RunMainLoop(ghCurrentProcess, &tCfg, nMod);
}

/* 0x00408450 -- D2ServerServiceMain. WINAPI service entry. Registers the
 * control handler, reports RUNNING, runs the game, reports STOPPED. Missed by
 * Ghidra (only referenced as a function pointer); recovered from the padding
 * boundary at 0x408448. Calls GameInit with the EAX/ECX convention. */
VOID WINAPI D2ServerServiceMain(DWORD dwArgc, char **lpszArgv)
{
    gbServiceRunning = 1;
    ghD2ServerServiceStatus =
        RegisterServiceCtrlHandlerA(SVC_NAME, ServiceControlHandler);
    SetServiceStatus(ghD2ServerServiceStatus, &gD2ServerServiceStatus);
    GAME_InitializeAndStartGame(dwArgc, lpszArgv);
    gD2ServerServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(ghD2ServerServiceStatus, &gD2ServerServiceStatus);
    gbServiceRunning = 0;
}

/* 0x004084b0 -- looks for a registered D2 service and, if present, hands the
 * process to the SCM as a service. 9x has no SCM, so it bails there. Returns
 * nonzero only when StartServiceCtrlDispatcher succeeds. Every external
 * reference (the SCM APIs, SVC_NAME, D2ServerServiceMain's address) is a
 * relocation, so this byte-matches independently of the stubs above. */
static int GAME_TryStartAsWindowsService(void)
{
    SC_HANDLE schSCManager, schService;
    SERVICE_TABLE_ENTRYA DispatchTable[2];

    /* NT only. Testing the high bit (9x sets it) makes MSVC emit `js`, where
     * a signed `< 0` would emit `jl` -- a one-byte difference in the encoding
     * of the same test. */
    if (!(GetVersion() & 0x80000000)) {
        schSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (schSCManager) {
            schService = OpenServiceA(schSCManager, SVC_NAME, SERVICE_ALL_ACCESS);
            if (schService)
                CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            if (schService) {
                DispatchTable[0].lpServiceName = SVC_NAME;
                DispatchTable[0].lpServiceProc = D2ServerServiceMain;
                DispatchTable[1].lpServiceName = NULL;
                DispatchTable[1].lpServiceProc = NULL;
                return StartServiceCtrlDispatcherA(DispatchTable) != 0;
            }
        }
    }
    return 0;
}

/* 0x00408120 -- ApplyProcessSecurityRestrictions. Applies a DENY-ALL DACL to
 * the current process so nothing can open it (anti-inject / anti-debug). The
 * four advapi32 primitives are resolved DYNAMICALLY from advapi32.dll, exactly
 * as the original does -- the observable wire sequence is
 *   LoadLibraryA("advapi32.dll")
 *   GetProcAddress x4  (AllocateAndInitializeSid, InitializeAcl,
 *                       AddAccessDeniedAce, SetSecurityInfo)
 *   AllocateAndInitializeSid / InitializeAcl / AddAccessDeniedAce /
 *   SetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT,
 *                   DACL_SECURITY_INFORMATION, ...)
 *   FreeLibrary / FreeSid
 * matching the baseline call-for-call (calls 169..180). GetCurrentProcess is
 * resolved through the IAT but the tracer cannot intercept it (a KERNELBASE
 * forwarder at 7675FBE0), so it is invisible on the wire -- yet its pseudo-
 * handle 0xFFFFFFFF is what SetSecurityInfo receives, confirming the read. The
 * SID authority is {0,0,0,0,0,1} = SECURITY_WORLD_SID_AUTHORITY, so the
 * deny-all ACE targets Everyone (S-1-1-0). noinline so WinMain must emit the
 * `call`, which is also the scheduling barrier that keeps the argv writes ahead
 * of GameInit's register setup. */
static volatile int g_apply_sink;
static __declspec(noinline) void ApplyProcessSecurityRestrictions(void)
{
    HANDLE  hProcess;
    HMODULE hAdvapi;
    SID_IDENTIFIER_AUTHORITY authWorld = { SECURITY_WORLD_SID_AUTHORITY };
    PSID  pSid = NULL;
    BYTE  aclBuf[0x200];
    BOOL  (WINAPI *pAllocSid)(PSID_IDENTIFIER_AUTHORITY, BYTE, DWORD, DWORD,
                              DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, PSID *);
    BOOL  (WINAPI *pInitAcl)(PACL, DWORD, DWORD);
    BOOL  (WINAPI *pAddAce)(PACL, DWORD, DWORD, PSID);
    DWORD (WINAPI *pSetSI)(HANDLE, int, DWORD, PSID, PSID, PACL, PACL);

    hProcess = GetCurrentProcess();
    hAdvapi  = LoadLibraryA("advapi32.dll");
    if (!hAdvapi) { g_apply_sink = 1; return; }

    pAllocSid = (void *)GetProcAddress(hAdvapi, "AllocateAndInitializeSid");
    pInitAcl  = (void *)GetProcAddress(hAdvapi, "InitializeAcl");
    pAddAce   = (void *)GetProcAddress(hAdvapi, "AddAccessDeniedAce");
    pSetSI    = (void *)GetProcAddress(hAdvapi, "SetSecurityInfo");

    if (pAllocSid && pInitAcl && pAddAce && pSetSI &&
        pAllocSid(&authWorld, 1, 0, 0, 0, 0, 0, 0, 0, 0, &pSid)) {
        if (pInitAcl((PACL)aclBuf, sizeof(aclBuf), ACL_REVISION) &&
            pAddAce((PACL)aclBuf, ACL_REVISION, 0xF01FFFFE, pSid)) {
            pSetSI(hProcess, 6 /*SE_KERNEL_OBJECT*/,
                   0x80000004 /*DACL_SECURITY_INFORMATION*/,
                   NULL, NULL, (PACL)aclBuf, NULL);
        }
        FreeLibrary(hAdvapi);
        FreeSid(pSid);
    } else {
        FreeLibrary(hAdvapi);
    }
    g_apply_sink = 1;
}

/* 0x00408540 -- GameEntryPoint (WinMain). __stdcall(hInstance, hPrev,
 * lpCmdLine, nShowCmd) -> `ret 10h`. Stashes the instance + show-cmd globals,
 * hands off to the service dispatcher, and if that declines, locks the process
 * down and runs the game with argv = { "d2server", lpCmdLine }. 1.13c is much
 * simpler than D2MOO's 1.10f WinMain: the SCM probe is factored into
 * GAME_TryStartAsWindowsService and the -install path is elsewhere. GameInit
 * is called at argc=2 in EAX, &argv in ECX -- the convention this file pins. */
int __stdcall GameEntryPoint(HINSTANCE hInstance, HINSTANCE hPrev,
                             char *lpCmdLine, int nShowCmd)
{
    const char *argv[2];
    (void)hPrev;
    ghCurrentProcess = hInstance;
    gnCmdShow = nShowCmd;

    if (GAME_TryStartAsWindowsService())
        return 0;

    argv[0] = "d2server";           /* INIT_NAME */
    argv[1] = lpCmdLine;
    ApplyProcessSecurityRestrictions();
    GAME_InitializeAndStartGame(2, (char **)argv);
    return 0;
}

/* Keep the static helpers with no real caller yet referenced so the TU still
 * emits them. As each one's real caller lands (GameInit pinned
 * GAME_ParseModState + D2ServerServiceMain already), it is removed from here so
 * its convention pins from that single site, exactly as in the original. Still
 * seeded: only the two with no real caller yet -- GetInstallRootDirectory and
 * ResolveProcAddress (GameStart's stub does not call it). The parser chain now
 * has GameInit as its real caller (GameInit -> ParseAllCommandLineOptions ->
 * ParseCommandLineOption), so those are out of the seed and pin from there. */
int __stdcall _seed_keepalive(char *s)
{
    void *proc;
    ConvertStringToLowercase(s);
    GetInstallRootDirectory(s);
    ResolveProcAddress((HMODULE)s, s, &proc);
    return IsWhitespaceOrColon(*s) + GAME_TryStartAsWindowsService()
         + (proc != 0);
}
