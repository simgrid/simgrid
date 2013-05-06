/* context_cojava - implementation of context switching for java coroutines */

/* Copyright 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <xbt/function_types.h>
#include <simgrid/simix.h>
#include <xbt/ex.h>
#include "smx_context_cojava.h"
#include "jxbt_utilities.h"
#include "xbt/dynar.h"


//Coroutine methodID/class cache.
jclass coclass;
jmethodID coroutine_init;
jmethodID coroutine_yield;
jmethodID coroutine_yieldTo;
jmethodID coroutine_stop;
//Maestro java coroutine
jobject cojava_maestro_coroutine;

JNIEnv *global_env;

static smx_context_t my_current_context = NULL;
static smx_context_t maestro_context = NULL;


xbt_dynar_t cojava_processes;
static unsigned long int cojava_process_index = 0;

extern JavaVM *__java_vm;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);


static smx_context_t
smx_ctx_cojava_factory_create_context(xbt_main_func_t code, int argc,
                                    char **argv,
                                    void_pfn_smxprocess_t cleanup_func,
                                    void *data);

static void smx_ctx_cojava_free(smx_context_t context);
static void smx_ctx_cojava_suspend(smx_context_t context);
static void smx_ctx_cojava_resume(smx_context_t new_context);
static void smx_ctx_cojava_runall(void);
static void* smx_ctx_cojava_run(void *data);
static void smx_ctx_cojava_create_coroutine(smx_ctx_cojava_t context);
void SIMIX_ctx_cojava_factory_init(smx_context_factory_t * factory)
{
  /* instantiate the context factory */
  smx_ctx_base_factory_init(factory);

  (*factory)->create_context = smx_ctx_cojava_factory_create_context;
  /* Leave default behavior of (*factory)->finalize */
  (*factory)->free = smx_ctx_cojava_free;
  (*factory)->stop = smx_ctx_cojava_stop;
  (*factory)->suspend = smx_ctx_cojava_suspend;
  (*factory)->runall = smx_ctx_cojava_runall;
  (*factory)->name = "ctx_cojava_factory";
  //(*factory)->finalize = smx_ctx_base_factory_finalize;
  (*factory)->self = smx_ctx_cojava_self;
  (*factory)->get_data = smx_ctx_base_get_data;

  global_env = get_current_thread_env();

  coclass = (*global_env)->FindClass(global_env, "java/dyn/Coroutine");
  xbt_assert((coclass != NULL), "Can't find java.dyn.Coroutine class.");
  //Cache the method id we are going to use
  coroutine_init = (*global_env)->GetMethodID(global_env, coclass, "<init>", "(Ljava/lang/Runnable;)V");
  xbt_assert((coroutine_init != NULL), "Can't find <init>");
  coroutine_stop = (*global_env)->GetMethodID(global_env, coclass, "stop", "()V");
  xbt_assert((coroutine_stop != NULL), "Method not found...");
  coroutine_yield = (*global_env)->GetStaticMethodID(global_env, coclass, "yield", "()V");
  xbt_assert((coroutine_yield != NULL), "Method yield not found.");
  coroutine_yieldTo = (*global_env)->GetStaticMethodID(global_env, coclass, "yieldTo", "(Ljava/dyn/Coroutine;)V");
  xbt_assert((coroutine_yieldTo != NULL), "Method yieldTo not found.");

  jclass class_thread = (*global_env)->FindClass(global_env, "java/lang/Thread");
  xbt_assert((class_thread != NULL), "Can't find java.lang.Thread class");
  
  jclass class_coroutine_support = (*global_env)->FindClass(global_env, "java/dyn/CoroutineSupport");
  xbt_assert((class_coroutine_support != NULL), "Can't find java.dyn.CoroutineSupport class");
  jmethodID thread_get_current = (*global_env)->GetStaticMethodID(global_env, class_thread, "currentThread", "()Ljava/lang/Thread;");
  xbt_assert((thread_get_current != NULL), "Can't find Thread.currentThread() method.");

  /**
   * Retrieve maetro coroutine object
   */
  jobject jthread;
  jthread = (*global_env)->CallStaticObjectMethod(global_env, class_thread, thread_get_current);
  xbt_assert((jthread != NULL), "Can't find current thread.");

  jmethodID thread_get_coroutine_support = (*global_env)->GetMethodID(global_env, class_thread, "getCoroutineSupport", "()Ljava/dyn/CoroutineSupport;");
  xbt_assert((thread_get_coroutine_support != NULL), "Can't find Thread.getCoroutineSupport method");

  jobject jcoroutine_support;
  jcoroutine_support = (*global_env)->CallObjectMethod(global_env, jthread, thread_get_coroutine_support);
  xbt_assert((jcoroutine_support != NULL), "Can't find coroutine support object");
  //FIXME ? Be careful, might change in the implementation (we are relying on private fields, so...).
  jfieldID coroutine_support_thread_coroutine = (*global_env)->GetFieldID(global_env, class_coroutine_support, "threadCoroutine", "Ljava/dyn/Coroutine;");
  xbt_assert((coroutine_support_thread_coroutine != NULL), "Can't find threadCoroutine field");
  cojava_maestro_coroutine = (jobject)(*global_env)->GetObjectField(global_env, jcoroutine_support, coroutine_support_thread_coroutine);
  xbt_assert((cojava_maestro_coroutine != NULL), "Can't find the thread coroutine.");
  cojava_maestro_coroutine = (*global_env)->NewGlobalRef(global_env, cojava_maestro_coroutine);
  xbt_assert((cojava_maestro_coroutine != NULL), "Can't get a global reference to the thread coroutine.");
}
smx_context_t smx_ctx_cojava_self(void)
{
	return my_current_context;
}

