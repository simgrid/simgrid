/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_PRIVATE_H
#define _XBT_CONTEXT_PRIVATE_H


#include "xbt/sysdep.h"
#include "xbt/context.h"
#include "portable.h" /* loads context system definitions */

#define STACK_SIZE 524288
typedef struct s_context {
  ucontext_t uc;                /* the thread that execute the code   */
  char stack[STACK_SIZE];
  context_function_t code;        /* the scheduler fonction   */
  int argc;
  char **argv;
  struct s_context *save;
} s_context_t;


#if 0 /* FIXME: KILLME */
//#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
typedef struct s_context {
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  pthread_t *thread;            /* the thread that execute the code   */
  context_function_t code;                /* the scheduler fonction   */
  int argc;
  char *argv[];
} s_context_t;
#endif /* ENDOFKILLME*/

#endif              /* _XBT_CONTEXT_PRIVATE_H */
