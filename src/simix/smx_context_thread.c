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
#include "simix/smx_context_private.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

typedef struct s_smx_ctx_thread {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  xbt_os_thread_t thread;       /* a plain dumb thread (portable to posix or windows) */
  xbt_os_sem_t begin;           /* this semaphore is used to schedule/yield the process  */
  xbt_os_sem_t end;             /* this semaphore is used to schedule/unschedule the process   */
} s_smx_ctx_thread_t, *smx_ctx_thread_t;

static smx_context_t
smx_ctx_thread_factory_create_context(xbt_main_func_t code, int argc, char** argv, 
                                      void_f_pvoid_t cleanup_func, void* cleanup_arg);

static void smx_ctx_thread_free(smx_context_t context);
static void smx_ctx_thread_stop(smx_context_t context);
static void smx_ctx_thread_suspend(smx_context_t context);
static void smx_ctx_thread_resume(smx_context_t new_context);

static void *smx_ctx_thread_wrapper(void *param);

void SIMIX_ctx_thread_factory_init(smx_context_factory_t * factory) {

  smx_ctx_base_factory_init(factory);

  (*factory)->create_context = smx_ctx_thread_factory_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_thread_free;
  (*factory)->stop = smx_ctx_thread_stop;
  (*factory)->suspend = smx_ctx_thread_suspend;
  (*factory)->resume = smx_ctx_thread_resume;
  (*factory)->name = "ctx_thread_factory";
}

static smx_context_t 
smx_ctx_thread_factory_create_context(xbt_main_func_t code, int argc, char** argv, 
                                      void_f_pvoid_t cleanup_func, void* cleanup_arg)
{
  smx_ctx_thread_t context = (smx_ctx_thread_t)smx_ctx_base_factory_create_context_sized
      (sizeof(s_smx_ctx_thread_t), code,argc,argv,cleanup_func,cleanup_arg);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if(code){
    context->begin = xbt_os_sem_init(0);
    context->end = xbt_os_sem_init(0);


    /* create and start the process */
    /* NOTE: The first argument to xbt_os_thread_create used to be the process *
     * name, but now the name is stored at SIMIX level, so we pass a null      */
    context->thread =
      xbt_os_thread_create(NULL, smx_ctx_thread_wrapper, context);

    /* wait the starting of the newly created process */
    xbt_os_sem_acquire(context->end);
  }
    
  return (smx_context_t)context;
}

static void smx_ctx_thread_free(smx_context_t pcontext)
{
  smx_ctx_thread_t context = (smx_ctx_thread_t)pcontext;

  /* check if this is the context of maestro (it doesn't has a real thread) */  
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

  smx_ctx_thread_t context = (smx_ctx_thread_t)pcontext;
  
  /* please no debug here: our procdata was already free'd */
  smx_ctx_base_stop(pcontext);

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

  (context->super.code) (context->super.argc, context->super.argv);

  smx_ctx_thread_stop((smx_context_t)context);
  return NULL;
}

static void smx_ctx_thread_suspend(smx_context_t context) {
  xbt_os_sem_release(((smx_ctx_thread_t) context)->end);
  xbt_os_sem_acquire(((smx_ctx_thread_t) context)->begin);
}

static void smx_ctx_thread_resume(smx_context_t new_context) {
  xbt_os_sem_release(((smx_ctx_thread_t) new_context)->begin);
  xbt_os_sem_acquire(((smx_ctx_thread_t) new_context)->end);
}
