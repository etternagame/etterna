#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

void
RageColor::PushTable(lua_State* L) const
{
	lua_newtable(L);
	const auto iTable = lua_gettop(L);

	lua_pushnumber(L, r);
	lua_rawseti(L, iTable, 1);
	lua_pushnumber(L, g);
	lua_rawseti(L, iTable, 2);
	lua_pushnumber(L, b);
	lua_rawseti(L, iTable, 3);
	lua_pushnumber(L, a);
	lua_rawseti(L, iTable, 4);
}

void
RageColor::FromStack(lua_State* L, int iPos)
{
	if (lua_type(L, iPos) != LUA_TTABLE)
		return;

	lua_pushvalue(L, iPos);
	const auto iFrom = lua_gettop(L);

	lua_rawgeti(L, iFrom, 1);
	r = static_cast<float>(lua_tonumber(L, -1));
	lua_rawgeti(L, iFrom, 2);
	g = static_cast<float>(lua_tonumber(L, -1));
	lua_rawgeti(L, iFrom, 3);
	b = static_cast<float>(lua_tonumber(L, -1));
	lua_rawgeti(L, iFrom, 4);
	a = static_cast<float>(lua_tonumber(L, -1));
	lua_pop(L, 5);
}

void
RageColor::FromStackCompat(lua_State* L, int iPos)
{
	if (lua_type(L, iPos) == LUA_TTABLE) {
		FromStack(L, iPos);
	} else {
		r = FArg(iPos + 0);
		g = FArg(iPos + 1);
		b = FArg(iPos + 2);
		a = FArg(iPos + 3);
	}
}

std::string
RageColor::ToString() const
{
	const auto iR = std::clamp(static_cast<int>(r * 255), 0, 255);
	const auto iG = std::clamp(static_cast<int>(g * 255), 0, 255);
	const auto iB = std::clamp(static_cast<int>(b * 255), 0, 255);
	const auto iA = std::clamp(static_cast<int>(a * 255), 0, 255);

	if (iA == 255)
		return ssprintf("#%02X%02X%02X", iR, iG, iB);

	return ssprintf("#%02X%02X%02X%02X", iR, iG, iB, iA);
}

std::string
RageColor::NormalizeColorString(const std::string& sColor)
{
	if (sColor.empty())
		return "";
	RageColor c;
	if (!c.FromString(sColor))
		return "";
	return c.ToString();
}

RageColor
RageColor::Lerp(RageColor const& a, RageColor const& b, float t)
{
	return RageColor(
		lerp(t, a.r, b.r),
		lerp(t, a.g, b.g),
		lerp(t, a.b, b.b),
		lerp(t, a.a, b.a)
	);
}

RageColor
RageColor::FromHue(float hue)
{
	hue = std::remainder(hue, 360);

	auto red = RageColor(1, 0, 0, 0);
	auto yellow = RageColor(1, 1, 0, 0);
	auto green = RageColor(0, 1, 0, 0);
	auto cyan = RageColor(0, 1, 1, 0);
	auto blue = RageColor(0, 0, 1, 0);
	auto magenta = RageColor(1, 0, 1, 0);

	if (hue < 60) {
		return RageColor::Lerp(red, yellow, hue / 60);
	} else if (hue < 120) {
		return RageColor::Lerp(yellow, green, (hue - 60) / 60);
	} else if (hue < 180) {
		return RageColor::Lerp(green, cyan, (hue - 120) / 60);
	} else if (hue < 240) {
		return RageColor::Lerp(cyan, blue, (hue - 180) / 60);
	} else if (hue < 300) {
		return RageColor::Lerp(blue, magenta, (hue - 240) / 60);
	} else {
		return RageColor::Lerp(magenta, red, (hue - 300) / 60);
	}
}

