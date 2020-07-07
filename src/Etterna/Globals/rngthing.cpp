#include "rngthing.h"
#include <RageUtil/Utils/RageUtil.h>
#include "Etterna/Models/Lua/LuaBinding.h"

#include <ctime>

typedef std::mt19937 RandomGen;
// RandomGen g_RandomNumberGenerator;

inline auto
random_up_to(RandomGen& rng, int limit) -> int
{
	RandomGen::result_type res = rng();
	// Cutting off the incomplete [0,n) chunk at the max value makes the result
	// more evenly distributed. -Kyz
	RandomGen::result_type up_to_max =
	  RandomGen::max() - (RandomGen::max() % limit);
	while (res > up_to_max) {
		res = rng();
	}

	return static_cast<int>(res % limit);
}

inline auto
random_up_to(int limit) -> int
{
	return random_up_to(g_RandomNumberGenerator, limit);
}

inline auto
RandomFloat() -> float
{
	return static_cast<float>(g_RandomNumberGenerator() / 4294967296.0);
}

inline auto
RandomFloat(float fLow, float fHigh) -> float
{
	return SCALE(RandomFloat(), 0.0F, 1.0F, fLow, fHigh);
}

inline auto
RandomInt(int low, int high) -> int
{
	return random_up_to(g_RandomNumberGenerator, high - low + 1) + low;
}

inline auto
RandomInt(int n) -> int
{
	return random_up_to(g_RandomNumberGenerator, n);
}

inline auto
randomf(const float low, const float high) -> float
{
	return RandomFloat(low, high);
}

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
