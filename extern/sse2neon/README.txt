SSE2Neon is used by us to allow the calc to use x86_64 vector intrinsic 
 instructions on ARM64.

x86_64's SSE SIMD instrinsics are transparently replaced with equivalent
ARM Neon instructions by this header-only library.

The project is available at https://github.com/DLTcollab/sse2neon and is 
MIT licensed.

This particular file was copied from the v1.7.0 release, available at 
https://github.com/DLTcollab/sse2neon/releases/tag/v1.7.0
