/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_HELPERS_INTERNAL_H
#define SMPI_HELPERS_INTERNAL_H

#include <getopt.h>
#include <stdio.h>  /* for getopt(), don't remove */
#include <stdlib.h> /* for getopt(), don't remove */
#include <unistd.h>

#include <sys/time.h>
#if _POSIX_TIMERS
#include <time.h>
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

#ifdef __cplusplus
} // extern "C"
#endif
#endif
