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

#ifndef KERNEL_H_X86
#define KERNEL_H_X86
#include "../fft_internal.h"

#undef MANGLE
#if __AVX__
#include <immintrin.h>
#define MANGLE(x) x ## _avx
#elif __SSE3__
#include <pmmintrin.h>
#define MANGLE(x) x ## _sse3
#elif __SSE__
#include <xmmintrin.h>
#define MANGLE(x) x ## _sse
#else
#error "This file must be built with x86 SSE/AVX support."
#endif

#if __AVX__
#define MM __m256
#define VSIZE 4 // Complex numbers per vector
#define permute_ps(a, x) _mm256_permute_ps(a, x)
#define moveldup_ps(x) _mm256_moveldup_ps(x)
#define movehdup_ps(x) _mm256_movehdup_ps(x)
#define xor_ps(a, b) _mm256_xor_ps(a, b)
#define add_ps(a, b) _mm256_add_ps(a, b)
#define sub_ps(a, b) _mm256_sub_ps(a, b)
#define mul_ps(a, b) _mm256_mul_ps(a, b)
#define addsub_ps(a, b) _mm256_addsub_ps(a, b)
#define load_ps(addr) _mm256_load_ps((const float*)(addr))
#define loadu_ps(addr) _mm256_loadu_ps((const float*)(addr))
#define store_ps(addr, x) _mm256_store_ps((float*)(addr), x)
#define storeu_ps(addr, x) _mm256_storeu_ps((float*)(addr), x)
#define splat_const_complex(real, imag) _mm256_set_ps(imag, real, imag, real, imag, real, imag, real)
#define splat_const_dual_complex(a, b, real, imag) _mm256_set_ps(imag, real, b, a, imag, real, b, a)
#define splat_complex(addr) (_mm256_castpd_ps(_mm256_broadcast_sd((const double*)(addr))))
#define unpacklo_pd(a, b) (_mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(a), _mm256_castps_pd(b))))
#define unpackhi_pd(a, b) (_mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(a), _mm256_castps_pd(b))))
#else
#define MM __m128
#define VSIZE 2 // Complex numbers per vector
#define permute_ps(a, x) _mm_shuffle_ps(a, a, x)
#define xor_ps(a, b) _mm_xor_ps(a, b)
#define add_ps(a, b) _mm_add_ps(a, b)
#define sub_ps(a, b) _mm_sub_ps(a, b)
#define mul_ps(a, b) _mm_mul_ps(a, b)
#define load_ps(addr) _mm_load_ps((const float*)(addr))
#define loadu_ps(addr) _mm_loadu_ps((const float*)(addr))
#define store_ps(addr, x) _mm_store_ps((float*)(addr), x)
#define storeu_ps(addr, x) _mm_storeu_ps((float*)(addr), x)
#define splat_const_complex(real, imag) _mm_set_ps(imag, real, imag, real)
#define splat_const_dual_complex(a, b, real, imag) _mm_set_ps(imag, real, b, a)

#if __SSE3__
#define addsub_ps(a, b) _mm_addsub_ps(a, b)
#define moveldup_ps(x) _mm_moveldup_ps(x)
#define movehdup_ps(x) _mm_movehdup_ps(x)
#define unpacklo_pd(a, b) (_mm_castpd_ps(_mm_unpacklo_pd(_mm_castps_pd(a), _mm_castps_pd(b))))
#define unpackhi_pd(a, b) (_mm_castpd_ps(_mm_unpackhi_pd(_mm_castps_pd(a), _mm_castps_pd(b))))
#else
#define moveldup_ps(x) permute_ps(x, _MM_SHUFFLE(2, 2, 0, 0))
#define movehdup_ps(x) permute_ps(x, _MM_SHUFFLE(3, 3, 1, 1))
#define unpacklo_pd(a, b) (_mm_shuffle_ps(a, b, _MM_SHUFFLE(1, 0, 1, 0)))
#define unpackhi_pd(a, b) (_mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 2, 3, 2)))

static inline __m128 addsub_ps(__m128 a, __m128 b)
{
    const MM flip_signs = splat_const_complex(-0.0f, 0.0f);
    return add_ps(a, xor_ps(b, flip_signs));
}
#endif

static inline __m128 splat_complex(const void *ptr)
{
#if __SSE3__
    __m128d reg = _mm_load_sd((const double*)ptr);
	return _mm_castpd_ps(_mm_unpacklo_pd(reg, reg));
#else
	__m128 reg = _mm_loadl_pi(_mm_setzero_ps(), (const __m64 *)ptr);
	return _mm_shuffle_ps(reg, reg, _MM_SHUFFLE(1, 0, 1, 0));
#endif
}
#endif

static inline MM cmul_ps(MM a, MM b)
{
    MM r3 = permute_ps(a, _MM_SHUFFLE(2, 3, 0, 1));
    MM r1 = moveldup_ps(b);
    MM R0 = mul_ps(a, r1);
    MM r2 = movehdup_ps(b);
    MM R1 = mul_ps(r2, r3);
    return addsub_ps(R0, R1);
}

