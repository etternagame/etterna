#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Misc/RageMath.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Tween.h"

static const char* TweenTypeNames[] = { "Linear",
										"Accelerate",
										"Decelerate",
										"Spring",
										"Bezier" };
XToString(TweenType);
LuaXType(TweenType);

struct TweenLinear : public ITween
{
	float Tween(float f) const override { return f; }
	ITween* Copy() const override { return new TweenLinear(*this); }
};
struct TweenAccelerate : public ITween
{
	float Tween(float f) const override { return f * f; }
	ITween* Copy() const override { return new TweenAccelerate(*this); }
};
struct TweenDecelerate : public ITween
{
	float Tween(float f) const override { return 1 - (1 - f) * (1 - f); }
	ITween* Copy() const override { return new TweenDecelerate(*this); }
};
struct TweenSpring : public ITween
{
	float Tween(float f) const override
	{
		return 1 - RageFastCos(f * PI * 2.5f) / (1 + f * 3);
	}
	ITween* Copy() const override { return new TweenSpring(*this); }
};

/*
 * Interpolation with 1-dimensional cubic Bezier curves.
 */
struct InterpolateBezier1D : public ITween
{
	float Tween(float f) const override;
	ITween* Copy() const override { return new InterpolateBezier1D(*this); }

	RageQuadratic m_Bezier;
};

float
InterpolateBezier1D::Tween(float f) const
{
	return m_Bezier.Evaluate(f);
}

/*
 * Interpolation with 2-dimensional cubic Bezier curves.
 */
struct InterpolateBezier2D : public ITween
{
	float Tween(float f) const override;
	ITween* Copy() const override { return new InterpolateBezier2D(*this); }

	RageBezier2D m_Bezier;
};

float
InterpolateBezier2D::Tween(float f) const
{
	return m_Bezier.EvaluateYFromX(f);
}

/* This interpolator combines multiple other interpolators, to allow
 * generating more complex tweens.  For example,
 * "compound,10,linear,0.25,accelerate,0.75" means 25% of the tween is linear,
 * and the rest is accelerate.  This can be used with Bezier to create spline
 * tweens. */
// InterpolateCompound

ITween*
ITween::CreateFromType(TweenType tt)
{
	switch (tt) {
		case TWEEN_LINEAR:
			return new TweenLinear;
		case TWEEN_ACCELERATE:
			return new TweenAccelerate;
		case TWEEN_DECELERATE:
			return new TweenDecelerate;
		case TWEEN_SPRING:
			return new TweenSpring;
		default:
			FAIL_M(ssprintf("Invalid TweenType: %i", tt));
	}
}

ITween*
ITween::CreateFromStack(Lua* L, int iStackPos)
{
	TweenType iType = Enum::Check<TweenType>(L, iStackPos);
	if (iType == TWEEN_BEZIER) {
		luaL_checktype(L, iStackPos + 1, LUA_TTABLE);
		int iArgs = lua_objlen(L, iStackPos + 1);
		if (iArgs != 4 && iArgs != 8) {
			LuaHelpers::ReportScriptErrorFmt("Tween::CreateFromStack: table "
											 "argument must have 4 or 8 "
											 "entries");
			return NULL;
		}

		float fC[8];
		for (int i = 0; i < iArgs; ++i) {
			lua_rawgeti(L, iStackPos + 1, i + 1);
			fC[i] = static_cast<float>(lua_tonumber(L, -1));
		}

		lua_pop(L, iArgs);
		if (iArgs == 4) {
			auto* pBezier = new InterpolateBezier1D;
			pBezier->m_Bezier.SetFromBezier(fC[0], fC[1], fC[2], fC[3]);
			return pBezier;
		} else if (iArgs == 8) {
			auto* pBezier = new InterpolateBezier2D;
			pBezier->m_Bezier.SetFromBezier(
			  fC[0], fC[1], fC[2], fC[3], fC[4], fC[5], fC[6], fC[7]);
			return pBezier;
		}
	}

	return CreateFromType(iType);
}
