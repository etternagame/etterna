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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fftw3.h> // Used as a reference.

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static double mufft_get_time(void)
{
	LARGE_INTEGER cnt, freq;
	QueryPerformanceCounter(&cnt);
	QueryPerformanceFrequency(&freq);
	return (double)cnt.QuadPart / (double)freq.QuadPart;
}
#else
#include <time.h>
static double mufft_get_time(void)
{
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return tv.tv_sec + tv.tv_nsec / 1000000000.0;
}
#endif

static double bench_fftw_1d(unsigned N, unsigned iterations, unsigned flags)
{
    cfloat *input = fftwf_malloc(N * sizeof(cfloat));
    cfloat *output = fftwf_malloc(N * sizeof(cfloat));

    fftwf_plan plan = fftwf_plan_dft_1d(N, (fftwf_complex *)input, (fftwf_complex *)output,
                                        FFTW_FORWARD, flags);

    srand(0);
    for (unsigned i = 0; i < N; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        fftwf_execute(plan);
    }
    double end_time = mufft_get_time();

    fftwf_free(input);
    fftwf_free(output);
    fftwf_destroy_plan(plan);

    return end_time - start_time;
}

static double bench_fftw_1d_real(unsigned N, unsigned iterations, unsigned flags)
{
    unsigned fftN = N / 2 + 1;
    float *input = fftwf_malloc(N * sizeof(float));
    float *dummy = fftwf_malloc(N * sizeof(float));
    cfloat *output = fftwf_malloc(fftN * sizeof(cfloat));

    fftwf_plan plan_r2c = fftwf_plan_dft_r2c_1d(N, input, (fftwf_complex *)output,
            flags);
    fftwf_plan plan_c2r = fftwf_plan_dft_c2r_1d(N, (fftwf_complex *)output, dummy,
            flags);

    srand(0);
    for (unsigned i = 0; i < N; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        input[i] = real;
    }

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        fftwf_execute(plan_r2c);
        fftwf_execute(plan_c2r);
    }
    double end_time = mufft_get_time();

    fftwf_free(input);
    fftwf_free(dummy);
    fftwf_free(output);
    fftwf_destroy_plan(plan_r2c);
    fftwf_destroy_plan(plan_c2r);

    return end_time - start_time;
}

static double bench_fftw_2d(unsigned Nx, unsigned Ny, unsigned iterations, unsigned flags)
{
    cfloat *input = fftwf_malloc(Nx * Ny * sizeof(cfloat));
    cfloat *output = fftwf_malloc(Nx * Ny * sizeof(cfloat));

    fftwf_plan plan = fftwf_plan_dft_2d(Ny, Nx, (fftwf_complex *)input, (fftwf_complex *)output,
            FFTW_FORWARD, flags);

    srand(0);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        fftwf_execute(plan);
    }
    double end_time = mufft_get_time();

    fftwf_free(input);
    fftwf_free(output);
    fftwf_destroy_plan(plan);

    return end_time - start_time;
}

static double bench_fftw_2d_r2c(unsigned Nx, unsigned Ny, unsigned iterations, unsigned flags)
{
    float *input = fftwf_malloc(Nx * Ny * sizeof(float));
    cfloat *output = fftwf_malloc(Nx * Ny * sizeof(cfloat));

    fftwf_plan plan = fftwf_plan_dft_r2c_2d(Ny, Nx, input, (fftwf_complex *)output, flags);

    srand(0);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        input[i] = real;
    }

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        fftwf_execute(plan);
    }
    double end_time = mufft_get_time();

    fftwf_free(input);
    fftwf_free(output);
    fftwf_destroy_plan(plan);

    return end_time - start_time;
}

static double bench_fftw_2d_c2r(unsigned Nx, unsigned Ny, unsigned iterations, unsigned flags)
{
    cfloat *input = fftwf_malloc(Nx * Ny * sizeof(cfloat));
    float *output = fftwf_malloc(2 * Nx * Ny * sizeof(float));

    fftwf_plan plan = fftwf_plan_dft_c2r_2d(Ny, Nx, (fftwf_complex *)input, output, flags);

    srand(0);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        fftwf_execute(plan);
    }
    double end_time = mufft_get_time();

    fftwf_free(input);
    fftwf_free(output);
    fftwf_destroy_plan(plan);

    return end_time - start_time;
}

