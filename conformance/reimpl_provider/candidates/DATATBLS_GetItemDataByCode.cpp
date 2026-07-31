#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemDataByCode
extern "C" int __stdcall DATATBLS_GetItemDataByCode(void* p, int nUnused2, int nUnused3) {
    if (p == nullptr) return 0;
    return (int)*(*(short**)p);
}
