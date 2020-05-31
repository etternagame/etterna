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
#include <math.h>

static void mufft_convolve_inner_c(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT a, const cfloat * MUFFT_RESTRICT b,
        float normalization, unsigned samples)
{
    for (unsigned i = 0; i < samples; i++)
    {
        output[i] = cfloat_mul_scalar(normalization, cfloat_mul(a[i], b[i]));
    }
}

void mufft_convolve_c(void *output, const void *a, const void *b, float normalization, unsigned samples)
{
    mufft_convolve_inner_c(output, a, b, normalization, samples);
}

void mufft_resolve_c2r_c(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned samples)
{
    for (unsigned i = 0; i < samples; i++)
    {
        cfloat a = input[i];
        cfloat b = cfloat_conj(input[samples - i]);
        output[i] = cfloat_add(cfloat_add(a, b), cfloat_mul(cfloat_sub(a, b), twiddles[i]));
    }
}

void mufft_resolve_r2c_c(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned samples)
{
    cfloat fe = cfloat_real(input[0]);
    cfloat fo = cfloat_imag(input[0]);
    output[0] = cfloat_add(fe, fo);
    output[samples] = cfloat_sub(fe, fo);

    for (unsigned i = 1; i < samples; i++)
    {
        cfloat a = input[i];
        cfloat b = cfloat_conj(input[samples - i]);
        cfloat fe = cfloat_add(a, b);
        cfloat fo = cfloat_mul(twiddles[i], cfloat_sub(a, b));
        output[i] = cfloat_mul_scalar(0.5f, cfloat_add(fe, fo));
    }
}

void mufft_resolve_r2c_full_c(cfloat * MUFFT_RESTRICT output, const cfloat * MUFFT_RESTRICT input,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned samples)
{
    cfloat fe = cfloat_real(input[0]);
    cfloat fo = cfloat_imag(input[0]);
    output[0] = cfloat_add(fe, fo);
    output[samples] = cfloat_sub(fe, fo);

    for (unsigned i = 1; i < samples; i++)
    {
        cfloat a = input[i];
        cfloat b = cfloat_conj(input[samples - i]);
        cfloat fe = cfloat_add(a, b);
        cfloat fo = cfloat_mul(twiddles[i], cfloat_sub(a, b));
        output[i] = cfloat_mul_scalar(0.5f, cfloat_add(fe, fo));
        output[i + samples] = cfloat_mul_scalar(0.5f, cfloat_sub(fe, fo));
    }
}

void mufft_radix2_p1_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned half_samples = samples >> 1;
    for (unsigned i = 0; i < half_samples; i++)
    {
        cfloat a = input[i];
        cfloat b = input[i + half_samples]; 

        unsigned j = i << 1;
        output[j + 0] = cfloat_add(a, b);
        output[j + 1] = cfloat_sub(a, b);
    }
}

void mufft_radix2_half_p1_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned half_samples = samples >> 1;
    for (unsigned i = 0; i < half_samples; i++)
    {
        cfloat a = input[i];
        cfloat b = cfloat_create(0.0f, 0.0f);

        unsigned j = i << 1;
        output[j + 0] = cfloat_add(a, b);
        output[j + 1] = cfloat_sub(a, b);
    }
}

void mufft_forward_radix2_p2_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned half_samples = samples >> 1;
    for (unsigned i = 0; i < half_samples; i++)
    {
        unsigned k = i & (2 - 1);
        cfloat a = input[i];
        cfloat b = cfloat_mul(twiddles[k], input[i + half_samples]);

        unsigned j = (i << 1) - k;
        output[j + 0] = cfloat_add(a, b);
        output[j + 2] = cfloat_sub(a, b);
    }
}

void mufft_inverse_radix2_p2_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    mufft_forward_radix2_p2_c(output_, input_, twiddles, p, samples);
}

void mufft_radix2_generic_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned half_samples = samples >> 1;
    for (unsigned i = 0; i < half_samples; i++)
    {
        unsigned k = i & (p - 1);
        cfloat a = input[i];
        cfloat b = cfloat_mul(twiddles[k], input[i + half_samples]);

        unsigned j = (i << 1) - k;
        output[j + 0] = cfloat_add(a, b);
        output[j + p] = cfloat_sub(a, b);
    }
}

