/* 	$Id$	 */

/* chrono.h - timer macros for GRAS                                         */

/* Copyright (c) 2005 Martin Quinson, Arnaud Legrand. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef GRAS_CHRONO_H
#define GRAS_CHRONO_H

#include "xbt/misc.h"
#include "gras/cond.h"

BEGIN_DECL()

int gras_bench_always_begin(const char *location, int line);
int gras_bench_always_end(void);
int gras_bench_once_begin(const char *location, int line);
int gras_bench_once_end(void);

/** \brief Start benchmark this part of the code
    \hideinitializer */
#define GRAS_BENCH_ALWAYS_BEGIN()  do { if(gras_if_SG()) gras_bench_always_begin(__FILE__, __LINE__); } while(0)
/** \brief Stop benchmark this part of the code
    \hideinitializer */
#define GRAS_BENCH_ALWAYS_END() do { if(gras_if_SG()) gras_bench_always_end(); } while(0)

/** \brief Start benchmark this part of the code if it has never been benchmarked before
    \hideinitializer */
#define GRAS_BENCH_ONCE_RUN_ALWAYS_BEGIN()  do { if(gras_if_SG()) gras_bench_once_begin(__FILE__, __LINE__); } while(0)
/** \brief Stop benchmarking this part of the code
    \hideinitializer */
#define GRAS_BENCH_ONCE_RUN_ALWAYS_END()    do { if(gras_if_SG()) gras_bench_once_end(); } while(0)

/** \brief Start benchmark this part of the code if it has never been benchmarked before
    \hideinitializer */
#define GRAS_BENCH_ONCE_RUN_ONCE_BEGIN()  if((gras_if_SG()&&(gras_bench_once_begin(__FILE__, __LINE__)))||(gras_if_RL())) { 
/** \brief Stop benchmarking this part of the code
    \hideinitializer */
#define GRAS_BENCH_ONCE_RUN_ONCE_END()    } GRAS_BENCH_ONCE_RUN_ALWAYS_END();

END_DECL()

#endif /* GRAS_CHRONO_H */
