/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_HELPERS_H
#define SMPI_HELPERS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <smpi/smpi_helpers_internal.h>
#ifndef TRACE_CALL_LOCATION /* Defined by smpicc on the command line */
#define sleep(x) smpi_sleep(x)
#define usleep(x) smpi_usleep(x)
#else
#define sleep(x) (smpi_trace_set_call_location(__FILE__, __LINE__), smpi_sleep(x))
#define usleep(x) (smpi_trace_set_call_location(__FILE__, __LINE__), smpi_usleep(x))
#endif

#define gettimeofday(x, y) smpi_gettimeofday((x), 0)
#if _POSIX_TIMERS > 0
#ifndef TRACE_CALL_LOCATION /* Defined by smpicc on the command line */
#define nanosleep(x, y) smpi_nanosleep((x), (y))
#else
#define nanosleep(x) (smpi_trace_set_call_location(__FILE__, __LINE__), smpi_nanosleep(x))
#endif
#define clock_gettime(x, y) smpi_clock_gettime((x), (y))
#endif

#define getopt(x, y, z) smpi_getopt((x), (y), (z))
#define getopt_long(x, y, z, a, b) smpi_getopt_long((x), (y), (z), (a), (b))
#define getopt_long_only(x, y, z, a, b) smpi_getopt_long_only((x), (y), (z), (a), (b))
#ifndef SMPI_NO_OVERRIDE_MALLOC
#ifdef __cplusplus
namespace std {
    extern "C" void* smpi_shared_malloc_intercept(size_t size, const char* file, int line);
    extern "C" void* smpi_shared_calloc_intercept(size_t num_elm, size_t elem_size, const char* file, int line);
    extern "C" void* smpi_shared_realloc_intercept(void* data, size_t size, const char* file, int line);
    extern "C" void smpi_shared_free(void* ptr);
}
#endif
#define malloc(x) smpi_shared_malloc_intercept((x), __FILE__, __LINE__)
#define calloc(x, y) smpi_shared_calloc_intercept((x), (y), __FILE__, __LINE__)
#define realloc(x, y) smpi_shared_realloc_intercept((x), (y), __FILE__, __LINE__)
#define free(x) smpi_shared_free(x)
#endif

#ifdef __cplusplus
namespace std {
  extern "C" __attribute__((noreturn)) void smpi_exit(int status);
}
#endif
#define exit(x) smpi_exit(x)

#endif
