#include "global.h"
#include "RageFile.h"
#include "RageLog.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurface_Save_PNG.h"
#include "RageUtil.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Note: This funnction is only called on RageDisplay.cpp:978 and the sError result is not used in the response.
bool RageSurfaceUtils::SavePNG(RageSurface* pImg, RageFile& f, RString& sError) {
    // Functions from "stb_image_write.h" return 0 on success
    f.Close(); // The RageFile reference is already opened. Should be closed for following function to succeed.
    return 0 != stbi_write_png(f.GetRealPath(), pImg->w, pImg->h, 4, pImg->pixels, pImg->w * 4);
}