static double bench_fft_1d(unsigned N, unsigned iterations, unsigned flags)
{
    cfloat *input = mufft_alloc(N * sizeof(cfloat));
    cfloat *output = mufft_alloc(N * sizeof(cfloat));

    srand(0);
    for (unsigned i = 0; i < N; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }

    mufft_plan_1d *muplan = mufft_create_plan_1d_c2c(N, MUFFT_FORWARD, flags);

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        mufft_execute_plan_1d(muplan, output, input);
    }
    double end_time = mufft_get_time();

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_1d(muplan);

    return end_time - start_time;
}

static double bench_fft_conv(unsigned N, unsigned iterations, unsigned flags)
{
    float *a = mufft_alloc(N * sizeof(float));
    float *b = mufft_alloc(N * sizeof(float));
    float *output = mufft_alloc(2 * N * sizeof(float));

    srand(0);
    for (unsigned i = 0; i < N; i++)
    {
        a[i] = (float)rand() / RAND_MAX - 0.5f;
        b[i] = (float)rand() / RAND_MAX - 0.5f;
    }

    mufft_plan_conv *plan = mufft_create_plan_conv(2 * N, flags,
            MUFFT_CONV_METHOD_FLAG_MONO_MONO |
            MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_FIRST |
            MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_SECOND);

    void *block0 = mufft_calloc(mufft_conv_get_transformed_block_size(plan));
    void *block1 = mufft_calloc(mufft_conv_get_transformed_block_size(plan));

    mufft_execute_conv_input(plan, 1, block1, b);
    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        mufft_execute_conv_input(plan, 0, block0, a);
        mufft_execute_conv_output(plan, output, block0, block1);
    }
    double end_time = mufft_get_time();

    mufft_free(a);
    mufft_free(b);
    mufft_free(block0);
    mufft_free(block1);
    mufft_free(output);
    mufft_free_plan_conv(plan);

    return end_time - start_time;
}

static double bench_fft_conv_stereo(unsigned N, unsigned iterations, unsigned flags)
{
    cfloat *a = mufft_alloc(N * sizeof(cfloat));
    float *b = mufft_alloc(N * sizeof(float));
    cfloat *output = mufft_alloc(2 * N * sizeof(cfloat));

    srand(0);
    for (unsigned i = 0; i < N; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        a[i] = cfloat_create(real, imag);
        b[i] = (float)rand() / RAND_MAX - 0.5f;
    }

    mufft_plan_conv *plan = mufft_create_plan_conv(2 * N, flags,
            MUFFT_CONV_METHOD_FLAG_STEREO_MONO |
            MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_FIRST |
            MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_SECOND);

    void *block0 = mufft_calloc(mufft_conv_get_transformed_block_size(plan));
    void *block1 = mufft_calloc(mufft_conv_get_transformed_block_size(plan));

    mufft_execute_conv_input(plan, 1, block1, b);
    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        mufft_execute_conv_input(plan, 0, block0, a);
        mufft_execute_conv_output(plan, output, block0, block1);
    }
    double end_time = mufft_get_time();

    mufft_free(a);
    mufft_free(b);
    mufft_free(block0);
    mufft_free(block1);
    mufft_free(output);
    mufft_free_plan_conv(plan);

    return end_time - start_time;
}

static double bench_fft_1d_real(unsigned N, unsigned iterations, unsigned flags)
{
    unsigned fftN = N / 2 + 1;
    float *input = mufft_alloc(N * sizeof(float));
    float *dummy = mufft_alloc(N * sizeof(float));
    cfloat *output = mufft_alloc(fftN * sizeof(cfloat));

    srand(0);
    for (unsigned i = 0; i < N; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        input[i] = real;
    }

    mufft_plan_1d *muplan_r2c = mufft_create_plan_1d_r2c(N, flags);
    mufft_plan_1d *muplan_c2r = mufft_create_plan_1d_c2r(N, flags);

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        mufft_execute_plan_1d(muplan_r2c, output, input);
        mufft_execute_plan_1d(muplan_c2r, dummy, output); // To avoid input degrading over time.
    }
    double end_time = mufft_get_time();

    mufft_free(input);
    mufft_free(dummy);
    mufft_free(output);
    mufft_free_plan_1d(muplan_r2c);
    mufft_free_plan_1d(muplan_c2r);

    return end_time - start_time;
}