static smx_context_t
smx_ctx_cojava_factory_create_context(xbt_main_func_t code, int argc,
                                    char **argv,
                                    void_pfn_smxprocess_t cleanup_func,
                                    void* data)
{
	smx_ctx_cojava_t context = xbt_new0(s_smx_ctx_cojava_t, 1);
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

    context->super.argc = argc;
    context->super.argv = argv;
    context->super.code = code;

    smx_ctx_cojava_run(context);
  }
  else {
  	context->jcoroutine = NULL;
  	my_current_context = (smx_context_t)context;
  	maestro_context = (smx_context_t)context;
  }
  context->bound = 0;
  context->super.data = data;
  return (smx_context_t) context;
}

static void* smx_ctx_cojava_run(void *data) {
  smx_ctx_cojava_t context = (smx_ctx_cojava_t)data;
  my_current_context = (smx_context_t)context;
  //Create the "Process" object if needed.
  if (context->super.argc <= 0) {
    smx_ctx_cojava_create_coroutine(context);
  }
  my_current_context = maestro_context;
  return NULL;
}
static void smx_ctx_cojava_free(smx_context_t context)
{
  if (context) {
    smx_ctx_cojava_t ctx_java = (smx_ctx_cojava_t) context;
    if (ctx_java->jcoroutine) { /* We are not in maestro context */
      JNIEnv *env = get_current_thread_env();
      (*env)->DeleteGlobalRef(env, ctx_java->jcoroutine);
      (*env)->DeleteGlobalRef(env, ctx_java->jprocess);
    }
  }
  smx_ctx_base_free(context);
}


void smx_ctx_cojava_stop(smx_context_t context)
{
  /*
   * The java stack needs to be empty, otherwise weird stuff
   * will happen
   */
  if (context->iwannadie) {
    context->iwannadie = 0;
    JNIEnv *env = get_current_thread_env();
    jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError", xbt_strdup("Process killed :)"));
    THROWF(cancel_error, 0, "process cancelled");
  }
  else {
    smx_ctx_base_stop(context);
    smx_ctx_cojava_suspend(context);
  }
}

