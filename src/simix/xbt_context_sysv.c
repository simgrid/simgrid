/* $Id$ */

/* context_sysv - implementation of context switching with ucontextes from Sys V */

/* Copyright (c) 2004-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex_interface.h"
#include "xbt_context_private.h"

#include "context_sysv_config.h"        /* loads context system definitions                             */
#include "portable.h"
#include <ucontext.h>           /* context relative declarations                                */
#define STACK_SIZE 128*1024     /* lower this if you want to reduce the memory consumption      */
#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#endif /* HAVE_VALGRIND_VALGRIND_H */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_context);

typedef struct s_xbt_ctx_sysv {
  XBT_CTX_BASE_T;
  ucontext_t uc;                /* the thread that execute the code                             */
  char stack[STACK_SIZE];       /* the thread stack size                                        */
  struct s_xbt_ctx_sysv *prev;  /* the previous thread                                          */
#ifdef HAVE_VALGRIND_VALGRIND_H
  unsigned int valgrind_stack_id;       /* the valgrind stack id.       */
#endif                          /* HAVE_VALGRIND_VALGRIND_H */
} s_xbt_ctx_sysv_t, *xbt_ctx_sysv_t;


/* callback: context fetching */
static ex_ctx_t *xbt_jcontext_ex_ctx(void);

/* callback: termination */
static void xbt_jcontext_ex_terminate(xbt_ex_t * e);

static xbt_context_t
xbt_ctx_sysv_factory_create_context(const char *name, xbt_main_func_t code,
                                    void_f_pvoid_t startup_func,
                                    void *startup_arg,
                                    void_f_pvoid_t cleanup_func,
                                    void *cleanup_arg, int argc, char **argv);

static int xbt_ctx_sysv_factory_finalize(xbt_context_factory_t * factory);

static int
xbt_ctx_sysv_factory_create_maestro_context(xbt_context_t * maestro);

static void xbt_ctx_sysv_free(xbt_context_t context);

static void xbt_ctx_sysv_kill(xbt_context_t context);

static void xbt_ctx_sysv_schedule(xbt_context_t context);

static void xbt_ctx_sysv_yield(void);

static void xbt_ctx_sysv_start(xbt_context_t context);

static void xbt_ctx_sysv_stop(int exit_code);

static void xbt_ctx_sysv_swap(xbt_context_t context);

static void xbt_ctx_sysv_schedule(xbt_context_t context);

static void xbt_ctx_sysv_yield(void);

static void xbt_ctx_sysv_suspend(xbt_context_t context);

static void xbt_ctx_sysv_resume(xbt_context_t context);

static void xbt_ctx_sysv_wrapper(void);

/* callback: context fetching */
static ex_ctx_t *xbt_ctx_sysv_ex_ctx(void)
{
  return current_context->exception;
}

/* callback: termination */
static void xbt_ctx_sysv_ex_terminate(xbt_ex_t * e)
{
  xbt_ex_display(e);
  abort();
}


void xbt_ctx_sysv_factory_init(xbt_context_factory_t * factory)
{
  *factory = xbt_new0(s_xbt_context_factory_t, 1);

  (*factory)->create_context = xbt_ctx_sysv_factory_create_context;
  (*factory)->finalize = xbt_ctx_sysv_factory_finalize;
  (*factory)->create_maestro_context = xbt_ctx_sysv_factory_create_maestro_context;
  (*factory)->free = xbt_ctx_sysv_free;
  (*factory)->kill = xbt_ctx_sysv_kill;
  (*factory)->schedule = xbt_ctx_sysv_schedule;
  (*factory)->yield = xbt_ctx_sysv_yield;
  (*factory)->start = xbt_ctx_sysv_start;
  (*factory)->stop = xbt_ctx_sysv_stop;
  (*factory)->name = "ctx_sysv_context_factory";

  /* context exception handlers */
  __xbt_ex_ctx = xbt_ctx_sysv_ex_ctx;
  __xbt_ex_terminate = xbt_ctx_sysv_ex_terminate;
}

static int
xbt_ctx_sysv_factory_create_maestro_context(xbt_context_t * maestro)
{

  xbt_ctx_sysv_t context = xbt_new0(s_xbt_ctx_sysv_t, 1);

  context->name = (char *) "maestro";

  context->exception = xbt_new(ex_ctx_t, 1);
  XBT_CTX_INITIALIZE(context->exception);

  *maestro = (xbt_context_t) context;

  return 0;

}


static int xbt_ctx_sysv_factory_finalize(xbt_context_factory_t * factory)
{
  free(maestro_context->exception);
  free(*factory);
  *factory = NULL;
  return 0;
}