static void MANGLE(mufft_convolve_inner)(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input_a, const cfloat * MUFFT_RESTRICT input_b,
        float normalization, unsigned samples)
{
    const MM n = splat_const_complex(normalization, normalization);
    for (unsigned i = 0; i < samples; i += VSIZE)
    {
        MM a = load_ps(&input_a[i]);
        MM b = load_ps(&input_b[i]);
        MM res = mul_ps(cmul_ps(a, b), n);
        store_ps(&output[i], res);
    }
}

void MANGLE(mufft_convolve)(void *output, const void *input_a, const void *input_b,
                            float normalization, unsigned samples)
{
	MANGLE(mufft_convolve_inner)(output, input_a, input_b, normalization, samples);
}

void MANGLE(mufft_resolve_c2r)(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned samples)
{
    const MM flip_signs = splat_const_complex(0.0f, -0.0f);
    for (unsigned i = 0; i < samples; i += VSIZE)
    {
        MM a = load_ps(&input[i]);
        MM b = loadu_ps(&input[samples - i - (VSIZE - 1)]);
        b = permute_ps(xor_ps(b, flip_signs), _MM_SHUFFLE(1, 0, 3, 2));
#if VSIZE == 4
        b = _mm256_permute2f128_ps(b, b, 1);
#endif
        MM even = add_ps(a, b);
        MM odd = cmul_ps(sub_ps(a, b), load_ps(&twiddles[i]));
        store_ps(&output[i], add_ps(even, odd));
    }
}

void MANGLE(mufft_resolve_r2c_full)(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned samples)
{
    cfloat fe = cfloat_real(input[0]);
    cfloat fo = cfloat_imag(input[0]);
    output[0] = cfloat_add(fe, fo);
    output[samples] = cfloat_sub(fe, fo);
    for (unsigned i = 1; i < VSIZE; i++)
    {
        cfloat a = input[i];
        cfloat b = cfloat_conj(input[samples - i]);
        cfloat fe = cfloat_add(a, b);
        cfloat fo = cfloat_mul(twiddles[i], cfloat_sub(a, b));
        output[i] = cfloat_mul_scalar(0.5f, cfloat_add(fe, fo));
        output[i + samples] = cfloat_mul_scalar(0.5f, cfloat_sub(fe, fo));
    }

    const MM flip_signs = splat_const_complex(0.0f, -0.0f);
    const MM half = splat_const_complex(0.5f, 0.5f);
    for (unsigned i = VSIZE; i < samples; i += VSIZE)
    {
        MM a = load_ps(&input[i]);
        MM b = loadu_ps(&input[samples - i - (VSIZE - 1)]);
        b = permute_ps(xor_ps(b, flip_signs), _MM_SHUFFLE(1, 0, 3, 2));
#if VSIZE == 4
        b = _mm256_permute2f128_ps(b, b, 1);
#endif
        MM fe = add_ps(a, b);
        MM fo = cmul_ps(load_ps(&twiddles[i]), sub_ps(a, b));
        store_ps(&output[i], mul_ps(half, add_ps(fe, fo)));
        store_ps(&output[i + samples], mul_ps(half, sub_ps(fe, fo)));
    }
}

void MANGLE(mufft_resolve_r2c)(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned samples)
{
    cfloat fe = cfloat_real(input[0]);
    cfloat fo = cfloat_imag(input[0]);
    output[0] = cfloat_add(fe, fo);
    output[samples] = cfloat_sub(fe, fo);
    for (unsigned i = 1; i < VSIZE; i++)
    {
        cfloat a = input[i];
        cfloat b = cfloat_conj(input[samples - i]);
        cfloat fe = cfloat_add(a, b);
        cfloat fo = cfloat_mul(twiddles[i], cfloat_sub(a, b));
        output[i] = cfloat_mul_scalar(0.5f, cfloat_add(fe, fo));
    }

    const MM flip_signs = splat_const_complex(0.0f, -0.0f);
    const MM half = splat_const_complex(0.5f, 0.5f);
    for (unsigned i = VSIZE; i < samples; i += VSIZE)
    {
        MM a = load_ps(&input[i]);
        MM b = loadu_ps(&input[samples - i - (VSIZE - 1)]);
        b = permute_ps(xor_ps(b, flip_signs), _MM_SHUFFLE(1, 0, 3, 2));
#if VSIZE == 4
        b = _mm256_permute2f128_ps(b, b, 1);
#endif
        MM fe = add_ps(a, b);
        MM fo = cmul_ps(load_ps(&twiddles[i]), sub_ps(a, b));
        store_ps(&output[i], mul_ps(half, add_ps(fe, fo)));
    }
}

