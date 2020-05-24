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

#ifndef MUFFT_H__
#define MUFFT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/// @file fft.h muFFT public API

#if __STDC_VERSION__ >= 199901L
/// Portable variant of C99 restrict keyword
#define MUFFT_RESTRICT restrict
#else
/// Portable variant of C99 restrict keyword
#define MUFFT_RESTRICT
#endif


/// \weakgroup muFFT muFFT public API
/// @{
///

/// The forward FFT transform.
#define MUFFT_FORWARD (-1)
/// The inverse FFT transform.
#define MUFFT_INVERSE  (1)

/// \addtogroup MUFFT_FLAG Planning options
/// @{

/// muFFT will use any SIMD instruction set it can if supported by the CPU.
#define MUFFT_FLAG_CPU_ANY (0)
/// muFFT will not use any SIMD instruction set.
#define MUFFT_FLAG_CPU_NO_SIMD ((1 << 16) - 1)
/// muFFT will not use the AVX instruction set.
#define MUFFT_FLAG_CPU_NO_AVX (1 << 0)
/// muFFT will not use the SSE3 instruction set.
#define MUFFT_FLAG_CPU_NO_SSE3 (1 << 1)
/// muFFT will not use the SSE instruction set.
#define MUFFT_FLAG_CPU_NO_SSE (1 << 2)
/// The real-to-complex 1D transform will also output the redundant conjugate values X(N - k) = X(k)*.
#define MUFFT_FLAG_FULL_R2C (1 << 16)
/// The second/upper half of the input array is assumed to be 0 and will not be read and memory for the second half of the input array does not have to be allocated.
/// This is mostly useful when you want to do zero-padded FFTs which are very common for convolution-type operations, see \ref MUFFT_CONV. This flag is only recognized for 1D transforms.
#define MUFFT_FLAG_ZERO_PAD_UPPER_HALF (1 << 17)
/// @}

/// \addtogroup MUFFT_1D 1D real and complex FFT
/// @{
/// The FFT performed by these functions are not normalized.
/// A forward transform followed by an inverse transform will scale the output by the transform size.
/// The FFTs implemented take input in regular order, no permutation step is required.
/// Similarly, the output of the FFT is in regular order, without any permutation.
/// If input is laid out as (x[0], x[1], x[2], x[3], ..., x[N - 1]) the FFT transform will output
/// (X[0], X[1], X[2], X[3], ..., X[N - 1]) as expected.

/// Opaque type representing a 1D FFT.
typedef struct mufft_plan_1d mufft_plan_1d;

/// \brief Create a plan for a 1D complex-to-complex inverse or forward FFT.
/// 
/// @param N The transform size. Must be power-of-two and at least 2. 
/// @param direction Forward (\ref MUFFT_FORWARD) or inverse (\ref MUFFT_INVERSE) transform.
/// @param flags Flags for the planning. See \ref MUFFT_FLAG.
/// @returns A 1D transform plan, or `NULL` if an error occured.
mufft_plan_1d *mufft_create_plan_1d_c2c(unsigned N, int direction, unsigned flags);

/// \brief Create a plan for real-to-complex forward transform.
///
/// The real-to-complex transform is optimized for the case when the input data to the transform is purely real.
/// The transform is implemented as an N / 2 complex transform with a final butterfly pass to complete the transform.
/// The transform may have different numerical precision characteristics compared to the purely complex transform.
/// 
/// @param N The transform size. Must be power-of-two and at least 4.
/// The required storage for the output is N / 2 + 1 complex values due to redundancies in the frequency plane as
/// X(k) = X(N - k)* when the input to an FFT is real.
/// @param flags Flags for the planning. See \ref MUFFT_FLAG. If \ref MUFFT_FLAG_FULL_R2C flag is added, the transform will output the full N complex frequency samples, instead of the minimum N / 2 + 1 samples.
/// @returns A 1D transform plan, or `NULL` if an error occured.
mufft_plan_1d *mufft_create_plan_1d_r2c(unsigned N, unsigned flags);

