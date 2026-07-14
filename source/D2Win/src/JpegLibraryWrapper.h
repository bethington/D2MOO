#pragma once
// Original game was using Intel's ijl.
// Since it's doing nothing else but convert a raw image to jpeg format, use our own lib instead.

#if D2MOO_USE_IJL
#include <ijl.h>
#endif

struct MooJpegProperties
{
	const char* szFileName;
	int nWidth;
	int nHeight;
	void* pBuffer;
	int nJQuality; // Jpeg quality (percentage)
#ifdef D2MOO_USE_IJL
	JPEG_CORE_PROPERTIES tJPEGProperties;
#else
	void* pInternalContext;
#endif
};

bool D2MooJpegLibInit(MooJpegProperties* pProperties);
bool D2MooJpegLibFree(MooJpegProperties* pProperties);
bool D2MooJpegLibWrite(MooJpegProperties* pProperties);
