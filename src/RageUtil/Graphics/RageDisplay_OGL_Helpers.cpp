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
