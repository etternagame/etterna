#ifndef __TRACYYIELD_HPP__
#define __TRACYYIELD_HPP__

#if defined __SSE2__ || defined _M_AMD64 || _M_IX86_FP == 2
#  include <emmintrin.h>
#else
#  include <thread>
#endif

#include "../common/TracyForceInline.hpp"

namespace tracy
{

static tracy_force_inline void YieldThread()
{
#if defined __SSE2__ || defined _M_AMD64 || _M_IX86_FP == 2
    _mm_pause();
#else
    std::this_thread::yield();
#endif
}

}

#endif
