/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MPI_H
#define MPI_H

#define SEED 221238

#include <smpi/smpi.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>
#include <simgrid/modelchecker.h>

#include <sys/time.h> /* Load it before the define next line to not mess with the system headers */

#define sleep(x) smpi_sleep(x)
#define usleep(x) smpi_usleep(x)
#define gettimeofday(x, y) smpi_gettimeofday(x, NULL)
#if _POSIX_TIMERS > 0
#define nanosleep(x, y) smpi_nanosleep(x, y)
#define clock_gettime(x, y) smpi_clock_gettime(x, y)
#endif
#if HAVE_MC
#undef assert
#define assert(x) MC_assert(x)
#endif

#ifdef TRACE_CALL_LOCATION /* Defined by smpicc on the command line */
#include <smpi/smpi_extended_traces.h>
#endif

#endif
