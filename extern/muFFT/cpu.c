/* Copyright (C) 2015 Hans-Kristian Arntzen <maister@archlinux.us>
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "fft_internal.h"
#include <stdint.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#endif

#ifdef MUFFT_HAVE_X86

/// \brief Wrapper routine for x86 CPUID
static void mufft_x86_cpuid(int func, int flags[4])
{
    // On 32-bit with PIC we are not allowed to clobber the ebx register.
#ifdef __x86_64__
#define REG_b "rbx"
#define REG_S "rsi"
#else
#define REG_b "ebx"
#define REG_S "esi"
#endif

#if defined(__GNUC__)
    asm volatile (
            "mov %%" REG_b ", %%" REG_S "\n"
            "cpuid\n"
            "xchg %%" REG_b ", %%" REG_S "\n"
            : "=a"(flags[0]), "=S"(flags[1]), "=c"(flags[2]), "=d"(flags[3])
            : "a"(func), "c"(0));
#elif defined(_MSC_VER)
    __cpuid(flags, func);
#else
#warning "Unknown compiler. Cannot check CPUID with inline assembly."
    memset(flags, 0, 4 * sizeof(int));
#endif
}

/// \brief Wrapper routine for x86 xgetbv
/// Only runs on i686 and above. Needs to be conditionally run.
static uint64_t mufft_xgetbv_x86(uint32_t idx)
{
#if defined(__GNUC__)
    uint32_t eax, edx;
    asm volatile (
            // Older GCC versions (Apple's GCC for example) do 
            // not understand xgetbv instruction.
            // Stamp out the machine code directly.
            ".byte 0x0f, 0x01, 0xd0\n"
            : "=a"(eax), "=d"(edx) : "c"(idx));
    return ((uint64_t)edx << 32) | eax;
#elif _MSC_VER
    return _xgetbv(idx);
#else
#warning "Unknown compiler. Cannot check xgetbv with inline assembly."
    return 0;
#endif
}

unsigned mufft_get_cpu_flags(void)
{
    int flags[4];
    mufft_x86_cpuid(0, flags);

    unsigned max_flag = flags[0];
    if (max_flag < 1) // Does CPUID not support func = 1? (unlikely ...)
    {
        return 0;
    }

    unsigned cpu = 0;

    mufft_x86_cpuid(1, flags);

    if (flags[3] & (1 << 25))
    {
        cpu |= MUFFT_FLAG_CPU_SSE;
    }

    if (flags[2] & (1 << 0))
    {
        cpu |= MUFFT_FLAG_CPU_SSE3;
    }

    const int avx_flags = (1 << 27) | (1 << 28);

    // Must only perform xgetbv check if we have 
    // AVX CPU support (guaranteed to have at least i686).
    if (((flags[2] & avx_flags) == avx_flags) 
            && ((mufft_xgetbv_x86(0) & 0x6) == 0x6))
    {
        cpu |= MUFFT_FLAG_CPU_AVX;
    }

    return cpu;
}

#else
unsigned mufft_get_cpu_flags(void)
{
    return 0;
}
#endif

