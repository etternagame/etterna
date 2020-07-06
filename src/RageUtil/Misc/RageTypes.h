/* RageTypes - vector and matrix types. */

#ifndef RAGETYPES_H
#define RAGETYPES_H

#include "Etterna/Models/Misc/EnumHelper.h"

enum BlendMode
{
	BLEND_NORMAL,
	BLEND_ADD,
	BLEND_SUBTRACT,
	BLEND_MODULATE,
	BLEND_COPY_SRC,
	BLEND_ALPHA_MASK,
	BLEND_ALPHA_KNOCK_OUT,
	BLEND_ALPHA_MULTIPLY,
	BLEND_WEIGHTED_MULTIPLY,
	BLEND_INVERT_DEST,
	BLEND_NO_EFFECT,
	NUM_BlendMode,
	BlendMode_Invalid
};
LuaDeclareType(BlendMode);

enum TextureMode
{
	// Affects one texture stage. Texture is modulated with the diffuse color.
	TextureMode_Modulate,

	/* Affects one texture stage. Color is replaced with white, leaving alpha.
	 * Used with BLEND_ADD to add glow. */
	TextureMode_Glow,

	// Affects one texture stage. Color is added to the previous texture stage.
	TextureMode_Add,

	NUM_TextureMode,
	TextureMode_Invalid
};
LuaDeclareType(TextureMode);

enum EffectMode
{
	EffectMode_Normal,
	EffectMode_Unpremultiply,
	EffectMode_ColorBurn,
	EffectMode_ColorDodge,
	EffectMode_VividLight,
	EffectMode_HardMix,
	EffectMode_Overlay,
	EffectMode_Screen,
	EffectMode_YUYV422,
	NUM_EffectMode,
	EffectMode_Invalid
};
LuaDeclareType(EffectMode);

enum CullMode
{
	CULL_BACK,
	CULL_FRONT,
	CULL_NONE,
	NUM_CullMode,
	CullMode_Invalid
};
LuaDeclareType(CullMode);

enum ZTestMode
{
	ZTEST_OFF,
	ZTEST_WRITE_ON_PASS,
	ZTEST_WRITE_ON_FAIL,
	NUM_ZTestMode,
	ZTestMode_Invalid
};
LuaDeclareType(ZTestMode);

enum PolygonMode
{
	POLYGON_FILL,
	POLYGON_LINE,
	NUM_PolygonMode,
	PolygonMode_Invalid
};
LuaDeclareType(PolygonMode);

enum TextGlowMode
{
	TextGlowMode_Inner,
	TextGlowMode_Stroke,
	TextGlowMode_Both,
	NUM_TextGlowMode,
	TextGlowMode_Invalid
};
LuaDeclareType(TextGlowMode);

struct lua_State;

struct RageVector2
{
  public:
	RageVector2() = default;
	RageVector2(const float* f)
	  : x(f[0])
	  , y(f[1])
	{
	}
	RageVector2(float x1, float y1)
	  : x(x1)
	  , y(y1)
	{
	}

	// casting
	operator float*() { return &x; };
	operator const float*() const { return &x; };

	// assignment operators
	auto operator+=(const RageVector2& other) -> RageVector2&
	{
		x += other.x;
		y += other.y;
		return *this;
	}
	auto operator-=(const RageVector2& other) -> RageVector2&
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}
	auto operator*=(float f) -> RageVector2&
	{
		x *= f;
		y *= f;
		return *this;
	}
	auto operator/=(float f) -> RageVector2&
	{
		x /= f;
		y /= f;
		return *this;
	}

	// binary operators
	auto operator+(const RageVector2& other) const -> RageVector2
	{
		return RageVector2(x + other.x, y + other.y);
	}
	auto operator-(const RageVector2& other) const -> RageVector2
	{
		return RageVector2(x - other.x, y - other.y);
	}
	auto operator*(float f) const -> RageVector2
	{
		return RageVector2(x * f, y * f);
	}
	auto operator/(float f) const -> RageVector2
	{
		return RageVector2(x / f, y / f);
	}

	friend auto operator*(float f, const RageVector2& other) -> RageVector2
	{
		return other * f;
	}

	float x{ 0 }, y{ 0 };
};

struct RageVector3
{
  public:
	RageVector3() = default;
	RageVector3(const float* f)
	  : x(f[0])
	  , y(f[1])
	  , z(f[2])
	{
	}
	RageVector3(float x1, float y1, float z1)
	  : x(x1)
	  , y(y1)
	  , z(z1)
	{
	}

	// casting
	operator float*() { return &x; };
	operator const float*() const { return &x; };

	// assignment operators
	auto operator+=(const RageVector3& other) -> RageVector3&
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}
	auto operator-=(const RageVector3& other) -> RageVector3&
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}
	auto operator*=(float f) -> RageVector3&
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}
	auto operator/=(float f) -> RageVector3&
	{
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	// binary operators
	auto operator+(const RageVector3& other) const -> RageVector3
	{
		return RageVector3(x + other.x, y + other.y, z + other.z);
	}
	auto operator-(const RageVector3& other) const -> RageVector3
	{
		return RageVector3(x - other.x, y - other.y, z - other.z);
	}
	auto operator*(float f) const -> RageVector3
	{
		return RageVector3(x * f, y * f, z * f);
	}
	auto operator/(float f) const -> RageVector3
	{
		return RageVector3(x / f, y / f, z / f);
	}

	friend auto operator*(float f, const RageVector3& other) -> RageVector3
	{
		return other * f;
	}

	float x{ 0 }, y{ 0 }, z{ 0 };
};

