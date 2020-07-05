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

#ifndef MUFFT_DEBUG
#define MUFFT_DEBUG
#endif

#include "fft.h"
#include "fft_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <fftw3.h> // Used as a reference.

static void test_fft_2d(unsigned Nx, unsigned Ny, int direction, unsigned flags)
{
    cfloat *input = mufft_alloc(Nx * Ny * sizeof(cfloat));
    cfloat *output = mufft_alloc(Nx * Ny * sizeof(cfloat));
    cfloat *input_fftw = fftwf_malloc(Nx * Ny * sizeof(fftwf_complex));
    cfloat *output_fftw = fftwf_malloc(Nx * Ny * sizeof(fftwf_complex));

    srand(0);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }

    fftwf_plan plan = fftwf_plan_dft_2d(Ny, Nx,
                                        (fftwf_complex *)input_fftw, (fftwf_complex *)output_fftw,
                                        direction, FFTW_ESTIMATE);
    mufft_assert(plan != NULL);
    memcpy(input_fftw, input, Nx * Ny * sizeof(cfloat));

    mufft_plan_2d *muplan = mufft_create_plan_2d_c2c(Nx, Ny, direction, flags);
    mufft_assert(muplan != NULL);

    fftwf_execute(plan);
    mufft_execute_plan_2d(muplan, output, input);

    const float epsilon = 0.000001f * sqrtf(Nx * Ny);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float delta = cfloat_abs(cfloat_sub(output[i], output_fftw[i]));
        mufft_assert(delta < epsilon);
    }

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_2d(muplan);
    fftwf_free(input_fftw);
    fftwf_free(output_fftw);
    fftwf_destroy_plan(plan);
}

static void test_fft_2d_r2c(unsigned Nx, unsigned Ny, unsigned flags)
{
    unsigned fftN = Nx / 2 + 1;
    float *input = mufft_alloc(Nx * Ny * sizeof(float));
    cfloat *output = mufft_alloc(Nx * Ny * sizeof(cfloat));
    float *input_fftw = fftwf_malloc(Nx * Ny * sizeof(float));
    cfloat *output_fftw = fftwf_malloc(fftN * Ny * sizeof(fftwf_complex));

    srand(0);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        input[i] = real;
    }

    fftwf_plan plan = fftwf_plan_dft_r2c_2d(Ny, Nx, input_fftw, (fftwf_complex *)output_fftw,
            FFTW_ESTIMATE);
    mufft_assert(plan != NULL);
    memcpy(input_fftw, input, Nx * Ny * sizeof(float));

    mufft_plan_2d *muplan = mufft_create_plan_2d_r2c(Nx, Ny, flags);
    mufft_assert(muplan != NULL);

    fftwf_execute(plan);
    mufft_execute_plan_2d(muplan, output, input);

    const float epsilon = 0.000001f * sqrtf(Nx * Ny);
    for (unsigned y = 0; y < Ny; y++)
    {
        for (unsigned x = 0; x < fftN; x++)
        {
            float delta = cfloat_abs(cfloat_sub(output[y * Nx + x], output_fftw[y * fftN + x]));
            mufft_assert(delta < epsilon);
        }
    }

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_2d(muplan);
    fftwf_free(input_fftw);
    fftwf_free(output_fftw);
    fftwf_destroy_plan(plan);
}

