#include "Etterna/Globals/global.h"
#include "RageUtil/File/RageFile.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurface_Save_BMP.h"
#include "RageUtil/Utils/RageUtil.h"

static void
WriteBytes(RageFile& f, std::string& sError, const void* buf, int size)
{
	if (!sError.empty())
		return;

	const auto ret = f.Write(buf, size);
	if (ret == -1)
		sError = f.GetError();
}

static void
write_le16(RageFile& f, std::string& sError, uint16_t val)
{
	val = Swap16LE(val);
	WriteBytes(f, sError, &val, sizeof(uint16_t));
}

static void
write_le32(RageFile& f, std::string& sError, uint32_t val)
{
	val = Swap32LE(val);
	WriteBytes(f, sError, &val, sizeof(uint32_t));
}

bool
RageSurfaceUtils::SaveBMP(RageSurface* surface, RageFile& f)
{
	/* Convert the surface to 24bpp. */
	const auto converted_surface = CreateSurface(surface->w,
												 surface->h,
												 24,
												 Swap24LE(0xFF0000),
												 Swap24LE(0x00FF00),
												 Swap24LE(0x0000FF),
												 0);
	CopySurface(surface, converted_surface);

	std::string sError;

	auto iFilePitch = converted_surface->pitch;
	iFilePitch = (iFilePitch + 3) & ~3; // round up a multiple of 4

	const auto iDataSize = converted_surface->h * iFilePitch;
	const auto iHeaderSize = 0x36;

	WriteBytes(f, sError, "BM", 2);
	write_le32(f, sError, iHeaderSize + iDataSize); // size (offset 0x2)
	write_le32(f, sError, 0);						// reserved (offset 0x6)
	write_le32(f, sError, iHeaderSize); // bitmap offset (offset 0xA)
	write_le32(f, sError, 0x28);		// header size (offset 0xE)
	write_le32(f, sError, surface->w);	// width (offset 0x14)
	write_le32(f, sError, surface->h);	// height (offset 0x18)
	write_le16(f, sError, 1);			// planes (offset 0x1A)
	write_le16(f,
			   sError,
			   static_cast<uint16_t>(converted_surface->fmt.BytesPerPixel *
									 8)); // bpp (offset 0x1C)
	write_le32(f, sError, 0);			  // compression (offset 0x1E)
	write_le32(f, sError, iDataSize);	  // bitmap size (offset 0x22)
	write_le32(f, sError, 0);			  // horiz resolution (offset 0x26)
	write_le32(f, sError, 0);			  // vert resolution (offset 0x2A)
	write_le32(f, sError, 0);			  // colors (offset 0x2E)
	write_le32(f, sError, 0);			  // important colors (offset 0x32)

	for (auto y = converted_surface->h - 1; y >= 0; --y) {
		const uint8_t* pRow =
		  converted_surface->pixels + converted_surface->pitch * y;
		WriteBytes(f, sError, pRow, converted_surface->pitch);

		/* Pad the row to the pitch. */
		uint8_t padding[4] = { 0, 0, 0, 0 };
		WriteBytes(f, sError, padding, iFilePitch - converted_surface->pitch);
	}

	delete converted_surface;

	if (!sError.empty())
		return false;

	if (f.Flush() == -1)
		return false;

	return true;
}
