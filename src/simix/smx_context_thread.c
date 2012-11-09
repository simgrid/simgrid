/* context_thread - implementation of context switching with native threads */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/function_types.h"
#include "smx_private.h"
#include "portable.h"           /* loads context system definitions */
#include "xbt/swag.h"
#include "xbt/xbt_os_thread.h"
#include "xbt_modinter.h"       /* prototype of os thread module's init/exit in XBT */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

typedef struct s_smx_ctx_thread {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  xbt_os_thread_t thread;       /* a plain dumb thread (portable to posix or windows) */
  xbt_os_sem_t begin;           /* this semaphore is used to schedule/yield the process  */
  xbt_os_sem_t end;             /* this semaphore is used to schedule/unschedule the process   */
} s_smx_ctx_thread_t, *smx_ctx_thread_t;

static xbt_os_sem_t smx_ctx_thread_sem;

static smx_context_t
smx_ctx_thread_factory_create_context(xbt_main_func_t code, int argc,
                                      char **argv,
                                      void_pfn_smxprocess_t cleanup_func,
                                      void *data);

static void smx_ctx_thread_free(smx_context_t context);
static void smx_ctx_thread_stop(smx_context_t context);
static void smx_ctx_thread_suspend(smx_context_t context);
static void smx_ctx_thread_runall_serial(void);
static void smx_ctx_thread_runall_parallel(void);
static smx_context_t smx_ctx_thread_self(void);

static int smx_ctx_thread_factory_finalize(smx_context_factory_t *factory);
static void *smx_ctx_thread_wrapper(void *param);

void SIMIX_ctx_thread_factory_init(smx_context_factory_t * factory)
{
  smx_ctx_base_factory_init(factory);
  XBT_VERB("Activating thread context factory");

  (*factory)->finalize  = smx_ctx_thread_factory_finalize;
  (*factory)->create_context = smx_ctx_thread_factory_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_thread_free;
  (*factory)->stop = smx_ctx_thread_stop;
  (*factory)->suspend = smx_ctx_thread_suspend;

  if (SIMIX_context_is_parallel())
    (*factory)->runall = smx_ctx_thread_runall_parallel;
  else
    (*factory)->runall = smx_ctx_thread_runall_serial;

  (*factory)->self = smx_ctx_thread_self;
  (*factory)->name = "ctx_thread_factory";

  if (SIMIX_context_is_parallel()) {
    smx_ctx_thread_sem = xbt_os_sem_init(SIMIX_context_get_nthreads());
  } else {
    smx_ctx_thread_sem = NULL;
  }
}

static int smx_ctx_thread_factory_finalize(smx_context_factory_t *factory)
{
  if (smx_ctx_thread_sem) {
    xbt_os_sem_destroy(smx_ctx_thread_sem);
    smx_ctx_thread_sem = NULL;
  }
  return smx_ctx_base_factory_finalize(factory);
}

static smx_context_t
smx_ctx_thread_factory_create_context(xbt_main_func_t code, int argc,
                                      char **argv,
                                      void_pfn_smxprocess_t cleanup_func,
                                      void *data)
{
  smx_ctx_thread_t context = (smx_ctx_thread_t)
      smx_ctx_base_factory_create_context_sized(sizeof(s_smx_ctx_thread_t),
                                                code, argc, argv,
                                                cleanup_func, data);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if (code) {
    context->begin = xbt_os_sem_init(0);
    context->end = xbt_os_sem_init(0);
    xbt_os_thread_setstacksize(smx_context_stack_size);

    /* create and start the process */
    /* NOTE: The first argument to xbt_os_thread_create used to be the process *
    * name, but now the name is stored at SIMIX level, so we pass a null  */
    context->thread =
      xbt_os_thread_create(NULL, smx_ctx_thread_wrapper, context, context);


    /* wait the starting of the newly created process */
    xbt_os_sem_acquire(context->end);

  } else {
    xbt_os_thread_set_extra_data(context);
  }

  return (smx_context_t) context;
}

static void smx_ctx_thread_free(smx_context_t pcontext)
{
  smx_ctx_thread_t context = (smx_ctx_thread_t) pcontext;

  /* check if this is the context of maestro (it doesn't have a real thread) */
  if (context->thread) {
    /* wait about the thread terminason */
    xbt_os_thread_join(context->thread, NULL);

    /* destroy the synchronisation objects */
    xbt_os_sem_destroy(context->begin);
    xbt_os_sem_destroy(context->end);
  }

  smx_ctx_base_free(pcontext);
}

static void smx_ctx_thread_stop(smx_context_t pcontext)
{
  smx_ctx_thread_t context = (smx_ctx_thread_t) pcontext;

  /* please no debug here: our procdata was already free'd */
  smx_ctx_base_stop(pcontext);

  if (smx_ctx_thread_sem)       /* parallel run */
    xbt_os_sem_release(smx_ctx_thread_sem);

  /* signal to the maestro that it has finished */
  xbt_os_sem_release(((smx_ctx_thread_t) context)->end);

  /* exit */
  /* We should provide return value in case other wants it */
  xbt_os_thread_exit(NULL);
}

static void *smx_ctx_thread_wrapper(void *param)
{
  smx_ctx_thread_t context = (smx_ctx_thread_t) param;

  /* Tell the maestro we are starting, and wait for its green light */
  xbt_os_sem_release(context->end);
  xbt_os_sem_acquire(context->begin);
  if (smx_ctx_thread_sem)       /* parallel run */
    xbt_os_sem_acquire(smx_ctx_thread_sem);

  (context->super.code) (context->super.argc, context->super.argv);

  smx_ctx_thread_stop((smx_context_t) context);
  return NULL;
}

static void smx_ctx_thread_suspend(smx_context_t context)
{
  if (smx_ctx_thread_sem)       /* parallel run */
    xbt_os_sem_release(smx_ctx_thread_sem);
  xbt_os_sem_release(((smx_ctx_thread_t) context)->end);
  xbt_os_sem_acquire(((smx_ctx_thread_t) context)->begin);
  if (smx_ctx_thread_sem)       /* parallel run */
    xbt_os_sem_acquire(smx_ctx_thread_sem);
}

static void smx_ctx_thread_runall_serial(void)
{
  smx_process_t process;
  unsigned int cursor;

  xbt_dynar_foreach(simix_global->process_to_run, cursor, process) {
    xbt_os_sem_release(((smx_ctx_thread_t) process->context)->begin);
    xbt_os_sem_acquire(((smx_ctx_thread_t) process->context)->end);
  }
}

static void smx_ctx_thread_runall_parallel(void)
{
  unsigned int index;
  smx_process_t process;

  xbt_dynar_foreach(simix_global->process_to_run, index, process)
    xbt_os_sem_release(((smx_ctx_thread_t) process->context)->begin);

  xbt_dynar_foreach(simix_global->process_to_run, index, process) {
     xbt_os_sem_acquire(((smx_ctx_thread_t) process->context)->end);
  }
}

static smx_context_t smx_ctx_thread_self(void)
{
  return (smx_context_t) xbt_os_thread_get_extra_data();
}
