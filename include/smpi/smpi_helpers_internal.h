/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_HELPERS_INTERNAL_H
#define SMPI_HELPERS_INTERNAL_H

#include <getopt.h>
#include <stdio.h>  /* for getopt() on OpenIndiana, don't remove */
#include <stdlib.h> /* for getopt() on OpenIndiana, don't remove */
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#if _POSIX_TIMERS
#include <time.h>
#endif

#if !defined(SMPI_NO_OVERRIDE_MALLOC) && !defined(__GLIBC__)
/* For musl libc, <sched.h> must be included before #defining calloc(). Testing if !defined(__GLIBC__) is a bit crude
 * but I don't know a better way. */
#include <sched.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int smpi_usleep(useconds_t usecs);
#if _POSIX_TIMERS > 0
int smpi_nanosleep(const struct timespec* tp, struct timespec* t);
int smpi_clock_gettime(clockid_t clk_id, struct timespec* tp);
#endif
unsigned int smpi_sleep(unsigned int secs);
int smpi_gettimeofday(struct timeval* tv, struct timezone* tz);

int smpi_getopt_long_only(int argc, char* const* argv, const char* options, const struct option* long_options,
                          int* opt_index);
int smpi_getopt_long(int argc, char* const* argv, const char* options, const struct option* long_options,
                     int* opt_index);
int smpi_getopt(int argc, char* const* argv, const char* options);

void* smpi_shared_malloc_intercept(size_t size, const char* file, int line);
void* smpi_shared_calloc_intercept(size_t num_elm, size_t elem_size, const char* file, int line);
void* smpi_shared_realloc_intercept(void* data, size_t size, const char* file, int line);
void smpi_shared_free(void* data);

pid_t smpi_getpid();
#ifdef __cplusplus
[[noreturn]] // c++11
#else
_Noreturn // c11
#endif
void smpi_exit(int status);
#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
namespace std {
extern "C" void* smpi_shared_malloc_intercept(size_t size, const char* file, int line);
extern "C" void* smpi_shared_calloc_intercept(size_t num_elm, size_t elem_size, const char* file, int line);
extern "C" void* smpi_shared_realloc_intercept(void* data, size_t size, const char* file, int line);
extern "C" void smpi_shared_free(void* ptr);

extern "C" [[noreturn]] void smpi_exit(int status);
} // namespace std
#endif
#endif