void mufft_forward_radix4_p1_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned quarter_samples = samples >> 2;
    for (unsigned i = 0; i < quarter_samples; i++)
    {
        cfloat a = input[i];
        cfloat b = input[i + quarter_samples];
        cfloat c = input[i + 2 * quarter_samples];
        cfloat d = input[i + 3 * quarter_samples];

        cfloat r0 = cfloat_add(a, c);
        cfloat r1 = cfloat_sub(a, c);
        cfloat r2 = cfloat_add(b, d);
        cfloat r3 = cfloat_sub(b, d);
        r3 = cfloat_mul(r3, twiddles[2]);

        unsigned j = i << 2;
        output[j + 0] = cfloat_add(r0, r2);
        output[j + 1] = cfloat_add(r1, r3);
        output[j + 2] = cfloat_sub(r0, r2);
        output[j + 3] = cfloat_sub(r1, r3);
    }
}

void mufft_forward_half_radix4_p1_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned quarter_samples = samples >> 2;
    for (unsigned i = 0; i < quarter_samples; i++)
    {
        cfloat a = input[i];
        cfloat b = input[i + quarter_samples];

        cfloat r0 = a;
        cfloat r1 = a;
        cfloat r2 = b;
        cfloat r3 = b;
        r3 = cfloat_mul(r3, twiddles[2]);

        unsigned j = i << 2;
        output[j + 0] = cfloat_add(r0, r2);
        output[j + 1] = cfloat_add(r1, r3);
        output[j + 2] = cfloat_sub(r0, r2);
        output[j + 3] = cfloat_sub(r1, r3);
    }
}

void mufft_inverse_radix4_p1_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
                               const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    mufft_forward_radix4_p1_c(output_, input_, twiddles, p, samples);
}

void mufft_radix4_generic_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
                            const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned quarter_samples = samples >> 2;
    for (unsigned i = 0; i < quarter_samples; i++)
    {
        unsigned k = i & (p - 1);

        cfloat a = input[i];
        cfloat b = input[i + quarter_samples];
        cfloat c = cfloat_mul(twiddles[k], input[i + 2 * quarter_samples]);
        cfloat d = cfloat_mul(twiddles[k], input[i + 3 * quarter_samples]);

        // DFT-2
        cfloat r0 = cfloat_add(a, c);
        cfloat r1 = cfloat_sub(a, c);
        cfloat r2 = cfloat_add(b, d);
        cfloat r3 = cfloat_sub(b, d);

        r2 = cfloat_mul(r2, twiddles[p + k]);
        r3 = cfloat_mul(r3, twiddles[p + k + p]);

        // DFT-2
        cfloat o0 = cfloat_add(r0, r2);
        cfloat o1 = cfloat_sub(r0, r2);
        cfloat o2 = cfloat_add(r1, r3);
        cfloat o3 = cfloat_sub(r1, r3);

        unsigned j = ((i - k) << 2) + k;
        output[j +     0] = o0;
        output[j + 1 * p] = o2;
        output[j + 2 * p] = o1;
        output[j + 3 * p] = o3;
    }
}