void MANGLE(mufft_radix2_p1)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned half_samples = samples >> 1;
    for (unsigned i = 0; i < half_samples; i += VSIZE)
    {
        MM a = load_ps(&input[i]);
        MM b = load_ps(&input[i + half_samples]);

        MM r0 = add_ps(a, b);
        MM r1 = sub_ps(a, b);
        a = unpacklo_pd(r0, r1);
        b = unpackhi_pd(r0, r1);
#if VSIZE == 4
        r0 = _mm256_permute2f128_ps(a, b, (2 << 4) | (0 << 0));
        r1 = _mm256_permute2f128_ps(a, b, (3 << 4) | (1 << 0));
#else
        r0 = a;
        r1 = b;
#endif

        unsigned j = i << 1;
        store_ps(&output[j + 0 * VSIZE], r0);
        store_ps(&output[j + 1 * VSIZE], r1);
    }
}

void MANGLE(mufft_radix2_half_p1)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned half_samples = samples >> 1;
    for (unsigned i = 0; i < half_samples; i += VSIZE)
    {
        MM a = load_ps(&input[i]);

        MM r0 = a;
        MM r1 = a;
        a = unpacklo_pd(r0, r1);
        MM b = unpackhi_pd(r0, r1);
#if VSIZE == 4
        r0 = _mm256_permute2f128_ps(a, b, (2 << 4) | (0 << 0));
        r1 = _mm256_permute2f128_ps(a, b, (3 << 4) | (1 << 0));
#else
        r0 = a;
        r1 = b;
#endif

        unsigned j = i << 1;
        store_ps(&output[j + 0 * VSIZE], r0);
        store_ps(&output[j + 1 * VSIZE], r1);
    }
}

#if VSIZE == 4
#define RADIX2_P2_END \
    a = _mm256_permute2f128_ps(r0, r1, (2 << 4) | (0 << 0)); \
    b = _mm256_permute2f128_ps(r0, r1, (3 << 4) | (1 << 0))
#else
#define RADIX2_P2_END \
    a = r0; \
    b = r1
#endif

#define RADIX2_P2(direction, twiddle_r, twiddle_i) \
void MANGLE(mufft_ ## direction ## _radix2_p2)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_, \
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples) \
{ \
    cfloat *output = output_; \
    const cfloat *input = input_; \
    (void)twiddles; \
    (void)p; \
 \
    unsigned half_samples = samples >> 1; \
    const MM flip_signs = splat_const_dual_complex(0.0f, 0.0f, twiddle_r, twiddle_i); \
 \
    for (unsigned i = 0; i < half_samples; i += VSIZE) \
    { \
        MM a = load_ps(&input[i]); \
        MM b = load_ps(&input[i + half_samples]); \
        b = xor_ps(permute_ps(b, _MM_SHUFFLE(2, 3, 1, 0)), flip_signs); \
 \
        MM r0 = add_ps(a, b); \
        MM r1 = sub_ps(a, b); \
        RADIX2_P2_END; \
 \
        unsigned j = i << 1; \
        store_ps(&output[j + 0], a); \
        store_ps(&output[j + VSIZE], b); \
    } \
}
RADIX2_P2(forward, 0.0f, -0.0f)
RADIX2_P2(inverse, -0.0f, 0.0f)

void MANGLE(mufft_radix2_generic)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned half_samples = samples >> 1;

    for (unsigned i = 0; i < half_samples; i += VSIZE)
    {
        unsigned k = i & (p - 1);

        MM w = load_ps(&twiddles[k]);
        MM a = load_ps(&input[i]);
        MM b = load_ps(&input[i + half_samples]);
        b = cmul_ps(b, w);

        MM r0 = add_ps(a, b);
        MM r1 = sub_ps(a, b);

        unsigned j = (i << 1) - k;
        store_ps(&output[j + 0], r0);
        store_ps(&output[j + p], r1);
    }
}

#if VSIZE == 4
#define RADIX4_P1_END \
    o0 = _mm256_permute2f128_ps(o0o1_lo, o2o3_lo, (2 << 4) | (0 << 0)); \
    o1 = _mm256_permute2f128_ps(o0o1_hi, o2o3_hi, (2 << 4) | (0 << 0)); \
    o2 = _mm256_permute2f128_ps(o0o1_lo, o2o3_lo, (3 << 4) | (1 << 0)); \
    o3 = _mm256_permute2f128_ps(o0o1_hi, o2o3_hi, (3 << 4) | (1 << 0))
#else
#define RADIX4_P1_END \
    o0 = o0o1_lo; \
    o1 = o2o3_lo; \
    o2 = o0o1_hi; \
    o3 = o2o3_hi
#endif