static void test_fft_2d_c2r(unsigned Nx, unsigned Ny, unsigned flags)
{
    unsigned fftN = Nx / 2 + 1;
    cfloat *input = mufft_calloc(Nx * Ny * sizeof(cfloat));
    float *output = mufft_calloc(2 * Nx * Ny * sizeof(float));
    cfloat *input_fftw = fftwf_malloc(Nx * Ny * sizeof(cfloat));
    float *output_fftw = fftwf_malloc(2 * Nx * Ny * sizeof(float));

    srand(0);
    for (unsigned y = 0; y < Ny; y++)
    {
        for (unsigned x = 1; x < Nx / 2; x++)
        {
            float real = (float)rand() / RAND_MAX - 0.5f;
            float imag = (float)rand() / RAND_MAX - 0.5f;
            input[y * Nx + x] = cfloat_create(real, imag);
        }
    }
    input[0].real = (float)rand() / RAND_MAX - 0.5f;
    input[Ny / 2 * Nx].real = (float)rand() / RAND_MAX - 0.5f;
    input[Nx / 2].real = (float)rand() / RAND_MAX - 0.5f;
    input[Ny / 2 * Nx + Nx / 2].real = (float)rand() / RAND_MAX - 0.5f;

    fftwf_plan plan = fftwf_plan_dft_c2r_2d(Ny, Nx, (fftwf_complex *)input_fftw, output_fftw,
                                            FFTW_ESTIMATE);
    mufft_assert(plan != NULL);
    for (unsigned y = 0; y < Ny; y++)
    {
        memcpy(input_fftw + fftN * y, input + Nx * y, fftN * sizeof(cfloat));
    }

    mufft_plan_2d *muplan = mufft_create_plan_2d_c2r(Nx, Ny, flags);
    mufft_assert(muplan != NULL);

    fftwf_execute(plan);
    mufft_execute_plan_2d(muplan, output, input);

    const float epsilon = 0.000001f * sqrtf(Nx * Ny);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float delta = fabsf(output[i] - output_fftw[i]);
        mufft_assert(delta < epsilon);
    }

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_2d(muplan);
    fftwf_free(input_fftw);
    fftwf_free(output_fftw);
    fftwf_destroy_plan(plan);
}

static void test_fft_1d(unsigned N, int direction, unsigned flags)
{
    cfloat *input = mufft_alloc(N * sizeof(cfloat));
    cfloat *output = mufft_alloc(N * sizeof(cfloat));
    cfloat *input_fftw = fftwf_malloc(N * sizeof(fftwf_complex));
    cfloat *output_fftw = fftwf_malloc(N * sizeof(fftwf_complex));

    srand(0);
    for (unsigned i = 0; i < N; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }

    fftwf_plan plan = fftwf_plan_dft_1d(N, (fftwf_complex *)input_fftw, (fftwf_complex *)output_fftw,
                                        direction, FFTW_ESTIMATE);
    mufft_assert(plan != NULL);
    memcpy(input_fftw, input, N * sizeof(cfloat));

    mufft_plan_1d *muplan = mufft_create_plan_1d_c2c(N, direction, flags);
    mufft_assert(muplan != NULL);

    fftwf_execute(plan);
    mufft_execute_plan_1d(muplan, output, input);

    const float epsilon = 0.000001f * sqrtf(N);
    for (unsigned i = 0; i < N; i++)
    {
        float delta = cfloat_abs(cfloat_sub(output[i], output_fftw[i]));
        mufft_assert(delta < epsilon);
    }

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_1d(muplan);
    fftwf_free(input_fftw);
    fftwf_free(output_fftw);
    fftwf_destroy_plan(plan);
}

static void test_fft_1d_c2r(unsigned N, unsigned flags)
{
    unsigned fftN = N / 2 + 1;
    cfloat *input = mufft_calloc(fftN * sizeof(cfloat));
    float *output = mufft_calloc(N * sizeof(float));
    cfloat *input_fftw = fftwf_malloc(fftN * sizeof(fftwf_complex));
    float *output_fftw = fftwf_malloc(N * sizeof(float));

    srand(0);
    input[0].real = (float)rand() / RAND_MAX - 0.5f;
    for (unsigned i = 1; i < N / 2; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }
    input[N / 2].real = (float)rand() / RAND_MAX - 0.5f;

    fftwf_plan plan = fftwf_plan_dft_c2r_1d(N, (fftwf_complex *)input_fftw, output_fftw, FFTW_ESTIMATE);
    mufft_assert(plan != NULL);
    memcpy(input_fftw, input, fftN * sizeof(cfloat));

    mufft_plan_1d *muplan = mufft_create_plan_1d_c2r(N, flags);
    mufft_assert(muplan != NULL);

    fftwf_execute(plan);
    mufft_execute_plan_1d(muplan, output, input);

    const float epsilon = 0.000001f * sqrtf(N);
    for (unsigned i = 0; i < N; i++)
    {
        float delta = output[i] - output_fftw[i];
        mufft_assert(delta < epsilon);
    }

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_1d(muplan);
    fftwf_free(input_fftw);
    fftwf_free(output_fftw);
    fftwf_destroy_plan(plan);
}

