#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ExtractStatusByte
extern "C" uint8_t __stdcall ExtractStatusByte(uint32_t status)
{
    return (uint8_t)(status & 0xFFu);
}