#define RADIX4_P1(direction, twiddle_r, twiddle_i) \
void MANGLE(mufft_ ## direction ## _radix4_p1)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_, \
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples) \
{ \
    cfloat *output = output_; \
    const cfloat *input = input_; \
    (void)twiddles; \
    (void)p; \
 \
    const MM flip_signs = splat_const_complex(twiddle_r, twiddle_i); \
    unsigned quarter_samples = samples >> 2; \
 \
    for (unsigned i = 0; i < quarter_samples; i += VSIZE) \
    { \
        RADIX4_LOAD_FIRST_BUTTERFLY; \
        r3 = xor_ps(permute_ps(r3, _MM_SHUFFLE(2, 3, 0, 1)), flip_signs); \
 \
        MM o0 = add_ps(r0, r2); \
        MM o1 = add_ps(r1, r3); \
        MM o2 = sub_ps(r0, r2); \
        MM o3 = sub_ps(r1, r3); \
 \
        MM o0o1_lo = unpacklo_pd(o0, o1); \
        MM o0o1_hi = unpackhi_pd(o0, o1); \
        MM o2o3_lo = unpacklo_pd(o2, o3); \
        MM o2o3_hi = unpackhi_pd(o2, o3); \
        RADIX4_P1_END; \
 \
        unsigned j = i << 2; \
        store_ps(&output[j + 0 * VSIZE], o0); \
        store_ps(&output[j + 1 * VSIZE], o1); \
        store_ps(&output[j + 2 * VSIZE], o2); \
        store_ps(&output[j + 3 * VSIZE], o3); \
    } \
}

#undef RADIX4_LOAD_FIRST_BUTTERFLY
#define RADIX4_LOAD_FIRST_BUTTERFLY \
        MM a = load_ps(&input[i]); \
        MM b = load_ps(&input[i + quarter_samples]); \
        MM c = load_ps(&input[i + 2 * quarter_samples]); \
        MM d = load_ps(&input[i + 3 * quarter_samples]); \
 \
        MM r0 = add_ps(a, c); \
        MM r1 = sub_ps(a, c); \
        MM r2 = add_ps(b, d); \
        MM r3 = sub_ps(b, d)
RADIX4_P1(forward, 0.0f, -0.0f)
RADIX4_P1(inverse, -0.0f, 0.0f)
#undef RADIX4_LOAD_FIRST_BUTTERFLY
#define RADIX4_LOAD_FIRST_BUTTERFLY \
        MM a = load_ps(&input[i]); \
        MM b = load_ps(&input[i + quarter_samples]); \
 \
        MM r0 = a; \
        MM r1 = a; \
        MM r2 = b; \
        MM r3 = b
RADIX4_P1(forward_half, 0.0f, -0.0f)

void MANGLE(mufft_radix4_generic)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned quarter_samples = samples >> 2;

    for (unsigned i = 0; i < quarter_samples; i += VSIZE)
    {
        unsigned k = i & (p - 1);

        MM w = load_ps(&twiddles[k]);
        MM w0 = load_ps(&twiddles[p + k]);
        MM w1 = load_ps(&twiddles[2 * p + k]);

        MM a = load_ps(&input[i]);
        MM b = load_ps(&input[i + quarter_samples]);
        MM c = load_ps(&input[i + 2 * quarter_samples]);
        MM d = load_ps(&input[i + 3 * quarter_samples]);

        c = cmul_ps(c, w);
        d = cmul_ps(d, w);

        MM r0 = add_ps(a, c);
        MM r1 = sub_ps(a, c);
        MM r2 = add_ps(b, d);
        MM r3 = sub_ps(b, d);

        r2 = cmul_ps(r2, w0);
        r3 = cmul_ps(r3, w1);

        MM o0 = add_ps(r0, r2);
        MM o1 = sub_ps(r0, r2);
        MM o2 = add_ps(r1, r3);
        MM o3 = sub_ps(r1, r3);

        unsigned j = ((i - k) << 2) + k;
        store_ps(&output[j + 0], o0);
        store_ps(&output[j + 1 * p], o2);
        store_ps(&output[j + 2 * p], o1);
        store_ps(&output[j + 3 * p], o3);
    }
}

#if VSIZE == 4
#define RADIX8_P1_END \
        o0 = _mm256_permute2f128_ps(o0o1_lo, o2o3_lo, (2 << 4) | (0 << 0)); \
        o1 = _mm256_permute2f128_ps(o4o5_lo, o6o7_lo, (2 << 4) | (0 << 0)); \
        o2 = _mm256_permute2f128_ps(o0o1_hi, o2o3_hi, (2 << 4) | (0 << 0)); \
        o3 = _mm256_permute2f128_ps(o4o5_hi, o6o7_hi, (2 << 4) | (0 << 0)); \
        o4 = _mm256_permute2f128_ps(o0o1_lo, o2o3_lo, (3 << 4) | (1 << 0)); \
        o5 = _mm256_permute2f128_ps(o4o5_lo, o6o7_lo, (3 << 4) | (1 << 0)); \
        o6 = _mm256_permute2f128_ps(o0o1_hi, o2o3_hi, (3 << 4) | (1 << 0)); \
        o7 = _mm256_permute2f128_ps(o4o5_hi, o6o7_hi, (3 << 4) | (1 << 0))
#else
#define RADIX8_P1_END \
        o0 = o0o1_lo; \
        o1 = o2o3_lo; \
        o2 = o4o5_lo; \
        o3 = o6o7_lo; \
        o4 = o0o1_hi; \
        o5 = o2o3_hi; \
        o6 = o4o5_hi; \
        o7 = o6o7_hi
#endif

