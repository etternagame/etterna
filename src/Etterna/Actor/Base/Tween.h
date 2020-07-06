#ifndef TWEEN_H
#define TWEEN_H

#include "Etterna/Models/Misc/EnumHelper.h"

struct lua_State;
using Lua = lua_State;

/** @brief The different tweening types available. */
enum TweenType
{
	TWEEN_LINEAR,	  /**< A linear tween. */
	TWEEN_ACCELERATE, /**< An accelerating tween. */
	TWEEN_DECELERATE, /**< A decelerating tween. */
	TWEEN_SPRING,	  /**< A spring tween. */
	TWEEN_BEZIER,	  /**< A bezier tween. */
	NUM_TweenType,	  /**< The number of tween types. */
	TweenType_Invalid
};
/** @brief A custom foreach loop iterating through the tween types. */
#define FOREACH_TweenType(tt) FOREACH_ENUM(TweenType, tt)
LuaDeclareType(TweenType);

/**
 * @brief The interface for simple interpolation.
 *
 * Funny enough, this is a class. */
class ITween
{
  public:
	/** @brief Create the initial interface. */
	virtual ~ITween() = default;
	[[nodiscard]] virtual auto Tween(float f) const -> float = 0;
	[[nodiscard]] virtual auto Copy() const -> ITween* = 0;

	static auto CreateFromType(TweenType iType) -> ITween*;
	static auto CreateFromStack(Lua* L, int iStackPos) -> ITween*;
};

#endif