static double bench_fft_1d_real_half(unsigned N, unsigned iterations, unsigned flags)
{
    unsigned fftN = N / 2 + 1;
    float *input = mufft_alloc((N / 2) * sizeof(float));
    float *dummy = mufft_alloc(N * sizeof(float));
    cfloat *output = mufft_alloc(fftN * sizeof(cfloat));

    srand(0);
    for (unsigned i = 0; i < N / 2; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        input[i] = real;
    }

    mufft_plan_1d *muplan_r2c = mufft_create_plan_1d_r2c(N, flags | MUFFT_FLAG_ZERO_PAD_UPPER_HALF);
    mufft_plan_1d *muplan_c2r = mufft_create_plan_1d_c2r(N, flags);

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        mufft_execute_plan_1d(muplan_r2c, output, input);
        mufft_execute_plan_1d(muplan_c2r, dummy, output); // To avoid input degrading over time.
    }
    double end_time = mufft_get_time();

    mufft_free(input);
    mufft_free(dummy);
    mufft_free(output);
    mufft_free_plan_1d(muplan_r2c);
    mufft_free_plan_1d(muplan_c2r);

    return end_time - start_time;
}

static double bench_fft_2d(unsigned Nx, unsigned Ny, unsigned iterations, unsigned flags)
{
    cfloat *input = mufft_alloc(Nx * Ny * sizeof(cfloat));
    cfloat *output = mufft_alloc(Nx * Ny * sizeof(cfloat));

    srand(0);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }

    mufft_plan_2d *muplan = mufft_create_plan_2d_c2c(Nx, Ny, MUFFT_FORWARD, flags);

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        mufft_execute_plan_2d(muplan, output, input);
    }
    double end_time = mufft_get_time();

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_2d(muplan);

    return end_time - start_time;
}

static double bench_fft_2d_r2c(unsigned Nx, unsigned Ny, unsigned iterations, unsigned flags)
{
    float *input = mufft_alloc(Nx * Ny * sizeof(float));
    cfloat *output = mufft_alloc(Nx * Ny * sizeof(cfloat));

    srand(0);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        input[i] = real;
    }

    mufft_plan_2d *muplan = mufft_create_plan_2d_r2c(Nx, Ny, flags);

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        mufft_execute_plan_2d(muplan, output, input);
    }
    double end_time = mufft_get_time();

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_2d(muplan);

    return end_time - start_time;
}

static double bench_fft_2d_c2r(unsigned Nx, unsigned Ny, unsigned iterations, unsigned flags)
{
    cfloat *input = mufft_alloc(Nx * Ny * sizeof(cfloat));
    float *output = mufft_alloc(2 * Nx * Ny * sizeof(float));

    srand(0);
    for (unsigned i = 0; i < Nx * Ny; i++)
    {
        float real = (float)rand() / RAND_MAX - 0.5f;
        float imag = (float)rand() / RAND_MAX - 0.5f;
        input[i] = cfloat_create(real, imag);
    }

    mufft_plan_2d *muplan = mufft_create_plan_2d_c2r(Nx, Ny, flags);

    double start_time = mufft_get_time();
    for (unsigned i = 0; i < iterations; i++)
    {
        mufft_execute_plan_2d(muplan, output, input);
    }
    double end_time = mufft_get_time();

    mufft_free(input);
    mufft_free(output);
    mufft_free_plan_2d(muplan);

    return end_time - start_time;
}

