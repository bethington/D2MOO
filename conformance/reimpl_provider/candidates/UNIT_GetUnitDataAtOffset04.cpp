// D2MOO_REIMPL_EXPORT: UNIT_GetUnitDataAtOffset04
#include "../provider_runtime.h"
#include <stdlib.h>

extern "C" uint32_t __stdcall UNIT_GetUnitDataAtOffset04(void* pUnit) {
    if (pUnit == (void*)0x0) {
        // Abort path: original calls GetReturnAddress() -> CleanupAndAbort() -> _exit(-1).
        // Reimpl-inline: for shadow purposes both paths terminate the process, so we
        // just match the terminal behavior with _exit(-1).
        _exit(-1);
    }
    return **(int**)((int)pUnit + 4);
}
