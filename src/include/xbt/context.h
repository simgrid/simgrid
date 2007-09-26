/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_H
#define _XBT_CONTEXT_H

#include "xbt/misc.h"
#include "xbt/function_types.h"
#include "xbt_modinter.h" /* init/exit of this module */

/** @addtogroup XBT_context
 *  
 *  INTERNALS OF SIMGRID, DON'T USE IT.
 *
 *  This is a portable to Unix and Windows context implementation.
 *
 * @{
 */

/** @name Context types
 *  @{
 */

  /** @brief A context */
  typedef struct s_xbt_context *xbt_context_t;

/* @}*/

void xbt_context_empty_trash(void);
XBT_PUBLIC(xbt_context_t) xbt_context_new(const char*name,xbt_main_func_t code,
			                  void_f_pvoid_t startup_func, void *startup_arg,
			                  void_f_pvoid_t cleanup_func, void *cleanup_arg,
			                  int argc, char *argv[]);
XBT_PUBLIC(void) xbt_context_kill(xbt_context_t context);
XBT_PUBLIC(void) xbt_context_start(xbt_context_t context);
XBT_PUBLIC(void) xbt_context_yield(void);
XBT_PUBLIC(void) xbt_context_schedule(xbt_context_t context);

void  xbt_context_set_jprocess(xbt_context_t context, void *jp);
void* xbt_context_get_jprocess(xbt_context_t context);

void  xbt_context_set_jenv(xbt_context_t context,void* je);
void* xbt_context_get_jenv(xbt_context_t context);
   
/* @} */
#endif				/* _XBT_CONTEXT_H */