struct RageVector4
{
  public:
	RageVector4() = default;
	RageVector4(const float* f)
	  : x(f[0])
	  , y(f[1])
	  , z(f[2])
	  , w(f[3])
	{
	}
	RageVector4(float x1, float y1, float z1, float w1)
	  : x(x1)
	  , y(y1)
	  , z(z1)
	  , w(w1)
	{
	}

	// casting
	operator float*() { return &x; };
	operator const float*() const { return &x; };

	// assignment operators
	auto operator+=(const RageVector4& other) -> RageVector4&
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}
	auto operator-=(const RageVector4& other) -> RageVector4&
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;
		return *this;
	}
	auto operator*=(float f) -> RageVector4&
	{
		x *= f;
		y *= f;
		z *= f;
		w *= f;
		return *this;
	}
	auto operator/=(float f) -> RageVector4&
	{
		x /= f;
		y /= f;
		z /= f;
		w /= f;
		return *this;
	}

	// binary operators
	auto operator+(const RageVector4& other) const -> RageVector4
	{
		return RageVector4(x + other.x, y + other.y, z + other.z, w + other.w);
	}
	auto operator-(const RageVector4& other) const -> RageVector4
	{
		return RageVector4(x - other.x, y - other.y, z - other.z, w - other.w);
	}
	auto operator*(float f) const -> RageVector4
	{
		return RageVector4(x * f, y * f, z * f, w * f);
	}
	auto operator/(float f) const -> RageVector4
	{
		return RageVector4(x / f, y / f, z / f, w / f);
	}

	friend auto operator*(float f, const RageVector4& other) -> RageVector4
	{
		return other * f;
	}

	float x{ 0 }, y{ 0 }, z{ 0 }, w{ 0 };
};

struct RageColor
{
  public:
	RageColor() = default;
	explicit RageColor(const float* f)
	  : r(f[0])
	  , g(f[1])
	  , b(f[2])
	  , a(f[3])
	{
	}
	RageColor(float r1, float g1, float b1, float a1)
	  : r(r1)
	  , g(g1)
	  , b(b1)
	  , a(a1)
	{
	}

	// casting
	operator float*() { return &r; };
	operator const float*() const { return &r; };

	// assignment operators
	auto operator+=(const RageColor& other) -> RageColor&
	{
		r += other.r;
		g += other.g;
		b += other.b;
		a += other.a;
		return *this;
	}
	auto operator-=(const RageColor& other) -> RageColor&
	{
		r -= other.r;
		g -= other.g;
		b -= other.b;
		a -= other.a;
		return *this;
	}
	auto operator*=(const RageColor& other) -> RageColor&
	{
		r *= other.r;
		g *= other.g;
		b *= other.b;
		a *= other.a;
		return *this;
	}
	auto operator*=(float f) -> RageColor&
	{
		r *= f;
		g *= f;
		b *= f;
		a *= f;
		return *this;
	}
	/* Divide is rarely useful: you can always use multiplication, and you don't
	 * have to worry about div/0. */
	//    RageColor& operator /= ( float f )		{ r/=f; g/=f; b/=f; a/=f;
	//    return *this; }

	// binary operators
	auto operator+(const RageColor& other) const -> RageColor
	{
		return RageColor(r + other.r, g + other.g, b + other.b, a + other.a);
	}
	auto operator-(const RageColor& other) const -> RageColor
	{
		return RageColor(r - other.r, g - other.g, b - other.b, a - other.a);
	}
	auto operator*(const RageColor& other) const -> RageColor
	{
		return RageColor(r * other.r, g * other.g, b * other.b, a * other.a);
	}
	auto operator*(float f) const -> RageColor
	{
		return RageColor(r * f, g * f, b * f, a * f);
	}
	// Divide is useful for using with the SCALE macro
	auto operator/(float f) const -> RageColor
	{
		return RageColor(r / f, g / f, b / f, a / f);
	}

	friend auto operator*(float f, const RageColor& other) -> RageColor
	{
		return other * f;
	} // What is this for?  Did I add this?  -Chris

