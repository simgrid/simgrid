/*
 * $Id$
 *
 * Copyright 2006,2007 Arnaud Legrand,Martin Quinson,Malek Cherier         
 * All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

#include "portable.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/ex_interface.h"
#include "java/jxbt_context.h"
#include "java/jmsg.h"
#include "java/jmsg_process.h"

/* get the right definition of the xbt_ctx with all we need here */
#ifndef JAVA_SIMGRID
#define JAVA_SIMGRID
#endif 

#include "xbt/context_private.h"

pfn_schedule_t __process_schedule= NULL;
pfn_schedule_t __process_unschedule= NULL;


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ctx, xbt, "Context");


static xbt_context_t current_context = NULL;    /* the current context			*/
static xbt_context_t init_context = NULL;       /* the initial context			*/
static xbt_swag_t context_to_destroy = NULL;    /* the list of the contexs to destroy	*/
static xbt_swag_t context_living = NULL;        /* the list of the contexts in use	*/
static xbt_os_mutex_t creation_mutex;              /* For syncronization during process creation */
static xbt_os_cond_t creation_cond;              /* For syncronization during process creation */
static xbt_os_mutex_t master_mutex;		/* For syncronization during process scheduling*/
static xbt_os_cond_t master_cond;		/* For syncronization during process scheduling*/


static void
__xbt_process_schedule(xbt_context_t context);

static void
__xbt_process_unschedule(xbt_context_t context);

static void 
__xbt_context_yield(xbt_context_t context);

static void
__xbt_process_schedule(xbt_context_t context) {
	(*(__process_schedule))(context);
}

static void
__xbt_process_unschedule(xbt_context_t context) {
	(*(__process_unschedule))(context);
}

static void 
__xbt_context_yield(xbt_context_t context) {

  if(context) {

    /* save the current context */
    xbt_context_t self = current_context;

    if(is_main_thread()) {
      /* the main thread has called this function
       * - update the current context
       * - signal the condition of the process to run
       * - wait on its condition
       * - restore thr current contex
       */
      xbt_os_mutex_lock(master_mutex);
      /* update the current context */
      current_context = context;
	 __xbt_process_schedule(context);
      xbt_os_cond_wait(master_cond, master_mutex);
      xbt_os_mutex_unlock(master_mutex);
      /* retore the current context */
      current_context = self;
	  
			
    } else {
	/* a java thread has called this function
	 * - update the current context
	 * - signal the condition of the main thread
	 * - wait on its condition
	 * - restore thr current contex
	 */
	xbt_os_mutex_lock(master_mutex);
	/* update the current context */
	current_context = context;
	xbt_os_cond_signal(master_cond);
	xbt_os_mutex_unlock(master_mutex);
	__xbt_process_unschedule(context);
	/* retore the current context */
	current_context = self;
      }
  }
  
	
  if(current_context->iwannadie)
    __context_exit(current_context, 1);
  
}



static void 
xbt_context_free(xbt_context_t context) {

	if(context) 
	{
		if(context->jprocess) 
		{
			jobject jprocess = context->jprocess;
			context->jprocess = NULL;

			/* if the java process is alive join it */
			if(jprocess_is_alive(jprocess,get_current_thread_env())) 
			{
				jprocess_join(jprocess,get_current_thread_env());
			}
		}

		/* destroy the mutex of the process */
		xbt_os_mutex_destroy(context->mutex);

		/* destroy the condition of the process */
		xbt_os_cond_destroy(context->cond);

		context->mutex = NULL;
		context->cond = NULL;

		if(context->exception) 
			free(context->exception);

		free(context->name);
		free(context);
		context = NULL;
  }
}
/*
 * This function is only called by the main thread.
 */
