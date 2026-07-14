#include "JpegLibraryWrapper.h"

#ifdef D2MOO_USE_IJL

bool D2MooJpegLibInit(MooJpegProperties* pProperties)
{
	return IJL_OK == ijlInit(&pProperties->tJPEGProperties);
}
bool D2MooJpegLibFree(MooJpegProperties* pProperties)
{
    return IJL_OK == ijlFree(&pProperties->tJPEGProperties);
}
bool D2MooJpegLibWrite(MooJpegProperties* pProperties)
{
    pProperties->tJPEGProperties.DIBPadBytes = ((3 * pProperties->nWidth + 3) & ~3u) - 3 * pProperties->nWidth;
    pProperties->tJPEGProperties.DIBWidth = pProperties->nWidth;
    pProperties->tJPEGProperties.DIBHeight = pProperties->nHeight;
    pProperties->tJPEGProperties.DIBBytes = pProperties->pBuffer;
    pProperties->tJPEGProperties.DIBColor = IJL_RGB;
    pProperties->tJPEGProperties.JPGFile = pProperties->szFileName;
    pProperties->tJPEGProperties.JPGWidth = pProperties->nWidth;
    pProperties->tJPEGProperties.JPGHeight = pProperties->nHeight;
    pProperties->tJPEGProperties.jquality = pProperties->nJQuality;
    return IJL_OK == ijlWrite(&pProperties->tJPEGProperties, IJL_JFILE_WRITEWHOLEIMAGE);
}

#else

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

bool D2MooJpegLibInit(MooJpegProperties* pProperties)
{
    // Noop
    return true;
}
bool D2MooJpegLibFree(MooJpegProperties* pProperties)
{
    // Noop
    return true;
}
bool D2MooJpegLibWrite(MooJpegProperties* pProperties)
{
    return 0 != stbi_write_jpg(pProperties->szFileName, pProperties->nWidth, pProperties->nHeight, 3, pProperties->pBuffer, pProperties->nJQuality);
}

#endif
