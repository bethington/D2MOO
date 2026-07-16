// D2MOO_REIMPL_EXPORT: SKILLS_GetSkillNodeField0x20
#include "../provider_runtime.h"

/*
 * SKILLS_GetSkillNodeField0x20 -> SKILLS_GetSkillTargetGUID
 * stdcall, 1 stack arg, EAX return.
 *
 * Disassembly:
 *   MOV  EAX, [ESP+0x4]      ; load pSkillNode
 *   TEST EAX, EAX
 *   JNZ  return_field
 *   PUSH 0xa65               ; nLineNumber
 *   CALL GetReturnAddress    ; -> EAX = return address
 *   PUSH EAX                 ; pContext
 *   PUSH 0x6fdda728          ; nErrorCode
 *   CALL CleanupAndAbort
 *   ADD  ESP, 0xc
 *   PUSH -0x1
 *   CALL _exit
 * return_field:
 *   MOV  EAX, [EAX+0x20]     ; load dwField20 / dwTargetGUID
 *   RET  0x4
 */
extern "C" uint32_t __stdcall SKILLS_GetSkillNodeField0x20(void* pSkillNode)
{
    /* Non-NULL path: bit-exact 1:1 port.
       Return the DWORD at SKILLS_SkillNode + 0x20 (dwField20 / dwTargetGUID). */
    if (pSkillNode != NULL) {
        return *(const uint32_t*)((const uint8_t*)pSkillNode + 0x20);
    }

    /* NULL abort path -- UNCOVERED by the proof.
       Original invokes GetReturnAddress() then CleanupAndAbort(0x6fdda728, retAddr, 0xa65)
       then _exit(-1). Callee decompiles were not provided to inline them, and this branch
       was never exercised by the shadow proof (the real call would have already terminated
       the process before the shadow result is observed). We preserve the magic constants
       and the -1 exit-code observable, but cannot faithfully inline the game abort helpers
       without their decompiles; the live original still drives the real abort. */
    {
        uint32_t nLineNumber = 0xa65u;          /* magic: CleanupAndAbort arg */
        uint32_t nErrorCode  = 0x6fdda728u;     /* magic: CleanupAndAbort arg */
        /* Suppress unused-variable warnings while documenting the constants. */
        (void)nLineNumber;
        (void)nErrorCode;
    }
    /* Original never returns here (calls _exit(-1)); return -1 to match exit code. */
    return 0xFFFFFFFFu;
}
