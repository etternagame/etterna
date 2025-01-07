#pragma once
#include "../../PatternModHelpers.h"

struct HandSwitchMod
{
	auto operator()(const metaItvGenericHandInfo& mitvghi)
	{
		return 0.1;
	}
};