#define RADIX8_P1(direction, twiddle_r, twiddle_i, twiddle8) \
void MANGLE(mufft_ ## direction ## _radix8_p1)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_, \
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples) \
{ \
    cfloat *output = output_; \
    const cfloat *input = input_; \
    (void)twiddles; \
    (void)p; \
 \
    const MM flip_signs = splat_const_complex(twiddle_r, twiddle_i); \
    const MM w_f = splat_const_complex((float)(+M_SQRT1_2), twiddle8); \
    const MM w_h = splat_const_complex((float)(-M_SQRT1_2), twiddle8); \
 \
    unsigned octa_samples = samples >> 3; \
    for (unsigned i = 0; i < octa_samples; i += VSIZE) \
    { \
        RADIX8_LOAD_FIRST_BUTTERFLY; \
        r5 = xor_ps(permute_ps(r5, _MM_SHUFFLE(2, 3, 0, 1)), flip_signs); \
        r7 = xor_ps(permute_ps(r7, _MM_SHUFFLE(2, 3, 0, 1)), flip_signs); \
 \
        a = add_ps(r0, r4); \
        b = add_ps(r1, r5); \
        c = sub_ps(r0, r4); \
        d = sub_ps(r1, r5); \
        e = add_ps(r2, r6); \
        f = add_ps(r3, r7); \
        g = sub_ps(r2, r6); \
        h = sub_ps(r3, r7); \
 \
        f = cmul_ps(f, w_f); \
        g = xor_ps(permute_ps(g, _MM_SHUFFLE(2, 3, 0, 1)), flip_signs); \
        h = cmul_ps(h, w_h); \
 \
        MM o0 = add_ps(a, e); \
        MM o1 = add_ps(b, f); \
        MM o2 = add_ps(c, g); \
        MM o3 = add_ps(d, h); \
        MM o4 = sub_ps(a, e); \
        MM o5 = sub_ps(b, f); \
        MM o6 = sub_ps(c, g); \
        MM o7 = sub_ps(d, h); \
 \
        MM o0o1_lo = unpacklo_pd(o0, o1); \
        MM o0o1_hi = unpackhi_pd(o0, o1); \
        MM o2o3_lo = unpacklo_pd(o2, o3); \
        MM o2o3_hi = unpackhi_pd(o2, o3); \
        MM o4o5_lo = unpacklo_pd(o4, o5); \
        MM o4o5_hi = unpackhi_pd(o4, o5); \
        MM o6o7_lo = unpacklo_pd(o6, o7); \
        MM o6o7_hi = unpackhi_pd(o6, o7); \
 \
        RADIX8_P1_END; \
        unsigned j = i << 3; \
        store_ps(&output[j + 0 * VSIZE], o0); \
        store_ps(&output[j + 1 * VSIZE], o1); \
        store_ps(&output[j + 2 * VSIZE], o2); \
        store_ps(&output[j + 3 * VSIZE], o3); \
        store_ps(&output[j + 4 * VSIZE], o4); \
        store_ps(&output[j + 5 * VSIZE], o5); \
        store_ps(&output[j + 6 * VSIZE], o6); \
        store_ps(&output[j + 7 * VSIZE], o7); \
    } \
}

#undef RADIX8_LOAD_FIRST_BUTTERFLY
#define RADIX8_LOAD_FIRST_BUTTERFLY \
        MM a = load_ps(&input[i]); \
        MM b = load_ps(&input[i + octa_samples]); \
        MM c = load_ps(&input[i + 2 * octa_samples]); \
        MM d = load_ps(&input[i + 3 * octa_samples]); \
        MM e = load_ps(&input[i + 4 * octa_samples]); \
        MM f = load_ps(&input[i + 5 * octa_samples]); \
        MM g = load_ps(&input[i + 6 * octa_samples]); \
        MM h = load_ps(&input[i + 7 * octa_samples]); \
 \
        MM r0 = add_ps(a, e); \
        MM r1 = sub_ps(a, e); \
        MM r2 = add_ps(b, f); \
        MM r3 = sub_ps(b, f); \
        MM r4 = add_ps(c, g); \
        MM r5 = sub_ps(c, g); \
        MM r6 = add_ps(d, h); \
        MM r7 = sub_ps(d, h)
RADIX8_P1(forward, 0.0f, -0.0f, (float)(-M_SQRT1_2))
RADIX8_P1(inverse, -0.0f, +0.0f, (float)(+M_SQRT1_2))
#undef RADIX8_LOAD_FIRST_BUTTERFLY
#define RADIX8_LOAD_FIRST_BUTTERFLY \
        MM a = load_ps(&input[i]); \
        MM b = load_ps(&input[i + octa_samples]); \
        MM c = load_ps(&input[i + 2 * octa_samples]); \
        MM d = load_ps(&input[i + 3 * octa_samples]); \
        MM e, f, g, h; \
 \
        MM r0 = a; \
        MM r1 = a; \
        MM r2 = b; \
        MM r3 = b; \
        MM r4 = c; \
        MM r5 = c; \
        MM r6 = d; \
        MM r7 = d
RADIX8_P1(forward_half, 0.0f, -0.0f, (float)(-M_SQRT1_2))

