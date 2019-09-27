/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_HELPERS_H
#define SMPI_HELPERS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <smpi/smpi_helpers_internal.h>

#define sleep(x) smpi_sleep(x)
#define usleep(x) smpi_usleep(x)
#define gettimeofday(x, y) smpi_gettimeofday((x), 0)
#if _POSIX_TIMERS > 0
#define nanosleep(x, y) smpi_nanosleep((x), (y))
#define clock_gettime(x, y) smpi_clock_gettime((x), (y))
#endif

#define getopt(x, y, z) smpi_getopt((x), (y), (z))
#define getopt_long(x, y, z, a, b) smpi_getopt_long((x), (y), (z), (a), (b))
#define getopt_long_only(x, y, z, a, b) smpi_getopt_long_only((x), (y), (z), (a), (b))

#endif
