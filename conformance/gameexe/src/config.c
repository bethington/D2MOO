/* Byte-verified reconstruction -- see parse.c for the ground rules. */

typedef struct CmdArg {
    char          szSection[16];
    char          szKey[16];
    char          szCommand[16];
    unsigned long dwType;
    unsigned long dwIndex;
    unsigned long dwDefault;
} CmdArg;   /* 60 bytes; the original's table stride is 0x3c */

extern CmdArg gaCmdArguments[57];   /* 57 * 0x3c = 0xd5c, the loop bound */

int __cdecl strcmp(const char *, const char *);
#pragma intrinsic(strcmp)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

/* Game.exe 0x004078e0 -- 92 bytes, 1 relocation, 88/88 informative, EXACT.
 *
 * The loop bound MUST be unsigned. The original ends its loop with `JB`;
 * `for (int i = 0; i < 57; i++)` emits `JL` and 96 bytes. ARRAY_SIZE is
 * sizeof-based and therefore size_t, which is what makes this match --
 * `int i < ARRAY_SIZE(...)` also works, since i promotes.
 */
int __stdcall FindConfigOptionIndex(const char *s)
{
    unsigned i;
    for (i = 0; i < ARRAY_SIZE(gaCmdArguments); i++)
    {
        if (0 == strcmp(gaCmdArguments[i].szCommand, s))
        {
            return i;
        }
    }
    return -1;
}
