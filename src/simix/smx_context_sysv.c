/* $Id$ */

/* context_sysv - implementation of context switching with ucontextes from Sys V */

/* Copyright (c) 2004-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex_interface.h"
#include "private.h"

#include "context_sysv_config.h"        /* loads context system definitions                             */
#include "portable.h"
#include <ucontext.h>           /* context relative declarations                                */
#define STACK_SIZE 128*1024     /* lower this if you want to reduce the memory consumption      */
#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#endif /* HAVE_VALGRIND_VALGRIND_H */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smx_context);

typedef struct s_smx_ctx_sysv {
  SMX_CTX_BASE_T;
  ucontext_t uc;                /* the thread that execute the code */
  char stack[STACK_SIZE];       /* the thread stack size */
  smx_process_t prev;           /* the previous process */
#ifdef HAVE_VALGRIND_VALGRIND_H
  unsigned int valgrind_stack_id;       /* the valgrind stack id */
#endif                          
} s_smx_ctx_sysv_t, *smx_ctx_sysv_t;


/* callback: context fetching */
static ex_ctx_t *xbt_jcontext_ex_ctx(void);

/* callback: termination */
static void xbt_jcontext_ex_terminate(xbt_ex_t * e);

static int
smx_ctx_sysv_factory_create_context(smx_process_t *smx_process, xbt_main_func_t code);

static int smx_ctx_sysv_factory_finalize(smx_context_factory_t * factory);

static int smx_ctx_sysv_factory_create_maestro_context(smx_process_t * maestro);

static void smx_ctx_sysv_free(smx_process_t process);

static void smx_ctx_sysv_kill(smx_process_t process);

static void smx_ctx_sysv_schedule(smx_process_t process);

static void smx_ctx_sysv_yield(void);

static void smx_ctx_sysv_start(smx_process_t process);

static void smx_ctx_sysv_stop(int exit_code);

static void smx_ctx_sysv_swap(smx_process_t process);

static void smx_ctx_sysv_schedule(smx_process_t process);

static void smx_ctx_sysv_yield(void);

static void smx_ctx_sysv_suspend(smx_process_t process);

static void smx_ctx_sysv_resume(smx_process_t process);

static void smx_ctx_sysv_wrapper(void);

/* callback: context fetching */
static ex_ctx_t *xbt_ctx_sysv_ex_ctx(void)
{
  return simix_global->current_process->context->exception;
}

/* callback: termination */
static void xbt_ctx_sysv_ex_terminate(xbt_ex_t * e)
{
  xbt_ex_display(e);
  abort();
}

void SIMIX_ctx_sysv_factory_init(smx_context_factory_t * factory)
{
  *factory = xbt_new0(s_smx_context_factory_t, 1);

  (*factory)->create_context = smx_ctx_sysv_factory_create_context;
  (*factory)->finalize = smx_ctx_sysv_factory_finalize;
  (*factory)->create_maestro_context = smx_ctx_sysv_factory_create_maestro_context;
  (*factory)->free = smx_ctx_sysv_free;
  (*factory)->kill = smx_ctx_sysv_kill;
  (*factory)->schedule = smx_ctx_sysv_schedule;
  (*factory)->yield = smx_ctx_sysv_yield;
  (*factory)->start = smx_ctx_sysv_start;
  (*factory)->stop = smx_ctx_sysv_stop;
  (*factory)->name = "smx_sysv_context_factory";

  /* context exception handlers */
  __xbt_ex_ctx = xbt_ctx_sysv_ex_ctx;
  __xbt_ex_terminate = xbt_ctx_sysv_ex_terminate;
}

static int smx_ctx_sysv_factory_create_maestro_context(smx_process_t *maestro)
{
  smx_ctx_sysv_t context = xbt_new0(s_smx_ctx_sysv_t, 1);

  context->exception = xbt_new(ex_ctx_t, 1);
  XBT_CTX_INITIALIZE(context->exception);

  (*maestro)->context = (smx_context_t) context;

  return 0;

}

static int smx_ctx_sysv_factory_finalize(smx_context_factory_t * factory)
{
  /*FIXME free(maestro_context->exception);*/
  free(*factory);
  *factory = NULL;
  return 0;
}

