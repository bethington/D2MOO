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
 * STATUS: seed. The pure/leaf functions are in; the convention-heavy ones
 * (GAME_RunMainLoop, the parsers, GameInit, WinMain) are still to come, and
 * show as "--" on the scoreboard until written.
 */

typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef int           BOOL;

/* --- globals the launcher owns (resolved by address at link/patch time) --- */
void *ghCurrentProcess;
int   gnCmdShow;

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

#define A(sec, key, cmd) { sec, key, cmd, 0, 0, 0 }
CmdArg gaCmdArguments[57] = {
    A("VIDEO","WINDOW","w"),          A("VIDEO","WINDOW","window"),
    A("VIDEO","WINDOW","windowed"),   A("VIDEO","ASPECT","nofixaspect"),
    A("VIDEO","3DFX","3dfx"),         A("VIDEO","OPENGL","opengl"),
    A("VIDEO","D3D","d3d"),           A("VIDEO","RAVE","rave"),
    A("VIDEO","PERSPECTIVE","per"),   A("VIDEO","QUALITY","lq"),
    A("VIDEO","GAMMA","gamma"),       A("VIDEO","VSYNC","vsync"),
    A("VIDEO","FRAMERATE","fr"),      A("NETWORK","SERVERIP","s"),
    A("NETWORK","GAMETYPE","gametype"),A("NETWORK","ARENA","arena"),
    A("NETWORK","JOINID","joinid"),   A("NETWORK","GAMENAME","gamename"),
    A("NETWORK","BATTLENETIP","bn"),  A("NETWORK","MCPIP","mcpip"),
    A("CHARACTER","AMAZON","ama"),    A("CHARACTER","PALADIN","pal"),
    A("CHARACTER","SORCERESS","sor"), A("CHARACTER","NECROMANCER","nec"),
    A("CHARACTER","BARBARIAN","bar"), A("CHARACTER","INVINCIBLE","i"),
    A("CHARACTER","NAME","name"),     A("CHARACTER","REALM","realm"),
    A("CHARACTER","CTEMP","ctemp"),   A("MONSTER","NOMONSTERS","nm"),
    A("MONSTER","MONSTERCLASS","m"),  A("MONSTER","MONSTERINFO","minfo"),
    A("MONSTER","MONSTERDEBUG","md"), A("ITEM","RARE","rare"),
    A("ITEM","UNIQUE","unique"),      A("INTERFACE","ACT","act"),
    A("DEBUG","LOG","log"),           A("DEBUG","MSGLOG","msglog"),
    A("DEBUG","SAFEMODE","safe"),     A("DEBUG","NOSAVE","nosave"),
    A("DEBUG","SEED","seed"),         A("NETWORK","NOPK","nopk"),
    A("DEBUG","CHEATS","cheats"),     A("DEBUG","TEEN","teen"),
    A("DEBUG","NOSOUND","ns"),        A("FILEIO","NOPREDLOAD","npl"),
    A("FILEIO","DIRECT","direct"),    A("FILEIO","LOWEND","lem"),
    A("DEBUG","QuEsTs","questall"),   A("NETWORK","COMINT","comint"),
    A("NETWORK","SKIPTOBNET","skiptobnet"),A("NETWORK","OPENC","openc"),
    A("FILEIO","NOCOMPRESS","nocompress"),A("TXT","TXT","txt"),
    A("BUILD","BUILD","build"),       A("DEBUG","NOSOUND","nosound"),
    A("DEBUG","SOUNDBKG","sndbkg"),
};
#undef A

int  __cdecl strcmp(const char *, const char *);
char * __cdecl strcpy(char *, const char *);
unsigned int __cdecl strlen(const char *);
#pragma intrinsic(strcmp, strcpy, strlen)
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

/* Keep the static helpers referenced so a seed TU (before their real callers
 * exist) still emits them. Removed once ConvertStringToLowercase and the
 * parsers call them for real. */
int __stdcall _seed_keepalive(char *s)
{
    ConvertStringToLowercase(s);
    return IsWhitespaceOrColon(*s);
}
