// D2MOO_REIMPL_EXPORT: SKILLS_GetSkillListEntryByTypeIndex2
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall SKILLS_GetSkillListEntryByTypeIndex2(void* pSkillData, int nIndex)
{
    uint32_t node;
    int count;
    uint16_t val;

    /* pSkillData->pSkillListHead at offset +8 */
    node = *(uint32_t*)((uint8_t*)pSkillData + 8);
    count = 0;

    while (1) {
        if (node == 0) {
            /* EAX==0 here, MOV AX,0xE8C -> EAX = 0x00000E8C */
            return 0xE8Cu;
        }
        if (*(int*)((uint8_t*)node + 4) == 2) {
            if (count == nIndex) {
                /* MOV AX,word ptr [EAX] only writes low 16 of EAX;
                   high 16 retain node>>16 (bit-exact) */
                val = *(uint16_t*)node;
                return (node & 0xFFFF0000u) | (uint32_t)val;
            }
            count = count + 1;
        }
        /* node = node->pNext at offset +8 */
        node = *(uint32_t*)((uint8_t*)node + 8);
    }
}
