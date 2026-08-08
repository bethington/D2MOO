/* DRAFT -- NOT YET BYTE-MATCHED. ApplyProcessSecurityRestrictions @ 0x00408120.
 *
 * First attempt: length matches EXACTLY at 283 bytes, but 218 of them differ,
 * so the overall shape is right and the register allocation is not. Iterate on
 * the diff the way parse.c's TrimWhitespaceDelimiters was landed -- read the
 * emitted code against the original and let the differences name the next
 * hypothesis (see ../README.md, "Reconstruction playbook").
 *
 * SPEC, derived from the disassembly and CORROBORATED BY THE LIVE TRACE:
 *
 *   SUB ESP,0x21c        540 bytes of locals: a 512-byte ACL, the SID
 *                        authority, and the resolved function pointers
 *   XOR EBX,EBX          EBX is the zero constant for the whole function
 *   [ESP+0xc..0x11] = 0,0,0,0,0,1
 *                        a SID_IDENTIFIER_AUTHORITY of {0,0,0,0,0,1} --
 *                        SECURITY_WORLD_SID_AUTHORITY, i.e. Everyone
 *   GetCurrentProcess, then LoadLibraryA("advapi32.dll"), bail if null
 *   then GetProcAddress in THIS ORDER, each bailing if null:
 *       AllocateAndInitializeSid, InitializeAcl,
 *       AddAccessDeniedAce, SetSecurityInfo
 *
 * The order is observed fact, not a reading of the listing: the behavioural
 * trace records exactly that sequence of GetProcAddress calls
 * (trace/baseline_original.jsonl).
 *
 * What the function DOES matters beyond byte-matching it -- this is what
 * makes a running Game.exe unkillable by an ordinary same-user caller, and
 * it cost real confusion before it was identified. See ../README.md.
 */


typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef void *HANDLE;
typedef void *HMODULE;
typedef const char *LPCSTR;
typedef int BOOL;
typedef void *PSID;
typedef void *PACL;
#define WINAPI __stdcall

typedef struct _SID_IDENTIFIER_AUTHORITY { BYTE Value[6]; } SID_IDENTIFIER_AUTHORITY;

HANDLE WINAPI GetCurrentProcess(void);
HMODULE WINAPI LoadLibraryA(LPCSTR);
void * WINAPI GetProcAddress(HMODULE, LPCSTR);
BOOL   WINAPI FreeLibrary(HMODULE);

typedef BOOL  (WINAPI *PFN_AllocSid)(SID_IDENTIFIER_AUTHORITY *, BYTE,
                                     DWORD, DWORD, DWORD, DWORD,
                                     DWORD, DWORD, DWORD, DWORD, PSID *);
typedef BOOL  (WINAPI *PFN_InitAcl)(PACL, DWORD, DWORD);
typedef BOOL  (WINAPI *PFN_AddDenyAce)(PACL, DWORD, DWORD, PSID);
typedef DWORD (WINAPI *PFN_SetSecInfo)(HANDLE, int, DWORD, PSID, PSID, PACL, PACL);
typedef void *(WINAPI *PFN_FreeSid)(PSID);

#define ACL_REVISION            2
#define DACL_SECURITY_INFORMATION 4
#define SE_KERNEL_OBJECT        6
#define DENY_ALL                0xF01FFFFE

int F(void)
{
    SID_IDENTIFIER_AUTHORITY auth;
    BYTE acl[512];
    PSID pSid;
    HANDLE hProcess;
    HMODULE hAdvapi;
    PFN_AllocSid   pAllocSid;
    PFN_InitAcl    pInitAcl;
    PFN_AddDenyAce pAddDenyAce;
    PFN_SetSecInfo pSetSecInfo;
    int ok;

    auth.Value[0] = 0; auth.Value[1] = 0; auth.Value[2] = 0;
    auth.Value[3] = 0; auth.Value[4] = 0; auth.Value[5] = 1;

    pSid = 0;
    ok = 0;
    hProcess = GetCurrentProcess();

    hAdvapi = LoadLibraryA("advapi32.dll");
    if (!hAdvapi) return 0;

    pAllocSid = (PFN_AllocSid)GetProcAddress(hAdvapi, "AllocateAndInitializeSid");
    if (!pAllocSid) goto done;
    pInitAcl = (PFN_InitAcl)GetProcAddress(hAdvapi, "InitializeAcl");
    if (!pInitAcl) goto done;
    pAddDenyAce = (PFN_AddDenyAce)GetProcAddress(hAdvapi, "AddAccessDeniedAce");
    if (!pAddDenyAce) goto done;
    pSetSecInfo = (PFN_SetSecInfo)GetProcAddress(hAdvapi, "SetSecurityInfo");
    if (!pSetSecInfo) goto done;

    if (!pAllocSid(&auth, 0, 0, 0, 0, 0, 0, 0, 0, 0, &pSid)) goto done;
    if (!pInitAcl((PACL)acl, sizeof(acl), ACL_REVISION)) goto done;
    if (!pAddDenyAce((PACL)acl, ACL_REVISION, DENY_ALL, pSid)) goto done;
    if (pSetSecInfo(hProcess, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,
                    0, 0, (PACL)acl, 0) == 0)
        ok = 1;

done:
    FreeLibrary(hAdvapi);
    return ok;
}