/* context_java - implementation of context switching for java threads */

/* Copyright (c) 2009, 2010, 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <xbt/function_types.h>
#include <simgrid/simix.h>
#include <xbt/ex.h>
#include "smx_context_java.h"
#include "jxbt_utilities.h"
#include "xbt/dynar.h"
extern JavaVM *__java_vm;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jmsg, bindings, "MSG for Java(TM)");

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
static void* smx_ctx_java_thread_run(void *data);
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
smx_context_t smx_ctx_java_self(void)
{
	return (smx_context_t)xbt_os_thread_get_extra_data();
}

static smx_context_t
smx_ctx_java_factory_create_context(xbt_main_func_t code, int argc,
                                    char **argv,
                                    void_pfn_smxprocess_t cleanup_func,
                                    void* data)
{
	xbt_ex_t e;
	static int thread_amount=0;
	smx_ctx_java_t context = xbt_new0(s_smx_ctx_java_t, 1);
	thread_amount++;
  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if (code) {
    if (argc == 0) {
      context->jprocess = (jobject) code;
    }
    else {
      context->jprocess = NULL;
    }
    context->super.cleanup_func = cleanup_func;
    context->begin = xbt_os_sem_init(0);
    context->end = xbt_os_sem_init(0);

    context->super.argc = argc;
    context->super.argv = argv;
    context->super.code = code;

    TRY {	  
       context->thread = xbt_os_thread_create(NULL,smx_ctx_java_thread_run,context,NULL);
    } CATCH(e) {
    	RETHROWF("Failed to create context #%d. You may want to switch to Java coroutines to increase your limits (error: %s)."
		 "See the Install section of simgrid-java documentation (in doc/install.html) for more on coroutines.",
    			thread_amount);
    }
  }
  else {
  	context->thread = NULL;
    xbt_os_thread_set_extra_data(context);
  }
  context->super.data = data;
  
  return (smx_context_t) context;
}

static void* smx_ctx_java_thread_run(void *data) {
  smx_ctx_java_t context = (smx_ctx_java_t)data;
  xbt_os_thread_set_extra_data(context);
  //Attach the thread to the JVM
  JNIEnv *env;
  jint error = (*__java_vm)->AttachCurrentThread(__java_vm, (void **) &env, NULL);
  xbt_assert((error == JNI_OK), "The thread could not be attached to the JVM");
  context->jenv = get_current_thread_env();
  //Wait for the first scheduling round to happen.
  xbt_os_sem_acquire(context->begin);
  //Create the "Process" object if needed.
  if (context->super.argc > 0) {
    (*(context->super.code))(context->super.argc, context->super.argv);
  }
  else {
    smx_process_t process = SIMIX_process_self();
    (*env)->SetLongField(env, context->jprocess, jprocess_field_Process_bind,
                         (intptr_t)process);
  }
  xbt_assert((context->jprocess != NULL), "Process not created...");
  //wait for the process to be able to begin
  //TODO: Cache it
	jfieldID jprocess_field_Process_startTime = jxbt_get_sfield(env, "org/simgrid/msg/Process", "startTime", "D");
  jdouble startTime =  (*env)->GetDoubleField(env, context->jprocess, jprocess_field_Process_startTime);
  if (startTime > MSG_get_clock()) {
  	MSG_process_sleep(startTime - MSG_get_clock());
  }
  //Execution of the "run" method.
  jmethodID id = jxbt_get_smethod(env, "org/simgrid/msg/Process", "run", "()V");
  xbt_assert( (id != NULL), "Method not found...");
  (*env)->CallVoidMethod(env, context->jprocess, id);
  smx_ctx_java_stop((smx_context_t)context);

  return NULL;
}

static void smx_ctx_java_free(smx_context_t context)
{
  if (context) {
    smx_ctx_java_t ctx_java = (smx_ctx_java_t) context;
    if (ctx_java->thread) { /* We are not in maestro context */
      xbt_os_thread_join(ctx_java->thread, NULL);
      xbt_os_sem_destroy(ctx_java->begin);
      xbt_os_sem_destroy(ctx_java->end);
    }
  }
  smx_ctx_base_free(context);
}


void smx_ctx_java_stop(smx_context_t context)
{
  smx_ctx_java_t ctx_java = (smx_ctx_java_t)context;
  /* I am the current process and I am dying */
  if (context->iwannadie) {
  	context->iwannadie = 0;
  	JNIEnv *env = get_current_thread_env();
  	jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError", bprintf("Process killed :)"));
  	THROWF(cancel_error, 0, "process cancelled");
  }
  else {
    smx_ctx_base_stop(context);
    /* detach the thread and kills it */
    JNIEnv *env = ctx_java->jenv;
    (*env)->DeleteGlobalRef(env,ctx_java->jprocess);
    jint error = (*__java_vm)->DetachCurrentThread(__java_vm);
    xbt_assert((error == JNI_OK), "The thread couldn't be detached.");
    xbt_os_sem_release(((smx_ctx_java_t)context)->end);
    xbt_os_thread_exit(NULL);
  }
}

static void smx_ctx_java_suspend(smx_context_t context)
{
  smx_ctx_java_t ctx_java = (smx_ctx_java_t) context;
  xbt_os_sem_release(ctx_java->end);
  xbt_os_sem_acquire(ctx_java->begin);
}

// FIXME: inline those functions
static void smx_ctx_java_resume(smx_context_t new_context)
{
  smx_ctx_java_t ctx_java = (smx_ctx_java_t) new_context;
  xbt_os_sem_release(ctx_java->begin);
  xbt_os_sem_acquire(ctx_java->end);
}

static void smx_ctx_java_runall(void)
{
  xbt_dynar_t processes = SIMIX_process_get_runnable();
  smx_process_t process;
  unsigned int cursor;
  xbt_dynar_foreach(processes, cursor, process) {
    smx_ctx_java_resume(SIMIX_process_get_context(process));
  }
}
