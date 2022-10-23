#include "Etterna/Globals/global.h"

#include "Etterna/Models/Misc/DisplaySpec.h"
#include "RageDisplay.h"
#include "RageDisplay_Null.h"
#include "Core/Services/Locator.hpp"
#include "RageSurface.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"

static RageDisplay::RagePixelFormatDesc
  PIXEL_FORMAT_DESC[NUM_RagePixelFormat] = {
	  { /* R8G8B8A8 */
		32,
		{ 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF } },
	  {
		/* R4G4B4A4 */
		16,
		{ 0xF000, 0x0F00, 0x00F0, 0x000F },
	  },
	  {
		/* R5G5B5A1 */
		16,
		{ 0xF800, 0x07C0, 0x003E, 0x0001 },
	  },
	  {
		/* R5G5B5 */
		16,
		{ 0xF800, 0x07C0, 0x003E, 0x0000 },
	  },
	  { /* R8G8B8 */
		24,
		{ 0xFF0000, 0x00FF00, 0x0000FF, 0x000000 } },
	  {
		/* Paletted */
		8,
		{ 0, 0, 0, 0 } /* N/A */
	  },
	  { /* B8G8R8A8 */
		24,
		{ 0x0000FF, 0x00FF00, 0xFF0000, 0x000000 } },
	  {
		/* A1B5G5R5 */
		16,
		{ 0x7C00, 0x03E0, 0x001F, 0x8000 },
	  }
  };

RageDisplay_Null::RageDisplay_Null()
{
	Locator::getLogger()->info("Current renderer: null");
}

std::string
RageDisplay_Null::Init(VideoModeParams&& p,
					   bool /* bAllowUnacceleratedRenderer */)
{
	auto bIgnore = false;
	SetVideoMode(std::move(p), bIgnore);
	return std::string();
}

void
RageDisplay_Null::GetDisplaySpecs(DisplaySpecs& out) const
{
	out.clear();
	const DisplayMode nullMode = { 640U, 480U, 30.0 };
	const DisplaySpec nullSpec("NullDisplay", "NullDisplay", nullMode);
	out.insert(nullSpec);
}

RageSurface*
RageDisplay_Null::CreateScreenshot()
{
	const auto& desc = PIXEL_FORMAT_DESC[RagePixelFormat_RGB8];
	const auto image = CreateSurface(640,
									 480,
									 desc.bpp,
									 desc.masks[0],
									 desc.masks[1],
									 desc.masks[2],
									 desc.masks[3]);

	memset(image->pixels, 0, 480 * image->pitch);

	return image;
}

const RageDisplay::RagePixelFormatDesc*
RageDisplay_Null::GetPixelFormatDesc(RagePixelFormat pf) const
{
	ASSERT(pf >= 0 && pf < NUM_RagePixelFormat);
	return &PIXEL_FORMAT_DESC[pf];
}

RageMatrix
RageDisplay_Null::GetOrthoMatrix(float l,
								 float r,
								 float b,
								 float t,
								 float zn,
								 float zf)
{
	RageMatrix m(2 / (r - l),
				 0,
				 0,
				 0,
				 0,
				 2 / (t - b),
				 0,
				 0,
				 0,
				 0,
				 -2 / (zf - zn),
				 0,
				 -(r + l) / (r - l),
				 -(t + b) / (t - b),
				 -(zf + zn) / (zf - zn),
				 1);
	return m;
}

void
RageDisplay_Null::EndFrame()
{
	ProcessStatsOnFlip();
}

bool
RageDisplay_Null::IsD3DInternal()
{
	return false;
}

class RageCompiledGeometryNull : public RageCompiledGeometry
{
  public:
	void Allocate(const std::vector<msMesh>&) override {}
	void Change(const std::vector<msMesh>&) override {}
	void Draw(int iMeshIndex) const override {}
};

RageCompiledGeometry*
RageDisplay_Null::CreateCompiledGeometry()
{
	return new RageCompiledGeometryNull;
}

void
RageDisplay_Null::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
}
