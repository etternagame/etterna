#include <random>

typedef std::mt19937 RandomGen;
extern RandomGen g_RandomNumberGenerator;

void
seed_lua_prng();

inline auto
random_up_to(RandomGen& rng, int limit) -> int;

inline auto
random_up_to(int limit) -> int;

inline auto
RandomFloat() -> float;

inline auto
RandomFloat(float fLow, float fHigh) -> float;

inline auto
RandomInt(int low, int high) -> int;

inline auto
RandomInt(int n) -> int;

inline auto
randomf(const float low = 1.F, const float high = 1.F) -> float;
