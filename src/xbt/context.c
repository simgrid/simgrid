/* 	$Id$	 */

/* a fast and simple context switching library                              */

/* Copyright (c) 2004 Arnaud Legrand.                                       */
/* Copyright (c) 2004, 2005 Martin Quinson.                                 */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "context_private.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "gras_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(context, xbt, "Context");

#define VOIRP(expr) DEBUG1("  {" #expr " = %p }", expr)

static xbt_context_t current_context = NULL;
static xbt_context_t init_context = NULL;
static xbt_swag_t context_to_destroy = NULL;
static xbt_swag_t context_living = NULL;

#ifndef USE_PTHREADS /* USE_PTHREADS and USE_CONTEXT are exclusive */
# ifndef USE_UCONTEXT
/* don't want to play with conditional compilation in automake tonight, sorry.
   include directly the c file from here when needed. */
#  include "context_win32.c" 
#  define USE_WIN_CONTEXT
# endif
#endif

static void __xbt_context_yield(xbt_context_t context)
{
  xbt_assert0(current_context,"You have to call context_init() first.");
  
  DEBUG2("--------- current_context (%p) is yielding to context(%p) ---------",
	 current_context,context);

#ifdef USE_PTHREADS
  if (context) {
    xbt_context_t self = current_context;
    DEBUG0("**** Locking ****");
    pthread_mutex_lock(&(context->mutex));
    DEBUG0("**** Updating current_context ****");
    current_context = context;
    DEBUG0("**** Releasing the prisonner ****");
    pthread_cond_signal(&(context->cond));
    DEBUG0("**** Going to jail ****");
    pthread_cond_wait(&(context->cond), &(context->mutex));
    DEBUG0("**** Unlocking ****");
    pthread_mutex_unlock(&(context->mutex));
    DEBUG0("**** Updating current_context ****");
    current_context = self;
  }
#else
  if(current_context)
    VOIRP(current_context->save);

  VOIRP(context);
  if(context) VOIRP(context->save);
  if (context) {

    int return_value = 0;

    if(context->save==NULL) {
      DEBUG0("**** Yielding to somebody else ****");
      DEBUG2("Saving current_context value (%p) to context(%p)->save",current_context,context);
      context->save = current_context ;
      DEBUG1("current_context becomes  context(%p) ",context);
      current_context = context ;
      DEBUG1("Current position memorized (context->save). Jumping to context (%p)",context);
      return_value = swapcontext (&(context->save->uc), &(context->uc));
      xbt_assert0((return_value==0),"Context swapping failure");
      DEBUG1("I am (%p). Coming back\n",context);
    } else {
      xbt_context_t old_context = context->save ;
      DEBUG0("**** Back ! ****");
      DEBUG2("Setting current_context (%p) to context(%p)->save",current_context,context);
      current_context = context->save ;
      DEBUG1("Setting context(%p)->save to NULL",context);
      context->save = NULL ;
      DEBUG2("Current position memorized (%p). Jumping to context (%p)",context,old_context);
      return_value = swapcontext (&(context->uc), &(old_context->uc));
      xbt_assert0((return_value==0),"Context swapping failure");
      DEBUG1("I am (%p). Coming back\n",context);
    }
  }
#endif
  return;
}

static void xbt_context_destroy(xbt_context_t context)
{
#ifdef USE_PTHREADS
  xbt_free(context->thread);
  pthread_mutex_destroy(&(context->mutex));
  pthread_cond_destroy(&(context->cond));
#endif
  if(context->exception) free(context->exception);
  free(context);
  return;
}