static void smx_ctx_cojava_suspend(smx_context_t context)
{
  smx_context_t previous_context = context;
  unsigned long int i = cojava_process_index++;
  jobject next_coroutine;

  if (i < xbt_dynar_length(cojava_processes)) {
    smx_context_t next_context = SIMIX_process_get_context(xbt_dynar_get_as(
    cojava_processes,i, smx_process_t));
    my_current_context = next_context;
    XBT_DEBUG("Switching to %p",my_current_context);
    smx_ctx_cojava_t java_context = (smx_ctx_cojava_t)(next_context);
    if (!java_context->jprocess) {
      java_context->super.code(java_context->super.argc, java_context->super.argv);
      smx_ctx_cojava_create_coroutine(java_context);
    }
    else if (!java_context->bound) {
      java_context->bound = 1;
      smx_process_t process = SIMIX_process_self();
      (*global_env)->SetLongField(global_env, java_context->jprocess,
                                  jprocess_field_Process_bind,
                                  (intptr_t)process);
    }

    next_coroutine = java_context->jcoroutine;
  }
  else {
    //Give maestro the control back.
    next_coroutine = cojava_maestro_coroutine;
    my_current_context = maestro_context;
  }
  (*global_env)->CallStaticVoidMethod(global_env, coclass, coroutine_yieldTo, next_coroutine);
  my_current_context = previous_context;
}

static void smx_ctx_cojava_resume(smx_context_t new_context) {
  my_current_context = new_context;
  smx_ctx_cojava_t java_context = (smx_ctx_cojava_t)(new_context);

  if (!java_context->jprocess) {
    java_context->super.code(java_context->super.argc, java_context->super.argv);
    smx_ctx_cojava_create_coroutine(java_context);
    java_context->bound = 1;
  }
  else if (!java_context->bound) {
    java_context->bound = 1;
    smx_process_t process = SIMIX_process_self();
    (*global_env)->SetLongField(global_env, java_context->jprocess,
                                jprocess_field_Process_bind, (intptr_t)process);
  }
  (*global_env)->CallStaticVoidMethod(global_env, coclass, coroutine_yieldTo, java_context->jcoroutine);
}

static void smx_ctx_cojava_runall(void)
{
  cojava_processes = SIMIX_process_get_runnable();
  smx_process_t process;
  if (!xbt_dynar_is_empty(cojava_processes)) {
    process = xbt_dynar_get_as(cojava_processes, 0, smx_process_t);
    cojava_process_index = 1;
    /* Execute the first process */
    smx_ctx_cojava_resume(SIMIX_process_get_context(process));
  }
}

static void smx_ctx_cojava_create_coroutine(smx_ctx_cojava_t context) {
  JNIEnv *env = get_current_thread_env();
  jclass coclass = (*env)->FindClass(env, "java/dyn/Coroutine");
  xbt_assert((coclass != NULL), "Can't find coroutine class ! :(");
  jobject jcoroutine = (*env)->NewObject(env, coclass, coroutine_init, context->jprocess);
  if (jcoroutine == NULL) {
     FILE *conf= fopen("/proc/sys/vm/max_map_count","r");
     if (conf) {
	int limit=-1;
        if(fscanf(conf,"%d",&limit) != 1)
           xbt_die("Error while creating a new coroutine. Parse error.");
	fclose(conf);
	if (limit!=-1 && SIMIX_process_count() > (limit - 100) /2)
	   xbt_die("Error while creating a new coroutine. "
		   "This seem due to the the vm.max_map_count system limit that is only equal to %d while we already have %d coroutines. "
		   "Please check the install documentation to see how to increase this limit", limit, SIMIX_process_count());
	if (limit == -1)
	  xbt_die("Error while creating a new coroutine. "
		  "This seems to be a non-linux system, disabling the automatic verification that the system limit on the amount of memory maps is high enough.");
	xbt_die("Error while creating a new coroutine. ");
     }
     
  }
   
  jcoroutine = (*env)->NewGlobalRef(env, jcoroutine);
  context->jcoroutine = jcoroutine;
}
