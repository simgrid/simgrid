/* context_java - implementation of context switching for java threads */

/* Copyright (c) 2007-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "xbt/function_types.h"
#include "private.h"
#include "smx_context_java.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jmsg, bindings, "MSG for Java(TM)");

static smx_context_t
smx_ctx_java_factory_create_context(xbt_main_func_t code, int argc, char** argv, 
                                    void_f_pvoid_t cleanup_func, void* cleanup_arg);

static void smx_ctx_java_free(smx_context_t context);
static void smx_ctx_java_start(smx_context_t context);
static void smx_ctx_java_stop(smx_context_t context);
static void smx_ctx_java_suspend(smx_context_t context);
static void smx_ctx_java_resume(smx_context_t new_context);

void SIMIX_ctx_java_factory_init(smx_context_factory_t * factory)
{
  /* instantiate the context factory */
  smx_ctx_base_factory_init(factory);

  (*factory)->create_context = smx_ctx_java_factory_create_context;
  /* Leave default behavior of (*factory)->finalize */
  (*factory)->free = smx_ctx_java_free;
  (*factory)->stop = smx_ctx_java_stop;
  (*factory)->suspend = smx_ctx_java_suspend;
  (*factory)->resume = smx_ctx_java_resume;

  (*factory)->name = "ctx_java_factory";
}

static smx_context_t
smx_ctx_java_factory_create_context(xbt_main_func_t code, int argc, char** argv, 
                                    void_f_pvoid_t cleanup_func, void* cleanup_arg)
{
  smx_ctx_java_t context = xbt_new0(s_smx_ctx_java_t, 1);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if(code){
    context->super.cleanup_func = cleanup_func;
    context->super.cleanup_arg = cleanup_arg;
    context->jprocess = (jobject) code;
    context->jenv = get_current_thread_env();
    jprocess_start(((smx_ctx_java_t) context)->jprocess,
                   get_current_thread_env());
  }
    
  return (smx_context_t) context;
}

static void smx_ctx_java_free(smx_context_t context)
{
  if (context) {
    smx_ctx_java_t ctx_java = (smx_ctx_java_t) context;

    if (ctx_java->jprocess) {
      jobject jprocess = ctx_java->jprocess;

      ctx_java->jprocess = NULL;

      /* if the java process is alive join it */
      if (jprocess_is_alive(jprocess, get_current_thread_env()))
        jprocess_join(jprocess, get_current_thread_env());
    }
  }

  smx_ctx_base_free(context);
} 

static void smx_ctx_java_stop(smx_context_t context)
{
  jobject jprocess = NULL;

  smx_ctx_java_t ctx_java;

  if (context->cleanup_func)
    (*(context->cleanup_func)) (context->cleanup_arg);

  ctx_java = (smx_ctx_java_t) context;

  /*FIXME: is this really necessary?*/
  if (simix_global->current_process->iwannadie) {
    /* The maestro call xbt_context_stop() with an exit code set to one */
    if (ctx_java->jprocess) {
      /* if the java process is alive schedule it */
      if (jprocess_is_alive(ctx_java->jprocess, get_current_thread_env())) {
        jprocess_schedule(simix_global->current_process->context);
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

/*static void smx_ctx_java_swap(smx_context_t context)
{
  if (context) {
    smx_context_t self = current_context;

    current_context = context;

    jprocess_schedule(context);

    current_context = self;
  }

  if (current_context->iwannadie)
    smx_ctx_java_stop(1);
}*/

static void smx_ctx_java_suspend(smx_context_t context)
{
  jprocess_unschedule(context);
}

// FIXME: inline those functions
static void 
smx_ctx_java_resume(smx_context_t new_context)
{
  jprocess_schedule(new_context);
}
