/* Scalar/SIMD regression tests; no FFTW dependency. */
#include "fft_internal.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Failed: %s at line %d\n", #condition, __LINE__); \
    exit(EXIT_FAILURE); } } while (0)

static unsigned comparisons;
static uint32_t seed = 1;

static float *buffer(unsigned count)
{
    float *p = mufft_calloc(count * sizeof(float));
    CHECK(p != NULL);
    return p;
}

static void fill(float *p, unsigned count)
{
    for (unsigned i = 0; i < count; i++)
    {
        seed = seed * 1664525u + 1013904223u;
        p[i] = (float)(seed >> 8) / 16777216.0f - 0.5f;
    }
}

static void compare(const float *actual, const float *expected,
                    unsigned count, float tolerance)
{
    for (unsigned i = 0; i < count; i++)
    {
        if (!isfinite(actual[i]) || !isfinite(expected[i]) ||
            fabsf(actual[i] - expected[i]) > tolerance)
        {
            fprintf(stderr, "Sample %u: %.9g != %.9g (tolerance %.9g)\n",
                    i, actual[i], expected[i], tolerance);
            exit(EXIT_FAILURE);
        }
    }
    comparisons++;
}

static void test_complex(unsigned n, int direction, unsigned flags)
{
    unsigned count = flags & MUFFT_FLAG_ZERO_PAD_UPPER_HALF ? n : 2 * n;
    float *input = buffer(count);
    float *scalar = buffer(2 * n);
    float *simd = buffer(2 * n);
    fill(input, count);
    mufft_plan_1d *a = mufft_create_plan_1d_c2c(n, direction, flags);
    mufft_plan_1d *b = mufft_create_plan_1d_c2c(n, direction, flags | MUFFT_FLAG_CPU_NO_SIMD);
    CHECK(a && b);
    mufft_execute_plan_1d(a, simd, input);
    mufft_execute_plan_1d(b, scalar, input);
    compare(simd, scalar, 2 * n, 0.000003f * sqrtf(n));

    /* Independent double-precision DFT for small transforms. */
    if (n <= 32)
    {
        float *reference = buffer(2 * n);
        for (unsigned k = 0; k < n; k++)
        {
            double re = 0.0, im = 0.0;
            for (unsigned j = 0; j < count / 2; j++)
            {
                double angle = direction * 6.283185307179586 * j * k / n;
                re += input[2 * j] * cos(angle) - input[2 * j + 1] * sin(angle);
                im += input[2 * j] * sin(angle) + input[2 * j + 1] * cos(angle);
            }
            reference[2 * k] = (float)re;
            reference[2 * k + 1] = (float)im;
        }
        compare(simd, reference, 2 * n, 0.000003f * sqrtf(n));
        mufft_free(reference);
    }
    mufft_free_plan_1d(a);
    mufft_free_plan_1d(b);
    mufft_free(input);
    mufft_free(scalar);
    mufft_free(simd);
}

