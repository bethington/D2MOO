#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: INV_IsItemTypeInInventory
//
// Walks the unit's inventory LINKED LIST looking for an item type.
//
// FIXED 2026-07-30 -- the previous version did pointer ARITHMETIC where the
// original does pointer DEREFERENCES, so it walked raw memory, not the list:
//
//     puVar1 = (unsigned int*)((char*)pUnit + 0x2c);  // the ADDRESS of the head
//                                                     // field, not the head
//     while (puVar1 != 0) {                           // an address is never NULL
//         if (*puVar1 == dwItemType) return 1;
//         puVar1 = (unsigned int*)((char*)puVar1 + 0x4);  // +4 bytes in memory
//     }
//
// It scanned linearly off the end of the unit struct until it either happened
// to read a word equal to dwItemType (one live 0-vs-1 mismatch) or
// access-violated (four SEH-caught faults in live shadow). Only the thunk's SEH
// guard kept that from taking the game down.
//
// The original (PD2-S12 D2Common+0x1dee0) is unambiguous -- both steps LOAD:
//     MOV  EAX,dword ptr [ESP + 0x4]              ; pUnit
//     TEST EAX,EAX                     / JZ  ret0
//     CMP  dword ptr [EAX],0x1020304   / JNZ ret0 ; unit sanity tag
//     MOV  EAX,dword ptr [EAX + 0x2c]             ; head = *(pUnit+0x2c)  <- LOAD
//     TEST EAX,EAX                     / JZ  ret0
//     MOV  ECX,dword ptr [ESP + 0x8]              ; dwItemType
//   loop:
//     CMP  dword ptr [EAX],ECX         / JZ  ret1 ; node->type == wanted
//     MOV  EAX,dword ptr [EAX + 0x4]              ; node = *(node+4)      <- LOAD
//     TEST EAX,EAX                     / JNZ loop
//     XOR  EAX,EAX / RET 0x8
//   ret1:
//     MOV  EAX,0x1 / RET 0x8
extern "C" int __stdcall INV_IsItemTypeInInventory(void* pUnit, unsigned int dwItemType)
{
	if (pUnit == nullptr)
		return 0;
	if (*(unsigned int*)pUnit != 0x1020304)   // unit sanity tag
		return 0;

	// head = *(pUnit + 0x2c) -- a LOAD, so an empty inventory terminates.
	unsigned int* pNode = *(unsigned int**)((char*)pUnit + 0x2c);
	while (pNode != nullptr)
	{
		if (*pNode == dwItemType)
			return 1;
		// node = *(node + 4) -- follow the link; do NOT advance by 4 bytes.
		pNode = *(unsigned int**)((char*)pNode + 0x4);
	}
	return 0;
}