void 
__context_exit(xbt_context_t context ,int value) {
  /* call the cleanup function of the context */
	
	if(context->cleanup_func)
		context->cleanup_func(context->cleanup_arg);
	
	/* remove the context from the list of the contexts in use. */
	xbt_swag_remove(context, context_living);
	
	/* insert the context in the list of contexts to destroy. */ 
	xbt_swag_insert(context, context_to_destroy);
	
	/*
	* signal the condition of the java process 
	*/

	xbt_os_mutex_lock(master_mutex);

	if (context->jprocess) 
	{
		if (jprocess_is_alive(context->jprocess,get_current_thread_env())) 
		{
			jobject jprocess;
			__xbt_process_schedule(context);
			jprocess = context->jprocess;
			context->jprocess = NULL;
			jprocess_exit(jprocess,get_current_thread_env());
		}
	}

	
}

/*
 * This function is called by a java process (only).
 */
void 
jcontext_exit(xbt_context_t context ,int value,JNIEnv* env) {
  jobject __jprocess = context->jprocess;
  context->jprocess = NULL;
	
  if(context->cleanup_func)
    context->cleanup_func(context->cleanup_arg);

  /* remove the context of the list of living contexts */
  xbt_swag_remove(context, context_living);

  /* insert the context in the list of the contexts to destroy */
  xbt_swag_insert(context, context_to_destroy);
	
  /*
   * signal the condition of the main thread.
   */
  xbt_os_mutex_lock(master_mutex);
  xbt_os_cond_signal(master_cond);
  xbt_os_mutex_unlock(master_mutex);
	
	
  /* the global reference to the java process instance is deleted */
  jprocess_delete_global_ref(__jprocess,get_current_thread_env());
}

/* callback: context fetching */
static ex_ctx_t *
__context_ex_ctx(void) {
  return current_context->exception;
}

/* callback: termination */
static void 
__context_ex_terminate(xbt_ex_t *e) {
  xbt_ex_display(e);

  abort();
}

/**
 * This function initialize the xbt module context which contains 
 * all functions of the context API .
 *
 * @remark This function must be called before use all other function 
 * of this API.
 */
void 
xbt_context_init(void) {
  if(!current_context) {
    /* allocate the current context. */
    current_context = init_context = xbt_new0(s_xbt_context_t,1);
		
    /* context exception */
    init_context->exception = xbt_new(ex_ctx_t,1);
    XBT_CTX_INITIALIZE(init_context->exception);
    __xbt_ex_ctx       = __context_ex_ctx;
    __xbt_ex_terminate = __context_ex_terminate;
    
    /* this list contains the context to destroy */
    context_to_destroy = xbt_swag_new(xbt_swag_offset(*current_context,hookup));
    /* this list contains the context in use */
    context_living = xbt_swag_new(xbt_swag_offset(*current_context,hookup));
    
    /* append the current context in the list of context in use */
    xbt_swag_insert(init_context, context_living);
	   
    /* this mutex is used to synchronize the creation of the java process */
    creation_mutex = xbt_os_mutex_init();
    /* this mutex is used to synchronize the creation of the java process */
    creation_cond = xbt_os_cond_init();

    /* this mutex is used to synchronize the scheduling of the java process */
    master_mutex = xbt_os_mutex_init();
    /* this mutex is used to synchronize the scheduling of the java process */
    master_cond = xbt_os_cond_init();
    
    __process_schedule = jprocess_schedule;
	__process_unschedule = jprocess_unschedule;
  }
}

/** 
 * This function is used as a garbage collection
 * Should be called some time to time to free the memory allocated for contexts
 * that have finished executing their main functions.
 */
void xbt_context_empty_trash(void) {
  xbt_context_t context=NULL;
	
  /* destroy all the context contained in the list of context to destroy */
  while((context=xbt_swag_extract(context_to_destroy)))
    xbt_context_free(context);
}

