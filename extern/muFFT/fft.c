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

#include "fft.h"
#include "fft_internal.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

/// ABI compatible struct for \ref mufft_step_1d and \ref mufft_step_2d.
struct mufft_step_base
{
    void (*func)(void); ///< Generic function pointer.
    unsigned radix; ///< Radix of the FFT step. 2, 4 or 8.
    unsigned p; ///< The current p factor of the FFT. Determines butterfly stride. It is equal to prev_step.p * prev_step.radix. Initial value is 1.
    unsigned twiddle_offset; ///< Offset into twiddle factor table.
};

/// Represents a single step of a complete 1D/horizontal FFT.
struct mufft_step_1d
{
    mufft_1d_func func; ///< Function pointer to a 1D partial FFT.
    unsigned radix; ///< Radix of the FFT step. 2, 4 or 8.
    unsigned p; ///< The current p factor of the FFT. Determines butterfly stride. It is equal to prev_step.p * prev_step.radix. Initial value is 1.
    unsigned twiddle_offset; ///< Offset into twiddle factor table.
};

/// Represents a single step of a 2D/vertical FFT.
struct mufft_step_2d
{
    mufft_2d_func func; ///< Function pointer to a 2D partial FFT.
    unsigned radix; ///< Radix of the FFT step. 2, 4 or 8.
    unsigned p; ///< The current p factor of the FFT. Determines butterfly stride. It is equal to prev_step.p * prev_step.radix. Initial value is 1.
    unsigned twiddle_offset; ///< Offset into twiddle factor table.
};

/// Represents a complete plan for a 1D FFT.
struct mufft_plan_1d
{
    struct mufft_step_1d *steps; ///< A list of steps to take to complete a full N-tap FFT.
    unsigned num_steps; ///< Number of steps contained in mufft_plan_1d::steps.
    unsigned N; ///< Size of the 1D transform.

    cfloat *tmp_buffer; ///< A temporary buffer used during intermediate steps of the FFT.
    cfloat *twiddles; ///< Buffer holding twiddle factors used in the FFT.

    mufft_r2c_resolve_func r2c_resolve; ///< If non-NULL, a function to turn a N / 2 complex transform into a N-tap real transform.
    mufft_r2c_resolve_func c2r_resolve; ///< If non-NULL, a function to turn a N real inverse transform into a N / 2 complex transform.
    cfloat *r2c_twiddles; ///< Special twiddle factors used in mufft_plan_1d::r2c_resolve or mufft_plan_1d::c2r_resolve.
};

/// Represents a complete plan for a 2D FFT.
struct mufft_plan_2d
{
    struct mufft_step_1d *steps_x; ///< A list of steps to take to complete a full horizontal Nx-tap FFT.
    unsigned num_steps_x; ///< Number of steps contained in mufft_plan_2d::steps_x.
    struct mufft_step_2d *steps_y; ///< Number of steps to take to complete to full vertical Ny-tap FFT.
    unsigned num_steps_y; ///< Number of steps contained in mufft_plan_2d::steps_y.
    unsigned Nx; ///< Size of the horizontal transform.
    unsigned Ny; ///< Size of the vertical transform.

    cfloat *tmp_buffer; ///< A temporary buffer used during intermediate steps of the FFT.
    cfloat *twiddles_x; ///< Buffer holding twiddle factors used in the horizontal FFT.
    cfloat *twiddles_y; ///< Buffer holding twiddle factors used in the vertical FFT.

    mufft_r2c_resolve_func r2c_resolve; ///< If non-NULL, a function to turn a N / 2 complex transform into a N-tap real transform.
    mufft_r2c_resolve_func c2r_resolve; ///< If non-NULL, a function to turn a N real inverse transform into a N / 2 complex transform.
    cfloat *r2c_twiddles; ///< Special twiddle factors used in mufft_plan_2d::r2c_resolve or mufft_plan_2d::c2r_resolve.
    unsigned vertical_nx; ///< Number of columns we should process during vertical transform. Usually mufft_plan_2d::Nx, but might be smaller due to real-to-complex transform.
};

/// Represents a complete plan for a 1D fast convolution.
struct mufft_plan_conv
{
    mufft_plan_1d *plans[2]; ///< 1D FFT plans for first and second inputs.
    mufft_plan_1d *output_plan; ///< 1D FFT plan for inverse FFT.
    size_t block_size; ///< Size required to hold output of mufft_execute_conv_input.
    void *conv_block; ///< Buffer for the result of multiplying the two buffers in mufft_plan_conv::block.
    float normalization; ///< Normalization factor 1 / N.

    mufft_convolve_func convolve_func; ///< Function pointer to complex multiply the two buffers in mufft_plan_conv::block.
    unsigned conv_multiply_n; ///< Count passed to mufft_plan_conv::convolve_func. Either N / 2 + 1 or N / 2 depending on the convolution method.
};

/// \brief Computes the twiddle factor exp(pi * I * direction * k / p)
static cfloat twiddle(int direction, int k, int p)
{
    double phase = (M_PI * direction * k) / p;
    return cfloat_create((float)cos(phase), (float)sin(phase));
}

