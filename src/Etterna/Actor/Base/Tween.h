#ifndef TWEEN_H
#define TWEEN_H

#include "Etterna/Models/Misc/EnumHelper.h"

struct lua_State;
using Lua = lua_State;

/** @brief The different tweening types available. */
enum TweenType
{
	TWEEN_LINEAR,	 /**< A linear tween. */
	TWEEN_ACCELERATE, /**< An accelerating tween. */
	TWEEN_DECELERATE, /**< A decelerating tween. */
	TWEEN_SPRING,	 /**< A spring tween. */
	TWEEN_BEZIER,	 /**< A bezier tween. */
	NUM_TweenType,	/**< The number of tween types. */
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
	virtual float Tween(float f) const = 0;
	virtual ITween* Copy() const = 0;

	static ITween* CreateFromType(TweenType iType);
	static ITween* CreateFromStack(Lua* L, int iStackPos);
};

#endif
