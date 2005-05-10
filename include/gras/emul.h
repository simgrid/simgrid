/* $Id$                     */

/* gras/emul.h - public interface to emulation support                      */
/*                (specific parts for SG or RL)                             */
 
/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_COND_H
#define GRAS_COND_H

#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL()

/** @addtogroup GRAS_emul
 *  @brief Handling code specific to the simulation or to the reality (Virtualization).
 * 
 *  Please note that those are real functions and not pre-processor defines. This is to ensure
 *  that the same object code can be linked against the SG library or the RL one without recompilation.
 * 
 *  @{
 */
  
/** \brief Returns true only if the program runs on real life */
int gras_if_RL(void);

/** \brief Returns true only if the program runs within the simulator */
int gras_if_SG(void);

/** @} */

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

#endif /* GRAS_COND_H */