void
WeightedAvergeOfRSVs(RageSpriteVertex& average_out,
					 RageSpriteVertex const& rsv1,
					 RageSpriteVertex const& rsv2,
					 float percent_between)
{
	average_out.p = lerp(percent_between, rsv1.p, rsv2.p);
	average_out.n = lerp(percent_between, rsv1.n, rsv2.n);
	average_out.c.b = lerp(percent_between, rsv1.c.b, rsv2.c.b);
	average_out.c.g = lerp(percent_between, rsv1.c.g, rsv2.c.g);
	average_out.c.r = lerp(percent_between, rsv1.c.r, rsv2.c.r);
	average_out.c.a = lerp(percent_between, rsv1.c.a, rsv2.c.a);
	average_out.t = lerp(percent_between, rsv1.t, rsv2.t);
}

/** @brief Utilities for working with Lua. */
namespace LuaHelpers {
template<>
bool
FromStack<RageColor>(lua_State* L, RageColor& Object, int iOffset)
{
	Object.FromStack(L, iOffset);
	return true;
}
template<>
void
Push<RageColor>(lua_State* L, const RageColor& Object)
{
	Object.PushTable(L);
}
}

static const char* CullModeNames[] = { "Back", "Front", "None" };
XToString(CullMode);
LuaXType(CullMode);

static const char* BlendModeNames[] = {
	"Normal",
	"Add",
	"Subtract",
	"Modulate",

	/*
	 * Copy the source directly to the destination.  Alpha is not premultiplied.
	 *
	 * Co = Cs
	 * Ao = As
	 */
	"CopySrc",

	/*
	 * Leave the color alone, and apply the source alpha on top of the existing
	 * alpha. Transparent areas in the source become transparent in the
	 * destination.
	 *
	 * Be sure to disable alpha test with this blend mode.
	 *
	 * Co = Cd
	 * Ao = Ad*As
	 */
	"AlphaMask",

	/*
	 * Leave the color alone, and apply the source alpha on top of the existing
	 * alpha. Transparent areas in the source become transparent in the
	 * destination. Co = Cd Ao = Ad*(1-As)
	 */
	"AlphaKnockOut",
	"AlphaMultiply",
	"WeightedMultiply",
	"InvertDest",
	"NoEffect"
};
XToString(BlendMode);
StringToX(BlendMode);
LuaXType(BlendMode);

static const char* TextureModeNames[] = {
	"Modulate",
	"Glow",
	"Add",
};
XToString(TextureMode);
LuaXType(TextureMode);

static const char* EffectModeNames[] = {
	/* Normal blending.  All supported texture modes have their standard
	   effects. */
	"Normal",

	/* After rendering to a destination alpha render target, the color will be
	 * premultiplied with its alpha.  An Unpremultiply pass with CopySrc
	 * blending must be performed to correct this. */
	"Unpremultiply",

	/* Layered blending.    These shaders take two source textures. */
	"ColorBurn",
	"ColorDodge",
	"VividLight",
	"HardMix",
	"Overlay",
	"Screen",

	"YUYV422"
};
XToString(EffectMode);
LuaXType(EffectMode);

static const char* ZTestModeNames[] = { "Off", "WriteOnPass", "WriteOnFail" };
XToString(ZTestMode);
LuaXType(ZTestMode);

static const char* TextGlowModeNames[] = { "Inner", "Stroke", "Both" };
XToString(TextGlowMode);
LuaXType(TextGlowMode);

int
LuaFunc_color(lua_State* L)
{
	const std::string sColor = SArg(1);
	RageColor c;
	c.FromString(sColor);
	c.PushTable(L);
	return 1;
}
LUAFUNC_REGISTER_COMMON(color);

int
LuaFunc_lerp_color(lua_State* L)
{
	// Args:  percent, color, color
	// Returns:  color
	const auto percent = FArg(1);
	RageColor a, b;
	a.FromStack(L, 2);
	b.FromStack(L, 3);
	RageColor::Lerp(a, b, percent).PushTable(L);
	return 1;
}
LUAFUNC_REGISTER_COMMON(lerp_color);