static void test_fft_1d_r2c(unsigned N, unsigned flags)
{
    unsigned fftN = N / 2 + 1;
    float *input = mufft_alloc(N * sizeof(float));
    cfloat *output = mufft_alloc(N * sizeof(cfloat));
    float *input_fftw = fftwf_malloc(N * sizeof(float));
    cfloat *output_fftw = fftwf_malloc(fftN * sizeof(fftwf_complex));

    srand(0);
    for (unsigned i = 0; i < N; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        input[i] = real;
    }

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(N, input_fftw, (fftwf_complex *)output_fftw, FFTW_ESTIMATE);
    mufft_assert(plan != NULL);
    memcpy(input_fftw, input, N * sizeof(float));

    mufft_plan_1d *muplan_full = mufft_create_plan_1d_r2c(N, flags | MUFFT_FLAG_FULL_R2C);
    mufft_assert(muplan_full != NULL);
    mufft_plan_1d *muplan = mufft_create_plan_1d_r2c(N, flags);
    mufft_assert(muplan != NULL);

    fftwf_execute(plan);
    mufft_execute_plan_1d(muplan, output, input);

    const float epsilon = 0.000001f * sqrtf(N);
    for (unsigned i = 0; i < fftN; i++)
    {
        float delta = cfloat_abs(cfloat_sub(output[i], output_fftw[i]));
        mufft_assert(delta < epsilon);
    }

    mufft_execute_plan_1d(muplan_full, output, input);

    for (unsigned i = 0; i < fftN; i++)
    {
        float delta = cfloat_abs(cfloat_sub(output[i], output_fftw[i]));
        mufft_assert(delta < epsilon);
    }

    // Verify stuff is properly conjugated as well.
    for (unsigned i = 1; i < N / 2; i++)
    {
        cfloat a = output[i];
        cfloat b = cfloat_conj(output[N - i]);
        float delta = cfloat_abs(cfloat_sub(a, b));
        mufft_assert(delta < epsilon);
    }

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_1d(muplan);
    mufft_free_plan_1d(muplan_full);
    fftwf_free(input_fftw);
    fftwf_free(output_fftw);
    fftwf_destroy_plan(plan);
}

static void test_fft_1d_r2c_half(unsigned N, unsigned flags)
{
    unsigned fftN = N / 2 + 1;
    float *input = mufft_alloc((N / 2) * sizeof(float));
    cfloat *output = mufft_alloc(N * sizeof(cfloat));
    float *input_fftw = fftwf_malloc(N * sizeof(float));
    cfloat *output_fftw = fftwf_malloc(fftN * sizeof(fftwf_complex));

    memset(input_fftw, 0, N * sizeof(float));

    srand(0);
    for (unsigned i = 0; i < N / 2; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        input[i] = real;
    }

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(N, input_fftw, (fftwf_complex *)output_fftw, FFTW_ESTIMATE);
    mufft_assert(plan != NULL);
    memcpy(input_fftw, input, (N / 2) * sizeof(float));

    mufft_plan_1d *muplan_full = mufft_create_plan_1d_r2c(N, flags | MUFFT_FLAG_FULL_R2C | MUFFT_FLAG_ZERO_PAD_UPPER_HALF);
    mufft_assert(muplan_full != NULL);
    mufft_plan_1d *muplan = mufft_create_plan_1d_r2c(N, flags | MUFFT_FLAG_ZERO_PAD_UPPER_HALF);
    mufft_assert(muplan != NULL);

    fftwf_execute(plan);
    mufft_execute_plan_1d(muplan, output, input);

    const float epsilon = 0.000001f * sqrtf(N);
    for (unsigned i = 0; i < fftN; i++)
    {
        float delta = cfloat_abs(cfloat_sub(output[i], output_fftw[i]));
        mufft_assert(delta < epsilon);
    }

    mufft_execute_plan_1d(muplan_full, output, input);

    for (unsigned i = 0; i < fftN; i++)
    {
        float delta = cfloat_abs(cfloat_sub(output[i], output_fftw[i]));
        mufft_assert(delta < epsilon);
    }

    // Verify stuff is properly conjugated as well.
    for (unsigned i = 1; i < N / 2; i++)
    {
        cfloat a = output[i];
        cfloat b = cfloat_conj(output[N - i]);
        float delta = cfloat_abs(cfloat_sub(a, b));
        mufft_assert(delta < epsilon);
    }

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_1d(muplan);
    mufft_free_plan_1d(muplan_full);
    fftwf_free(input_fftw);
    fftwf_free(output_fftw);
    fftwf_destroy_plan(plan);
}