void 
xbt_context_start(xbt_context_t context)  {
  DEBUG3("xbt_context_start of %p (jproc=%p, jenv=%p)",
	 context, context->jprocess, get_current_thread_env());

  /* the main thread locks the mutex used to create all the process	*/
  xbt_os_mutex_lock(creation_mutex);

  /* the main thread starts the java process				*/
  jprocess_start(context->jprocess,get_current_thread_env());

  /* the main thread waits the startup of the java process		*/
  xbt_os_cond_wait(creation_cond, creation_mutex);
	
  /* the java process is started, the main thread unlocks the mutex
   * used during the creation of the java process
   */
  xbt_os_mutex_unlock(creation_mutex);
}


xbt_context_t 
xbt_context_new(const char *name, xbt_main_func_t code, 
		void_f_pvoid_t startup_func, void *startup_arg,
		void_f_pvoid_t cleanup_func, void *cleanup_arg,
		int argc, char *argv[]) {
  xbt_context_t context = xbt_new0(s_xbt_context_t,1);

  context->code = code;
  context->name = xbt_strdup(name);
	
  context->mutex = xbt_os_mutex_init();
  context->cond = xbt_os_cond_init();
	
	
  context->argc = argc;
  context->argv = argv;
  context->startup_func = startup_func;
  context->startup_arg = startup_arg;
  context->cleanup_func = cleanup_func;
  context->cleanup_arg = cleanup_arg;
  context->exception = xbt_new(ex_ctx_t,1);
  XBT_CTX_INITIALIZE(context->exception);
	
  xbt_swag_insert(context, context_living);
	
  return context;
}


void xbt_context_yield(void) {
  __xbt_context_yield(current_context);
}

/** 
 * \param context the winner
 *
 * Calling this function blocks the current context and schedule \a context.  
 * When \a context will call xbt_context_yield, it will return
 * to this function as if nothing had happened.
 */
void xbt_context_schedule(xbt_context_t context) {
  xbt_assert0((current_context==init_context),"You are not supposed to run this function here!");
  __xbt_context_yield(context);
}

/** 
 * This function kill all existing context and free all the memory
 * that has been allocated in this module.
 */
void xbt_context_exit(void)  {

  xbt_context_t context=NULL;
	
  xbt_context_empty_trash();
	
  while((context=xbt_swag_extract(context_living))) {
    if(context!=init_context) 
      xbt_context_kill(context);
  }

  free(init_context->exception); 
  free(init_context);   

  init_context = current_context = NULL ;

  xbt_context_empty_trash();

  xbt_swag_free(context_to_destroy);
  xbt_swag_free(context_living);
	
  xbt_os_mutex_destroy(creation_mutex);
  xbt_os_cond_destroy(creation_cond);
  xbt_os_mutex_destroy(master_mutex);
  xbt_os_cond_destroy(master_cond);
}

/** 
 * \param context poor victim
 *
 * This function simply kills \a context... scarry isn't it ?
 */
void xbt_context_kill(xbt_context_t context) {

  context->iwannadie=1;
  __xbt_context_yield(context);
}

xbt_os_cond_t
xbt_creation_cond_get(void) {
  return creation_cond;
}

xbt_os_mutex_t
xbt_creation_mutex_get(void) {
  return creation_mutex;
}

void  xbt_context_set_jprocess(xbt_context_t context, void *jp) {
  context->jprocess = jp;
}
void* xbt_context_get_jprocess(xbt_context_t context)           { 
   return context->jprocess;
}

void  xbt_context_set_jmutex(xbt_context_t context,void *jm)    {
   context->mutex = jm;
}
void* xbt_context_get_jmutex(xbt_context_t context)             { 
   return context->mutex; 
}

void  xbt_context_set_jcond(xbt_context_t context,void *jc)     {
   context->cond = jc;
}
void* xbt_context_get_jcond(xbt_context_t context)              { 
   return context->cond;
}

void  xbt_context_set_jenv(xbt_context_t context,void* je)      {
   context->jenv = je;
}
void* xbt_context_get_jenv(xbt_context_t context)               { 
   return get_current_thread_env();
}



/* @} */
