#include "../provider_runtime.h"

/* D2MOO_REIMPL_EXPORT: HaveLightResBonus */

// [RE-DRAFTED 2026-07-08] Prior reimpl's else-branch (type 2/4-5) returned the RAW
// pointer at +0x2c instead of DEREFERENCING it -> 1.2% shadow divergence (only
// objects/missiles/items hit that branch). Ground truth (disasm @0x6fd7fe10):
//   else-branch:  MOV EAX,[EAX+0x2c]; MOV EAX,[EAX]   -> *(int*)( *(void**)(pUnit+0x2c) )
//   if-branch:    MOV EAX,[EAX+0x2c]; ...; MOV EAX,[EAX+0x1c]
// The if-branch was already correct; only the else-branch missed the second deref.
extern "C" int __stdcall HaveLightResBonus(void* pUnit)
{
    if (pUnit == nullptr) return 0;  // original aborts; never hit (game validates)

    unsigned int type = *(unsigned int*)((char*)pUnit + 0x00);

    if ((type != 2) && ((int)type < 4 || (int)type > 5)) {
        void* p2c = *(void**)((char*)pUnit + 0x2c);
        if (p2c != nullptr) {
            return *(int*)((char*)p2c + 0x1c);
        }
        return 0;
    }

    // type is 2/4-5: dereference the pointer at +0x2c and read the int it points to
    void* p2c = *(void**)((char*)pUnit + 0x2c);
    return *(int*)p2c;
}