void MANGLE(mufft_radix8_generic)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned octa_samples = samples >> 3;
    for (unsigned i = 0; i < octa_samples; i += VSIZE)
    {
        unsigned k = i & (p - 1);
        const MM w = load_ps(&twiddles[k]);
        MM a = load_ps(&input[i]);
        MM b = load_ps(&input[i + octa_samples]);
        MM c = load_ps(&input[i + 2 * octa_samples]);
        MM d = load_ps(&input[i + 3 * octa_samples]);
        MM e = load_ps(&input[i + 4 * octa_samples]);
        MM f = load_ps(&input[i + 5 * octa_samples]);
        MM g = load_ps(&input[i + 6 * octa_samples]);
        MM h = load_ps(&input[i + 7 * octa_samples]);

        e = cmul_ps(e, w);
        f = cmul_ps(f, w);
        g = cmul_ps(g, w);
        h = cmul_ps(h, w);

        MM r0 = add_ps(a, e);
        MM r1 = sub_ps(a, e);
        MM r2 = add_ps(b, f);
        MM r3 = sub_ps(b, f);
        MM r4 = add_ps(c, g);
        MM r5 = sub_ps(c, g);
        MM r6 = add_ps(d, h);
        MM r7 = sub_ps(d, h);

        const MM w0 = load_ps(&twiddles[p + k]);
        const MM w1 = load_ps(&twiddles[2 * p + k]);
        r4 = cmul_ps(r4, w0);
        r5 = cmul_ps(r5, w1);
        r6 = cmul_ps(r6, w0);
        r7 = cmul_ps(r7, w1);

        a = add_ps(r0, r4);
        b = add_ps(r1, r5);
        c = sub_ps(r0, r4);
        d = sub_ps(r1, r5);
        e = add_ps(r2, r6);
        f = add_ps(r3, r7);
        g = sub_ps(r2, r6);
        h = sub_ps(r3, r7);

        const MM we = load_ps(&twiddles[3 * p + k]);
        const MM wf = load_ps(&twiddles[3 * p + k + p]);
        const MM wg = load_ps(&twiddles[3 * p + k + 2 * p]);
        const MM wh = load_ps(&twiddles[3 * p + k + 3 * p]);
        e = cmul_ps(e, we);
        f = cmul_ps(f, wf);
        g = cmul_ps(g, wg);
        h = cmul_ps(h, wh);

        MM o0 = add_ps(a, e);
        MM o1 = add_ps(b, f);
        MM o2 = add_ps(c, g);
        MM o3 = add_ps(d, h);
        MM o4 = sub_ps(a, e);
        MM o5 = sub_ps(b, f);
        MM o6 = sub_ps(c, g);
        MM o7 = sub_ps(d, h);

        unsigned j = ((i - k) << 3) + k;
        store_ps(&output[j + 0 * p], o0);
        store_ps(&output[j + 1 * p], o1);
        store_ps(&output[j + 2 * p], o2);
        store_ps(&output[j + 3 * p], o3);
        store_ps(&output[j + 4 * p], o4);
        store_ps(&output[j + 5 * p], o5);
        store_ps(&output[j + 6 * p], o6);
        store_ps(&output[j + 7 * p], o7);
    }
}


void MANGLE(mufft_radix2_p1_vert)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned half_lines = samples_y >> 1;
    unsigned half_stride = stride * half_lines;

    for (unsigned line = 0; line < half_lines;
            line++, input += stride, output += stride << 1)
    {
        for (unsigned i = 0; i < samples_x; i += VSIZE)
        {
            MM a = load_ps(&input[i]);
            MM b = load_ps(&input[i + half_stride]);

            MM r0 = add_ps(a, b);
            MM r1 = sub_ps(a, b);

            store_ps(&output[i], r0);
            store_ps(&output[i + 1 * stride], r1);
        }
    }
}

void MANGLE(mufft_radix2_generic_vert)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned half_lines = samples_y >> 1;
    unsigned half_stride = stride * half_lines;
    unsigned out_stride = p * stride;

    for (unsigned line = 0; line < half_lines;
            line++, input += stride)
    {
        unsigned k = line & (p - 1);
        unsigned j = ((line << 1) - k) * stride;
        const MM w = splat_complex(&twiddles[k]);

        for (unsigned i = 0; i < samples_x; i += VSIZE)
        {
            MM a = load_ps(&input[i]);
            MM b = load_ps(&input[i + half_stride]);
            b = cmul_ps(b, w);

            MM r0 = add_ps(a, b);
            MM r1 = sub_ps(a, b);

            store_ps(&output[i + j], r0);
            store_ps(&output[i + j + 1 * out_stride], r1);
        }
    }
}