static void run_benchmark_1d(unsigned N, unsigned iterations)
{
    double flops = 5.0 * N * log2(N); // Estimation
    double fftw_time = bench_fftw_1d(N, iterations, FFTW_ESTIMATE);
    double fftw_measured_time = bench_fftw_1d(N, iterations, FFTW_MEASURE);
    double mufft_time = bench_fft_1d(N, iterations, 0);
    flops *= iterations;

    double fftw_mflops = flops / (1000000.0 * fftw_time);
    double fftw_measured_mflops = flops / (1000000.0 * fftw_measured_time);
    double mufft_mflops = flops / (1000000.0 * mufft_time);

    printf("FFTW C2C estimate:      %06u %12.3f Mflops %12.3f us iteration\n",
            N, fftw_mflops, 1000000.0 * fftw_time / iterations);
    printf("FFTW C2C measure:       %06u %12.3f Mflops %12.3f us iteration\n",
            N, fftw_measured_mflops, 1000000.0 * fftw_measured_time / iterations);
    printf("muFFT C2C:              %06u %12.3f Mflops %12.3f us iteration\n",
            N, mufft_mflops, 1000000.0 * mufft_time / iterations);
    fflush(stdout);
}

static void run_benchmark_conv(unsigned N, unsigned iterations)
{
    double mufft_time = bench_fft_conv(N, iterations, 0);
    double mufft_stereo_time = bench_fft_conv_stereo(N, iterations, 0);
    printf("muFFT conv mono:        %06u %12.3f us iteration\n",
            N, 1000000.0 * mufft_time / iterations);
    printf("muFFT conv stereo:      %06u %12.3f us iteration\n",
            N, 1000000.0 * mufft_stereo_time / iterations);
    fflush(stdout);
}

static void run_benchmark_1d_real(unsigned N, unsigned iterations)
{
    double flops = 5.0 * N * log2(N); // Estimation
    double fftw_time = bench_fftw_1d_real(N, iterations, FFTW_ESTIMATE);
    double fftw_measured_time = bench_fftw_1d_real(N, iterations, FFTW_MEASURE);
    double mufft_time = bench_fft_1d_real(N, iterations, 0);
    double mufft_half_time = bench_fft_1d_real_half(N, iterations, 0);
    flops *= iterations;

    double fftw_mflops = flops / (1000000.0 * fftw_time);
    double fftw_measured_mflops = flops / (1000000.0 * fftw_measured_time);
    double mufft_mflops = flops / (1000000.0 * mufft_time);
    double mufft_half_mflops = flops / (1000000.0 * mufft_half_time);

    printf("FFTW R2C-C2R estimate:  %06u %12.3f Mflops %12.3f us iteration\n",
            N, fftw_mflops, 1000000.0 * fftw_time / iterations);
    printf("FFTW R2C-C2R measure:   %06u %12.3f Mflops %12.3f us iteration\n",
            N, fftw_measured_mflops, 1000000.0 * fftw_measured_time / iterations);
    printf("muFFT R2C-C2R:          %06u %12.3f Mflops %12.3f us iteration\n",
            N, mufft_mflops, 1000000.0 * mufft_time / iterations);
    printf("muFFT half R2C-C2R:     %06u %12.3f Mflops %12.3f us iteration\n",
            N, mufft_half_mflops, 1000000.0 * mufft_half_time / iterations);
    fflush(stdout);
}

