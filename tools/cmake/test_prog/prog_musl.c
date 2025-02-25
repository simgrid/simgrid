/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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
