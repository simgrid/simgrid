/* $Id$ */

/* context_thread - implementation of context switching with native threads */

/* Copyright (c) 2004-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/function_types.h"
#include "private.h"

#include "portable.h"           /* loads context system definitions */
#include "xbt/swag.h"
#include "xbt/xbt_os_thread.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smx_context);

typedef struct s_smx_ctx_thread {
  SMX_CTX_BASE_T;
  xbt_os_thread_t thread;       /* a plain dumb thread (portable to posix or windows) */
  xbt_os_sem_t begin;           /* this semaphore is used to schedule/yield the process  */
  xbt_os_sem_t end;             /* this semaphore is used to schedule/unschedule the process   */
} s_smx_ctx_thread_t, *smx_ctx_thread_t;

static int
smx_ctx_thread_factory_create_context(smx_process_t *smx_process, xbt_main_func_t code);

static int
smx_ctx_thread_factory_create_master_context(smx_process_t * maestro);

static int smx_ctx_thread_factory_finalize(smx_context_factory_t * factory);

static void smx_ctx_thread_free(smx_process_t process);

static void smx_ctx_thread_kill(smx_process_t process);

static void smx_ctx_thread_schedule(smx_process_t process);

static void smx_ctx_thread_yield(void);

static void smx_ctx_thread_start(smx_process_t process);

static void smx_ctx_thread_stop(int exit_code);

static void smx_ctx_thread_swap(smx_process_t process);

static void smx_ctx_thread_schedule(smx_process_t process);

static void smx_ctx_thread_yield(void);

static void smx_ctx_thread_suspend(smx_process_t process);

static void smx_ctx_thread_resume(smx_process_t process);

static void *smx_ctx_thread_wrapper(void *param);

void SIMIX_ctx_thread_factory_init(smx_context_factory_t * factory)
{
  *factory = xbt_new0(s_smx_context_factory_t, 1);

  (*factory)->create_context = smx_ctx_thread_factory_create_context;
  (*factory)->finalize = smx_ctx_thread_factory_finalize;
  (*factory)->create_maestro_context = smx_ctx_thread_factory_create_master_context;
  (*factory)->free = smx_ctx_thread_free;
  (*factory)->kill = smx_ctx_thread_kill;
  (*factory)->schedule = smx_ctx_thread_schedule;
  (*factory)->yield = smx_ctx_thread_yield;
  (*factory)->start = smx_ctx_thread_start;
  (*factory)->stop = smx_ctx_thread_stop;
  (*factory)->name = "ctx_thread_factory";
}

static int
smx_ctx_thread_factory_create_master_context(smx_process_t * maestro)
{
  (*maestro)->context = (smx_context_t) xbt_new0(s_smx_ctx_thread_t, 1);
  return 0;
}

static int smx_ctx_thread_factory_finalize(smx_context_factory_t * factory)
{
  free(*factory);
  *factory = NULL;
  return 0;
}

static int
smx_ctx_thread_factory_create_context(smx_process_t *smx_process, xbt_main_func_t code)
{
  smx_ctx_thread_t context = xbt_new0(s_smx_ctx_thread_t, 1);

  VERB1("Create context %s", (*smx_process)->name);
  context->code = code;
  context->begin = xbt_os_sem_init(0);
  context->end = xbt_os_sem_init(0);

  (*smx_process)->context = (smx_context_t)context;
  (*smx_process)->iwannadie = 0;

  /* FIXME: Check what should return */
  return 1;
}

static void smx_ctx_thread_free(smx_process_t process)
{
  smx_ctx_thread_t context = (smx_ctx_thread_t)process->context;

  /* Check if this is the context of maestro (it doesn't has a real thread) */  
  if (context->thread) {
    /* wait about the thread terminason */
    xbt_os_thread_join(context->thread, NULL);
    free(context->thread);
    
    /* destroy the synchronisation objects */
    xbt_os_sem_destroy(context->begin);
    xbt_os_sem_destroy(context->end);
  }
    
  /* finally destroy the context */
  free(context);
}

static void smx_ctx_thread_kill(smx_process_t process)
{
  DEBUG1("Kill process '%s'", process->name);
  process->iwannadie = 1;
  //smx_ctx_thread_swap(process);
}

/** 
 * \param context the winner
 *
 * Calling this function blocks the current context and schedule \a context.  
 * When \a context will call smx_context_yield, it will return
 * to this function as if nothing had happened.
 * 
 * Only the maestro can call this function to run a given process.
 */
static void smx_ctx_thread_schedule(smx_process_t process)
{
  DEBUG1("Schedule process '%s'", process->name);
  /*xbt_assert0((simix_global->current_process == simix_global->maestro_process),
              "You are not supposed to run this function here!");*/
  smx_ctx_thread_resume(process);
  /*smx_ctx_thread_swap(process);*/
}

/** 
 * Calling this function makes the current context yield. The context
 * that scheduled it returns from smx_context_schedule as if nothing
 * had happened.
 * 
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
static void smx_ctx_thread_yield(void)
{
  smx_ctx_thread_suspend(simix_global->current_process);
}

static void smx_ctx_thread_start(smx_process_t process)
{
  smx_ctx_thread_t ctx_thread = (smx_ctx_thread_t) process->context;

  DEBUG1("Start context '%s'", process->name);
  /* create and start the process */
  ctx_thread->thread =
    xbt_os_thread_create(process->name, smx_ctx_thread_wrapper,
                         ctx_thread);

  /* wait the starting of the newly created process */
  xbt_os_sem_acquire(ctx_thread->end);
}

static void smx_ctx_thread_stop(int exit_code)
{
  /* please no debug here: our procdata was already free'd */
  if (simix_global->current_process->cleanup_func)
    ((*simix_global->current_process->cleanup_func)) (simix_global->current_process->cleanup_arg);

  /* signal to the maestro that it has finished */
  xbt_os_sem_release(((smx_ctx_thread_t) simix_global->current_process->context)->end);

  /* exit */
  xbt_os_thread_exit(NULL);     /* We should provide return value in case other wants it */
}

/*FIXME: erase this function*/
static void smx_ctx_thread_swap(smx_process_t process)
{
  return;
}

static void *smx_ctx_thread_wrapper(void *param)
{
  smx_ctx_thread_t context = (smx_ctx_thread_t) param;

  /* Tell the maestro we are starting, and wait for its green light */
  xbt_os_sem_release(context->end);
  xbt_os_sem_acquire(context->begin);

  smx_ctx_thread_stop((context->code) (simix_global->current_process->argc, 
                                       simix_global->current_process->argv));
  return NULL;
}

static void smx_ctx_thread_suspend(smx_process_t process)
{
  DEBUG1("Suspend context '%s'", process->name);
  xbt_os_sem_release(((smx_ctx_thread_t) process->context)->end);
  xbt_os_sem_acquire(((smx_ctx_thread_t) process->context)->begin);
}

static void smx_ctx_thread_resume(smx_process_t process)
{
  DEBUG1("Resume context '%s'", process->name);
  xbt_os_sem_release(((smx_ctx_thread_t) process->context)->begin);
  xbt_os_sem_acquire(((smx_ctx_thread_t) process->context)->end);
}