/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_PRIVATE_H
#define _XBT_CONTEXT_PRIVATE_H


#include "xbt/sysdep.h"
#include "xbt/swag.h"
#include "xbt/dynar.h" /* void_f_pvoid_t */
#include "portable.h"  /* loads context system definitions */

#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__TOS_WIN__)
#include "ucontext_stack.h"  /* loads context system definitions */
#endif

#include "xbt/context.h"
#include "xbt/ex.h"

#ifdef CONTEXT_THREADS
#  include "xbt/xbt_thread.h"
#else
#  include <ucontext.h>
#  define STACK_SIZE 128*1024 /* Lower this if you want to reduce the memory consumption */
#endif     /* not CONTEXT_THREADS */


typedef struct s_xbt_context 
{
	s_xbt_swag_hookup_t hookup;
#ifdef CONTEXT_THREADS
	xbt_thcond_t cond;
	xbt_mutex_t mutex;
	xbt_thread_t thread;		/* the thread that execute the code */
#else
	ucontext_t uc;			/* the thread that execute the code */
	char stack[STACK_SIZE];
	struct s_xbt_context *save;
#endif /* CONTEXT_THREADS */
	xbt_context_function_t code;	/* the scheduler fonction	    */
	int argc;
	char **argv;
	void_f_pvoid_t *startup_func;
	void *startup_arg;
	void_f_pvoid_t *cleanup_func;
	void *cleanup_arg;
	ex_ctx_t *exception;		/* exception 			    */
	int iwannadie;
} s_xbt_context_t;


#endif	/* !_XBT_CONTEXT_PRIVATE_H */
