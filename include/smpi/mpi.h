/* Copyright (c) 2010. The SimGrid Team.
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

#define sleep(x) smpi_sleep(x)
#define gettimeofday(x, y) smpi_gettimeofday(x, y)

#endif