void mufft_forward_radix8_p1_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned octa_samples = samples >> 3;
    for (unsigned i = 0; i < octa_samples; i++)
    {
        cfloat a = input[i];
        cfloat b = input[i + octa_samples];
        cfloat c = input[i + 2 * octa_samples];
        cfloat d = input[i + 3 * octa_samples];
        cfloat e = input[i + 4 * octa_samples];
        cfloat f = input[i + 5 * octa_samples];
        cfloat g = input[i + 6 * octa_samples];
        cfloat h = input[i + 7 * octa_samples];

        cfloat r0 = cfloat_add(a, e); // 0O + 0
        cfloat r1 = cfloat_sub(a, e); // 0O + 1
        cfloat r2 = cfloat_add(b, f); // 2O + 0
        cfloat r3 = cfloat_sub(b, f); // 2O + 1
        cfloat r4 = cfloat_add(c, g); // 4O + 0
        cfloat r5 = cfloat_sub(c, g); // 4O + 1
        cfloat r6 = cfloat_add(d, h); // 60 + 0
        cfloat r7 = cfloat_sub(d, h); // 6O + 1

        // p == 2 twiddles
        r5 = cfloat_mul(r5, twiddles[2]);
        r7 = cfloat_mul(r7, twiddles[2]);

        a = cfloat_add(r0, r4); // 0O + 0
        b = cfloat_add(r1, r5); // 0O + 1
        c = cfloat_sub(r0, r4); // 00 + 2
        d = cfloat_sub(r1, r5); // O0 + 3
        e = cfloat_add(r2, r6); // 4O + 0
        f = cfloat_add(r3, r7); // 4O + 1
        g = cfloat_sub(r2, r6); // 4O + 2
        h = cfloat_sub(r3, r7); // 4O + 3

        // p == 4 twiddles
        e = cfloat_mul(e, twiddles[4]);
        f = cfloat_mul(f, twiddles[5]);
        g = cfloat_mul(g, twiddles[6]);
        h = cfloat_mul(h, twiddles[7]);

        unsigned j = i << 3;
        output[j + 0] = cfloat_add(a, e);
        output[j + 1] = cfloat_add(b, f);
        output[j + 2] = cfloat_add(c, g);
        output[j + 3] = cfloat_add(d, h);
        output[j + 4] = cfloat_sub(a, e);
        output[j + 5] = cfloat_sub(b, f);
        output[j + 6] = cfloat_sub(c, g);
        output[j + 7] = cfloat_sub(d, h);
    }
}

void mufft_forward_half_radix8_p1_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned octa_samples = samples >> 3;
    for (unsigned i = 0; i < octa_samples; i++)
    {
        cfloat a = input[i];
        cfloat b = input[i + octa_samples];
        cfloat c = input[i + 2 * octa_samples];
        cfloat d = input[i + 3 * octa_samples];

        cfloat r0 = a; // 0O + 0
        cfloat r1 = a; // 0O + 1
        cfloat r2 = b; // 2O + 0
        cfloat r3 = b; // 2O + 1
        cfloat r4 = c; // 4O + 0
        cfloat r5 = c; // 4O + 1
        cfloat r6 = d; // 60 + 0
        cfloat r7 = d; // 6O + 1

        // p == 2 twiddles
        r5 = cfloat_mul(r5, twiddles[2]);
        r7 = cfloat_mul(r7, twiddles[2]);

        a = cfloat_add(r0, r4); // 0O + 0
        b = cfloat_add(r1, r5); // 0O + 1
        c = cfloat_sub(r0, r4); // 00 + 2
        d = cfloat_sub(r1, r5); // O0 + 3
        cfloat e = cfloat_add(r2, r6); // 4O + 0
        cfloat f = cfloat_add(r3, r7); // 4O + 1
        cfloat g = cfloat_sub(r2, r6); // 4O + 2
        cfloat h = cfloat_sub(r3, r7); // 4O + 3

        // p == 4 twiddles
        e = cfloat_mul(e, twiddles[4]);
        f = cfloat_mul(f, twiddles[5]);
        g = cfloat_mul(g, twiddles[6]);
        h = cfloat_mul(h, twiddles[7]);

        unsigned j = i << 3;
        output[j + 0] = cfloat_add(a, e);
        output[j + 1] = cfloat_add(b, f);
        output[j + 2] = cfloat_add(c, g);
        output[j + 3] = cfloat_add(d, h);
        output[j + 4] = cfloat_sub(a, e);
        output[j + 5] = cfloat_sub(b, f);
        output[j + 6] = cfloat_sub(c, g);
        output[j + 7] = cfloat_sub(d, h);
    }
}

void mufft_inverse_radix8_p1_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    mufft_forward_radix8_p1_c(output_, input_, twiddles, p, samples);
}

