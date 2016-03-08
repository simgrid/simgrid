/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MPI_H
#define MPI_H

#define SEED 221238

#define sleep(x) smpi_sleep(x)

#include <smpi/smpi.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>
#include <simgrid/modelchecker.h>

#include <sys/time.h> /* Load it before the define next line to not mess with the system headers */
#define gettimeofday(x, y) smpi_gettimeofday(x, NULL)

#if HAVE_MC
#undef assert
#define assert(x) MC_assert(x)
#endif

#endif