static int
smx_ctx_sysv_factory_create_context(smx_process_t *smx_process, xbt_main_func_t code)
{
  VERB1("Create context %s", (*smx_process)->name);
  smx_ctx_sysv_t context = xbt_new0(s_smx_ctx_sysv_t, 1);

  context->code = code;

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
  (*smx_process)->context = (smx_context_t)context;
  (*smx_process)->iwannadie = 0;

  /* FIXME: Check what should return */
  return 1;
}

static void smx_ctx_sysv_free(smx_process_t process)
{
  smx_ctx_sysv_t context = (smx_ctx_sysv_t)process->context;   
  if (context){

    if (context->exception)
      free(context->exception);

#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_STACK_DEREGISTER(((smx_ctx_sysv_t) context)->valgrind_stack_id);
#endif /* HAVE_VALGRIND_VALGRIND_H */

    /* destroy the context */
    free(context);
  }
}

static void smx_ctx_sysv_kill(smx_process_t process)
{
  DEBUG2("Kill process '%s' (from '%s')", process->name,
         simix_global->current_process->name);
  process->iwannadie = 1;
  smx_ctx_sysv_swap(process);
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
static void smx_ctx_sysv_schedule(smx_process_t process)
{
  DEBUG1("Schedule process '%s'", process->name);
  xbt_assert0((simix_global->current_process == simix_global->maestro_process),
              "You are not supposed to run this function here!");
  smx_ctx_sysv_swap(process);
}

/** 
 * Calling this function makes the current context yield. The context
 * that scheduled it returns from xbt_context_schedule as if nothing
 * had happened.
 * 
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
static void smx_ctx_sysv_yield(void)
{
  DEBUG1("Yielding process '%s'", simix_global->current_process->name);
  xbt_assert0((simix_global->current_process != simix_global->maestro_process),
              "You are not supposed to run this function here!");
  smx_ctx_sysv_swap(simix_global->current_process);
}

static void smx_ctx_sysv_start(smx_process_t process)
{
  /*DEBUG1("Start context '%s'", context->name);*/
  makecontext(&(((smx_ctx_sysv_t) process->context)->uc), smx_ctx_sysv_wrapper, 0);
}

static void smx_ctx_sysv_stop(int exit_code)
{
  /* please no debug here: our procdata was already free'd */
  if (simix_global->current_process->cleanup_func)
    ((*simix_global->current_process->cleanup_func)) (simix_global->current_process->cleanup_arg);

  smx_ctx_sysv_swap(simix_global->current_process);
}

static void smx_ctx_sysv_swap(smx_process_t process)
{
  DEBUG2("Swap context: '%s' -> '%s'", simix_global->current_process->name, process->name);
  xbt_assert0(simix_global->current_process, "You have to call context_init() first.");
  xbt_assert0(process, "Invalid argument");

  if (((smx_ctx_sysv_t) process->context)->prev == NULL)
    smx_ctx_sysv_resume(process);
  else
    smx_ctx_sysv_suspend(process);

  if (simix_global->current_process->iwannadie)
    smx_ctx_sysv_stop(1);
}

static void smx_ctx_sysv_wrapper(void)
{
  smx_ctx_sysv_stop((*(simix_global->current_process->context->code))
                    (simix_global->current_process->argc, simix_global->current_process->argv));
}

static void smx_ctx_sysv_suspend(smx_process_t process)
{
  int rv;

  DEBUG1("Suspend context: '%s'", simix_global->current_process->name);
  smx_process_t prev_process = ((smx_ctx_sysv_t) process->context)->prev;

  simix_global->current_process = prev_process;

  ((smx_ctx_sysv_t) process->context)->prev = NULL;

  rv = swapcontext(&(((smx_ctx_sysv_t) process->context)->uc), &(((smx_ctx_sysv_t)prev_process->context)->uc));

  xbt_assert0((rv == 0), "Context swapping failure");
}

static void smx_ctx_sysv_resume(smx_process_t process)
{
  int rv;
  smx_ctx_sysv_t new_context = (smx_ctx_sysv_t)process->context;
  smx_ctx_sysv_t prev_context = (smx_ctx_sysv_t)simix_global->current_process->context;

  DEBUG2("Resume context: '%s' (from '%s')", process->name,
         simix_global->current_process->name);

  ((smx_ctx_sysv_t) process->context)->prev = simix_global->current_process;

  simix_global->current_process = process;

  rv = swapcontext(&prev_context->uc, &new_context->uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}