void mufft_radix8_generic_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned octa_samples = samples >> 3;
    for (unsigned i = 0; i < octa_samples; i++)
    {
        unsigned k = i & (p - 1);
        cfloat a = input[i];
        cfloat b = input[i + octa_samples];
        cfloat c = input[i + 2 * octa_samples];
        cfloat d = input[i + 3 * octa_samples];
        cfloat e = cfloat_mul(twiddles[k], input[i + 4 * octa_samples]);
        cfloat f = cfloat_mul(twiddles[k], input[i + 5 * octa_samples]);
        cfloat g = cfloat_mul(twiddles[k], input[i + 6 * octa_samples]);
        cfloat h = cfloat_mul(twiddles[k], input[i + 7 * octa_samples]);

        cfloat r0 = cfloat_add(a, e); // 0O + 0
        cfloat r1 = cfloat_sub(a, e); // 0O + 1
        cfloat r2 = cfloat_add(b, f); // 2O + 0
        cfloat r3 = cfloat_sub(b, f); // 2O + 1
        cfloat r4 = cfloat_add(c, g); // 4O + 0
        cfloat r5 = cfloat_sub(c, g); // 4O + 1
        cfloat r6 = cfloat_add(d, h); // 60 + 0
        cfloat r7 = cfloat_sub(d, h); // 6O + 1

        r4 = cfloat_mul(r4, twiddles[p + k]);
        r5 = cfloat_mul(r5, twiddles[p + k + p]);
        r6 = cfloat_mul(r6, twiddles[p + k]);
        r7 = cfloat_mul(r7, twiddles[p + k + p]);

        a = cfloat_add(r0, r4); // 0O + 0
        b = cfloat_add(r1, r5); // 0O + 1
        c = cfloat_sub(r0, r4); // 00 + 2
        d = cfloat_sub(r1, r5); // O0 + 3
        e = cfloat_add(r2, r6); // 4O + 0
        f = cfloat_add(r3, r7); // 4O + 1
        g = cfloat_sub(r2, r6); // 4O + 2
        h = cfloat_sub(r3, r7); // 4O + 3

        // p == 4 twiddles
        e = cfloat_mul(e, twiddles[3 * p + k]);
        f = cfloat_mul(f, twiddles[3 * p + k + p]);
        g = cfloat_mul(g, twiddles[3 * p + k + 2 * p]);
        h = cfloat_mul(h, twiddles[3 * p + k + 3 * p]);

        unsigned j = ((i - k) << 3) + k;
        output[j + 0 * p] = cfloat_add(a, e);
        output[j + 1 * p] = cfloat_add(b, f);
        output[j + 2 * p] = cfloat_add(c, g);
        output[j + 3 * p] = cfloat_add(d, h);
        output[j + 4 * p] = cfloat_sub(a, e);
        output[j + 5 * p] = cfloat_sub(b, f);
        output[j + 6 * p] = cfloat_sub(c, g);
        output[j + 7 * p] = cfloat_sub(d, h);
    }
}

void mufft_radix2_p1_vert_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
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
        for (unsigned i = 0; i < samples_x; i++)
        {
            cfloat a = input[i];
            cfloat b = input[i + half_stride];

            cfloat r0 = cfloat_add(a, b); // 0O + 0
            cfloat r1 = cfloat_sub(a, b); // 0O + 1

            output[i] = r0;
            output[i + 1 * stride] = r1;
        }
    }
}

void mufft_radix2_generic_vert_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
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

        for (unsigned i = 0; i < samples_x; i++)
        {
            cfloat a = input[i];
            cfloat b = cfloat_mul(twiddles[k], input[i + half_stride]);

            cfloat r0 = cfloat_add(a, b); // 0O + 0
            cfloat r1 = cfloat_sub(a, b); // 0O + 1

            output[i + j] = r0;
            output[i + j + 1 * out_stride] = r1;
        }
    }
}

void mufft_forward_radix4_p1_vert_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)twiddles;
    (void)p;

    unsigned quarter_lines = samples_y >> 2;
    unsigned quarter_stride = stride * quarter_lines;

    for (unsigned line = 0; line < quarter_lines;
            line++, input += stride, output += stride << 2)
    {
        for (unsigned i = 0; i < samples_x; i++)
        {
            cfloat a = input[i];
            cfloat b = input[i + quarter_stride];
            cfloat c = input[i + 2 * quarter_stride];
            cfloat d = input[i + 3 * quarter_stride];

            cfloat r0 = cfloat_add(a, c); // 0O + 0
            cfloat r1 = cfloat_sub(a, c); // 0O + 1
            cfloat r2 = cfloat_add(b, d); // 2O + 0
            cfloat r3 = cfloat_sub(b, d); // 2O + 1

            // p == 2 twiddles
            r3 = cfloat_mul(r3, twiddles[2]);

            a = cfloat_add(r0, r2); // 0O + 0
            b = cfloat_add(r1, r3); // 0O + 1
            c = cfloat_sub(r0, r2); // 00 + 2
            d = cfloat_sub(r1, r3); // O0 + 3

            output[i] = a;
            output[i + 1 * stride] = b;
            output[i + 2 * stride] = c;
            output[i + 3 * stride] = d;
        }
    }
}

