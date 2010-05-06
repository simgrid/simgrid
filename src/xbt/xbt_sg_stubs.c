/* xbt_sg_stubs -- empty functions sometimes used in SG (never in RL)       */

/* This is always part of SG, never of RL. Content:                         */
/*  - a bunch of stub functions of the thread related function that we need */
/*    to add to the lib to please the linker when using ucontextes.         */
/*  - a bunch of stub functions of the java related function when we don't  */
/*    compile java bindings.                                                */

/* In RL, java is useless, and threads are always part of the picture,      */
/*  ucontext never */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt_modinter.h"
#include "xbt/sysdep.h"
#include "xbt/xbt_os_thread.h"
#include "portable.h"           /* CONTEXT_THREADS */

#ifndef CONTEXT_THREADS

/* xbt_threads is loaded in libsimgrid when they are used to implement the xbt_context.
 * The decision (and the loading) is made in xbt/context.c.
 */

/* Mod_init/exit mecanism */
void xbt_os_thread_mod_preinit(void)
{
}

void xbt_os_thread_mod_postexit(void)
{
}


/* Main functions */

xbt_os_thread_t xbt_os_thread_create(const char *name,
                                     pvoid_f_pvoid_t start_routine,
                                     void *param)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_thread_create)");
}

void xbt_os_thread_exit(int *retcode)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_thread_exit)");
}

xbt_os_thread_t xbt_os_thread_self(void)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_thread_self)");
}

void xbt_os_thread_yield(void)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_thread_yield)");
}


xbt_os_mutex_t xbt_os_mutex_init(void)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_mutex_init)");
}

void xbt_os_mutex_acquire(xbt_os_mutex_t mutex)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_mutex_acquire)");
}

void xbt_os_mutex_release(xbt_os_mutex_t mutex)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_mutex_release)");
}

void xbt_os_mutex_destroy(xbt_os_mutex_t mutex)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_mutex_destroy)");
}

xbt_os_cond_t xbt_os_cond_init(void)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_cond_init)");
}

void xbt_os_cond_wait(xbt_os_cond_t cond, xbt_os_mutex_t mutex)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_cond_wait)");
}

void xbt_os_cond_signal(xbt_os_cond_t cond)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_cond_signal)");
}

void xbt_os_cond_broadcast(xbt_os_cond_t cond)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_cond_broadcast)");
}

void xbt_os_cond_destroy(xbt_os_cond_t cond)
{
  xbt_backtrace_display_current();
  xbt_die
    ("No pthread in SG when compiled against the ucontext (xbt_os_cond_destroy)");
}
#endif