static void __context_exit(xbt_context_t context ,int value)
{
  int i;
  DEBUG0("Freeing arguments");
  for(i=0;i<context->argc; i++) 
    if(context->argv[i]) free(context->argv[i]);
  if(context->argv) free(context->argv);

  if(context->cleanup_func) {
    DEBUG0("Calling cleanup function");
    context->cleanup_func(context->cleanup_arg);
  }

  DEBUG0("Putting context in the to_destroy set");
  xbt_swag_remove(context, context_living);
  xbt_swag_insert(context, context_to_destroy);
  DEBUG0("Context put in the to_destroy set");

  DEBUG0("Yielding");
#ifdef USE_PTHREADS
  DEBUG0("**** Locking ****");
  pthread_mutex_lock(&(context->mutex));
  DEBUG0("**** Updating current_context ****");
  current_context = context;
  DEBUG0("**** Releasing the prisonner ****");
  pthread_cond_signal(&(context->cond));  
  DEBUG0("**** Unlocking ****");
  pthread_mutex_unlock(&(context->mutex));
  DEBUG0("**** Exiting ****");
  pthread_exit(0);
#else
  __xbt_context_yield(context);
#endif
  xbt_assert0(0,"You can't be here!");
}

static void *__context_wrapper(void *c)
{
  xbt_context_t context = c;

#ifdef USE_PTHREADS
  DEBUG2("**[%p:%p]** Lock ****",context,(void*)pthread_self());
  pthread_mutex_lock(&(context->mutex));
  DEBUG2("**[%p:%p]** Releasing the prisonner ****",context,(void*)pthread_self());
  pthread_cond_signal(&(context->cond));
  DEBUG2("**[%p:%p]** Going to Jail ****",context,(void*)pthread_self());
  pthread_cond_wait(&(context->cond), &(context->mutex));
  DEBUG2("**[%p:%p]** Unlocking ****",context,(void*)pthread_self());
  pthread_mutex_unlock(&(context->mutex));
#endif

  if(context->startup_func)
    context->startup_func(context->startup_arg);

  DEBUG0("Calling the main function");

  __context_exit(context, (context->code) (context->argc,context->argv));
  return NULL;
}

/* callback: context fetching */
static ex_ctx_t *__context_ex_ctx(void)
{
  return current_context->exception;
}

/* callback: termination */
static void __context_ex_terminate(xbt_ex_t *e) {
  xbt_ex_display(e);

  __context_exit(current_context, e->value);
}

/** \name Functions 
 *  \ingroup XBT_context
 */
/* @{ */
/** Context module initialization
 *
 * \warning It has to be called before using any other function of this module.
 */
void xbt_context_init(void)
{
  if(!current_context) {
    current_context = init_context = xbt_new0(s_xbt_context_t,1);
    init_context->exception = xbt_new(ex_ctx_t,1);
    XBT_CTX_INITIALIZE(init_context->exception);
    __xbt_ex_ctx       = __context_ex_ctx;
    __xbt_ex_terminate = __context_ex_terminate;
    context_to_destroy = xbt_swag_new(xbt_swag_offset(*current_context,hookup));
    context_living = xbt_swag_new(xbt_swag_offset(*current_context,hookup));
    xbt_swag_insert(init_context, context_living);
  }
}

/** Garbage collection
 *
 * Should be called some time to time to free the memory allocated for contexts
 * that have finished executing their main functions.
 */
void xbt_context_empty_trash(void)
{
  xbt_context_t context=NULL;

  while((context=xbt_swag_extract(context_to_destroy)))
    xbt_context_destroy(context);
}

/** 
 * \param context the context to start
 * 
 * Calling this function prepares \a context to be run. It will 
   however run effectively only when calling #xbt_context_schedule
 */
void xbt_context_start(xbt_context_t context) 
{
#ifdef USE_PTHREADS
  /* Launch the thread */
  DEBUG1("**[%p]** Locking ****",context);
  pthread_mutex_lock(&(context->mutex));
  DEBUG1("**[%p]** Pthread create ****",context);
  xbt_assert0(!pthread_create(context->thread, NULL, __context_wrapper, context),
	      "Unable to create a thread.");
  DEBUG2("**[%p]** Pthread created : %p ****",context,(void*)(*(context->thread)));
  DEBUG1("**[%p]** Going to jail ****",context);
  pthread_cond_wait(&(context->cond), &(context->mutex));
  DEBUG1("**[%p]** Unlocking ****",context);
  pthread_mutex_unlock(&(context->mutex));  
#else
  makecontext (&(context->uc), (void (*) (void)) __context_wrapper,
	       1, context);
#endif
  return;
}

