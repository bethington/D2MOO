#pragma once

#include "D2CommonDefinitions.h"
#include "Path.h"
#pragma pack(1)

#pragma pack()


//1.10f: D2Common.0x6FDAB890
//1.13c: D2Common.0x6FD8E080
void __fastcall PATHUtil_AdvanceTowardsTarget_6FDAB890(Path* ptPath);

//1.10f: D2Common.0x6FDAB940
//1.13c: D2Common.0x6FD8E310
void __fastcall sub_6FDAB940(PathPoint* pOutPathPoint, Path* ptPath);