#define RADIX4_P1_VERT(direction, twiddle_r, twiddle_i) \
void MANGLE(mufft_ ## direction ## _radix4_p1_vert)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_, \
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y) \
{ \
    cfloat *output = output_; \
    const cfloat *input = input_; \
    (void)twiddles; \
    (void)p; \
 \
    unsigned quarter_lines = samples_y >> 2; \
    unsigned quarter_stride = stride * quarter_lines; \
    const MM flip_signs = splat_const_complex(twiddle_r, twiddle_i); \
 \
    for (unsigned line = 0; line < quarter_lines; \
            line++, input += stride, output += stride << 2) \
    { \
        for (unsigned i = 0; i < samples_x; i += VSIZE) \
        { \
            MM a = load_ps(&input[i]); \
            MM b = load_ps(&input[i + quarter_stride]); \
            MM c = load_ps(&input[i + 2 * quarter_stride]); \
            MM d = load_ps(&input[i + 3 * quarter_stride]); \
 \
            MM r0 = add_ps(a, c); \
            MM r1 = sub_ps(a, c); \
            MM r2 = add_ps(b, d); \
            MM r3 = sub_ps(b, d); \
            r3 = xor_ps(permute_ps(r3, _MM_SHUFFLE(2, 3, 0, 1)), flip_signs); \
 \
            a = add_ps(r0, r2); \
            b = add_ps(r1, r3); \
            c = sub_ps(r0, r2); \
            d = sub_ps(r1, r3); \
 \
            store_ps(&output[i], a); \
            store_ps(&output[i + 1 * stride], b); \
            store_ps(&output[i + 2 * stride], c); \
            store_ps(&output[i + 3 * stride], d); \
        } \
    } \
}
RADIX4_P1_VERT(forward, 0.0f, -0.0f)
RADIX4_P1_VERT(inverse, -0.0f, 0.0f)

void MANGLE(mufft_radix4_generic_vert)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned quarter_lines = samples_y >> 2;
    unsigned quarter_stride = stride * quarter_lines;
    unsigned out_stride = p * stride;

    for (unsigned line = 0; line < quarter_lines; line++, input += stride)
    {
        unsigned k = line & (p - 1);
        unsigned j = (((line - k) << 2) + k) * stride;

        for (unsigned i = 0; i < samples_x; i += VSIZE)
        {
            const MM w = splat_complex(&twiddles[k]);
            MM a = load_ps(&input[i]);
            MM b = load_ps(&input[i + quarter_stride]);
            MM c = cmul_ps(load_ps(&input[i + 2 * quarter_stride]), w);
            MM d = cmul_ps(load_ps(&input[i + 3 * quarter_stride]), w);

            MM r0 = add_ps(a, c);
            MM r1 = sub_ps(a, c);
            MM r2 = add_ps(b, d);
            MM r3 = sub_ps(b, d);

            MM w0 = splat_complex(&twiddles[p + k]);
            MM w1 = splat_complex(&twiddles[p + k + p]);
            r2 = cmul_ps(r2, w0);
            r3 = cmul_ps(r3, w1);

            a = add_ps(r0, r2);
            b = add_ps(r1, r3);
            c = sub_ps(r0, r2);
            d = sub_ps(r1, r3);

            store_ps(&output[i + j + 0 * out_stride], a);
            store_ps(&output[i + j + 1 * out_stride], b);
            store_ps(&output[i + j + 2 * out_stride], c);
            store_ps(&output[i + j + 3 * out_stride], d);
        }
    }
}

#define RADIX8_P1_VERT(direction, twiddle_r, twiddle_i, twiddle8) \
void MANGLE(mufft_ ## direction ## _radix8_p1_vert)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_, \
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y) \
{ \
    cfloat *output = output_; \
    const cfloat *input = input_; \
    (void)p; \
    (void)twiddles; \
 \
    unsigned octa_lines = samples_y >> 3; \
    unsigned octa_stride = stride * octa_lines; \
    const MM flip_signs = splat_const_complex(twiddle_r, twiddle_i); \
    const MM w_f = splat_const_complex((float)(+M_SQRT1_2), twiddle8); \
    const MM w_h = splat_const_complex((float)(-M_SQRT1_2), twiddle8); \
 \
    for (unsigned line = 0; line < octa_lines; \
            line++, input += stride, output += stride << 3) \
    { \
        for (unsigned i = 0; i < samples_x; i += VSIZE) \
        { \
            MM a = load_ps(&input[i]); \
            MM b = load_ps(&input[i + octa_stride]); \
            MM c = load_ps(&input[i + 2 * octa_stride]); \
            MM d = load_ps(&input[i + 3 * octa_stride]); \
            MM e = load_ps(&input[i + 4 * octa_stride]); \
            MM f = load_ps(&input[i + 5 * octa_stride]); \
            MM g = load_ps(&input[i + 6 * octa_stride]); \
            MM h = load_ps(&input[i + 7 * octa_stride]); \
 \
            MM r0 = add_ps(a, e); \
            MM r1 = sub_ps(a, e); \
            MM r2 = add_ps(b, f); \
            MM r3 = sub_ps(b, f); \
            MM r4 = add_ps(c, g); \
            MM r5 = sub_ps(c, g); \
            MM r6 = add_ps(d, h); \
            MM r7 = sub_ps(d, h); \
            r5 = xor_ps(permute_ps(r5, _MM_SHUFFLE(2, 3, 0, 1)), flip_signs); \
            r7 = xor_ps(permute_ps(r7, _MM_SHUFFLE(2, 3, 0, 1)), flip_signs); \
 \
            a = add_ps(r0, r4); \
            b = add_ps(r1, r5); \
            c = sub_ps(r0, r4); \
            d = sub_ps(r1, r5); \
            e = add_ps(r2, r6); \
            f = add_ps(r3, r7); \
            g = sub_ps(r2, r6); \
            h = sub_ps(r3, r7); \
            f = cmul_ps(f, w_f); \
            g = xor_ps(permute_ps(g, _MM_SHUFFLE(2, 3, 0, 1)), flip_signs); \
            h = cmul_ps(h, w_h); \
 \
            r0 = add_ps(a, e); \
            r1 = add_ps(b, f); \
            r2 = add_ps(c, g); \
            r3 = add_ps(d, h); \
            r4 = sub_ps(a, e); \
            r5 = sub_ps(b, f); \
            r6 = sub_ps(c, g); \
            r7 = sub_ps(d, h); \
 \
            store_ps(&output[i], r0); \
            store_ps(&output[i + 1 * stride], r1); \
            store_ps(&output[i + 2 * stride], r2); \
            store_ps(&output[i + 3 * stride], r3); \
            store_ps(&output[i + 4 * stride], r4); \
            store_ps(&output[i + 5 * stride], r5); \
            store_ps(&output[i + 6 * stride], r6); \
            store_ps(&output[i + 7 * stride], r7); \
        } \
    } \
}
RADIX8_P1_VERT(forward, 0.0f, -0.0f, (float)(-M_SQRT1_2))
RADIX8_P1_VERT(inverse, -0.0f, 0.0f, (float)(+M_SQRT1_2))