static void convolve_float(float *output, const float *a, const float *b, unsigned N)
{
    for (unsigned i = 0; i < 2 * N; i++)
    {
        float sum = 0.0f;
        unsigned start_x = 0;
        start_x = (i >= N) ? (i - (N - 1)) : 0;
        unsigned end_x = i;
        end_x = (end_x >= N) ? N - 1 : end_x;
        for (unsigned x = start_x; x <= end_x; x++)
            sum += a[i - x] * b[x];
        output[i] = sum;
    }
}

static void convolve_complex(cfloat *output, const cfloat *a, const float *b, unsigned N)
{
    for (unsigned i = 0; i < 2 * N; i++)
    {
        cfloat sum = cfloat_create(0.0f, 0.0f);
        unsigned start_x = 0;
        start_x = (i >= N) ? (i - (N - 1)) : 0;
        unsigned end_x = i;
        end_x = (end_x >= N) ? N - 1 : end_x;
        for (unsigned x = start_x; x <= end_x; x++)
            sum = cfloat_add(sum, cfloat_mul(a[i - x], cfloat_create(b[x], 0.0f)));
        output[i] = sum;
    }
}

static void test_conv(unsigned N, unsigned flags)
{
    float *a = mufft_calloc((N / 2) * sizeof(float));
    float *b = mufft_calloc((N / 2) * sizeof(float));

    srand(0);
    for (unsigned i = 0; i < N / 2; i++)
    {
        a[i] = (float)rand() / RAND_MAX - 0.5f;
        b[i] = (float)rand() / RAND_MAX - 0.5f;
    }

    float *output = mufft_alloc(N * sizeof(float));
    float *ref_output = mufft_alloc(N * sizeof(float));
    mufft_plan_conv *plan = mufft_create_plan_conv(N, flags, MUFFT_CONV_METHOD_FLAG_MONO_MONO | MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_FIRST | MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_SECOND);
    mufft_assert(plan != NULL);

    void *block0 = mufft_calloc(mufft_conv_get_transformed_block_size(plan));
    void *block1 = mufft_calloc(mufft_conv_get_transformed_block_size(plan));

    mufft_execute_conv_input(plan, 0, block0, a);
    mufft_execute_conv_input(plan, 1, block1, b);
    mufft_execute_conv_output(plan, output, block0, block1);

    convolve_float(ref_output, a, b, N / 2);

    const float epsilon = 0.000002f * sqrtf(N);
    for (unsigned i = 0; i < N; i++)
    {
        float delta = fabsf(ref_output[i] - output[i]);
        mufft_assert(delta < epsilon);
    }

    mufft_free(a);
    mufft_free(b);
    mufft_free(block0);
    mufft_free(block1);
    mufft_free(output);
    mufft_free(ref_output);
    mufft_free_plan_conv(plan);
}

