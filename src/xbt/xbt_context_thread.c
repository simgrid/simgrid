/* $Id$ */

/* context_thread - implementation of context switching with native threads */

/* Copyright (c) 2004-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/function_types.h"
#include "xbt/xbt_context_private.h"

#include "portable.h"           /* loads context system definitions */
#include "xbt/swag.h"
#include "xbt/xbt_os_thread.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_context);

typedef struct s_xbt_ctx_thread {
  XBT_CTX_BASE_T;
  xbt_os_thread_t thread;       /* a plain dumb thread (portable to posix or windows) */
  xbt_os_sem_t begin;           /* this semaphore is used to schedule/yield the process  */
  xbt_os_sem_t end;             /* this semaphore is used to schedule/unschedule the process   */
} s_xbt_ctx_thread_t, *xbt_ctx_thread_t;

static xbt_context_t
xbt_ctx_thread_factory_create_context(const char *name, xbt_main_func_t code,
                                      void_f_pvoid_t startup_func,
                                      void *startup_arg,
                                      void_f_pvoid_t cleanup_func,
                                      void *cleanup_arg, int argc,
                                      char **argv);


static int
xbt_ctx_thread_factory_create_master_context(xbt_context_t * maestro);

static int xbt_ctx_thread_factory_finalize(xbt_context_factory_t * factory);

static void xbt_ctx_thread_free(xbt_context_t context);

static void xbt_ctx_thread_kill(xbt_context_t context);

static void xbt_ctx_thread_schedule(xbt_context_t context);

static void xbt_ctx_thread_yield(void);

static void xbt_ctx_thread_start(xbt_context_t context);

static void xbt_ctx_thread_stop(int exit_code);

static void xbt_ctx_thread_swap(xbt_context_t context);

static void xbt_ctx_thread_schedule(xbt_context_t context);

static void xbt_ctx_thread_yield(void);

static void xbt_ctx_thread_suspend(xbt_context_t context);

static void xbt_ctx_thread_resume(xbt_context_t context);

static void *xbt_ctx_thread_wrapper(void *param);

void xbt_ctx_thread_factory_init(xbt_context_factory_t * factory)
{
  *factory = xbt_new0(s_xbt_context_factory_t, 1);

  (*factory)->create_context = xbt_ctx_thread_factory_create_context;
  (*factory)->finalize = xbt_ctx_thread_factory_finalize;
  (*factory)->create_maestro_context =
    xbt_ctx_thread_factory_create_master_context;
  (*factory)->name = "ctx_thread_factory";
}

static int
xbt_ctx_thread_factory_create_master_context(xbt_context_t * maestro)
{
  *maestro = (xbt_context_t) xbt_new0(s_xbt_ctx_thread_t, 1);
  (*maestro)->name = (char *) "maestro";
  return 0;
}

static int xbt_ctx_thread_factory_finalize(xbt_context_factory_t * factory)
{
  free(*factory);
  *factory = NULL;
  return 0;
}

static xbt_context_t
xbt_ctx_thread_factory_create_context(const char *name, xbt_main_func_t code,
                                      void_f_pvoid_t startup_func,
                                      void *startup_arg,
                                      void_f_pvoid_t cleanup_func,
                                      void *cleanup_arg, int argc,
                                      char **argv)
{
  xbt_ctx_thread_t context = xbt_new0(s_xbt_ctx_thread_t, 1);

  VERB1("Create context %s", name);
  context->code = code;
  context->name = xbt_strdup(name);
  context->begin = xbt_os_sem_init(0);
  context->end = xbt_os_sem_init(0);
  context->iwannadie = 0;       /* useless but makes valgrind happy */
  context->argc = argc;
  context->argv = argv;
  context->startup_func = startup_func;
  context->startup_arg = startup_arg;
  context->cleanup_func = cleanup_func;
  context->cleanup_arg = cleanup_arg;

  context->free = xbt_ctx_thread_free;
  context->kill = xbt_ctx_thread_kill;
  context->schedule = xbt_ctx_thread_schedule;
  context->yield = xbt_ctx_thread_yield;
  context->start = xbt_ctx_thread_start;
  context->stop = xbt_ctx_thread_stop;

  return (xbt_context_t) context;
}

static void xbt_ctx_thread_free(xbt_context_t context)
{
  if (context) {
    xbt_ctx_thread_t ctx_thread = (xbt_ctx_thread_t) context;

    free(ctx_thread->name);

    if (ctx_thread->argv) {
      int i;

      for (i = 0; i < ctx_thread->argc; i++)
        if (ctx_thread->argv[i])
          free(ctx_thread->argv[i]);

      free(ctx_thread->argv);
    }

    /* wait about the thread terminason */
    xbt_os_thread_join(ctx_thread->thread, NULL);

    /* destroy the synchronisation objects */
    xbt_os_sem_destroy(ctx_thread->begin);
    xbt_os_sem_destroy(ctx_thread->end);

    /* finally destroy the context */
    free(context);
  }
}

static void xbt_ctx_thread_kill(xbt_context_t context)
{
  DEBUG1("Kill context '%s'", context->name);
  context->iwannadie = 1;
  xbt_ctx_thread_swap(context);
}

