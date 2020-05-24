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

#ifndef MUFFT_INTERNAL_H__
#define MUFFT_INTERNAL_H__

/// @file fft_internal.h muFFT internal helper functions

#include "fft.h"
#include <math.h>

#ifndef M_PI
/// Portable definition of M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT1_2
/// Portable definition of M_SQRT1_2
#define M_SQRT1_2 0.707106781186547524401
#endif

/// Default alignment for \ref mufft_alloc
#define MUFFT_ALIGNMENT 64

/// Helper to swap two complex float pointers
#define SWAP(a, b) do { cfloat *tmp = b; b = a; a = tmp; } while(0)

/// Helper macro to get number of elements in an array.
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/// Internal short definition of C99 complex float
typedef struct cfloat
{
    /// The real value of a complex number.
    float real;
    /// The imaginary value of a complex number.
    float imag;
} cfloat;

static inline cfloat cfloat_create(float real, float imag)
{
    cfloat ret = { real, imag };
    return ret;
}

static inline float cfloat_abs(cfloat v)
{
    return sqrtf(v.real * v.real + v.imag * v.imag);
}

static inline cfloat cfloat_real(cfloat v)
{
    cfloat ret = { v.real, 0.0f };
    return ret;
}

static inline cfloat cfloat_imag(cfloat v)
{
    cfloat ret = { v.imag, 0.0f };
    return ret;
}

static inline cfloat cfloat_conj(cfloat v)
{
    cfloat ret = { v.real, -v.imag };
    return ret;
}

static inline cfloat cfloat_add(cfloat a, cfloat b)
{
    cfloat ret = { a.real + b.real, a.imag + b.imag };
    return ret;
}

static inline cfloat cfloat_sub(cfloat a, cfloat b)
{
    cfloat ret = { a.real - b.real, a.imag - b.imag };
    return ret;
}

static inline cfloat cfloat_mul(cfloat a, cfloat b)
{
    cfloat ret = { a.real * b.real - a.imag * b.imag, a.real * b.imag + b.real * a.imag };
    return ret;
}

static inline cfloat cfloat_mul_scalar(float s, cfloat a)
{
    cfloat ret = { a.real * s, a.imag * s };
    return ret;
}

/// 1D/horizontal FFT routine signature
typedef void (*mufft_1d_func)(void * MUFFT_RESTRICT output, const void * MUFFT_RESTRICT input,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples);

/// 2D/vertical FFT routine signature
typedef void (*mufft_2d_func)(void * MUFFT_RESTRICT output, const void * MUFFT_RESTRICT input,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y);

/// Real-to-complex and complex-to-real resolve routine signature
typedef void (*mufft_r2c_resolve_func)(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input, const cfloat * MUFFT_RESTRICT twiddles, unsigned samples);

/// Helper macro to mangle function signatures for specific SIMD instruction sets
#define MANGLE(name, arch) mufft_ ## name ## _ ## arch

/// Declares a mangled complex multiply function
#define FFT_CONVOLVE_FUNC(name, arch) void MANGLE(name, arch) (void *output, const void *a, const void *b, float normalization, unsigned samples);

/// Declares a mangled C2R-R2C resolve function
#define FFT_RESOLVE_FUNC(name, arch) void MANGLE(name, arch) (cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input, const cfloat * MUFFT_RESTRICT twiddles, unsigned samples);

/// Declares a mangled 1D FFT function
#define FFT_1D_FUNC(name, arch) void MANGLE(name, arch) (void * MUFFT_RESTRICT output, const void * MUFFT_RESTRICT input, const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples);

/// Declared a mangled 2D FFT function
#define FFT_2D_FUNC(name, arch) void MANGLE(name, arch) (void * MUFFT_RESTRICT output, const void * MUFFT_RESTRICT input, const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y);

/// Declares all available routines for a specific SIMD instruction set
#define DECLARE_FFT_CPU(arch) \
    FFT_CONVOLVE_FUNC(convolve, arch) \
    FFT_RESOLVE_FUNC(resolve_r2c, arch) \
    FFT_RESOLVE_FUNC(resolve_r2c_full, arch) \
    FFT_RESOLVE_FUNC(resolve_c2r, arch) \
    FFT_1D_FUNC(forward_radix8_p1, arch) \
    FFT_1D_FUNC(forward_radix4_p1, arch) \
    FFT_1D_FUNC(radix2_p1, arch) \
    FFT_1D_FUNC(forward_radix2_p2, arch) \
    FFT_1D_FUNC(radix2_half_p1, arch) \
    FFT_1D_FUNC(forward_half_radix8_p1, arch) \
    FFT_1D_FUNC(forward_half_radix4_p1, arch) \
    FFT_1D_FUNC(forward_radix2_p2, arch) \
    FFT_1D_FUNC(inverse_radix8_p1, arch) \
    FFT_1D_FUNC(inverse_radix4_p1, arch) \
    FFT_1D_FUNC(inverse_radix2_p2, arch) \
    FFT_1D_FUNC(radix8_generic, arch) \
    FFT_1D_FUNC(radix4_generic, arch) \
    FFT_1D_FUNC(radix2_generic, arch) \
    FFT_2D_FUNC(radix2_p1_vert, arch) \
    FFT_2D_FUNC(forward_radix8_p1_vert, arch) \
    FFT_2D_FUNC(forward_radix4_p1_vert, arch) \
    FFT_2D_FUNC(inverse_radix8_p1_vert, arch) \
    FFT_2D_FUNC(inverse_radix4_p1_vert, arch) \
    FFT_2D_FUNC(radix8_generic_vert, arch) \
    FFT_2D_FUNC(radix4_generic_vert, arch) \
    FFT_2D_FUNC(radix2_generic_vert, arch)

DECLARE_FFT_CPU(avx)
DECLARE_FFT_CPU(sse3)
DECLARE_FFT_CPU(sse)
DECLARE_FFT_CPU(c)

/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_MASK_CPU MUFFT_FLAG_CPU_NO_SIMD
/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_CPU_AVX MUFFT_FLAG_CPU_NO_AVX
/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_CPU_SSE3 MUFFT_FLAG_CPU_NO_SSE3
/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_CPU_SSE MUFFT_FLAG_CPU_NO_SSE

/// \brief Gets a mask of all relevant SIMD features the running CPU supports.
unsigned mufft_get_cpu_flags(void);

/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_DIRECTION_INVERSE (1 << 24)
/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_DIRECTION_FORWARD (1 << 25)
/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_DIRECTION_ANY 0

/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_R2C (1 << 26)
/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_C2R (1 << 27)
/// Internal flag used for choosing FFT routines
#define MUFFT_FLAG_NO_ZERO_PAD_UPPER_HALF (1 << 28)

#ifdef MUFFT_DEBUG
/// Assert macro which doesn't rely on NDEBUG not being set.
#define mufft_assert(x) do { if (!(x)) { abort(); } } while(0)
#else
/// Assert macro which doesn't rely on NDEBUG not being set.
#define mufft_assert(x) ((void)0)
#endif

/// Number of samples we need to properly pad an array. This should be equal to the widest SIMD instruction set supported by muFFT. Currently, this is AVX 256-bit which holds 4 complex floats.
#define MUFFT_PADDING_COMPLEX_SAMPLES 4

#endif