void MANGLE(mufft_radix8_generic_vert)(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned octa_lines = samples_y >> 3;
    unsigned octa_stride = stride * octa_lines;
    unsigned out_stride = p * stride;

    for (unsigned line = 0; line < octa_lines; line++, input += stride)
    {
        unsigned k = line & (p - 1);
        unsigned j = (((line - k) << 3) + k) * stride;

        for (unsigned i = 0; i < samples_x; i += VSIZE)
        {
            const MM w = splat_complex(&twiddles[k]);
            MM a = load_ps(&input[i]);
            MM b = load_ps(&input[i + octa_stride]);
            MM c = load_ps(&input[i + 2 * octa_stride]);
            MM d = load_ps(&input[i + 3 * octa_stride]);
            MM e = cmul_ps(load_ps(&input[i + 4 * octa_stride]), w);
            MM f = cmul_ps(load_ps(&input[i + 5 * octa_stride]), w);
            MM g = cmul_ps(load_ps(&input[i + 6 * octa_stride]), w);
            MM h = cmul_ps(load_ps(&input[i + 7 * octa_stride]), w);

            MM r0 = add_ps(a, e);
            MM r1 = sub_ps(a, e);
            MM r2 = add_ps(b, f);
            MM r3 = sub_ps(b, f);
            MM r4 = add_ps(c, g);
            MM r5 = sub_ps(c, g);
            MM r6 = add_ps(d, h);
            MM r7 = sub_ps(d, h);

            MM w0 = splat_complex(&twiddles[p + k]);
            MM w1 = splat_complex(&twiddles[p + k + p]);
            r4 = cmul_ps(r4, w0);
            r5 = cmul_ps(r5, w1);
            r6 = cmul_ps(r6, w0);
            r7 = cmul_ps(r7, w1);

            a = add_ps(r0, r4);
            b = add_ps(r1, r5);
            c = sub_ps(r0, r4);
            d = sub_ps(r1, r5);
            e = add_ps(r2, r6);
            f = add_ps(r3, r7);
            g = sub_ps(r2, r6);
            h = sub_ps(r3, r7);

            e = cmul_ps(e, splat_complex(&twiddles[3 * p + k]));
            f = cmul_ps(f, splat_complex(&twiddles[3 * p + k + p]));
            g = cmul_ps(g, splat_complex(&twiddles[3 * p + k + 2 * p]));
            h = cmul_ps(h, splat_complex(&twiddles[3 * p + k + 3 * p]));

            r0 = add_ps(a, e);
            r1 = add_ps(b, f);
            r2 = add_ps(c, g);
            r3 = add_ps(d, h);
            r4 = sub_ps(a, e);
            r5 = sub_ps(b, f);
            r6 = sub_ps(c, g);
            r7 = sub_ps(d, h);

            store_ps(&output[i + j + 0 * out_stride], r0);
            store_ps(&output[i + j + 1 * out_stride], r1);
            store_ps(&output[i + j + 2 * out_stride], r2);
            store_ps(&output[i + j + 3 * out_stride], r3);
            store_ps(&output[i + j + 4 * out_stride], r4);
            store_ps(&output[i + j + 5 * out_stride], r5);
            store_ps(&output[i + j + 6 * out_stride], r6);
            store_ps(&output[i + j + 7 * out_stride], r7);
        }
    }
}
#endif
