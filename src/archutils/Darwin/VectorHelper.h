#ifndef VECTOR_HELPER_H
#define VECTOR_HELPER_H

#if (defined(__VEC__) || (defined(__SSE__) && defined(__SSE2__))) &&           \
  defined(__GNUC__)
namespace Vector {
bool
CheckForVector();
void
FastSoundWrite(float* dest, const float* src, unsigned size);
}
#define USE_VEC
#endif

#endif
