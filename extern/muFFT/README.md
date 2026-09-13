# muFFT

muFFT is a library for doing the fast fourier transform (FFT) in one or two dimensions.
The FFT has many applications in digital signal processing.
The main use cases are fast linear convolution and conversion from time domain into frequency domain and vice versa.
See [The Fast Fourier Transform](@ref FFT) for details on how the algorithm works and how it is implemented in muFFT.

## Features

muFFT is a moderately featured single-precision FFT library.
It focuses particularly on linear convolution for audio applications and being optimized for modern architectures.

 - Power-of-two transforms
 - 1D/2D complex-to-complex transform
 - 1D/2D real-to-complex transform
 - 1D/2D complex-to-real transform
 - 1D fast convolution for applying large filters.
   Supports both complex/real convolutions and real/real convolutions.
   The complex/real convolution is particularly useful for filtering interleaved stereo audio.
 - Designed and optimized for SIMD architectures,
   with optimizations for SSE, SSE3 and AVX-256 currently implemented.
   Etterna also enables AArch64 NEON through the bundled sse2neon library.
 - Radix-2, radix-4 and radix-8 butterfly implementations.
 - Input and output does not have to be reordered, as is sometimes the case with FFT algorithms.
   muFFT implements the Stockham autosort algorithm to avoid any explicit permutation of FFT coefficients.
 - Detects SIMD support for your hardware in runtime.
   Same muFFT binary can support wide ranges of hardware feature sets.

## Building

muFFT is built with straight CMake. Use `add_subdirectory` in your project.

### Apple Silicon / AArch64 in Etterna

Native ARM64 builds enable `MUFFT_SIMD_NEON` by default. The SSE3 kernels
are compiled to NEON with [sse2neon](https://github.com/DLTcollab/sse2neon),
which is already bundled with Etterna. No x86 instructions or Rosetta are
used by this path. This accelerates the FFTs used by the playback spectrum
callback and the high-quality audio speed changer.

The dispatch table retains the SSE3 kernel names and feature bit internally.
`MUFFT_FLAG_CPU_NO_SIMD` and `MUFFT_FLAG_CPU_NO_SSE3` select the scalar fallback
on AArch64. Configure with `-DMUFFT_SIMD_NEON=OFF` to omit the NEON kernels
entirely. The existing x86 SSE/AVX implementation remains available for Intel
builds. Standalone builds locate the header in the neighboring sse2neon
directory; other layouts can set `MUFFT_SSE2NEON_INCLUDE_DIR` explicitly.

To verify and benchmark the audio FFTs from the Etterna repository root
(tested with CMake 3.31 and AppleClang):

```sh
cmake -S extern/muFFT -B build/fft-arm64 \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DMUFFT_BUILD_REGRESSION_TESTS=ON
cmake --build build/fft-arm64 --parallel
ctest --test-dir build/fft-arm64 --output-on-failure
./build/fft-arm64/mufft-regression --benchmark
```

The regression executable needs no FFTW. It compares SIMD and scalar results
for real and complex transforms, both directions, zero padding, full spectra,
2D transforms and frequency-domain convolution. Small complex transforms also
use an independent double-precision DFT, and real transforms must round-trip.
The benchmark reports median CPU time over seven alternating scalar/SIMD trials,
with plan creation and allocation outside the timed region.

For memory and undefined-behavior checks, use a separate build directory with
`-DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"`.
Benchmark the Release build without sanitizers. These measurements describe
FFT execution time, not overall game frame rate or audio latency.

muFFT uses the C99 and C++ ABI for complex numbers, interleaved real and imaginary samples, i.e.:

```
struct complex_float {
	float real;
	float imag;
};
```

C99 `complex float` from `<complex.h>` and C++ `std::complex<float>` from `<complex>` can safely be used with muFFT.

## Performance 

muFFT is written for performance and is usually competitive with highly optimized libraries like FFTW3 and FFmpeg/libavcodec FFT.
See [Benchmark](#benchmark) for how to run your own benchmarks.

muFFT is designed with moderate size FFTs in mind.
Very large FFTs which don't fit in cache could be better optimized by designing for cache utilization
and tiny FFTs (N = 2, 4, 8) don't have special handcoded vectorized transforms.

muFFT does not need to run micro benchmarks ahead of time to determine optimal FFT decompositions,
as is supported in more sophisticated FFT libraries. Reasonable decompositions are found statically.

## License

The muFFT library is licensed under the permissive MIT license, see COPYING and preambles in source files for more detail.

Note that the `muFFT-bench` and `muFFT-test` binaries link against FFTW3 for verification purposes, a GPLv2+ library.
If you choose to distribute either of these binaries, muFFT source must be provided as well.
See COPYING.GPLv2 for details.
These binaries are non-essential, and are only intended for use during development and verification,
and not for distribution.

## Documentation

The public muFFT API is documented with doxygen.
Run `doxygen` to generate documentation. Doxygen 1.8.3 is required.

After running Doxygen, documents are found in `docs/index.html`.

## Sample code

There is currently no dedicated sample code for muFFT. See `test.c`, `bench.c` and the documentation for reference on how to use the API.
The various test and benchmark routines flex most of the API. It it also a good way to see how the API calls match up to equivalent FFTW3 routines.

## Unit tests

All muFFT APIs have unit tests. muFFT output is verified against the FFTW library.
The convolution API is verified against a straight O(N^2) convolution.

The FFTW3 library must be present on your system via pkg-config when building this.
Note that FFTW3 (as of writing) is licensed under GPLv2+.
The `muFFT-test` binary falls under licensing requirements of GPLv2 as per FFTW license.

## <a name="benchmark"></a>  Benchmark

muFFT can be benchmarked using FFTW as a reference.

Gflops values reported are based on the estimated number of flops consumed by a generic complex FFT, which is 5.0 * N * log2(N).
Values reported should be taken with a grain of salt, but it gives a reasonable estimate for throughput.
Average time consumed by a single FFT is reported as well.

To run the benchmark:

```
    ./muFFT-bench 1000000 64  # 1 million iterations of various N = 64 FFTs variants
    ./muFFT-bench 10000 64 64 # 10k iterations of 64-by-64 2D FFT
    ./muFFT-bench # Run various 1D and 2D benchmarks
```

The benchmark for 1D tests various things:

 - Complex-to-complex transform
 - Real-to-complex and Complex-to-real in one iteration (typical convolution scenario)
 - Mono convolution, stereo convolution

The FFTW3 library must be present on your system via pkg-config when building this.
Note that FFTW3 (as of writing) is licensed under GPLv2+.
The `muFFT-bench` binary falls under licensing requirements of GPLv2 as per FFTW3 license.
