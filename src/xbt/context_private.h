/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_PRIVATE_H
#define _XBT_CONTEXT_PRIVATE_H

#define HAVE_CONTEXT 1

#include "xbt/sysdep.h"
#include "xbt/context.h"

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
typedef struct s_context {
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  pthread_t *thread;            /* the thread that execute the code   */
  context_function_t code;                /* the scheduler fonction   */
  int argc;
  char *argv[];
} s_context_t;
#endif

#if HAVE_CONTEXT==1
#include <stdlib.h>
#include <ucontext.h>
#define STACK_SIZE 524288
typedef struct s_context {
  ucontext_t uc;                /* the thread that execute the code   */
  char stack[STACK_SIZE];
  context_function_t code;        /* the scheduler fonction   */
  int argc;
  char **argv;
  struct s_context *save;
} s_context_t;
#endif

#endif              /* _XBT_CONTEXT_PRIVATE_H */
