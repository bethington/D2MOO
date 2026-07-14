#include <D2Config.h>
#include <Main.h>
#include <cstddef>

// Thanks to galaxyhaxz for providing the base to work on ! https://github.com/galaxyhaxz/d2src
// Offsets are the 1.13c layout: +2 vs 1.10f (bNoFixedAspect and bSoundBackground added in 1.13).
static_assert(offsetof(Config, szCharacterRealm) == 0x229, "");
static_assert(offsetof(Config, szGamePassword) == 0x241, "");

static_assert(offsetof(Config, bSkipToBNet) == 0x359, "");
static_assert(offsetof(Config, bShownLogo) == 0x35B, "");


static_assert(offsetof(Config, szCurrentChannelName) == 0x35D, "");
static_assert(offsetof(Config, szDefaultChannelName) == 0x37D, "");
static_assert(offsetof(Config, nComponents) == 0x39D, "");
static_assert(offsetof(Config, nComponentsColors) == 0x3AD, "");
static_assert(offsetof(Config, nCharacterLevel) == 0x3BD, "");
static_assert(offsetof(Config, nAccountPasswordHash) == 0x3BF, "");
static_assert(offsetof(Config, nSaveFlags) == 0x3C7, "");
static_assert(sizeof(Config) == 0x3C9, "");


static_assert(sizeof(CmdArg) == 0x3C, "Check CmdArg matches original size");
