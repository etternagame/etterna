#include "global.h"
#include "RageFile.h"
#include "RageLog.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurface_Save_PNG.h"
#include "RageUtil.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Note: This function is only called on RageDisplay.cpp:978 and the sError result is not used in the response.
bool RageSurfaceUtils::SavePNG(RageSurface* pImg, RageFile& f, RString& sError) {
    // Functions from "stb_image_write.h" return 0 on failure
    f.Close(); // The RageFile reference is already opened. Should be closed for following function to succeed.

    RageSurface* res;
    RageSurfaceUtils::ConvertSurface(pImg, res, pImg->w, pImg->h, 32,
					Swap32BE(0xFF000000),
					Swap32BE(0x00FF0000),
					Swap32BE(0x0000FF00),
					Swap32BE(0x000000FF));

    return 0 != stbi_write_png(f.GetRealPath(), pImg->w, pImg->h, 4, res->pixels, pImg->w * 4);
}
