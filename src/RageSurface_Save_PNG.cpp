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
    bool converted = RageSurfaceUtils::ConvertSurface(pImg, res, pImg->w, pImg->h, 32,
					Swap32BE(0xFF000000),
					Swap32BE(0x00FF0000),
					Swap32BE(0x0000FF00),
					Swap32BE(0x000000FF));

    if (!converted) res = pImg;

    // stride_in_bytes is image width in bytes
    bool success = 0 != stbi_write_png(f.GetRealPath(), res->w, res->h, 4, res->pixels, res->w * 4);
    if (converted) delete res; // If we converted then we created a new surface which we need to delete

    return success;
}
