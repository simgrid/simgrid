/* 	$Id$	 */

/* chrono.h - timer macros for GRAS                                         */

/* Copyright (c) 2005 Martin Quinson, Arnaud Legrand. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef GRAS_CHRONO_H
#define GRAS_CHRONO_H

#include "xbt/misc.h"

BEGIN_DECL()

void gras_bench_always_begin(const char *location);
void gras_bench_always_end(void);

/** \brief Start benchmark this part of the code
    \hideinitializer */
#define GRAS_BENCH_ALWAYS_BEGIN gras_bench_always_begin(__FILE__ __LINE__ __FUNCTION__)
/** \brief Stop benchmark this part of the code
    \hideinitializer */
#define GRAS_BENCH_ALWAYS_END gras_bench_always_end()

/** \brief Start benchmark this part of the code if it has never been benchmarked before
    \hideinitializer */
#define GRAS_BENCH_ONCE_BEGIN
/** \brief Stop benchmarking this part of the code
    \hideinitializer */
#define GRAS_BENCH_ONCE_END

END_DECL()

#endif /* GRAS_CHRONO_H */