/// \brief Create a plan for complex-to-real inverse transform.
///
/// The complex-to-real transform is optimized for the case when the output data of the transform is purely real.
/// The transform is implemented as an N / 2 complex inverse transform with an initial butterfly pass to turn the real N-point transform into an N / 2 complex transform.
/// The transform may have different numerical precision characteristics compared to the purely complex transform.
/// 
/// @param N The transform size. Must be power-of-two and at least 4.
/// The required input for the transform is N / 2 + 1 complex values.
/// @param flags Flags for the planning. See \ref MUFFT_FLAG.
/// @returns A 1D transform plan, or `NULL` if an error occured.
mufft_plan_1d *mufft_create_plan_1d_c2r(unsigned N, unsigned flags);

/// \brief Executes a 1D FFT plan.
/// @param plan Previously allocated 1D FFT plan.
/// @param output Output of the transform. The data must be aligned. See \ref MUFFT_MEMORY.
/// @param input Input to the transform. The data must be aligned. See \ref MUFFT_MEMORY.
void mufft_execute_plan_1d(mufft_plan_1d *plan, void * MUFFT_RESTRICT output, const void * MUFFT_RESTRICT input);

/// \brief Free a previously allocated 1D FFT plan.
/// @param plan A plan. May be `NULL` in which case nothing happens.
void mufft_free_plan_1d(mufft_plan_1d *plan);
/// @}

/// \addtogroup MUFFT_CONV 1D fast convolution
/// @{

/// \addtogroup MUFFT_CONV_METHOD Convolution methods
/// @{

/// The convolution will convolve a single channel of data with a single channel filter.
#define MUFFT_CONV_METHOD_FLAG_MONO_MONO 0

/// The convolution will convolve stereo data with a real filter or stereo filter with single channel data.
/// The stereo channel should be interleaved so that the real part of the input represents the first channel, and the imaginary channel represents the other channel.
/// This format matches perfectly to the commonly used interleaved format used to represent stereo audio of (L, R, L, R, L, R, ...).
/// The first block is a stereo channel (complex) and second block is single channel (real) input.
#define MUFFT_CONV_METHOD_FLAG_STEREO_MONO 1

/// The first block is assumed to be zero padded as defined by \ref MUFFT_FLAG_ZERO_PAD_UPPER_HALF.
#define MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_FIRST (1 << 2)

/// The second block is assumed to be zero padded as defined by \ref MUFFT_FLAG_ZERO_PAD_UPPER_HALF.
#define MUFFT_CONV_METHOD_FLAG_ZERO_PAD_UPPER_HALF_SECOND (1 << 3)
/// @}

/// The first block.
#define MUFFT_CONV_BLOCK_FIRST 0

/// The second block.
#define MUFFT_CONV_BLOCK_SECOND 1

/// Opaque type representing a plan to convolve data.
typedef struct mufft_plan_conv mufft_plan_conv;

/// \brief Create a plan to convolve zero-padded input data of length N with zero-padded input data of length N.
///
/// Due to use of the FFT to perform convolution,
/// this convolution is circular and to obtain proper convolution,
/// the user must ensure that both input arrays are properly zero-padded at the end of each array.
/// The length of a convolution is equal to K + L - 1, where K and L are the lengths of the two input arrays which are to be convolved. If the output array is overrun, the results will spill into the start of the output array which is rarely a desirable result.
/// A very common approach when convolving two arrays of size N in filtering applications is to use an FFT of length N * 2, which can perfectly contain the result of the convolution with just outputing a single redundant value since we need an array of N * 2 - 1.
/// Linear phase FIR filters tend to be of odd length, and we can e.g. implement a 33-tap FIR filter by convolving 32 input samples with 33 FIR samples to form a 64 sample result.
/// 
/// @param N the number of samples in the FFT. N must be at least 4 and power-of-two.
/// @param flags Flags to be passed to the FFT planning. See \ref MUFFT_FLAG.
/// @param method The convolution method to use. See \ref MUFFT_CONV_METHOD. Stereo convolution is supported as well as zero-padding the input data without extra memory copies.
/// @returns An instance of a convolution plan, or `NULL` if failed.
mufft_plan_conv *mufft_create_plan_conv(unsigned N, unsigned flags, unsigned method);

