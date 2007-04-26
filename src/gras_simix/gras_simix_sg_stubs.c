/* $Id$ */

/* rl_gras.c -- empty body of functions used in SG, but harmful in RL       */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt_modinter.h"
#include "xbt/sysdep.h"
#include "xbt/xbt_thread.h"
#include "portable.h" /* CONTEXT_THREADS */

#ifndef CONTEXT_THREADS

/* xbt_threads is loaded in libsimgrid when they are used to implement the xbt_context.
 * The decision (and the loading) is made in xbt/context.c.
 */

/* Mod_init/exit mecanism */
void xbt_thread_mod_init(void) {}
   
void xbt_thread_mod_exit(void) {}


/* Main functions */

xbt_thread_t xbt_thread_create(pvoid_f_pvoid_t start_routine,void* param) {
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thread_create)");
}

void xbt_thread_exit(int *retcode){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thread_exit)");
}

xbt_thread_t xbt_thread_self(void){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thread_self)");
}

void xbt_thread_yield(void){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thread_yield)");
}


xbt_mutex_t xbt_mutex_init(void){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_mutex_init)");
}

void xbt_mutex_lock(xbt_mutex_t mutex){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_mutex_lock)");
}

void xbt_mutex_unlock(xbt_mutex_t mutex){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_mutex_unlock)");
}

void xbt_mutex_destroy(xbt_mutex_t mutex){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_mutex_destroy)");
}

xbt_thcond_t xbt_thcond_init(void){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thcond_init)");
}

void xbt_thcond_wait(xbt_thcond_t cond, xbt_mutex_t mutex){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thcond_wait)");
}

void xbt_thcond_signal(xbt_thcond_t cond){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thcond_signal)");
}

void xbt_thcond_broadcast(xbt_thcond_t cond){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thcond_broadcast)");
}

void xbt_thcond_destroy(xbt_thcond_t cond){
   xbt_backtrace_display();
   xbt_die("No pthread in SG when compiled against the ucontext (xbt_thcond_destroy)");
}


#endif
