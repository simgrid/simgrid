/* $Id$ */

/* context_java - implementation of context switching for java threads */

/* Copyright (c) 2007-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "xbt/function_types.h"
#include "xbt/ex_interface.h"
#include "xbt/xbt_context_private.h"
#include "xbt/xbt_context_java.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(jmsg, "MSG for Java(TM)");

/* callback: context fetching */
static ex_ctx_t *xbt_ctx_java_ex_ctx(void);

/* callback: termination */
static void xbt_ctx_java_ex_terminate(xbt_ex_t * e);

static xbt_context_t
xbt_ctx_java_factory_create_context(const char *name, xbt_main_func_t code,
                                    void_f_pvoid_t startup_func,
                                    void *startup_arg,
                                    void_f_pvoid_t cleanup_func,
                                    void *cleanup_arg, int argc, char **argv);

static int
xbt_ctx_java_factory_create_maestro_context(xbt_context_t * maestro);

static int xbt_ctx_java_factory_finalize(xbt_context_factory_t * factory);

static void xbt_ctx_java_free(xbt_context_t context);

static void xbt_ctx_java_kill(xbt_context_t context);

static void xbt_ctx_java_schedule(xbt_context_t context);

static void xbt_ctx_java_yield(void);

static void xbt_ctx_java_start(xbt_context_t context);

static void xbt_ctx_java_stop(int exit_code);

static void xbt_ctx_java_swap(xbt_context_t context);

static void xbt_ctx_java_schedule(xbt_context_t context);

static void xbt_ctx_java_yield(void);

static void xbt_ctx_java_suspend(xbt_context_t context);

static void xbt_ctx_java_resume(xbt_context_t context);


/* callback: context fetching */
static ex_ctx_t *xbt_ctx_java_ex_ctx(void)
{
  return current_context->exception;
}

/* callback: termination */
static void xbt_ctx_java_ex_terminate(xbt_ex_t * e)
{
  xbt_ex_display(e);
  abort();
}

void xbt_ctx_java_factory_init(xbt_context_factory_t * factory)
{
  /* context exception handlers */
  __xbt_ex_ctx = xbt_ctx_java_ex_ctx;
  __xbt_ex_terminate = xbt_ctx_java_ex_terminate;

  /* instantiate the context factory */
  *factory = xbt_new0(s_xbt_context_factory_t, 1);

  (*factory)->create_context = xbt_ctx_java_factory_create_context;
  (*factory)->finalize = xbt_ctx_java_factory_finalize;
  (*factory)->create_maestro_context = xbt_ctx_java_factory_create_maestro_context;
  (*factory)->free = xbt_ctx_java_free;
  (*factory)->kill = xbt_ctx_java_kill;
  (*factory)->schedule = xbt_ctx_java_schedule;
  (*factory)->yield = xbt_ctx_java_yield;
  (*factory)->start = xbt_ctx_java_start;
  (*factory)->stop = xbt_ctx_java_stop;
  (*factory)->name = "ctx_java_factory";
}

static int
xbt_ctx_java_factory_create_maestro_context(xbt_context_t * maestro)
{
  xbt_ctx_java_t context = xbt_new0(s_xbt_ctx_java_t, 1);

  context->exception = xbt_new(ex_ctx_t, 1);
  XBT_CTX_INITIALIZE(context->exception);

  *maestro = (xbt_context_t) context;

  return 0;
}

static int xbt_ctx_java_factory_finalize(xbt_context_factory_t * factory)
{
  free(maestro_context->exception);
  free(*factory);
  *factory = NULL;

  return 0;
}

static xbt_context_t
xbt_ctx_java_factory_create_context(const char *name, xbt_main_func_t code,
                                    void_f_pvoid_t startup_func,
                                    void *startup_arg,
                                    void_f_pvoid_t cleanup_func,
                                    void *cleanup_arg, int argc, char **argv)
{
  xbt_ctx_java_t context = xbt_new0(s_xbt_ctx_java_t, 1);

  context->name = xbt_strdup(name);
  context->cleanup_func = cleanup_func;
  context->cleanup_arg = cleanup_arg;
  context->exception = xbt_new(ex_ctx_t, 1);
  XBT_CTX_INITIALIZE(context->exception);
  context->jprocess = (jobject) startup_arg;
  context->jenv = get_current_thread_env();

  return (xbt_context_t) context;
}

static void xbt_ctx_java_free(xbt_context_t context)
{
  if (context) {
    xbt_ctx_java_t ctx_java = (xbt_ctx_java_t) context;

    free(ctx_java->name);

    if (ctx_java->jprocess) {
      jobject jprocess = ctx_java->jprocess;

      ctx_java->jprocess = NULL;

      /* if the java process is alive join it */
      if (jprocess_is_alive(jprocess, get_current_thread_env()))
        jprocess_join(jprocess, get_current_thread_env());
    }

    if (ctx_java->exception)
      free(ctx_java->exception);

    free(context);
    context = NULL;
  }
}

static void xbt_ctx_java_kill(xbt_context_t context)
{
  context->iwannadie = 1;
  xbt_ctx_java_swap(context);
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
static void xbt_ctx_java_schedule(xbt_context_t context)
{
  xbt_assert0((current_context == maestro_context),
              "You are not supposed to run this function here!");
  xbt_ctx_java_swap(context);
}

/** 
 * Calling this function makes the current context yield. The context
 * that scheduled it returns from xbt_context_schedule as if nothing
 * had happened.
 * 
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
static void xbt_ctx_java_yield(void)
{
  xbt_assert0((current_context != maestro_context),
              "You are not supposed to run this function here!");
  jprocess_unschedule(current_context);
}

static void xbt_ctx_java_start(xbt_context_t context)
{
  jprocess_start(((xbt_ctx_java_t) context)->jprocess,
                 get_current_thread_env());
}

static void xbt_ctx_java_stop(int exit_code)
{
  jobject jprocess = NULL;

  xbt_ctx_java_t ctx_java;

  if (current_context->cleanup_func)
    (*(current_context->cleanup_func)) (current_context->cleanup_arg);

  xbt_swag_remove(current_context, context_living);
  xbt_swag_insert(current_context, context_to_destroy);

  ctx_java = (xbt_ctx_java_t) current_context;

  if (ctx_java->iwannadie) {
    /* The maestro call xbt_context_stop() with an exit code set to one */
    if (ctx_java->jprocess) {
      /* if the java process is alive schedule it */
      if (jprocess_is_alive(ctx_java->jprocess, get_current_thread_env())) {
        jprocess_schedule(current_context);
        jprocess = ctx_java->jprocess;
        ctx_java->jprocess = NULL;

        /* interrupt the java process */
        jprocess_exit(jprocess, get_current_thread_env());

      }
    }
  } else {
    /* the java process exits */
    jprocess = ctx_java->jprocess;
    ctx_java->jprocess = NULL;
  }

  /* delete the global reference associated with the java process */
  jprocess_delete_global_ref(jprocess, get_current_thread_env());
}

static void xbt_ctx_java_swap(xbt_context_t context)
{
  if (context) {
    xbt_context_t self = current_context;

    current_context = context;

    jprocess_schedule(context);

    current_context = self;
  }

  if (current_context->iwannadie)
    xbt_ctx_java_stop(1);
}