/// \brief Applies forward FFT of either first or second input block.
/// 
/// When doing block-based overlap-add convolution with FFTs the filter generally stays constant over many transforms.
/// We can do the forward transform of the filter input once and cache the FFT of the filter. Only the FFT of the input blocks have to be performed in this case.
///
/// @param plan Convolution instance
/// @param block Which block to forward FFT transform. \ref MUFFT_CONV_BLOCK_FIRST or \ref MUFFT_CONV_BLOCK_SECOND are accepted. Which input is first and second is arbitrary and up to the API user.
/// @param output The FFT of input. Its required buffer size can be queried with mufft_conv_get_transformed_block_size.
/// @param input Input data to be transformed. Must be aligned to a boundary which matches with the SIMD instruction set used by your hardware, typically 16 or 32 bytes. Use \ref MUFFT_MEMORY to allocate properly aligned memory. Depending on the convolution method used, this input array is either treated as a complex array or real array with length either N or N / 2 if zero padding is used.
void mufft_execute_conv_input(mufft_plan_conv *plan, unsigned block, void *output, const void *input);

/// \brief Queries the buffer size for intermediate FFT blocks.
///
/// @param plan Convolution instance
/// @returns The number of bytes required to hold the output of mufft_execute_conv_input. Can be passed directly to \ref MUFFT_MEMORY.
size_t mufft_conv_get_transformed_block_size(mufft_plan_conv *plan);

/// \brief Multiply together FFTs of the two input arrays obtained from \ref mufft_execute_conv_input and perform a normalized inverse FFT.
///
/// A convolution in the time domain happens by multiplying together the frequency response of them. The convolved output is obtained with an inverse transform. Unlike the regular 1D FFT interface, this inverse transform is normalized.
///
/// @param plan Convolution instance
/// @param output Output data. Must be aligned to a boundary which matches with the SIMD instruction set used by your hardware, typically 16 or 32 bytes. Use \ref MUFFT_MEMORY to allocate properly aligned memory. Depending on the convolution method used, the type of output is either treated as a complex array or real array of length N.
/// @param input_first The output obtained earlier by mufft_execute_conv_input for \ref MUFFT_CONV_BLOCK_FIRST.
/// @param input_second The output obtained earlier by mufft_execute_conv_input for \ref MUFFT_CONV_BLOCK_SECOND.
void mufft_execute_conv_output(mufft_plan_conv *plan, void *output, const void *input_first, const void *input_second);

/// \brief Free a previously allocated convolution plan obtained from \ref mufft_create_plan_conv.
void mufft_free_plan_conv(mufft_plan_conv *plan);

/// \brief Vector complex multiply routine signature
typedef void (*mufft_convolve_func)(void *output, const void *a, const void *b,
                                    float normalization, unsigned samples);

/// \brief Gets a function pointer which implements complex multiplication (convolution in frequency domain).
/// @param flags See \ref MUFFT_FLAG.
/// @returns A function which can multiply complex numbers, or `NULL` if failed.
mufft_convolve_func mufft_get_convolve_func(unsigned flags);
/// @}

/// \addtogroup MUFFT_2D 2D real and complex FFT
/// @{
/// The FFT performed by these functions are not normalized.
/// A forward transform followed by an inverse transform will scale the output by the transform size (Nx * Ny).
/// The FFTs implemented take input in regular order, no permutation step is required.
/// Similarly, the output of the FFT is in regular order, without any permutation.
/// If input is laid out as (x[0], x[1], x[2], x[3], ..., x[N - 1]) the FFT transform will output
/// (X[0], X[1], X[2], X[3], ..., X[N - 1]) as expected.
///

/// Opaque type representing a 2D FFT.
typedef struct mufft_plan_2d mufft_plan_2d;

/// \brief Create a plan for a 2D complex-to-complex inverse or forward FFT.
///
/// The input and output data to the 2D transform is represented as a row-major array.
/// 
/// @param Nx The transform size in X dimension (number of columns). Must be power-of-two and at least 2.
/// @param Ny The transform size in Y dimension (number of rows). Must be power-of-two and at least 2.
/// @param direction Forward (\ref MUFFT_FORWARD) or inverse (\ref MUFFT_INVERSE) transform.
/// @param flags Flags for the planning. See \ref MUFFT_FLAG.
/// @returns A 2D transform plan, or `NULL` if an error occured.
mufft_plan_2d *mufft_create_plan_2d_c2c(unsigned Nx, unsigned Ny, int direction, unsigned flags);

