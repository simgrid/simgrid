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

#include "xbt/context.h"
#include "xbt/ex.h"

#ifdef CONTEXT_THREADS
#  include "xbt/xbt_os_thread.h"
#else
#  include "ucontext_stack.h"  /* loads context system definitions */
#  include <ucontext.h>
#  define STACK_SIZE 128*1024 /* Lower this if you want to reduce the memory consumption */
#endif     /* not CONTEXT_THREADS */


typedef struct s_xbt_context {
	s_xbt_swag_hookup_t hookup;
   	char *name;

	/* Declaration of the thread running the process */
#ifdef JAVA_SIMGRID /* come first because other ones are defined too */
	jobject jprocess;  /* the java process instance			*/
	JNIEnv* jenv;	   /* jni interface pointer for this thread	*/
	ex_ctx_t *exception; /* exception container -- only in ucontext&java, os_threads deals with it for us otherwise */
#else   
# ifdef CONTEXT_THREADS
	xbt_os_thread_t thread; /* a plain dumb thread (portable to posix or windows) */
	xbt_os_sem_t begin;		/* this semaphore is used to schedule/unschedule the process */
	xbt_os_sem_t end;		/* this semaphore is used to schedule/unschedule the process */
# else
	ucontext_t uc;	     /* the thread that execute the code */
	char stack[STACK_SIZE];
	struct s_xbt_context *save;
	ex_ctx_t *exception; /* exception container -- only in ucontext&java, os_threads deals with it for us otherwise */
# endif /* CONTEXT_THREADS */
#endif /* JAVA_SIMGRID */

	/* What to run */
	xbt_main_func_t code;	/* the scheduled fonction	    */
	int argc;
	char **argv;
   
	/* Init/exit functions */   
	void_f_pvoid_t startup_func;
	void *startup_arg;
	void_f_pvoid_t cleanup_func;
	void *cleanup_arg;
   	int iwannadie;                  /* Set to true by the context when it wants to commit suicide */
} s_xbt_context_t;


#endif	/* !_XBT_CONTEXT_PRIVATE_H */