static void test_real(unsigned n, unsigned flags, unsigned signal)
{
    unsigned count = flags & MUFFT_FLAG_ZERO_PAD_UPPER_HALF ? n / 2 : n;
    unsigned bins = flags & MUFFT_FLAG_FULL_R2C ? 2 * n : n + 2;
    float *input = buffer(count);
    float *scalar = buffer(bins);
    float *simd = buffer(bins);
    float *inverse = buffer(n);
    float *inverse_scalar = buffer(n);
    if (signal == 0)
        fill(input, count);
    else if (signal == 1)
        input[0] = 1.0f;
    else
        for (unsigned i = 0; i < count; i++)
            input[i] = signal == 2 ? 0.25f : (i & 1 ? -0.25f : 0.25f);

    mufft_plan_1d *a = mufft_create_plan_1d_r2c(n, flags);
    mufft_plan_1d *b = mufft_create_plan_1d_r2c(n, flags | MUFFT_FLAG_CPU_NO_SIMD);
    mufft_plan_1d *c = mufft_create_plan_1d_c2r(n, 0);
    mufft_plan_1d *d = mufft_create_plan_1d_c2r(n, MUFFT_FLAG_CPU_NO_SIMD);
    CHECK(a && b && c && d);
    mufft_execute_plan_1d(a, simd, input);
    mufft_execute_plan_1d(b, scalar, input);
    compare(simd, scalar, bins, 0.000003f * sqrtf(n));
    /* The same spectrum isolates inverse-transform correctness. */
    mufft_execute_plan_1d(c, inverse, simd);
    mufft_execute_plan_1d(d, inverse_scalar, simd);
    for (unsigned i = 0; i < n; i++)
    {
        inverse[i] /= n;
        inverse_scalar[i] /= n;
    }
    compare(inverse, inverse_scalar, n, 0.000003f);
    compare(inverse, input, count, 0.000003f);
    for (unsigned i = count; i < n; i++)
        CHECK(fabsf(inverse[i]) < 0.000003f);
    mufft_free_plan_1d(a);
    mufft_free_plan_1d(b);
    mufft_free_plan_1d(c);
    mufft_free_plan_1d(d);
    mufft_free(input);
    mufft_free(scalar);
    mufft_free(simd);
    mufft_free(inverse);
    mufft_free(inverse_scalar);
}

static void test_2d(unsigned nx, unsigned ny, int direction, int real)
{
    unsigned n = nx * ny;
    float *input = buffer(2 * n);
    float *scalar = buffer(2 * n);
    float *simd = buffer(2 * n);
    fill(input, 2 * n);
    mufft_plan_2d *a, *b;
    if (real && direction == MUFFT_FORWARD)
    {
        a = mufft_create_plan_2d_r2c(nx, ny, MUFFT_FLAG_FULL_R2C);
        b = mufft_create_plan_2d_r2c(nx, ny, MUFFT_FLAG_FULL_R2C | MUFFT_FLAG_CPU_NO_SIMD);
    }
    else if (real)
    {
        mufft_plan_2d *forward = mufft_create_plan_2d_r2c(nx, ny, MUFFT_FLAG_FULL_R2C | MUFFT_FLAG_CPU_NO_SIMD);
        CHECK(forward);
        mufft_execute_plan_2d(forward, scalar, input);
        memcpy(input, scalar, 2 * n * sizeof(float));
        mufft_free_plan_2d(forward);
        a = mufft_create_plan_2d_c2r(nx, ny, 0);
        b = mufft_create_plan_2d_c2r(nx, ny, MUFFT_FLAG_CPU_NO_SIMD);
    }
    else
    {
        a = mufft_create_plan_2d_c2c(nx, ny, direction, 0);
        b = mufft_create_plan_2d_c2c(nx, ny, direction, MUFFT_FLAG_CPU_NO_SIMD);
    }
    CHECK(a && b);
    mufft_execute_plan_2d(a, simd, input);
    mufft_execute_plan_2d(b, scalar, input);
    unsigned count = real && direction == MUFFT_INVERSE ? n : 2 * n;
    if (real && direction == MUFFT_INVERSE)
        for (unsigned i = 0; i < count; i++)
        {
            simd[i] /= n;
            scalar[i] /= n;
        }
    compare(simd, scalar, count, 0.000003f * sqrtf(n));
    mufft_free_plan_2d(a);
    mufft_free_plan_2d(b);
    mufft_free(input);
    mufft_free(scalar);
    mufft_free(simd);
}

static void test_convolve(unsigned n)
{
    float *a = buffer(2 * n), *b = buffer(2 * n);
    float *scalar = buffer(2 * n), *simd = buffer(2 * n);
    fill(a, 2 * n);
    fill(b, 2 * n);
    mufft_get_convolve_func(0)(simd, a, b, 1.0f / n, n);
    mufft_get_convolve_func(MUFFT_FLAG_CPU_NO_SIMD)(scalar, a, b, 1.0f / n, n);
    compare(simd, scalar, 2 * n, 0.000001f);
    mufft_free(a);
    mufft_free(b);
    mufft_free(scalar);
    mufft_free(simd);
}