/** 
 * \param context the winner
 *
 * Calling this function blocks the current context and schedule \a context.  
 * When \a context will call xbt_context_yield, it will return
 * to this function as if nothing had happened.
 * 
 * Only the maestro can call this function to run a given process.
 */
static void xbt_ctx_thread_schedule(xbt_context_t context)
{
  DEBUG1("Schedule context '%s'", context->name);
  xbt_assert0((current_context == maestro_context),
              "You are not supposed to run this function here!");
  xbt_ctx_thread_swap(context);
}

/** 
 * Calling this function makes the current context yield. The context
 * that scheduled it returns from xbt_context_schedule as if nothing
 * had happened.
 * 
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
static void xbt_ctx_thread_yield(void)
{
  DEBUG1("Yield context '%s'", current_context->name);
  xbt_assert0((current_context != maestro_context),
              "You are not supposed to run this function here!");
  xbt_ctx_thread_swap(current_context);
}

static void xbt_ctx_thread_start(xbt_context_t context)
{
  xbt_ctx_thread_t ctx_thread = (xbt_ctx_thread_t) context;

  DEBUG1("Start context '%s'", context->name);
  /* create and start the process */
  ctx_thread->thread =
    xbt_os_thread_create(ctx_thread->name, xbt_ctx_thread_wrapper,
                         ctx_thread);

  /* wait the starting of the newly created process */
  xbt_os_sem_acquire(ctx_thread->end);
}

static void xbt_ctx_thread_stop(int exit_code)
{
  /* please no debug here: our procdata was already free'd */
  if (current_context->cleanup_func)
    ((*current_context->cleanup_func)) (current_context->cleanup_arg);

  xbt_swag_remove(current_context, context_living);
  xbt_swag_insert(current_context, context_to_destroy);

  /* signal to the maestro that it has finished */
  xbt_os_sem_release(((xbt_ctx_thread_t) current_context)->end);

  /* exit */
  xbt_os_thread_exit(NULL);     /* We should provide return value in case other wants it */
}

static void xbt_ctx_thread_swap(xbt_context_t context)
{
  DEBUG2("Swap context: '%s' -> '%s'", current_context->name, context->name);
  if ((current_context != maestro_context) && !context->iwannadie) {
    /* (0) it's not the scheduler and the process doesn't want to die, it just wants to yield */

    /* yield itself, resume the maestro */
    xbt_ctx_thread_suspend(context);
  } else {
    /* (1) the current process is the scheduler and the process doesn't want to die
     *      <-> the maestro wants to schedule the process
     *              -> the maestro schedules the process and waits
     *
     * (2) the current process is the scheduler and the process wants to die
     *      <-> the maestro wants to kill the process (has called the function xbt_context_kill())
     *              -> the maestro schedule the process and waits (xbt_os_sem_acquire(context->end))
     *              -> if the process stops (xbt_context_stop())
     *                      -> the process resumes the maestro (xbt_os_sem_release(current_context->end)) and exit (xbt_os_thread_exit())
     *              -> else the process call xbt_context_yield()
     *                      -> goto (3.1)
     *
     * (3) the current process is not the scheduler and the process wants to die
     *              -> (3.1) if the current process is the process who wants to die
     *                      -> (resume not need) goto (4)
     *              -> (3.2) else the current process is not the process who wants to die
     *                      <-> the current process wants to kill an other process
     *                              -> the current process resumes the process to die and waits
     *                              -> if the process to kill stops
     *                                      -> it resumes the process who kill it and exit
     *                              -> else if the process to kill calls to xbt_context_yield()
     *                                      -> goto (3.1)
     */
    /* schedule the process associated with this context */
    xbt_ctx_thread_resume(context);

  }

  /* (4) the current process wants to die */
  if (current_context->iwannadie)
    xbt_ctx_thread_stop(1);
}

static void *xbt_ctx_thread_wrapper(void *param)
{
  xbt_ctx_thread_t context = (xbt_ctx_thread_t) param;

  /* Tell the maestro we are starting, and wait for its green light */
  xbt_os_sem_release(context->end);
  xbt_os_sem_acquire(context->begin);

  if (context->startup_func)
    (*(context->startup_func)) (context->startup_arg);


  xbt_ctx_thread_stop((context->code) (context->argc, context->argv));
  return NULL;
}

static void xbt_ctx_thread_suspend(xbt_context_t context)
{
  /* save the current context */
  xbt_context_t self = current_context;

  DEBUG1("Suspend context '%s'", context->name);

  /* update the current context to this context */
  current_context = context;

  xbt_os_sem_release(((xbt_ctx_thread_t) context)->end);
  xbt_os_sem_acquire(((xbt_ctx_thread_t) context)->begin);

  /* restore the current context to the previously saved context */
  current_context = self;
}

static void xbt_ctx_thread_resume(xbt_context_t context)
{
  /* save the current context */
  xbt_context_t self = current_context;

  DEBUG1("Resume context '%s'", context->name);

  /* update the current context */
  current_context = context;

  xbt_os_sem_release(((xbt_ctx_thread_t) context)->begin);
  xbt_os_sem_acquire(((xbt_ctx_thread_t) context)->end);

  /* restore the current context to the previously saved context */
  current_context = self;
}