static xbt_context_t
xbt_ctx_sysv_factory_create_context(const char *name, xbt_main_func_t code,
                                    void_f_pvoid_t startup_func,
                                    void *startup_arg,
                                    void_f_pvoid_t cleanup_func,
                                    void *cleanup_arg, int argc, char **argv)
{
  VERB1("Create context %s", name);
  xbt_ctx_sysv_t context = xbt_new0(s_xbt_ctx_sysv_t, 1);

  context->code = code;
  context->name = xbt_strdup(name);

  xbt_assert2(getcontext(&(context->uc)) == 0,
              "Error in context saving: %d (%s)", errno, strerror(errno));
  context->uc.uc_link = NULL;
  context->uc.uc_stack.ss_sp =
    pth_skaddr_makecontext(context->stack, STACK_SIZE);
  context->uc.uc_stack.ss_size =
    pth_sksize_makecontext(context->stack, STACK_SIZE);
#ifdef HAVE_VALGRIND_VALGRIND_H
  context->valgrind_stack_id =
    VALGRIND_STACK_REGISTER(context->uc.uc_stack.ss_sp,
                            ((char *) context->uc.uc_stack.ss_sp) +
                            context->uc.uc_stack.ss_size);
#endif /* HAVE_VALGRIND_VALGRIND_H */

  context->exception = xbt_new(ex_ctx_t, 1);
  XBT_CTX_INITIALIZE(context->exception);
  context->iwannadie = 0;       /* useless but makes valgrind happy */
  context->argc = argc;
  context->argv = argv;
  context->startup_func = startup_func;
  context->startup_arg = startup_arg;
  context->cleanup_func = cleanup_func;
  context->cleanup_arg = cleanup_arg;

  return (xbt_context_t) context;
}

static void xbt_ctx_sysv_free(xbt_context_t context)
{
  if (context) {
    free(context->name);

    if (context->argv) {
      int i;

      for (i = 0; i < context->argc; i++)
        if (context->argv[i])
          free(context->argv[i]);

      free(context->argv);
    }

    if (context->exception)
      free(context->exception);

#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_STACK_DEREGISTER(((xbt_ctx_sysv_t) context)->valgrind_stack_id);
#endif /* HAVE_VALGRIND_VALGRIND_H */

    /* finally destroy the context */
    free(context);
  }
}

static void xbt_ctx_sysv_kill(xbt_context_t context)
{
  DEBUG2("Kill context '%s' (from '%s')", context->name,
         current_context->name);
  context->iwannadie = 1;
  xbt_ctx_sysv_swap(context);
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
static void xbt_ctx_sysv_schedule(xbt_context_t context)
{
  DEBUG1("Schedule context '%s'", context->name);
  xbt_assert0((current_context == maestro_context),
              "You are not supposed to run this function here!");
  xbt_ctx_sysv_swap(context);
}

/** 
 * Calling this function makes the current context yield. The context
 * that scheduled it returns from xbt_context_schedule as if nothing
 * had happened.
 * 
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
static void xbt_ctx_sysv_yield(void)
{
  DEBUG1("Yielding context '%s'", current_context->name);
  xbt_assert0((current_context != maestro_context),
              "You are not supposed to run this function here!");
  xbt_ctx_sysv_swap(current_context);
}

static void xbt_ctx_sysv_start(xbt_context_t context)
{
  DEBUG1("Start context '%s'", context->name);
  makecontext(&(((xbt_ctx_sysv_t) context)->uc), xbt_ctx_sysv_wrapper, 0);
}

static void xbt_ctx_sysv_stop(int exit_code)
{
  /* please no debug here: our procdata was already free'd */
  if (current_context->cleanup_func)
    ((*current_context->cleanup_func)) (current_context->cleanup_arg);

  xbt_swag_remove(current_context, context_living);
  xbt_swag_insert(current_context, context_to_destroy);

  xbt_ctx_sysv_swap(current_context);
}

static void xbt_ctx_sysv_swap(xbt_context_t context)
{
  DEBUG2("Swap context: '%s' -> '%s'", current_context->name, context->name);
  xbt_assert0(current_context, "You have to call context_init() first.");
  xbt_assert0(context, "Invalid argument");

  if (((xbt_ctx_sysv_t) context)->prev == NULL)
    xbt_ctx_sysv_resume(context);
  else
    xbt_ctx_sysv_suspend(context);

  if (current_context->iwannadie)
    xbt_ctx_sysv_stop(1);
}

static void xbt_ctx_sysv_wrapper(void)
{
  if (current_context->startup_func)
    (*current_context->startup_func) (current_context->startup_arg);

  xbt_ctx_sysv_stop((*(current_context->code))
                    (current_context->argc, current_context->argv));
}

static void xbt_ctx_sysv_suspend(xbt_context_t context)
{
  int rv;

  DEBUG1("Suspend context: '%s'", current_context->name);
  xbt_ctx_sysv_t prev_context = ((xbt_ctx_sysv_t) context)->prev;

  current_context = (xbt_context_t) (((xbt_ctx_sysv_t) context)->prev);

  ((xbt_ctx_sysv_t) context)->prev = NULL;

  rv = swapcontext(&(((xbt_ctx_sysv_t) context)->uc), &(prev_context->uc));

  xbt_assert0((rv == 0), "Context swapping failure");
}

static void xbt_ctx_sysv_resume(xbt_context_t context)
{
  int rv;

  DEBUG2("Resume context: '%s' (from '%s')", context->name,
         current_context->name);
  ((xbt_ctx_sysv_t) context)->prev = (xbt_ctx_sysv_t) current_context;

  current_context = context;

  rv = swapcontext(&(((xbt_ctx_sysv_t) context)->prev->uc),
                   &(((xbt_ctx_sysv_t) context)->uc));

  xbt_assert0((rv == 0), "Context swapping failure");
}
