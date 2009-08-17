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
#include "xbt_modinter.h"       /* prototype of os thread module's init/exit in XBT */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

typedef struct s_smx_ctx_thread {
  SMX_CTX_BASE_T;
  xbt_os_thread_t thread;       /* a plain dumb thread (portable to posix or windows) */
  xbt_os_sem_t begin;           /* this semaphore is used to schedule/yield the process  */
  xbt_os_sem_t end;             /* this semaphore is used to schedule/unschedule the process   */
} s_smx_ctx_thread_t, *smx_ctx_thread_t;

static smx_context_t
smx_ctx_thread_factory_create_context(xbt_main_func_t code, int argc, char** argv, 
                                      void_f_pvoid_t cleanup_func, void* cleanup_arg);

static int smx_ctx_thread_factory_finalize(smx_context_factory_t * factory);

static void smx_ctx_thread_free(smx_context_t context);

static void smx_ctx_thread_start(smx_context_t context);

static void smx_ctx_thread_stop(smx_context_t context);

static void smx_ctx_thread_suspend(smx_context_t context);

static void 
  smx_ctx_thread_resume(smx_context_t old_context, smx_context_t new_context);

static void *smx_ctx_thread_wrapper(void *param);

void SIMIX_ctx_thread_factory_init(smx_context_factory_t * factory)
{
  /* Initialize the thread portability layer 
  xbt_os_thread_mod_init();*/
  
  *factory = xbt_new0(s_smx_context_factory_t, 1);

  (*factory)->create_context = smx_ctx_thread_factory_create_context;
  (*factory)->finalize = smx_ctx_thread_factory_finalize;
  (*factory)->free = smx_ctx_thread_free;
  (*factory)->start = smx_ctx_thread_start;
  (*factory)->stop = smx_ctx_thread_stop;
  (*factory)->suspend = smx_ctx_thread_suspend;
  (*factory)->resume = smx_ctx_thread_resume;
  (*factory)->name = "ctx_thread_factory";
}

static int smx_ctx_thread_factory_finalize(smx_context_factory_t * factory)
{
  /* Stop the thread portability layer 
  xbt_os_thread_mod_exit();*/
  free(*factory);
  *factory = NULL;
  return 0;
}

static smx_context_t 
smx_ctx_thread_factory_create_context(xbt_main_func_t code, int argc, char** argv, 
                                      void_f_pvoid_t cleanup_func, void* cleanup_arg)
{
  smx_ctx_thread_t context = xbt_new0(s_smx_ctx_thread_t, 1);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if(code){
    context->code = code;
    context->argc = argc;
    context->argv = argv;
    context->cleanup_func = cleanup_func;
    context->cleanup_arg = cleanup_arg;
    context->begin = xbt_os_sem_init(0);
    context->end = xbt_os_sem_init(0);
  }
    
  return (smx_context_t)context;
}

static void smx_ctx_thread_free(smx_context_t pcontext)
{
  int i;
  smx_ctx_thread_t context = (smx_ctx_thread_t)pcontext;

  /* check if this is the context of maestro (it doesn't has a real thread) */  
  if (context->thread) {
    /* wait about the thread terminason */
    xbt_os_thread_join(context->thread, NULL);
    
    /* destroy the synchronisation objects */
    xbt_os_sem_destroy(context->begin);
    xbt_os_sem_destroy(context->end);
  }
  
  /* free argv */
  if (context->argv) {
    for (i = 0; i < context->argc; i++)
      if (context->argv[i])
        free(context->argv[i]);

    free(context->argv);
  }
    
  /* finally destroy the context */
  free(context);
}

static void smx_ctx_thread_start(smx_context_t context)
{
  smx_ctx_thread_t ctx_thread = (smx_ctx_thread_t)context;

  /* create and start the process */
  /* NOTE: The first argument to xbt_os_thread_create used to be the process *
   * name, but now the name is stored at SIMIX level, so we pass a null      */
  ctx_thread->thread =
    xbt_os_thread_create(NULL, smx_ctx_thread_wrapper, ctx_thread);

  /* wait the starting of the newly created process */
  xbt_os_sem_acquire(ctx_thread->end);
}

static void smx_ctx_thread_stop(smx_context_t pcontext)
{

  smx_ctx_thread_t context = (smx_ctx_thread_t)pcontext;
  
  /* please no debug here: our procdata was already free'd */
  if (context->cleanup_func)
    (*context->cleanup_func) (context->cleanup_arg);

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

  (context->code) (context->argc, context->argv);

  smx_ctx_thread_stop((smx_context_t)context);
  return NULL;
}

static void smx_ctx_thread_suspend(smx_context_t context)
{
  xbt_os_sem_release(((smx_ctx_thread_t) context)->end);
  xbt_os_sem_acquire(((smx_ctx_thread_t) context)->begin);
}

static void smx_ctx_thread_resume(smx_context_t not_used, 
                                  smx_context_t new_context)
{
  xbt_os_sem_release(((smx_ctx_thread_t) new_context)->begin);
  xbt_os_sem_acquire(((smx_ctx_thread_t) new_context)->end);
}