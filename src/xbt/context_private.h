/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_PRIVATE_H
#define _XBT_CONTEXT_PRIVATE_H


#include "xbt/sysdep.h"
#include "xbt/swag.h"
#include "xbt/context.h"
#include "portable.h" /* loads context system definitions */

#define STACK_SIZE 524288
typedef struct s_xbt_context {
  s_xbt_swag_hookup_t hookup;
  ucontext_t uc;                /* the thread that execute the code   */
  char stack[STACK_SIZE];
  xbt_context_function_t code;        /* the scheduler fonction   */
  int argc;
  char **argv;
  struct s_xbt_context *save;
  void_f_pvoid_t *startup_func;
  void *startup_arg;
  void_f_pvoid_t *cleanup_func;
  void *cleanup_arg;
} s_xbt_context_t;

#endif              /* _XBT_CONTEXT_PRIVATE_H */
