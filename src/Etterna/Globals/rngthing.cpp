#include "rngthing.h"
#include <RageUtil/Utils/RageUtil.h>
#include "Etterna/Models/Lua/LuaBinding.h"

#include <ctime>

RandomGen g_RandomNumberGenerator;

/* Extend MersenneTwister into Lua space. This is intended to replace
 * math.randomseed and math.random, so we conform to their behavior. */

namespace {
RandomGen g_LuaPRNG;

/* To map from [0..2^32-1] to [0..1), we divide by 2^32. */
const double DIVISOR = 4294967296.0;

static int
Seed(lua_State* L)
{
	g_LuaPRNG.seed(IArg(1));
	return 0;
}

static int
Random(lua_State* L)
{
	switch (lua_gettop(L)) {
		/* [0..1) */
		case 0: {
			const auto r = static_cast<double>(g_LuaPRNG()) / DIVISOR;
			lua_pushnumber(L, r);
			return 1;
		}

		/* [1..u] */
		case 1: {
			const auto upper = IArg(1);
			luaL_argcheck(L, 1 <= upper, 1, "interval is empty");
			lua_pushnumber(L, random_up_to(g_LuaPRNG, upper) + 1);
			return 1;
		}

		/* [l..u] */
		case 2: {
			const auto lower = IArg(1);
			const auto upper = IArg(2);
			luaL_argcheck(L, lower < upper, 2, "interval is empty");
			lua_pushnumber(L,
						   random_up_to(g_LuaPRNG, upper - lower + 1) + lower);
			return 1;
		}

		/* wrong amount of arguments */
		default: {
			return luaL_error(L, "wrong number of arguments");
		}
	}
}

const luaL_Reg MersenneTwisterTable[] = { LIST_METHOD(Seed),
										  LIST_METHOD(Random),
										  { nullptr, nullptr } };
} // namespace

LUA_REGISTER_NAMESPACE(MersenneTwister);

void
seed_lua_prng()
{
	g_LuaPRNG.seed(static_cast<unsigned int>(time(nullptr)));
}