static double measure(mufft_plan_1d *plan, float *output, float *input,
                      unsigned iterations)
{
    for (unsigned i = 0; i < 200; i++)
        mufft_execute_plan_1d(plan, output, input);
    clock_t start = clock();
    for (unsigned i = 0; i < iterations; i++)
        mufft_execute_plan_1d(plan, output, input);
    return (double)(clock() - start) / CLOCKS_PER_SEC / iterations * 1e6;
}

static int sort_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static void benchmark(void)
{
    puts("transform,size,scalar_us,simd_us,speedup");
    for (unsigned n = 1024; n <= 8192; n *= 2)
        for (unsigned inverse = 0; inverse < 2; inverse++)
        {
            float *input = buffer(n + 2), *output = buffer(n + 2);
            fill(input, n + 2);
            input[1] = input[n + 1] = 0.0f;
            mufft_plan_1d *a = inverse ? mufft_create_plan_1d_c2r(n, 0) : mufft_create_plan_1d_r2c(n, 0);
            mufft_plan_1d *b = inverse ? mufft_create_plan_1d_c2r(n, MUFFT_FLAG_CPU_NO_SIMD) : mufft_create_plan_1d_r2c(n, MUFFT_FLAG_CPU_NO_SIMD);
            CHECK(a && b);
            double simd[7], scalar[7];
            for (unsigned trial = 0; trial < 7; trial++)
                for (unsigned order = 0; order < 2; order++)
                {
                    unsigned iterations = 20000000 / n;
                    if ((trial + order) & 1)
                        simd[trial] = measure(a, output, input, iterations);
                    else
                        scalar[trial] = measure(b, output, input, iterations);
                }
            qsort(simd, 7, sizeof(double), sort_double);
            qsort(scalar, 7, sizeof(double), sort_double);
            printf("%s,%u,%.3f,%.3f,%.2f\n", inverse ? "C2R" : "R2C",
                    n, scalar[3], simd[3], scalar[3] / simd[3]);
            mufft_free_plan_1d(a);
            mufft_free_plan_1d(b);
            mufft_free(input);
            mufft_free(output);
        }
}

int main(int argc, char **argv)
{
#ifdef MUFFT_TEST_EXPECT_NEON
    CHECK(mufft_get_cpu_flags() == MUFFT_FLAG_CPU_SSE3);
    CHECK(mufft_get_convolve_func(0) != mufft_get_convolve_func(MUFFT_FLAG_CPU_NO_SIMD));
    CHECK(mufft_get_convolve_func(MUFFT_FLAG_CPU_NO_SSE3) == mufft_get_convolve_func(MUFFT_FLAG_CPU_NO_SIMD));
#endif
    for (unsigned n = 2; n <= 16384; n *= 2)
        for (int direction = MUFFT_FORWARD; direction <= MUFFT_INVERSE; direction += 2)
        {
            test_complex(n, direction, 0);
            test_complex(n, direction, MUFFT_FLAG_ZERO_PAD_UPPER_HALF);
        }
    for (unsigned n = 4; n <= 16384; n *= 2)
    {
        test_convolve(n);
        for (unsigned variant = 0; variant < 4; variant++)
            for (unsigned signal = 0; signal < 4; signal++)
                test_real(n, (variant & 1 ? MUFFT_FLAG_FULL_R2C : 0) |
                          (variant & 2 ? MUFFT_FLAG_ZERO_PAD_UPPER_HALF : 0), signal);
    }
    for (unsigned nx = 4; nx <= 64; nx *= 2)
        for (unsigned ny = 2; ny <= 64; ny *= 2)
            for (int direction = MUFFT_FORWARD; direction <= MUFFT_INVERSE; direction += 2)
                for (int real = 0; real < 2; real++)
                    test_2d(nx, ny, direction, real);
    printf("Passed %u numerical comparisons.\n", comparisons);
    if (argc > 1 && strcmp(argv[1], "--benchmark") == 0)
        benchmark();
    return EXIT_SUCCESS;
}
