// This little program only compiles on Linux, and returns 1 on MUSL and 0 on GNU system
// Loosely inspired by https://stackoverflow.com/a/70211227

#define _GNU_SOURCE
#include <features.h>

#if !defined(__linux__)
#error This test is only meant to run on Linux
#endif

int main()
{
#if defined(__USE_GNU)
  return 0; // GNU
#else
  return 1; // non-GNU thus MUSL
#endif
}