static void test_conv_stereo(unsigned N, unsigned flags)
{
    cfloat *a = mufft_calloc((N / 2) * sizeof(cfloat));
    float *b = mufft_calloc((N / 2) * sizeof(float));

    srand(0);
    for (unsigned i = 0; i < N / 2; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        a[i] = cfloat_create(real, imag);
        b[i] = (float)rand() / RAND_MAX - 0.5f;
    }

    cfloat *output = mufft_alloc(N * sizeof(cfloat));
    cfloat *ref_output = mufft_alloc(N * sizeof(cfloat));
    mufft_plan_conv *plan = mufft_create_plan_conv(N, flags, MUFFT_CONV_METHOD_FLAG_STEREO_MONO | MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_FIRST | MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_SECOND);
    mufft_assert(plan != NULL);

    void *block0 = mufft_calloc(mufft_conv_get_transformed_block_size(plan));
    void *block1 = mufft_calloc(mufft_conv_get_transformed_block_size(plan));

    mufft_execute_conv_input(plan, 0, block0, a);
    mufft_execute_conv_input(plan, 1, block1, b);
    mufft_execute_conv_output(plan, output, block0, block1);
    convolve_complex(ref_output, a, b, N / 2);

    const float epsilon = 0.000002f * sqrtf(N);
    for (unsigned i = 0; i < N; i++)
    {
        float delta = cfloat_abs(cfloat_sub(ref_output[i], output[i]));
        mufft_assert(delta < epsilon);
    }

    mufft_free(a);
    mufft_free(b);
    mufft_free(block0);
    mufft_free(block1);
    mufft_free(output);
    mufft_free(ref_output);
    mufft_free_plan_conv(plan);
}

int main(void)
{
    for (unsigned N = 2; N < 128 * 1024; N <<= 1)
    {
        for (unsigned flags = 0; flags < 8; flags++)
        {
            printf("Testing 1D forward transform size %u, flags = %u.\n", N, flags);
            test_fft_1d(N, -1, flags);
            printf("    ... Passed\n");

            printf("Testing 1D inverse transform size %u, flags = %u.\n", N, flags);
            test_fft_1d(N, +1, flags);
            printf("    ... Passed\n");
            fflush(stdout);
        }
    }

    for (unsigned N = 4; N < 128 * 1024; N <<= 1)
    {
        for (unsigned flags = 0; flags < 8; flags++)
        {
            printf("Testing 1D real-to-complex transform size %u, flags = %u.\n", N, flags);
            test_fft_1d_r2c(N, flags);
            printf("    ... Passed\n");

            printf("Testing 1D zero-padded real-to-complex transform size %u, flags = %u.\n", N, flags);
            test_fft_1d_r2c_half(N, flags);
            printf("    ... Passed\n");

            printf("Testing 1D complex-to-real transform size %u, flags = %u.\n", N, flags);
            test_fft_1d_c2r(N, flags);
            printf("    ... Passed\n");
            fflush(stdout);
        }
    }

    for (unsigned N = 4; N < 32 * 1024; N <<= 1)
    {
        for (unsigned flags = 0; flags < 8; flags++)
        {
            printf("Testing 1D convolution transform size %u, flags = %u.\n", N, flags);
            test_conv(N, flags);
            printf("    ... Passed\n");

            printf("Testing 1D convolution transform size %u, flags = %u.\n", N, flags);
            test_conv_stereo(N, flags);
            printf("    ... Passed\n");
            fflush(stdout);
        }
    }

    for (unsigned Ny = 2; Ny < 1024; Ny <<= 1)
    {
        for (unsigned Nx = 2; Nx < 1024; Nx <<= 1)
        {
            for (unsigned flags = 0; flags < 8; flags++)
            {
                printf("Testing 2D forward transform size %u-by-%u, flags = %u.\n", Nx, Ny, flags);
                test_fft_2d(Nx, Ny, -1, flags);
                printf("    ... Passed\n");

                printf("Testing 2D inverse transform size %u-by-%u, flags = %u.\n", Nx, Ny, flags);
                test_fft_2d(Nx, Ny, +1, flags);
                printf("    ... Passed\n");
                fflush(stdout);
            }
        }
    }

    for (unsigned Ny = 4; Ny < 1024; Ny <<= 1)
    {
        for (unsigned Nx = 4; Nx < 1024; Nx <<= 1)
        {
            for (unsigned flags = 0; flags < 8; flags++)
            {
                printf("Testing 2D real-to-complex transform size %u-by-%u, flags = %u.\n", Nx, Ny, flags);
                test_fft_2d_r2c(Nx, Ny, flags);
                printf("    ... Passed\n");

                printf("Testing 2D complex-to-real transform size %u-by-%u, flags = %u.\n", Nx, Ny, flags);
                test_fft_2d_c2r(Nx, Ny, flags);
                printf("    ... Passed\n");
                fflush(stdout);
            }
        }
    }

    fftwf_cleanup();
    printf("All tests passed!\n");
}