static void run_benchmark_2d(unsigned Nx, unsigned Ny, unsigned iterations)
{
    double flops = 5.0 * Ny * Nx * log2(Nx) + 5.0 * Nx * Ny * log2(Ny); // Estimation
    double fftw_time = bench_fftw_2d(Nx, Ny, iterations, FFTW_ESTIMATE);
    double fftw_measured_time = bench_fftw_2d(Nx, Ny, iterations, FFTW_MEASURE);
    double mufft_time = bench_fft_2d(Nx, Ny, iterations, 0);
    flops *= iterations;

    double fftw_mflops = flops / (1000000.0 * fftw_time);
    double fftw_measured_mflops = flops / (1000000.0 * fftw_measured_time);
    double mufft_mflops = flops / (1000000.0 * mufft_time);

    printf("FFTW estimate:          %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, fftw_mflops, 1000000.0 * fftw_time / iterations);
    printf("FFTW measure:           %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, fftw_measured_mflops, 1000000.0 * fftw_measured_time / iterations);
    printf("muFFT:                  %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, mufft_mflops, 1000000.0 * mufft_time / iterations);
    fflush(stdout);
}

static void run_benchmark_2d_r2c(unsigned Nx, unsigned Ny, unsigned iterations)
{
    double flops = 2.5 * Ny * Nx * log2(Nx) + 2.5 * Nx * Ny * log2(Ny); // Estimation
    double fftw_time = bench_fftw_2d_r2c(Nx, Ny, iterations, FFTW_ESTIMATE);
    double fftw_measured_time = bench_fftw_2d_r2c(Nx, Ny, iterations, FFTW_MEASURE);
    double mufft_time = bench_fft_2d_r2c(Nx, Ny, iterations, 0);
    flops *= iterations;

    double fftw_mflops = flops / (1000000.0 * fftw_time);
    double fftw_measured_mflops = flops / (1000000.0 * fftw_measured_time);
    double mufft_mflops = flops / (1000000.0 * mufft_time);

    printf("FFTW R2C estimate:      %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, fftw_mflops, 1000000.0 * fftw_time / iterations);
    printf("FFTW R2C measure:       %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, fftw_measured_mflops, 1000000.0 * fftw_measured_time / iterations);
    printf("muFFT R2C:              %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, mufft_mflops, 1000000.0 * mufft_time / iterations);
    fflush(stdout);
}

static void run_benchmark_2d_c2r(unsigned Nx, unsigned Ny, unsigned iterations)
{
    double flops = 2.5 * Ny * Nx * log2(Nx) + 2.5 * Nx * Ny * log2(Ny); // Estimation
    double fftw_time = bench_fftw_2d_c2r(Nx, Ny, iterations, FFTW_ESTIMATE);
    double fftw_measured_time = bench_fftw_2d_c2r(Nx, Ny, iterations, FFTW_MEASURE);
    double mufft_time = bench_fft_2d_c2r(Nx, Ny, iterations, 0);
    flops *= iterations;

    double fftw_mflops = flops / (1000000.0 * fftw_time);
    double fftw_measured_mflops = flops / (1000000.0 * fftw_measured_time);
    double mufft_mflops = flops / (1000000.0 * mufft_time);

    printf("FFTW C2R estimate:      %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, fftw_mflops, 1000000.0 * fftw_time / iterations);
    printf("FFTW C2R measure:       %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, fftw_measured_mflops, 1000000.0 * fftw_measured_time / iterations);
    printf("muFFT C2R:              %04u by %04u, %12.3f Mflops %12.3f us iteration\n",
            Nx, Ny, mufft_mflops, 1000000.0 * mufft_time / iterations);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc == 2 || argc > 4)
    {
        fprintf(stderr, "Usage: %s [iterations] [Nx] [Ny]\n",
                argv[0]);
        return 1;
    }

    if (argc == 1)
    {
        printf("\n1D benchmarks ...\n");
        for (unsigned N = 4; N <= 128 * 1024; N <<= 1)
        {
            run_benchmark_1d(N, 400000000ull / (N + 16));
            run_benchmark_1d_real(N, 400000000ull / (N + 16));
            run_benchmark_conv(N, 400000000ull / (N + 16));
        }

        printf("\n2D benchmarks ...\n");
        for (unsigned Ny = 4; Ny <= 1024; Ny <<= 1)
        {
            for (unsigned Nx = 4; Nx <= 1024; Nx <<= 1)
            {
                run_benchmark_2d(Nx, Ny, 400000000ull / (Nx * Ny + 16));
            }
        }
    }
    else if (argc == 3)
    {
        unsigned iterations = strtoul(argv[1], NULL, 0);
        unsigned Nx = strtoul(argv[2], NULL, 0);
        run_benchmark_1d(Nx, iterations);
        run_benchmark_1d_real(Nx, iterations);
        run_benchmark_conv(Nx, iterations);
    }
    else if (argc == 4)
    {
        unsigned iterations = strtoul(argv[1], NULL, 0);
        unsigned Nx = strtoul(argv[2], NULL, 0);
        unsigned Ny = strtoul(argv[3], NULL, 0);
        run_benchmark_2d(Nx, Ny, iterations);
        run_benchmark_2d_r2c(Nx, Ny, iterations);
        run_benchmark_2d_c2r(Nx, Ny, iterations);
    }

    fftwf_cleanup();
}

