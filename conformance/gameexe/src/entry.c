/* STATUS: compiles to 74 bytes against the original's 72 -- patch_function
 * refuses it, correctly, under the same-or-smaller rule.
 *
 * THE CAUSE IS STRUCTURAL, NOT A DETAIL OF THIS FUNCTION. MSVC forces a
 * FRAME POINTER on any function containing inline __asm:
 *
 *     original   83 ec 08                 SUB ESP,8              3 bytes
 *     ours       55 8b ec 83 ec 08        PUSH EBP/MOV EBP,ESP/  6 bytes
 *                                         SUB ESP,8
 *
 * The original was compiled WITHOUT a frame pointer, so any C-plus-__asm
 * reconstruction of it starts three bytes behind and cannot catch up. That
 * applies to every one of the four standard-ABI functions, because all four
 * call an LTCG-convention callee and therefore all four need __asm.
 *
 * So the three options are a genuine fork, not a detail:
 *   * __declspec(naked) -- we emit every byte, so byte-matching becomes
 *     possible again, but the "reimplementation" is assembly. Arguably
 *     honest: a convention C cannot express is not expressible in C.
 *   * append-a-section patching -- lifts the size limit, but was rejected
 *     deliberately, because code larger than the original is evidence the
 *     reconstruction is wrong.
 *   * leave these four to a full link, where size is not constrained.
 */

/* GameEntryPoint @ 0x00408540 -- Game.exe's WinMain. 72 bytes.
 *
 * This is the first function written to be VERIFIED BY BEHAVIOUR rather than
 * by byte identity, because it cannot reach byte identity: two of the three
 * functions it calls take their arguments in registers the ABI does not name
 * (link-time codegen inventing a private convention across translation
 * units), and C has no way to say "argument in EAX".
 *
 *   TryStartAsService(hInstance in EAX)        -> result in EAX
 *   ApplyProcessSecurityRestrictions()          -> ordinary, no arguments
 *   GameInit(count in EAX, argv in ECX)
 *
 * So the bodies stay in C and only the call sites drop to __asm -- the same
 * thing matching-decompilation projects do, and the only way a patched-in
 * version can actually run: a pure-C version would push arguments onto the
 * stack that the callees never read, and the game would take a wrong branch
 * or fault.
 *
 * Shape read off the original before any of this was written:
 *   MOV EAX,[ESP+4] / MOV ECX,[ESP+0x10]   hInstance and nShowCmd
 *   SUB ESP,8                              the two-element argv
 *   store both globals, then CALL with hInstance STILL LIVE IN EAX
 *   TEST EAX,EAX / JNZ  -> skip everything and return 0
 *   argv[0] = "d2server" (INIT_NAME), argv[1] = lpCmdLine
 *   LEA ECX,[ESP] / MOV EAX,2 / CALL GameInit
 *   XOR EAX,EAX / ADD ESP,8 / RET 0x10     __stdcall, four arguments
 */

typedef void *HINSTANCE;
typedef unsigned long DWORD;

/* The launcher's two globals, at 0x0040cf24 and 0x0040cf28. Declared here
 * and resolved by the patcher through --symbols; nothing links this. */
extern HINSTANCE ghCurrentProcess;
extern int       gnCmdShow;

/* Callees. The two with custom conventions are declared only so the
 * assembler has a symbol to emit a relocation against -- their declared
 * signatures are deliberately not used for argument passing. */
int  __cdecl TryStartAsService(void);
void __cdecl ApplyProcessSecurityRestrictions(void);
int  __cdecl GameInit(void);

#define INIT_NAME "d2server"

int __stdcall GameEntryPoint(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                             char *lpCmdLine, int nShowCmd)
{
    const char *argv[2];

    ghCurrentProcess = hInstance;
    gnCmdShow = nShowCmd;

    /* hInstance travels in EAX. Loading it explicitly costs bytes the
     * original does not spend (it simply leaves the value live after the
     * global store), which is one reason this function cannot byte-match. */
    __asm {
        mov  eax, hInstance
        call TryStartAsService
        test eax, eax
        jnz  SHORT finished
    }

    argv[0] = INIT_NAME;
    argv[1] = lpCmdLine;

    ApplyProcessSecurityRestrictions();

    __asm {
        lea  ecx, argv
        mov  eax, 2
        call GameInit
    }

finished:
    return 0;
}
