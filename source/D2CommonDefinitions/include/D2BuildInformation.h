#pragma once

// D2MOO targets Project Diablo 2 (built on 1.13c) exclusively.
// The build system normally defines these from D2MOO_ORDINALS_VERSION; this is the fallback.
#if !defined(D2_VERSION_MAJOR)
#define D2_VERSION_113C
#define D2_VERSION_MAJOR 1
#define D2_VERSION_MINOR 13
#define D2_VERSION_PATCH 'C'
#endif

#define D2_IS_MONOLITHIC FALSE // 1.13c ships separate .DLLs

#define D2_VERSION_EXPANSION 1

#define D2_HAS_OPENGL 0
#define D2_HAS_RAVE 0

#define D2_HAS_MULTILAN 0