/// \brief Create a plan for a 2D real-to-complex forward FFT.
///
/// The input and output data to the 2D transform is represented as a row-major array.
///
/// Note that even if only N / 2 + 1 complex samples are required for a real-to-complex transform,
/// the output array of the transform is still expected to contain N columns of complex data.
/// This is to help SIMD optimization since N / 2 + 1 is odd and would require unaligned memory accesses to work properly.
/// The vertical transform will only transform the first N / 2 + D columns,
/// where D is some convenient value which aligns well to the SIMD instruction set used.
/// The full N complex samples can be processed vertically as well if \ref MUFFT_FLAG_FULL_R2C is used.
/// 
/// @param Nx The transform size in X dimension (number of columns). Must be power-of-two and at least 4.
/// @param Ny The transform size in Y dimension (number of rows). Must be power-of-two and at least 2.
/// @param flags Flags for the planning. See \ref MUFFT_FLAG. If \ref MUFFT_FLAG_FULL_R2C flag is added, the transform will output the full N complex frequency samples, instead of the minimum N / 2 + 1 samples.
/// @returns A 2D transform plan, or `NULL` if an error occured.
mufft_plan_2d *mufft_create_plan_2d_r2c(unsigned Nx, unsigned Ny, unsigned flags);

/// \brief Create a plan for a 2D complex-to-real inverse FFT.
///
/// The input and output data to the 2D transform is represented as a row-major array.
///
/// Note that even if only N / 2 + 1 complex samples are required for a complex-to-real transform,
/// the input array of the transform is still expected to contain N columns of padded complex data.
/// This is to help SIMD optimization since N / 2 + 1 is odd and would require unaligned memory accesses to work properly.
/// The output array needs a minimum size of 2 * Nx * Ny * sizeof(float) and not the expected Nx * Ny * sizeof(float).
/// muFFT uses the output buffer as a scratch buffer during the FFT computation.
/// The end result however, will only require Nx * Ny * sizeof(float) size.
/// 
/// @param Nx The transform size in X dimension (number of columns). Must be power-of-two and at least 4.
/// @param Ny The transform size in Y dimension (number of rows). Must be power-of-two and at least 2.
/// @param flags Flags for the planning. See \ref MUFFT_FLAG.
/// @returns A 2D transform plan, or `NULL` if an error occured.
mufft_plan_2d *mufft_create_plan_2d_c2r(unsigned Nx, unsigned Ny, unsigned flags);

/// \brief Executes a 2D FFT plan.
/// @param plan Previously allocated 2D FFT plan.
/// @param output Output of the transform. The data must be aligned. See \ref MUFFT_MEMORY.
/// @param input Input to the transform. The data must be aligned. See \ref MUFFT_MEMORY.
void mufft_execute_plan_2d(mufft_plan_2d *plan, void * MUFFT_RESTRICT output, const void * MUFFT_RESTRICT input);

/// \brief Free a previously allocated 2D FFT plan.
/// @param plan A plan. May be `NULL` in which case nothing happens.
void mufft_free_plan_2d(mufft_plan_2d *plan);
/// @}

/// \addtogroup MUFFT_MEMORY Memory allocation
/// @{

/// \brief Allocate aligned storage suitable for muFFT.
/// Must be freed with \ref mufft_free.
/// @param size Number of bytes to allocate.
/// @returns Allocated storage, or `NULL`.
void *mufft_alloc(size_t size);

/// \brief Allocate zeroed out aligned storage suitable for muFFT.
/// Same as calloc(), but aligned.
/// Must be freed with \ref mufft_free.
/// @param size Number of bytes to allocate.
/// @returns Allocated storage, or `NULL`.
void *mufft_calloc(size_t size);

/// \brief Free previously allocated storage obtained from \ref mufft_alloc or \ref mufft_calloc.
/// @param ptr Pointer to be freed. Can be `NULL` in which case nothing happens.
void mufft_free(void *ptr);
/// @}

///
/// @}
///

#ifdef __cplusplus
}
#endif

#endif

