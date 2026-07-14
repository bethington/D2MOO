#pragma once 

#include <D2Common.h>
#include <D2BasicTypes.h>
#include <D2Inventory.h>
#include <Archive.h>

#pragma pack(1)


struct InvCompGrid
{
	InvRect rect;						//0x00
	uint8_t nWidth;							//0x10
	uint8_t nHeight;						//0x11
	uint16_t pad0x12;						//0x12
};

struct InventoryGridInfo
{
	uint8_t nGridX;							//0x00
	uint8_t nGridY;							//0x01
	uint16_t pad0x02;						//0x02
	int32_t nGridLeft;						//0x04
	int32_t nGridRight;						//0x08
	int32_t nGridTop;						//0x0C
	int32_t nGridBottom;					//0x10
	uint8_t nGridBoxWidth;					//0x14
	uint8_t nGridBoxHeight;					//0x15
	uint16_t pad0x16;						//0x16
};

enum InventoryComponent
{
	//INV_COMP_INV,
	//INV_COMP_GRID,
	INV_COMP_RARM,
	INV_COMP_TORSO,
	INV_COMP_LARM,
	INV_COMP_HEAD,
	INV_COMP_NECK,
	INV_COMP_RHAND,
	INV_COMP_LHAND,
	INV_COMP_BELT,
	INV_COMP_FEET,
	INV_COMP_GLOVES,
	NUM_INV_COMPS

};
struct InventoryTxt
{
	InvRect pRect;					//0x00
	InventoryGridInfo pGridInfo;		//0x10
	InvCompGrid pComponents[NUM_INV_COMPS];//0x28
};

#pragma pack()

//D2Common.0x6FD542D0
void __fastcall DATATBLS_LoadInventoryTxt(HD2ARCHIVE hArchive);
//D2Common.0x6FD54F10
void __fastcall DATATBLS_UnloadInventoryTxt();
//D2Common.0x6FD54F20 (#10635)
D2COMMON_DLL_DECL void __stdcall DATATBLS_GetInventoryRect(int nInventoryTxtId, int bHigherRes, InvRect* pInvRect);
//D2Common.0x6FD54FB0 (#10636)
D2COMMON_DLL_DECL void __stdcall DATATBLS_GetInventoryGridInfo(int nInventoryTxtId, int bHigherRes, InventoryGridInfo* pInventoryGridInfo);
//D2Common.0x6FD55030 (#10637)
D2COMMON_DLL_DECL void __stdcall DATATBLS_GetInventoryComponentGrid(int nInventoryTxtId, int bHigherRes, InvCompGrid* pInvCompGrid, int nComponent);
