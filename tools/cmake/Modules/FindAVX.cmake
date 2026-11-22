# Find the most recent AVX instruction set (AVX2 or AVX) supported by both the compiler and the processor.
#
#   find_package(AVX [version] [QUIET] [REQUIRED])
#
# Sets AVX_FOUND, and AVX_VERSION to "2.0" (AVX2) or "1.0" (AVX).
# The detection runs a test program, so it checks the processor of the build machine.

include(CheckCXXSourceRuns)
include(CMakePushCheckState)

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_QUIET ${AVX_FIND_QUIETLY})

set(CMAKE_REQUIRED_FLAGS "-mavx2")
check_cxx_source_runs("
  #include <immintrin.h>
  int main()
  {
    volatile int x = -1; // Prevents the compiler from computing the result at compile time
    return _mm256_extract_epi32(_mm256_abs_epi32(_mm256_set1_epi32(x)), 0) == 1 ? 0 : 1;
  }" AVX_HAS_AVX2)

if(NOT AVX_HAS_AVX2)
  set(CMAKE_REQUIRED_FLAGS "-mavx")
  check_cxx_source_runs("
    #include <immintrin.h>
    int main()
    {
      volatile float x = 1.0f; // Prevents the compiler from computing the result at compile time
      __m256 a = _mm256_set1_ps(x);
      return _mm256_cvtss_f32(_mm256_add_ps(a, a)) == 2.0f ? 0 : 1;
    }" AVX_HAS_AVX)
endif()

cmake_pop_check_state()

unset(AVX_VERSION)
if(AVX_HAS_AVX2)
  set(AVX_VERSION "2.0")
elseif(AVX_HAS_AVX)
  set(AVX_VERSION "1.0")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVX REQUIRED_VARS AVX_VERSION VERSION_VAR AVX_VERSION)