/// \brief Builds a table of twiddle factors.
/// The table is built for a DIT transform with increasing butterfly strides.
/// The table is suitable for any FFT radix.
/// @param N Transform size
/// @param direction Direction of transform. See \ref MUFFT_FORWARD and \ref MUFFT_INVERSE.
/// @returns Newly allocated twiddle factor table.
static cfloat *build_twiddles(unsigned N, int direction)
{
    cfloat *twiddles = mufft_alloc(N * sizeof(cfloat));
    if (twiddles == NULL)
    {
        return NULL;
    }

    cfloat *pt = twiddles;

    for (unsigned p = 1; p < N; p <<= 1)
    {
        for (unsigned k = 0; k < p; k++)
        {
            pt[k] = twiddle(direction, k, p);
        }
        pt += p == 2 ? 3 : p; // Make sure that twiddles for p == 4 and up are aligned properly for AVX.
    }

    return twiddles;
}

/// ABI compatible base struct for \ref fft_step_1d and \ref fft_step_2d.
struct fft_step_base
{
    void (*func)(void); ///< Generic function pointer.
    unsigned radix; ///< Radix of the FFT. 2, 4 or 8.
};

/// Represents a single step of the complete 1D/horizontal FFT with requirements on use.
struct fft_step_1d
{
    mufft_1d_func func; ///< Function pointer to a 1D partial FFT.
    unsigned radix; ///< Radix of the FFT step. 2, 4 or 8.
    unsigned minimum_elements; ///< Minimum transform size for which this function can be used.
    unsigned fixed_p; ///< Non-zero if this can only be used with a fixed value for mufft_step_base::p.
    unsigned minimum_p; ///< Minimum p-factor for which this can be used. Set to -1u if it can only be used with fft_step_1d::fixed_p.
    unsigned flags; ///< Flags which determine under which conditions this function can be used.
};

/// Represents a single step of the complete 2D/vertical FFT with requirements on use.
struct fft_step_2d
{
    mufft_2d_func func; ///< Function pointer to a 1D partial FFT.
    unsigned radix; ///< Radix of the FFT step. 2, 4 or 8.
    unsigned minimum_elements_x; ///< Minimum horizontal transform size for which this function can be used.
    unsigned minimum_elements_y; ///< Minimum vertical transform size for which this function can be used.
    unsigned fixed_p; ///< Non-zero if this can only be used with a fixed value for mufft_step_base::p.
    unsigned minimum_p; ///< Minimum p-factor for which this can be used. Set to -1u if it can only be used with fft_step_1d::fixed_p.
    unsigned flags; ///< Flags which determine under which conditions this function can be used.
};

/// Represents a resolve step for real-to-complex transform or complex-to-real.
struct fft_r2c_resolve_step
{
    mufft_r2c_resolve_func func; ///< Function pointer to a R2C/C2R resolve function.
    unsigned minimum_elements; ///< Minimum transform size for which this function can be used.
    unsigned flags; ///< Flags which determine under which conditions this function can be used.
};

/// Represents an array complex multiply routine.
struct fft_convolve_step
{
    mufft_convolve_func func; ///< Function pointer to a complex multiply routine.
    unsigned flags; ///< Flags which determine under which conditions this function can be used.
};

