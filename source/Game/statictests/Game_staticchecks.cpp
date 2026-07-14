#include <D2Config.h>
#include <Main.h>
#include <cstddef>

// Thanks to galaxyhaxz for providing the base to work on ! https://github.com/galaxyhaxz/d2src
// Offsets are the 1.13c layout: +2 vs 1.10f (bNoFixedAspect and bSoundBackground added in 1.13).
static_assert(offsetof(D2ConfigStrc, szCharacterRealm) == 0x229, "");
static_assert(offsetof(D2ConfigStrc, szGamePassword) == 0x241, "");

static_assert(offsetof(D2ConfigStrc, bSkipToBNet) == 0x359, "");
static_assert(offsetof(D2ConfigStrc, bShownLogo) == 0x35B, "");


static_assert(offsetof(D2ConfigStrc, szCurrentChannelName) == 0x35D, "");
static_assert(offsetof(D2ConfigStrc, szDefaultChannelName) == 0x37D, "");
static_assert(offsetof(D2ConfigStrc, nComponents) == 0x39D, "");
static_assert(offsetof(D2ConfigStrc, nComponentsColors) == 0x3AD, "");
static_assert(offsetof(D2ConfigStrc, nCharacterLevel) == 0x3BD, "");
static_assert(offsetof(D2ConfigStrc, nAccountPasswordHash) == 0x3BF, "");
static_assert(offsetof(D2ConfigStrc, nSaveFlags) == 0x3C7, "");
static_assert(sizeof(D2ConfigStrc) == 0x3C9, "");


static_assert(sizeof(D2CmdArgStrc) == 0x3C, "Check D2CmdArgStrc matches original size");
