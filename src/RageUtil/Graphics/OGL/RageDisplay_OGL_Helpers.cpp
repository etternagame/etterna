#include "Etterna/Globals/global.h"
#include "RageDisplay_OGL_Helpers.h"
#include "RageUtil/Utils/RageUtil.h"

#include <map>
#include <set>

namespace {
std::map<GLenum, std::string> g_Strings;
void
InitStringMap()
{
	static auto bInitialized = false;
	if (bInitialized)
		return;
	bInitialized = true;

#define X(a) g_Strings[a] = #a;
	X(GL_RGBA8);
	X(GL_RGBA4);
	X(GL_RGB5_A1);
	X(GL_RGB5);
	X(GL_RGBA);
	X(GL_RGB);
	X(GL_BGR);
	X(GL_BGRA);
	X(GL_COLOR_INDEX8_EXT);
	X(GL_COLOR_INDEX4_EXT);
	X(GL_COLOR_INDEX);
	X(GL_UNSIGNED_BYTE);
	X(GL_UNSIGNED_SHORT_4_4_4_4);
	X(GL_UNSIGNED_SHORT_5_5_5_1);
	X(GL_UNSIGNED_SHORT_1_5_5_5_REV);
	X(GL_INVALID_ENUM);
	X(GL_INVALID_VALUE);
	X(GL_INVALID_OPERATION);
	X(GL_STACK_OVERFLOW);
	X(GL_STACK_UNDERFLOW);
	X(GL_OUT_OF_MEMORY);
#undef X
}
};

void
RageDisplay_Legacy_Helpers::Init()
{
	InitStringMap();
}

std::string
RageDisplay_Legacy_Helpers::GLToString(GLenum e)
{
	if (g_Strings.find(e) != g_Strings.end())
		return g_Strings[e];

	return ssprintf("%i", static_cast<int>(e));
}

const RageDisplay_Legacy_Helpers::GLPixFmtInfo_t
  RageDisplay_Legacy_Helpers::g_GLPixFmtInfo[NUM_RagePixelFormat] = {
	  {
		/* R8G8B8A8 */
		GL_RGBA8,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
	  },
	  {
		/* R8G8B8A8 */
		GL_RGBA8,
		GL_BGRA,
		GL_UNSIGNED_BYTE,
	  },
	  {
		/* B4G4R4A4 */
		GL_RGBA4,
		GL_RGBA,
		GL_UNSIGNED_SHORT_4_4_4_4,
	  },
	  {
		/* B5G5R5A1 */
		GL_RGB5_A1,
		GL_RGBA,
		GL_UNSIGNED_SHORT_5_5_5_1,
	  },
	  {
		/* B5G5R5 */
		GL_RGB5,
		GL_RGBA,
		GL_UNSIGNED_SHORT_5_5_5_1,
	  },
	  {
		/* B8G8R8 */
		GL_RGB8,
		GL_RGB,
		GL_UNSIGNED_BYTE,
	  },
	  {
		/* Paletted */
		GL_COLOR_INDEX8_EXT,
		GL_COLOR_INDEX,
		GL_UNSIGNED_BYTE,
	  },
	  {
		/* B8G8R8 */
		GL_RGB8,
		GL_BGR,
		GL_UNSIGNED_BYTE,
	  },
	  {
		/* A1R5G5B5 (matches D3DFMT_A1R5G5B5) */
		GL_RGB5_A1,
		GL_BGRA,
		GL_UNSIGNED_SHORT_1_5_5_5_REV,
	  },
	  {
		/* X1R5G5B5 */
		GL_RGB5,
		GL_BGRA,
		GL_UNSIGNED_SHORT_1_5_5_5_REV,
	  }
  };

RageDisplay::RagePixelFormatDesc
  RageDisplay_Legacy_Helpers::PIXEL_FORMAT_DESC[NUM_RagePixelFormat] = {
	  { /* R8G8B8A8 */
		32,
		{ 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF } },
	  { /* B8G8R8A8 */
		32,
		{ 0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF } },
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
		/* R5G5B5X1 */
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
	  { /* B8G8R8 */
		24,
		{ 0x0000FF, 0x00FF00, 0xFF0000, 0x000000 } },
	  {
		/* A1R5G5B5 */
		16,
		{ 0x7C00, 0x03E0, 0x001F, 0x8000 },
	  },
	  {
		/* X1R5G5B5 */
		16,
		{ 0x7C00, 0x03E0, 0x001F, 0x0000 },
	  }
  };