static const struct fft_convolve_step convolve_table[] = {
#define STAMP_CPU_CONVOLVE(arch, ext) \
    { .flags = arch, .func = mufft_convolve_ ## ext }
#ifdef MUFFT_HAVE_AVX
    STAMP_CPU_CONVOLVE(MUFFT_FLAG_CPU_AVX, avx),
#endif
#ifdef MUFFT_HAVE_SSE3
    STAMP_CPU_CONVOLVE(MUFFT_FLAG_CPU_SSE3, sse3),
#endif
#ifdef MUFFT_HAVE_SSE
    STAMP_CPU_CONVOLVE(MUFFT_FLAG_CPU_SSE, sse),
#endif
    STAMP_CPU_CONVOLVE(0, c),
};

static const struct fft_r2c_resolve_step fft_r2c_resolve_table[] = {
#define STAMP_CPU_RESOLVE(arch, ext, min_x) \
    { .flags = arch | MUFFT_FLAG_FULL_R2C, \
        .func = mufft_resolve_r2c_full_ ## ext, .minimum_elements = 2 * min_x }, \
    { .flags = arch | MUFFT_FLAG_R2C, \
        .func = mufft_resolve_r2c_ ## ext, .minimum_elements = 2 * min_x }, \
    { .flags = arch | MUFFT_FLAG_C2R, \
        .func = mufft_resolve_c2r_ ## ext, .minimum_elements = 2 * min_x }

#ifdef MUFFT_HAVE_AVX
    STAMP_CPU_RESOLVE(MUFFT_FLAG_CPU_AVX, avx, 4),
#endif
#ifdef MUFFT_HAVE_SSE3
    STAMP_CPU_RESOLVE(MUFFT_FLAG_CPU_SSE3, sse3, 2),
#endif
#ifdef MUFFT_HAVE_SSE
    STAMP_CPU_RESOLVE(MUFFT_FLAG_CPU_SSE, sse, 2),
#endif
    STAMP_CPU_RESOLVE(0, c, 1),
};

static const struct fft_step_1d fft_1d_table[] = {
#define STAMP_CPU_1D(arch, ext, min_x) \
    { .flags = arch | MUFFT_FLAG_DIRECTION_FORWARD | MUFFT_FLAG_NO_ZERO_PAD_UPPER_HALF, \
        .func = mufft_forward_radix8_p1_ ## ext, .minimum_elements = 8 * min_x, .radix = 8, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_FORWARD | MUFFT_FLAG_NO_ZERO_PAD_UPPER_HALF, \
        .func = mufft_forward_radix4_p1_ ## ext, .minimum_elements = 4 * min_x, .radix = 4, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY | MUFFT_FLAG_NO_ZERO_PAD_UPPER_HALF, \
        .func = mufft_radix2_p1_ ## ext, .minimum_elements = 2 * min_x, .radix = 2, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_FORWARD | MUFFT_FLAG_ZERO_PAD_UPPER_HALF, \
        .func = mufft_forward_half_radix8_p1_ ## ext, .minimum_elements = 8 * min_x, .radix = 8, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_FORWARD | MUFFT_FLAG_ZERO_PAD_UPPER_HALF, \
        .func = mufft_forward_half_radix4_p1_ ## ext, .minimum_elements = 4 * min_x, .radix = 4, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY | MUFFT_FLAG_ZERO_PAD_UPPER_HALF, \
        .func = mufft_radix2_half_p1_ ## ext, .minimum_elements = 2 * min_x, .radix = 2, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_FORWARD, \
        .func = mufft_forward_radix2_p2_ ## ext, .minimum_elements = 2 * min_x, .radix = 2, .fixed_p = 2, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_INVERSE, \
        .func = mufft_inverse_radix8_p1_ ## ext, .minimum_elements = 8 * min_x, .radix = 8, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_INVERSE, \
        .func = mufft_inverse_radix4_p1_ ## ext, .minimum_elements = 4 * min_x, .radix = 4, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_INVERSE, \
        .func = mufft_inverse_radix2_p2_ ## ext, .minimum_elements = 2 * min_x, .radix = 2, .fixed_p = 2, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY, \
        .func = mufft_radix8_generic_ ## ext, .minimum_elements = 8 * min_x, .radix = 8, .minimum_p = 8 }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY, \
        .func = mufft_radix4_generic_ ## ext, .minimum_elements = 4 * min_x, .radix = 4, .minimum_p = 4 }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY, \
        .func = mufft_radix2_generic_ ## ext, .minimum_elements = 2 * min_x, .radix = 2, .minimum_p = 4 }

#ifdef MUFFT_HAVE_AVX
    STAMP_CPU_1D(MUFFT_FLAG_CPU_AVX, avx, 4),
#endif
#ifdef MUFFT_HAVE_SSE3
    STAMP_CPU_1D(MUFFT_FLAG_CPU_SSE3, sse3, 2),
#endif
#ifdef MUFFT_HAVE_SSE
    STAMP_CPU_1D(MUFFT_FLAG_CPU_SSE, sse, 2),
#endif
    STAMP_CPU_1D(0, c, 1),
};

static const struct fft_step_2d fft_2d_table[] = {
#define STAMP_CPU_2D(arch, ext, min_x) \
    { .flags = arch | MUFFT_FLAG_DIRECTION_FORWARD, \
        .func = mufft_forward_radix8_p1_vert_ ## ext, .minimum_elements_x = min_x, .minimum_elements_y = 8, .radix = 8, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_FORWARD, \
        .func = mufft_forward_radix4_p1_vert_ ## ext, .minimum_elements_x = min_x, .minimum_elements_y = 4, .radix = 4, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY, \
        .func = mufft_radix2_p1_vert_ ## ext, .minimum_elements_x = min_x, .minimum_elements_y = 2, .radix = 2, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_INVERSE, \
        .func = mufft_inverse_radix8_p1_vert_ ## ext, .minimum_elements_x = min_x, .minimum_elements_y = 8, .radix = 8, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_INVERSE, \
        .func = mufft_inverse_radix4_p1_vert_ ## ext, .minimum_elements_x = min_x, .minimum_elements_y = 4, .radix = 4, .fixed_p = 1, .minimum_p = ~0u }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY, \
        .func = mufft_radix8_generic_vert_ ## ext, .minimum_elements_x = min_x, .minimum_elements_y = 8, .radix = 8, .minimum_p = 8 }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY, \
        .func = mufft_radix4_generic_vert_ ## ext, .minimum_elements_x = min_x, .minimum_elements_y = 4, .radix = 4, .minimum_p = 4 }, \
    { .flags = arch | MUFFT_FLAG_DIRECTION_ANY, \
        .func = mufft_radix2_generic_vert_ ## ext, .minimum_elements_x = min_x, .minimum_elements_y = 2, .radix = 2, .minimum_p = 2 }

#ifdef MUFFT_HAVE_AVX
    STAMP_CPU_2D(MUFFT_FLAG_CPU_AVX, avx, 4),
#endif
#ifdef MUFFT_HAVE_SSE3
    STAMP_CPU_2D(MUFFT_FLAG_CPU_SSE3, sse3, 2),
#endif
#ifdef MUFFT_HAVE_SSE
    STAMP_CPU_2D(MUFFT_FLAG_CPU_SSE, sse, 2),
#endif
    STAMP_CPU_2D(0, c, 1),
};

/// \brief Adds a new FFT step to either \ref mufft_step_1d or \ref mufft_step_2d.
static bool add_step(struct mufft_step_base **steps, unsigned *num_steps,
        const struct fft_step_base *step, unsigned p)
{
    unsigned twiddle_offset = 0;
    if (*num_steps != 0)
    {
        struct mufft_step_base prev = (*steps)[*num_steps - 1];
        twiddle_offset = prev.twiddle_offset +
            (prev.p == 2 ? 3 : (prev.p * (prev.radix - 1)));

        // We skipped radix2 kernels, we have to add the padding twiddle here.
        if (p >= 4 && prev.p == 1)
        {
            twiddle_offset++;
        }
    }

    struct mufft_step_base *new_steps = realloc(*steps, (*num_steps + 1) * sizeof(*new_steps));
    if (new_steps == NULL)
    {
        return false;
    }

    *steps = new_steps;
    (*steps)[*num_steps] = (struct mufft_step_base) {
        .func = step->func,
        .radix = step->radix,
        .p = p,
        .twiddle_offset = twiddle_offset,
    };
    (*num_steps)++;
    return true;
}

/// \brief Builds a plan for a horizontal transform.
static bool build_plan_1d(struct mufft_step_1d **steps, unsigned *num_steps, unsigned N, int direction, unsigned flags)
{
    unsigned radix = N;
    unsigned p = 1;

    unsigned step_flags = 0;
    switch (direction)
    {
        case MUFFT_FORWARD:
            step_flags |= MUFFT_FLAG_DIRECTION_FORWARD;
            break;

        case MUFFT_INVERSE:
            step_flags |= MUFFT_FLAG_DIRECTION_INVERSE;
            break;
    }

    // Add CPU flags. Just accept any CPU for now, but mask out flags we don't want.
    step_flags |= mufft_get_cpu_flags() & ~(MUFFT_FLAG_CPU_NO_SIMD & flags);
    step_flags |= (flags & MUFFT_FLAG_ZERO_PAD_UPPER_HALF) != 0 ?
        MUFFT_FLAG_ZERO_PAD_UPPER_HALF : MUFFT_FLAG_NO_ZERO_PAD_UPPER_HALF;

    while (radix > 1)
    {
        bool found = false;

        // Find first (optimal?) routine which can do work.
        for (unsigned i = 0; i < ARRAY_SIZE(fft_1d_table); i++)
        {
            const struct fft_step_1d *step = &fft_1d_table[i];

            if (radix % step->radix == 0 &&
                    N >= step->minimum_elements &&
                    (step_flags & step->flags) == step->flags &&
                    (p >= step->minimum_p || p == step->fixed_p))
            {
                // Ugly casting, but add_step_1d and add_step_2d are ABI-wise exactly the same, and we don't have templates :(
                if (add_step((struct mufft_step_base**)steps, num_steps, (const struct fft_step_base*)step, p))
                {
                    found = true;
                    radix /= step->radix;
                    p *= step->radix;
                    break;
                }
            }
        }

        if (!found)
        {
            return false;
        }
    }

    return true;
}

/// \brief Builds a plan for a vertical transform.
static bool build_plan_2d(struct mufft_step_2d **steps, unsigned *num_steps, unsigned Nx, unsigned Ny, int direction, unsigned flags)
{
    unsigned radix = Ny;
    unsigned p = 1;

    unsigned step_flags = 0;
    switch (direction)
    {
        case MUFFT_FORWARD:
            step_flags |= MUFFT_FLAG_DIRECTION_FORWARD;
            break;

        case MUFFT_INVERSE:
            step_flags |= MUFFT_FLAG_DIRECTION_INVERSE;
            break;
    }
    // Add CPU flags. Just accept any CPU for now, but mask out flags we don't want.
    step_flags |= mufft_get_cpu_flags() & ~(MUFFT_FLAG_CPU_NO_SIMD & flags);

    while (radix > 1)
    {
        bool found = false;

        // Find first (optimal?) routine which can do work.
        for (unsigned i = 0; i < ARRAY_SIZE(fft_2d_table); i++)
        {
            const struct fft_step_2d *step = &fft_2d_table[i];

            if (radix % step->radix == 0 &&
                    Ny >= step->minimum_elements_y &&
                    Nx >= step->minimum_elements_x &&
                    (step_flags & step->flags) == step->flags &&
                    (p >= step->minimum_p || p == step->fixed_p))
            {
                // Ugly casting, but add_step_1d and add_step_2d are ABI-wise exactly the same,
                // and we don't have templates :(
                if (add_step((struct mufft_step_base**)steps, num_steps, (const struct fft_step_base*)step, p))
                {
                    found = true;
                    radix /= step->radix;
                    p *= step->radix;
                    break;
                }
            }
        }

        if (!found)
        {
            return false;
        }
    }

    return true;
}

// The real-to-complex transform is implemented with a N / 2 complex transform with a
// final butterfly which extracts real/imag parts of the complex transform.
// See http://www.engineeringproductivitytools.com/stuff/T0001/PT10.HTM for details on algorithm.

static cfloat *build_r2c_twiddles(int direction, unsigned N)
{
    cfloat *twiddles = mufft_alloc(N * sizeof(cfloat));
    if (twiddles == NULL)
    {
        return NULL;
    }

    for (unsigned i = 0; i < N; i++)
    {
        twiddles[i] = cfloat_mul(cfloat_create(0.0f, direction), twiddle(direction, i, N));
    }

    return twiddles;
}

static mufft_r2c_resolve_func find_r2c_resolve_func(unsigned flags, unsigned N)
{
    // Add CPU flags. Just accept any CPU for now, but mask out flags we don't want.
    unsigned resolve_flags = mufft_get_cpu_flags() & ~(MUFFT_FLAG_CPU_NO_SIMD & flags);
    resolve_flags |= flags & (MUFFT_FLAG_R2C | MUFFT_FLAG_C2R | MUFFT_FLAG_FULL_R2C);

    for (unsigned i = 0; i < ARRAY_SIZE(fft_r2c_resolve_table); i++)
    {
        const struct fft_r2c_resolve_step *step = &fft_r2c_resolve_table[i];
        if ((step->flags & resolve_flags) == step->flags &&
                N >= step->minimum_elements)
        {
            return step->func;
        }
    }

    return NULL;
}

mufft_plan_1d *mufft_create_plan_1d_r2c(unsigned N, unsigned flags)
{
    if ((N & (N - 1)) != 0 || N == 1)
    {
        return NULL;
    }

    unsigned complex_n = N / 2;

    mufft_plan_1d *plan = mufft_create_plan_1d_c2c(complex_n, MUFFT_FORWARD, flags);
    if (plan == NULL)
    {
        goto error;
    }

    plan->r2c_twiddles = build_r2c_twiddles(MUFFT_FORWARD, complex_n);
    if (plan->r2c_twiddles == NULL)
    {
        goto error;
    }

    if ((flags & MUFFT_FLAG_FULL_R2C) == 0)
    {
        flags |= MUFFT_FLAG_R2C;
    }

    plan->r2c_resolve = find_r2c_resolve_func(flags, N);
    if (plan->r2c_resolve == NULL)
    {
        goto error;
    }

    return plan;

error:
    mufft_free_plan_1d(plan);
    return NULL;
}

mufft_plan_1d *mufft_create_plan_1d_c2r(unsigned N, unsigned flags)
{
    if ((N & (N - 1)) != 0 || N == 1)
    {
        return NULL;
    }

    unsigned complex_n = N / 2;

    mufft_plan_1d *plan = mufft_create_plan_1d_c2c(complex_n, MUFFT_INVERSE, flags);
    if (plan == NULL)
    {
        goto error;
    }

    plan->r2c_twiddles = build_r2c_twiddles(MUFFT_INVERSE, complex_n);
    if (plan->r2c_twiddles == NULL)
    {
        goto error;
    }

    plan->c2r_resolve = find_r2c_resolve_func(flags | MUFFT_FLAG_C2R, N);
    if (plan->c2r_resolve == NULL)
    {
        goto error;
    }

    return plan;

error:
    mufft_free_plan_1d(plan);
    return NULL;
}

mufft_convolve_func mufft_get_convolve_func(unsigned flags)
{
    unsigned convolve_flags = mufft_get_cpu_flags() & ~(MUFFT_FLAG_CPU_NO_SIMD & flags);

    for (unsigned i = 0; i < ARRAY_SIZE(convolve_table); i++)
    {
        const struct fft_convolve_step *step = &convolve_table[i];
        if ((step->flags & convolve_flags) == step->flags)
        {
            return step->func;
        }
    }

    return NULL;
}

mufft_plan_conv *mufft_create_plan_conv(unsigned N, unsigned flags, unsigned method)
{
    if ((N & (N - 1)) != 0 || N == 1)
    {
        return NULL;
    }

    mufft_plan_conv *conv = mufft_calloc(sizeof(*conv));
    if (conv == NULL)
    {
        goto error;
    }

    unsigned first_extra_flag = (method & MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_FIRST) != 0 ?
        MUFFT_FLAG_ZERO_PAD_UPPER_HALF : 0;
    unsigned second_extra_flag = (method & MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_SECOND) != 0 ?
        MUFFT_FLAG_ZERO_PAD_UPPER_HALF : 0;

    switch (method & 1)
    {
        case MUFFT_CONV_METHOD_FLAG_MONO_MONO:
            conv->block_size = (N / 2 + MUFFT_PADDING_COMPLEX_SAMPLES) * sizeof(cfloat);
            conv->plans[0] = mufft_create_plan_1d_r2c(N, flags | first_extra_flag);
            conv->plans[1] = mufft_create_plan_1d_r2c(N, flags | second_extra_flag);
            conv->output_plan = mufft_create_plan_1d_c2r(N, flags);
            conv->conv_multiply_n = N / 2 + 1;
            break;

        case MUFFT_CONV_METHOD_FLAG_STEREO_MONO:
            conv->block_size = N * sizeof(cfloat);
            conv->plans[0] = mufft_create_plan_1d_c2c(N, MUFFT_FORWARD, flags | first_extra_flag);
            conv->plans[1] = mufft_create_plan_1d_r2c(N, flags | second_extra_flag | MUFFT_FLAG_FULL_R2C);
            conv->output_plan = mufft_create_plan_1d_c2c(N, MUFFT_INVERSE, flags);
            conv->conv_multiply_n = N;
            break;
    }

    conv->normalization = 1.0f / N;
    conv->conv_block = mufft_calloc(conv->block_size);

    if (conv->plans[0] == NULL ||
            conv->plans[1] == NULL ||
            conv->conv_block == NULL ||
            conv->output_plan == NULL)
    {
        goto error;
    }

    conv->convolve_func = mufft_get_convolve_func(flags);
    if (conv->convolve_func == NULL)
    {
        goto error;
    }

    return conv;

error:
    mufft_free_plan_conv(conv);
    return NULL;
}

mufft_plan_1d *mufft_create_plan_1d_c2c(unsigned N, int direction, unsigned flags)
{
    if ((N & (N - 1)) != 0 || N == 1)
    {
        return NULL;
    }

    mufft_plan_1d *plan = mufft_calloc(sizeof(*plan));
    if (plan == NULL)
    {
        goto error;
    }

    plan->twiddles = build_twiddles(N, direction);
    if (plan->twiddles == NULL)
    {
        goto error;
    }

    plan->tmp_buffer = mufft_alloc(N * sizeof(cfloat));
    if (plan->tmp_buffer == NULL)
    {
        goto error;
    }

    if (!build_plan_1d(&plan->steps, &plan->num_steps, N, direction, flags))
    {
        goto error;
    }

    plan->N = N;
    return plan;

error:
    mufft_free_plan_1d(plan);
    return NULL;
}

mufft_plan_2d *mufft_create_plan_2d_c2c(unsigned Nx, unsigned Ny, int direction, unsigned flags)
{
    if ((Nx & (Nx - 1)) != 0 || (Ny & (Ny - 1)) != 0 || Nx == 1 || Ny == 1)
    {
        return NULL;
    }

    mufft_plan_2d *plan = mufft_calloc(sizeof(*plan));
    if (plan == NULL)
    {
        goto error;
    }

    plan->twiddles_x = build_twiddles(Nx, direction);
    plan->twiddles_y = build_twiddles(Ny, direction);
    if (plan->twiddles_x == NULL || plan->twiddles_y == NULL)
    {
        goto error;
    }

    if ((flags & (MUFFT_FLAG_R2C | MUFFT_FLAG_C2R)) != 0)
    {
        plan->tmp_buffer = mufft_alloc(2 * Nx * Ny * sizeof(cfloat));
    }
    else
    {
        plan->tmp_buffer = mufft_alloc(Nx * Ny * sizeof(cfloat));
    }

    if (plan->tmp_buffer == NULL)
    {
        goto error;
    }

    if (!build_plan_1d(&plan->steps_x, &plan->num_steps_x, Nx, direction, flags))
    {
        goto error;
    }

    if (!build_plan_2d(&plan->steps_y, &plan->num_steps_y, Nx, Ny, direction, flags))
    {
        goto error;
    }

    plan->Nx = Nx;
    plan->Ny = Ny;
    plan->vertical_nx = Nx;
    return plan;

error:
    mufft_free_plan_2d(plan);
    return NULL;
}

mufft_plan_2d *mufft_create_plan_2d_r2c(unsigned Nx, unsigned Ny, unsigned flags)
{
    if ((Nx & (Nx - 1)) != 0 || (Ny & (Ny - 1)) != 0 || Nx == 1 || Ny == 1)
    {
        return NULL;
    }

    unsigned complex_n = Nx / 2;
    mufft_plan_2d *plan = mufft_create_plan_2d_c2c(complex_n, Ny, MUFFT_FORWARD, flags | MUFFT_FLAG_R2C);
    if (plan == NULL)
    {
        goto error;
    }

    plan->r2c_twiddles = build_r2c_twiddles(MUFFT_FORWARD, complex_n);
    if (plan->r2c_twiddles == NULL)
    {
        goto error;
    }

    if ((flags & MUFFT_FLAG_FULL_R2C) == 0)
    {
        flags |= MUFFT_FLAG_R2C;
        plan->vertical_nx = Nx / 2 + 1;
    }
    else
    {
        plan->vertical_nx = Nx;
    }

    plan->r2c_resolve = find_r2c_resolve_func(flags, Nx);
    if (plan->r2c_resolve == NULL)
    {
        goto error;
    }

    return plan;

error:
    mufft_free_plan_2d(plan);
    return NULL;
}

mufft_plan_2d *mufft_create_plan_2d_c2r(unsigned Nx, unsigned Ny, unsigned flags)
{
    if ((Nx & (Nx - 1)) != 0 || (Ny & (Ny - 1)) != 0 || Nx == 1 || Ny == 1)
    {
        return NULL;
    }

    unsigned complex_n = Nx / 2;
    mufft_plan_2d *plan = mufft_create_plan_2d_c2c(complex_n, Ny, MUFFT_INVERSE, flags | MUFFT_FLAG_C2R);
    if (plan == NULL)
    {
        goto error;
    }

    plan->r2c_twiddles = build_r2c_twiddles(MUFFT_INVERSE, complex_n);
    if (plan->r2c_twiddles == NULL)
    {
        goto error;
    }

    plan->c2r_resolve = find_r2c_resolve_func(flags | MUFFT_FLAG_C2R, Nx);
    if (plan->c2r_resolve == NULL)
    {
        goto error;
    }

    return plan;

error:
    mufft_free_plan_2d(plan);
    return NULL;
}

size_t mufft_conv_get_transformed_block_size(mufft_plan_conv *plan)
{
    return plan->block_size;
}

void mufft_execute_conv_input(mufft_plan_conv *plan, unsigned block, void *output, const void *input)
{
    mufft_execute_plan_1d(plan->plans[block], output, input);
}

void mufft_execute_conv_output(mufft_plan_conv *plan, void *output, const void *input_first, const void *input_second)
{
    plan->convolve_func(plan->conv_block, input_first, input_second,
            plan->normalization, plan->conv_multiply_n);
    mufft_execute_plan_1d(plan->output_plan, output, plan->conv_block);
}

void mufft_execute_plan_1d(mufft_plan_1d *plan, void * MUFFT_RESTRICT output, const void * MUFFT_RESTRICT input)
{
    const cfloat *pt = plan->twiddles;
    cfloat *out = output;
    cfloat *in = plan->tmp_buffer;
    unsigned N = plan->N;

    // If we're doing real-to-complex, we need an extra step.
    unsigned steps = plan->num_steps + (plan->r2c_resolve != NULL);

    // We want final step to write to output.
    if ((steps & 1) == 1)
    {
        SWAP(out, in);
    }

    const struct mufft_step_1d *first_step = &plan->steps[0];
    if (plan->c2r_resolve != NULL)
    {
        plan->c2r_resolve(out, input, plan->r2c_twiddles, N);
        first_step->func(in, out, pt, 1, N);
    }
    else
    {
        first_step->func(in, input, pt, 1, N);
    }

    for (unsigned i = 1; i < plan->num_steps; i++)
    {
        const struct mufft_step_1d *step = &plan->steps[i];
        step->func(out, in, pt + step->twiddle_offset, step->p, N);
        SWAP(out, in);
    }

    // Do Real-to-complex butterfly resolve.
    if (plan->r2c_resolve != NULL)
    {
        plan->r2c_resolve(out, in, plan->r2c_twiddles, N);
    }
}

void mufft_execute_plan_2d(mufft_plan_2d *plan, void * MUFFT_RESTRICT output, const void * MUFFT_RESTRICT input_)
{
    const cfloat *ptx = plan->twiddles_x;
    const cfloat *pty = plan->twiddles_y;
    const cfloat *input = input_;

    unsigned Nx = plan->Nx;
    unsigned Ny = plan->Ny;

    // If we're doing complex-to-real transform, we have to do the inverse transform vertically first, then horizontally.
    // Otherwise, our assumption that the FFT has conjugates due to real-to-complex transform doesn't hold anymore.
    if (plan->c2r_resolve != NULL)
    {
        cfloat *hout = output;
        cfloat *hin = plan->tmp_buffer;
        if ((plan->num_steps_x & 1) == 1)
        {
            SWAP(hout, hin);
        }

        cfloat *out = hout;
        cfloat *in = hin;

        unsigned num_steps_y = plan->num_steps_y;
        if ((num_steps_y & 1) == 0)
        {
            SWAP(out, in);
        }

        // First, vertical transforms.
        const struct mufft_step_2d *first_step = &plan->steps_y[0];
        first_step->func(in, input, pty, 1, Nx + 1, 2 * Nx, Ny);
        for (unsigned i = 1; i < plan->num_steps_y; i++)
        {
            const struct mufft_step_2d *step = &plan->steps_y[i];
            step->func(out, in, pty + step->twiddle_offset, step->p, Nx + 1, 2 * Nx, Ny);
            SWAP(out, in);
        }

        mufft_assert(in == hin);

        // Do first inverse FFT butterfly pass horizontally.
        for (unsigned y = 0; y < Ny; y++)
        {
            plan->c2r_resolve(hout + y * Nx, hin + 2 * y * Nx, plan->r2c_twiddles, Nx);
        }

        // Then, horizontal transforms over all lines individually.
        for (unsigned y = 0; y < Ny; y++)
        {
            cfloat *tin = hin;
            cfloat *tout = hout;

            const struct mufft_step_1d *first_step = &plan->steps_x[0];
            first_step->func(tin + y * Nx, tout + y * Nx, ptx, 1, Nx);
            for (unsigned i = 1; i < plan->num_steps_x; i++)
            {
                const struct mufft_step_1d *step = &plan->steps_x[i];
                step->func(tout + y * Nx, tin + y * Nx, ptx + step->twiddle_offset, step->p, Nx);
                SWAP(tout, tin);
            }

            mufft_assert(tin == output);
        }
    }
    else
    {
        // Complex-to-complex or real-to-complex transform.
        unsigned vertical_stride_x = (plan->r2c_resolve != NULL) ? 2 * Nx : Nx;

        cfloat *hout = output;
        cfloat *hin = plan->tmp_buffer;
        if ((plan->num_steps_y & 1) == 0)
        {
            SWAP(hout, hin);
        }

        cfloat *out = hin;
        cfloat *in = hout;

        unsigned num_steps_x = plan->num_steps_x + (plan->r2c_resolve != NULL);
        if ((num_steps_x & 1) == 1)
        {
            SWAP(out, in);
        }

        // First, horizontal transforms over all lines individually.
        for (unsigned y = 0; y < Ny; y++)
        {
            cfloat *tin = in;
            cfloat *tout = out;

            const struct mufft_step_1d *first_step = &plan->steps_x[0];
            if (plan->c2r_resolve != NULL)
            {
                plan->c2r_resolve(tout + y * Nx, input + 2 * y * Nx, plan->r2c_twiddles, Nx);
                first_step->func(tin + y * Nx, tout + y * Nx, ptx, 1, Nx);
            }
            else
            {
                first_step->func(tin + y * Nx, input + y * Nx, ptx, 1, Nx);
            }

            for (unsigned i = 1; i < plan->num_steps_x; i++)
            {
                const struct mufft_step_1d *step = &plan->steps_x[i];
                step->func(tout + y * Nx, tin + y * Nx, ptx + step->twiddle_offset, step->p, Nx);
                SWAP(tout, tin);
            }
        }

        if (plan->r2c_resolve != NULL)
        {
            // Do Real-to-complex butterfly resolve.
            // Double the strides now.
            for (unsigned y = 0; y < Ny; y++)
            {
                plan->r2c_resolve(hin + 2 * y * Nx, hout + y * Nx,
                        plan->r2c_twiddles, Nx);
            }
        }

        // Since Nx is actually N / 2 if R2C, this is either Nx + 1 or Nx * 2 if we're doing R2C transform.
        Nx = plan->vertical_nx;

        // Vertical transforms.
        const struct mufft_step_2d *first_step = &plan->steps_y[0];
        first_step->func(hout, hin, pty, 1, Nx, vertical_stride_x, Ny);
        SWAP(hout, hin);

        for (unsigned i = 1; i < plan->num_steps_y; i++)
        {
            const struct mufft_step_2d *step = &plan->steps_y[i];
            step->func(hout, hin, pty + step->twiddle_offset, step->p, Nx, vertical_stride_x, Ny);
            SWAP(hout, hin);
        }
    }
}

void mufft_free_plan_1d(mufft_plan_1d *plan)
{
    if (plan == NULL)
    {
        return;
    }
    free(plan->steps);
    mufft_free(plan->tmp_buffer);
    mufft_free(plan->twiddles);
    mufft_free(plan->r2c_twiddles);
    mufft_free(plan);
}

void mufft_free_plan_2d(mufft_plan_2d *plan)
{
    if (plan == NULL)
    {
        return;
    }
    free(plan->steps_x);
    free(plan->steps_y);
    mufft_free(plan->tmp_buffer);
    mufft_free(plan->twiddles_x);
    mufft_free(plan->twiddles_y);
    mufft_free(plan->r2c_twiddles);
    mufft_free(plan);
}

void mufft_free_plan_conv(mufft_plan_conv *plan)
{
    if (plan == NULL)
    {
        return;
    }
    mufft_free_plan_1d(plan->plans[0]);
    mufft_free_plan_1d(plan->plans[1]);
    mufft_free_plan_1d(plan->output_plan);
    mufft_free(plan->conv_block);
    mufft_free(plan);
}

void *mufft_alloc(size_t size)
{
#if defined(_ISOC11_SOURCE)
    return aligned_alloc(MUFFT_ALIGNMENT, size);
#elif (_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600)
    void *ptr = NULL;
    if (posix_memalign(&ptr, MUFFT_ALIGNMENT, size) < 0)
    {
        return NULL;
    }
    return ptr;
#else
    // Align stuff ourselves. Kinda ugly, but will work anywhere.
    void **place;
    uintptr_t addr = 0;
    void *ptr = malloc(MUFFT_ALIGNMENT + size + sizeof(uintptr_t));

    if (ptr == NULL)
    {
        return NULL;
    }

    addr = ((uintptr_t)ptr + sizeof(uintptr_t) + MUFFT_ALIGNMENT)
        & ~(MUFFT_ALIGNMENT - 1);
    place = (void**)addr;
    place[-1] = ptr;

    return (void*)addr;
#endif
}

void *mufft_calloc(size_t size)
{
    void *ptr = mufft_alloc(size);
    if (ptr != NULL)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}

void mufft_free(void *ptr)
{
#if !defined(_ISOC11_SOURCE) && !((_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600))
    if (ptr != NULL)
    {
        void **p = (void**)ptr;
        free(p[-1]);
    }
#else
    free(ptr);
#endif
}