void mufft_inverse_radix4_p1_vert_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    mufft_forward_radix4_p1_vert_c(output_, input_, twiddles, p, samples_x, stride, samples_y);
}

void mufft_radix4_generic_vert_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    cfloat *output = output_;
    const cfloat *input = input_;

    unsigned quarter_lines = samples_y >> 2;
    unsigned quarter_stride = stride* quarter_lines;
    unsigned out_stride = p * stride;

    for (unsigned line = 0; line < quarter_lines; line++, input += stride)
    {
        unsigned k = line & (p - 1);
        unsigned j = (((line - k) << 2) + k) * stride;

        for (unsigned i = 0; i < samples_x; i++)
        {
            cfloat a = input[i];
            cfloat b = input[i + quarter_stride];
            cfloat c = cfloat_mul(twiddles[k], input[i + 2 * quarter_stride]);
            cfloat d = cfloat_mul(twiddles[k], input[i + 3 * quarter_stride]);

            cfloat r0 = cfloat_add(a, c); // 0O + 0
            cfloat r1 = cfloat_sub(a, c); // 0O + 1
            cfloat r2 = cfloat_add(b, d); // 2O + 0
            cfloat r3 = cfloat_sub(b, d); // 2O + 1

            r2 = cfloat_mul(r2, twiddles[p + k]);
            r3 = cfloat_mul(r3, twiddles[p + k + p]);

            a = cfloat_add(r0, r2); // 0O + 0
            b = cfloat_add(r1, r3); // 0O + 1
            c = cfloat_sub(r0, r2); // 00 + 2
            d = cfloat_sub(r1, r3); // O0 + 3

            output[i + j + 0 * out_stride] = a;
            output[i + j + 1 * out_stride] = b;
            output[i + j + 2 * out_stride] = c;
            output[i + j + 3 * out_stride] = d;
        }
    }
}

void mufft_forward_radix8_p1_vert_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    cfloat *output = output_;
    const cfloat *input = input_;
    (void)p;

    unsigned octa_lines = samples_y >> 3;
    unsigned octa_stride = stride * octa_lines;

    for (unsigned line = 0; line < octa_lines;
            line++, input += stride, output += stride << 3)
    {
        for (unsigned i = 0; i < samples_x; i++)
        {
            cfloat a = input[i];
            cfloat b = input[i + octa_stride];
            cfloat c = input[i + 2 * octa_stride];
            cfloat d = input[i + 3 * octa_stride];
            cfloat e = input[i + 4 * octa_stride];
            cfloat f = input[i + 5 * octa_stride];
            cfloat g = input[i + 6 * octa_stride];
            cfloat h = input[i + 7 * octa_stride];

            cfloat r0 = cfloat_add(a, e); // 0O + 0
            cfloat r1 = cfloat_sub(a, e); // 0O + 1
            cfloat r2 = cfloat_add(b, f); // 2O + 0
            cfloat r3 = cfloat_sub(b, f); // 2O + 1
            cfloat r4 = cfloat_add(c, g); // 4O + 0
            cfloat r5 = cfloat_sub(c, g); // 4O + 1
            cfloat r6 = cfloat_add(d, h); // 60 + 0
            cfloat r7 = cfloat_sub(d, h); // 6O + 1

            // p == 2 twiddles
            r5 = cfloat_mul(r5, twiddles[2]);
            r7 = cfloat_mul(r7, twiddles[2]);

            a = cfloat_add(r0, r4); // 0O + 0
            b = cfloat_add(r1, r5); // 0O + 1
            c = cfloat_sub(r0, r4); // 00 + 2
            d = cfloat_sub(r1, r5); // O0 + 3
            e = cfloat_add(r2, r6); // 4O + 0
            f = cfloat_add(r3, r7); // 4O + 1
            g = cfloat_sub(r2, r6); // 4O + 2
            h = cfloat_sub(r3, r7); // 4O + 3

            // p == 4 twiddles
            f = cfloat_mul(f, twiddles[5]);
            g = cfloat_mul(g, twiddles[6]);
            h = cfloat_mul(h, twiddles[7]);

            output[i] = cfloat_add(a, e);
            output[i + 1 * stride] = cfloat_add(b, f);
            output[i + 2 * stride] = cfloat_add(c, g);
            output[i + 3 * stride] = cfloat_add(d, h);
            output[i + 4 * stride] = cfloat_sub(a, e);
            output[i + 5 * stride] = cfloat_sub(b, f);
            output[i + 6 * stride] = cfloat_sub(c, g);
            output[i + 7 * stride] = cfloat_sub(d, h);
        }
    }
}