/** 
 * \param code a main function
 * \param startup_func a function to call when running the context for
 *      the first time and just before the main function \a code
 * \param startup_arg the argument passed to the previous function (\a startup_func)
 * \param cleanup_func a function to call when running the context, just after 
        the termination of the main function \a code
 * \param cleanup_arg the argument passed to the previous function (\a cleanup_func)
 * \param argc first argument of function \a code
 * \param argv seconde argument of function \a code
 */
xbt_context_t xbt_context_new(xbt_context_function_t code, 
			      void_f_pvoid_t startup_func, void *startup_arg,
			      void_f_pvoid_t cleanup_func, void *cleanup_arg,
			      int argc, char *argv[])
{
  xbt_context_t res = NULL;

  res = xbt_new0(s_xbt_context_t,1);

  res->code = code;
#ifdef USE_PTHREADS
  res->thread = xbt_new0(pthread_t,1);
  xbt_assert0(!pthread_mutex_init(&(res->mutex), NULL), "Mutex initialization error");
  xbt_assert0(!pthread_cond_init(&(res->cond), NULL), "Condition initialization error");
#else 
  /* FIXME: strerror is not thread safe */
  xbt_assert2(getcontext(&(res->uc))==0,"Error in context saving: %d (%s)", errno, strerror(errno));
  res->uc.uc_link = NULL;
  /*   res->uc.uc_link = &(current_context->uc); */
  /* WARNING : when this context is over, the current_context (i.e. the 
     father), is awaken... Theorically, the wrapper should prevent using 
     this feature. */
# ifndef USE_WIN_CONTEXT    
  res->uc.uc_stack.ss_sp = pth_skaddr_makecontext(res->stack,STACK_SIZE);
  res->uc.uc_stack.ss_size = pth_sksize_makecontext(res->stack,STACK_SIZE);
# endif /* USE_WIN_CONTEXT */
#endif /* USE_PTHREADS or not */
  res->argc = argc;
  res->argv = argv;
  res->startup_func = startup_func;
  res->startup_arg = startup_arg;
  res->cleanup_func = cleanup_func;
  res->cleanup_arg = cleanup_arg;
  res->exception = xbt_new(ex_ctx_t,1);
  XBT_CTX_INITIALIZE(res->exception);

  xbt_swag_insert(res, context_living);

  return res;
}

/** 
 * Calling this function makes the current context yield. The context
 * that scheduled it returns from xbt_context_schedule as if nothing
 * had happened.
 */
void xbt_context_yield(void)
{
  __xbt_context_yield(current_context);
}

/** 
 * \param context the winner
 *
 * Calling this function blocks the current context and schedule \a context.  
 * When \a context will call xbt_context_yield, it will return
 * to this function as if nothing had happened.
 */
void xbt_context_schedule(xbt_context_t context)
{
  DEBUG1("Scheduling %p",context);
  xbt_assert0((current_context==init_context),
	      "You are not supposed to run this function here!");
  __xbt_context_yield(context);
}

/** 
 * This function kill all existing context and free all the memory
 * that has been allocated in this module.
 */
void xbt_context_exit(void) {
  xbt_context_t context=NULL;

  xbt_context_empty_trash();
  xbt_swag_free(context_to_destroy);

  while((context=xbt_swag_extract(context_living)))
    xbt_context_free(context);

  xbt_swag_free(context_living);

  init_context = current_context = NULL ;
}

/** 
 * \param context poor victim
 *
 * This function simply kills \a context... scarry isn't it ?
 */
void xbt_context_free(xbt_context_t context)
{
  int i ;

  xbt_swag_remove(context, context_living);  
  for(i=0;i<context->argc; i++) 
    if(context->argv[i]) free(context->argv[i]);
  if(context->argv) free(context->argv);
  
  if(context->cleanup_func)
    context->cleanup_func(context->cleanup_arg);
  xbt_context_destroy(context);

  return;
}
/* @} */
