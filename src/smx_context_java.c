/* context_java - implementation of context switching for java threads */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <xbt/function_types.h>
#include <simix/simix.h>
#include "smx_context_java.h"
#include "xbt/dynar.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jmsg, bindings, "MSG for Java(TM)");

static smx_context_t my_current_context = NULL;

static smx_context_t smx_ctx_java_self(void);
static smx_context_t
smx_ctx_java_factory_create_context(xbt_main_func_t code, int argc,
                                    char **argv,
                                    void_pfn_smxprocess_t cleanup_func,
                                    void *data);

static void smx_ctx_java_free(smx_context_t context);
static void smx_ctx_java_start(smx_context_t context);
static void smx_ctx_java_suspend(smx_context_t context);
static void smx_ctx_java_resume(smx_context_t new_context);
static void smx_ctx_java_runall(void);

void SIMIX_ctx_java_factory_init(smx_context_factory_t * factory)
{
  /* instantiate the context factory */
  smx_ctx_base_factory_init(factory);

  (*factory)->create_context = smx_ctx_java_factory_create_context;
  /* Leave default behavior of (*factory)->finalize */
  (*factory)->free = smx_ctx_java_free;
  (*factory)->stop = smx_ctx_java_stop;
  (*factory)->suspend = smx_ctx_java_suspend;
  (*factory)->runall = smx_ctx_java_runall;
  (*factory)->name = "ctx_java_factory";
  //(*factory)->finalize = smx_ctx_base_factory_finalize;
  (*factory)->self = smx_ctx_java_self;
  (*factory)->get_data = smx_ctx_base_get_data;
}

static smx_context_t smx_ctx_java_self(void)
{
	return my_current_context;
}

static smx_context_t
smx_ctx_java_factory_create_context(xbt_main_func_t code, int argc,
                                    char **argv,
                                    void_pfn_smxprocess_t cleanup_func,
                                    void* data)
{
  XBT_DEBUG("XXXX Create Context\n");
  smx_ctx_java_t context = xbt_new0(s_smx_ctx_java_t, 1);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if (code) {
    context->super.cleanup_func = cleanup_func;
    context->jprocess = (jobject) code;
    context->jenv = get_current_thread_env();
    jprocess_start(((smx_ctx_java_t) context)->jprocess,
                   get_current_thread_env());
  }else{
    my_current_context = (smx_context_t)context;
  }
  context->super.data = data;
  
  return (smx_context_t) context;
}

static void smx_ctx_java_free(smx_context_t context)
{
  if (context) {
    smx_ctx_java_t ctx_java = (smx_ctx_java_t) context;

    if (ctx_java->jprocess) { /* the java process still exists */
      jobject jprocess = ctx_java->jprocess;
      ctx_java->jprocess = NULL;

      /* stop it */
      XBT_DEBUG("The process still exists, making it exit now");
      jprocess_exit(jprocess, get_current_thread_env());

      /* it's dead now, remove it from the JVM */
      jprocess_delete_global_ref(jprocess, get_current_thread_env());
    }
  }

  smx_ctx_base_free(context);
}


void smx_ctx_java_stop(smx_context_t context)
{
 xbt_assert(context == my_current_context,
     "The context to stop must be the current one");
  /* I am the current process and I am dying */
  
  smx_ctx_base_stop(context);

  XBT_DEBUG("I am dying");

  /* suspend myself again, smx_ctx_java_free() will destroy me later
   * from maeastro */
  jprocess_unschedule(context);
  XBT_INFO("Java stop finished");
}

static void smx_ctx_java_suspend(smx_context_t context)
{
  jprocess_unschedule(context);
}

// FIXME: inline those functions
static void smx_ctx_java_resume(smx_context_t new_context)
{
  XBT_DEBUG("XXXX Context Resume\n");
  jprocess_schedule(new_context);
}

static void smx_ctx_java_runall(void)
{
  xbt_dynar_t processes = SIMIX_process_get_runnable();
  XBT_DEBUG("XXXX Run all\n");
  smx_process_t process;
  smx_context_t old_context;
  unsigned int cursor;
  xbt_dynar_foreach(processes, cursor, process) {
    old_context = my_current_context;
    my_current_context = SIMIX_process_get_context(process);
    smx_ctx_java_resume(my_current_context);
    my_current_context = old_context;
  }

  XBT_DEBUG("XXXX End of run all\n");
}