void mufft_inverse_radix8_p1_vert_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
        const cfloat * MUFFT_RESTRICT twiddles, unsigned p, unsigned samples_x, unsigned stride, unsigned samples_y)
{
    mufft_forward_radix8_p1_vert_c(output_, input_, twiddles, p, samples_x, stride, samples_y);
}

void mufft_radix8_generic_vert_c(void * MUFFT_RESTRICT output_, const void * MUFFT_RESTRICT input_,
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

        for (unsigned i = 0; i < samples_x; i++)
        {
            cfloat a = input[i];
            cfloat b = input[i + octa_stride];
            cfloat c = input[i + 2 * octa_stride];
            cfloat d = input[i + 3 * octa_stride];
            cfloat e = cfloat_mul(twiddles[k], input[i + 4 * octa_stride]);
            cfloat f = cfloat_mul(twiddles[k], input[i + 5 * octa_stride]);
            cfloat g = cfloat_mul(twiddles[k], input[i + 6 * octa_stride]);
            cfloat h = cfloat_mul(twiddles[k], input[i + 7 * octa_stride]);

            cfloat r0 = cfloat_add(a, e); // 0O + 0
            cfloat r1 = cfloat_sub(a, e); // 0O + 1
            cfloat r2 = cfloat_add(b, f); // 2O + 0
            cfloat r3 = cfloat_sub(b, f); // 2O + 1
            cfloat r4 = cfloat_add(c, g); // 4O + 0
            cfloat r5 = cfloat_sub(c, g); // 4O + 1
            cfloat r6 = cfloat_add(d, h); // 60 + 0
            cfloat r7 = cfloat_sub(d, h); // 6O + 1

            r4 = cfloat_mul(r4, twiddles[p + k]);
            r5 = cfloat_mul(r5, twiddles[p + k + p]);
            r6 = cfloat_mul(r6, twiddles[p + k]);
            r7 = cfloat_mul(r7, twiddles[p + k + p]);

            a = cfloat_add(r0, r4); // 0O + 0
            b = cfloat_add(r1, r5); // 0O + 1
            c = cfloat_sub(r0, r4); // 00 + 2
            d = cfloat_sub(r1, r5); // O0 + 3
            e = cfloat_add(r2, r6); // 4O + 0
            f = cfloat_add(r3, r7); // 4O + 1
            g = cfloat_sub(r2, r6); // 4O + 2
            h = cfloat_sub(r3, r7); // 4O + 3

            // p == 4 twiddles
            e = cfloat_mul(e, twiddles[3 * p + k]);
            f = cfloat_mul(f, twiddles[3 * p + k + p]);
            g = cfloat_mul(g, twiddles[3 * p + k + 2 * p]);
            h = cfloat_mul(h, twiddles[3 * p + k + 3 * p]);

            output[i + j + 0 * out_stride] = cfloat_add(a, e);
            output[i + j + 1 * out_stride] = cfloat_add(b, f);
            output[i + j + 2 * out_stride] = cfloat_add(c, g);
            output[i + j + 3 * out_stride] = cfloat_add(d, h);
            output[i + j + 4 * out_stride] = cfloat_sub(a, e);
            output[i + j + 5 * out_stride] = cfloat_sub(b, f);
            output[i + j + 6 * out_stride] = cfloat_sub(c, g);
            output[i + j + 7 * out_stride] = cfloat_sub(d, h);
        }
    }
}

