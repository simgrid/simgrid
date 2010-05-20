/* 	$Id: simix.h 5497 2008-05-26 12:19:15Z cristianrosa $	 */

/* Copyright (c) 2008 Martin Quinson, Cristian Rosa.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _MC_MC_H
#define _MC_MC_H

#include "xbt/misc.h"
#include "xbt/fifo.h"
#include "xbt/dict.h"
#include "xbt/function_types.h"
#include "mc/datatypes.h"
#include "simix/datatypes.h"

SG_BEGIN_DECL()

/********************************* Global *************************************/
XBT_PUBLIC(void) MC_init(int);
XBT_PUBLIC(void) MC_exit(int);
XBT_PUBLIC(void) MC_assert(int);
XBT_PUBLIC(void) MC_modelcheck(int);
XBT_PUBLIC(int) MC_random(int,int);

/******************************* Transitions **********************************/
XBT_PUBLIC(void) MC_trans_intercept_isend(smx_rdv_t);
XBT_PUBLIC(void) MC_trans_intercept_irecv(smx_rdv_t);
XBT_PUBLIC(void) MC_trans_intercept_wait(smx_comm_t);
XBT_PUBLIC(void) MC_trans_intercept_test(smx_comm_t);
XBT_PUBLIC(void) MC_trans_intercept_waitany(xbt_dynar_t);
XBT_PUBLIC(void) MC_trans_intercept_random(int,int);

/********************************* Memory *************************************/
XBT_PUBLIC(void) MC_memory_init(void);   /* Initialize the memory subsystem */
XBT_PUBLIC(void) MC_memory_exit(void);

/*
 * Boolean indicating whether we want to activate the model-checker
 */
extern int _surf_do_model_check;


SG_END_DECL()

#endif                          /* _MC_MC_H */