	auto operator==(const RageColor& other) const -> bool
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}
	auto operator!=(const RageColor& other) const -> bool
	{
		return !operator==(other);
	}

	auto FromString(const std::string& str) -> bool
	{
		int result = sscanf(str.c_str(), "%f,%f,%f,%f", &r, &g, &b, &a);
		if (result == 3) {
			a = 1;
			return true;
		}
		if (result == 4) {
			return true;
		}

		int ir = 255;
		int ib = 255;
		int ig = 255;
		int ia = 255;
		result = sscanf(str.c_str(), "#%2x%2x%2x%2x", &ir, &ig, &ib, &ia);
		if (result >= 3) {
			r = ir / 255.0F;
			g = ig / 255.0F;
			b = ib / 255.0F;
			if (result == 4) {
				a = ia / 255.0F;
			} else {
				a = 1;
			}
			return true;
		}

		r = 1;
		b = 1;
		g = 1;
		a = 1;
		return false;
	}

	[[nodiscard]] auto ToString() const -> std::string;
	static auto NormalizeColorString(const std::string& sColor) -> std::string;

	void PushTable(lua_State* L) const;
	void FromStack(lua_State* L, int iPos);
	void FromStackCompat(lua_State* L, int iPos);

	float r{ 0 }, g{ 0 }, b{ 0 }, a{ 0 };
};

static auto
FTOC(float a) -> unsigned char
{
	auto ret = static_cast<int>(a * 256.F);
	CLAMP(ret, 0, 255);
	return static_cast<unsigned char>(ret);
}

/* Color type used only in vertex lists.  OpenGL expects colors in
 * r, g, b, a order, independent of endianness, so storing them this
 * way avoids endianness problems.  Don't try to manipulate this; only
 * manip RageColors. */
class RageVColor
{
  public:
	uint8_t b{ 0 }, g{ 0 }, r{ 0 },
	  a{ 0 }; // specific ordering required by Direct3D
	RageVColor() = default;
	RageVColor(const RageColor& rc)
	{
		r = FTOC(rc.r);
		g = FTOC(rc.g);
		b = FTOC(rc.b);
		a = FTOC(rc.a);
	}
};

namespace StepMania {
template<class T>
class Rect
{
  public:
	Rect()
	  : left(0)
	  , top(0)
	  , right(0)
	  , bottom(0)
	{
	}
	Rect(T l, T t, T r, T b)
	  : left(l)
	  , top(t)
	  , right(r)
	  , bottom(b)
	{
	}

	[[nodiscard]] auto GetWidth() const -> T { return right - left; };
	[[nodiscard]] auto GetHeight() const -> T { return bottom - top; };
	[[nodiscard]] auto GetCenterX() const -> T { return (left + right) / 2; };
	[[nodiscard]] auto GetCenterY() const -> T { return (top + bottom) / 2; };

	auto operator==(const Rect& other) const -> bool
	{
#define COMPARE(x)                                                             \
	if ((x) != other.x)                                                        \
	return false
		COMPARE(left);
		COMPARE(top);
		COMPARE(right);
		COMPARE(bottom);
#undef COMPARE
		return true;
	}
	auto operator!=(const Rect& other) const -> bool
	{
		return !operator==(other);
	}

	T left, top, right, bottom;
};
} // namespace StepMania
using RectI = StepMania::Rect<int>;
using RectF = StepMania::Rect<float>;

/* Structure for our custom vertex type.  Note that these data structes
 * have the same layout that D3D expects. */
struct RageSpriteVertex // has color
{
	RageVector3 p; // position
	RageVector3 n; // normal
	RageVColor c;  // diffuse color
	RageVector2 t; // texture coordinates
};

void
lerp_rage_color(RageColor& out,
				RageColor const& a,
				RageColor const& b,
				float t);
void
WeightedAvergeOfRSVs(RageSpriteVertex& average_out,
					 RageSpriteVertex const& rsv1,
					 RageSpriteVertex const& rsv2,
					 float percent_between);

struct RageModelVertex // doesn't have color.  Relies on material color
{
	/* Zero out by default. */
	RageModelVertex()
	  : p(0, 0, 0)
	  , n(0, 0, 0)
	  , t(0, 0)
	  , TextureMatrixScale(1, 1)
	{
	}
	RageVector3 p; // position
	RageVector3 n; // normal
	RageVector2 t; // texture coordinates
	int8_t bone{ 0 };
	RageVector2 TextureMatrixScale; // usually 1,1
};

// RageMatrix elements are specified in row-major order.  This
// means that the translate terms are located in the fourth row and the
// projection terms in the fourth column.  This is consistent with the way
// MAX, Direct3D, and OpenGL all handle matrices.  Even though the OpenGL
// documentation is in column-major form, the OpenGL code is designed to
// handle matrix operations in row-major form.
struct RageMatrix
{
  public:
	RageMatrix() = default;
	RageMatrix(const float* f)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				m[j][i] = f[j * 4 + i];
			}
		}
	}
	RageMatrix(const RageMatrix& other)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				m[j][i] = other.m[j][i];
			}
		}
	}
	RageMatrix(float v00,
			   float v01,
			   float v02,
			   float v03,
			   float v10,
			   float v11,
			   float v12,
			   float v13,
			   float v20,
			   float v21,
			   float v22,
			   float v23,
			   float v30,
			   float v31,
			   float v32,
			   float v33);

	// access grants
	auto operator()(int iRow, int iCol) -> float& { return m[iCol][iRow]; }
	auto operator()(int iRow, int iCol) const -> float { return m[iCol][iRow]; }

	// casting operators
	operator float*() { return m[0]; }
	operator const float*() const { return m[0]; }

	[[nodiscard]] auto GetTranspose() const -> RageMatrix;

	float m[4][4]{};
};

#endif
