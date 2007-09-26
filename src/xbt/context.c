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
#include "xbt/xbt_os_thread.h"
#include "xbt/ex_interface.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ctx, xbt, "Context");

#define VOIRP(expr) DEBUG1("  {" #expr " = %p }", expr)

static xbt_context_t current_context = NULL;
static xbt_context_t init_context = NULL;
static xbt_swag_t context_to_destroy = NULL;
static xbt_swag_t context_living = NULL;
#ifdef CONTEXT_THREADS
static xbt_os_mutex_t creation_mutex;
static xbt_os_cond_t creation_cond;
#endif


/********************/
/* Module init/exit */
/********************/
#ifndef CONTEXT_THREADS
/* callback: context fetching (used only with ucontext, os_thread deal with it
                               for us otherwise) */
static ex_ctx_t *_context_ex_ctx(void) {
  return current_context->exception;
}

/* callback: termination */
static void _context_ex_terminate(xbt_ex_t * e) {
  xbt_ex_display(e);

  abort();
  /* FIXME: there should be a configuration variable to 
     choose to kill everyone or only this one */
}
#endif 

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
  if (!current_context) {
    current_context = init_context = xbt_new0(s_xbt_context_t, 1);
    DEBUG1("Init Context (%p)", init_context);

    init_context->iwannadie = 0; /* useless but makes valgrind happy */
    context_to_destroy =
	xbt_swag_new(xbt_swag_offset(*current_context, hookup));
    context_living =
	xbt_swag_new(xbt_swag_offset(*current_context, hookup));
    xbt_swag_insert(init_context, context_living);
#ifdef CONTEXT_THREADS
    creation_mutex = xbt_os_mutex_init();
    creation_cond = xbt_os_cond_init();
#else
    init_context->exception = xbt_new(ex_ctx_t, 1);
    XBT_CTX_INITIALIZE(init_context->exception);
    __xbt_ex_ctx = _context_ex_ctx;
    __xbt_ex_terminate = _context_ex_terminate;
#endif
  }
}

/** 
 * This function kill all existing context and free all the memory
 * that has been allocated in this module.
 */
void xbt_context_exit(void)
{
  xbt_context_t context = NULL;

  xbt_context_empty_trash();
  while ((context = xbt_swag_extract(context_living))) {
    if (context != init_context) {
      xbt_context_kill(context);
    }
  }
#ifdef CONTEXT_THREADS
  xbt_os_mutex_destroy(creation_mutex);
  xbt_os_cond_destroy(creation_cond);
#else   
  free(init_context->exception);
#endif
  free(init_context);
  init_context = current_context = NULL;

  xbt_context_empty_trash();
  xbt_swag_free(context_to_destroy);
  xbt_swag_free(context_living);

}

/*******************************/
/* Object creation/destruction */
/*******************************/
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
xbt_context_t xbt_context_new(const char *name,xbt_main_func_t code,
			      void_f_pvoid_t startup_func,
			      void *startup_arg,
			      void_f_pvoid_t cleanup_func,
			      void *cleanup_arg, int argc, char *argv[])
{
  xbt_context_t res = NULL;

  res = xbt_new0(s_xbt_context_t, 1);

  res->code = code;
  res->name = xbt_strdup(name);
#ifdef CONTEXT_THREADS
  res->mutex = xbt_os_mutex_init();
  res->cond = xbt_os_cond_init();
#else

  xbt_assert2(getcontext(&(res->uc)) == 0,
	      "Error in context saving: %d (%s)", errno, strerror(errno));
  res->uc.uc_link = NULL;
  /*   res->uc.uc_link = &(current_context->uc); */
  /* WARNING : when this context is over, the current_context (i.e. the 
     father), is awaken... Theorically, the wrapper should prevent using 
     this feature. */
  res->uc.uc_stack.ss_sp = pth_skaddr_makecontext(res->stack, STACK_SIZE);
  res->uc.uc_stack.ss_size =
      pth_sksize_makecontext(res->stack, STACK_SIZE);
   
  res->exception = xbt_new(ex_ctx_t, 1);
  XBT_CTX_INITIALIZE(res->exception);
#endif				/* CONTEXT_THREADS or not */

  res->iwannadie = 0; /* useless but makes valgrind happy */

  res->argc = argc;
  res->argv = argv;
  res->startup_func = startup_func;
  res->startup_arg = startup_arg;
  res->cleanup_func = cleanup_func;
  res->cleanup_arg = cleanup_arg;

  xbt_swag_insert(res, context_living);

  return res;
}

/* Scenario for the end of a context:
 * 
 * CASE 1: death after end of function
 *   __context_wrapper, called by os thread, calls xbt_context_stop after user code stops
 *   xbt_context_stop calls user cleanup_func if any (in context settings),
 *                    add current to trashbin
 *                    yields back to maestro (destroy os thread on need)
 *   From time to time, maestro calls xbt_context_empty_trash, 
 *       which maps xbt_context_free on the content
 *   xbt_context_free frees some more memory, 
 *                    joins os thread
 * 
 * CASE 2: brutal death
 *   xbt_context_kill (from any context)
 *                    set context->wannadie to 1
 *                    yields to the context
 *   the context is awaken in the middle of __yield. 
 *   At the end of it, it checks that wannadie == 1, and call xbt_context_stop
 *   (same than first case afterward)
 */

/* Argument must be stopped first -- runs in maestro context */
static void xbt_context_free(xbt_context_t context) {
  int i;
   
  if (!context)
    return;
  DEBUG1("Freeing %p", context);
  free(context->name);
   
  DEBUG0("Freeing arguments");
  for (i = 0; i < context->argc; i++)
    if (context->argv[i])
      free(context->argv[i]);
  if (context->argv)
    free(context->argv);

#ifdef CONTEXT_THREADS
  DEBUG1("\t joining %p", (void *) context->thread);

  xbt_os_thread_join(context->thread, NULL);

  xbt_os_mutex_destroy(context->mutex);
  xbt_os_cond_destroy(context->cond);

  context->thread = NULL;
  context->mutex = NULL;
  context->cond = NULL;
#else
  if (context->exception)
    free(context->exception);
#endif
   
  free(context);
  return;
}

/************************/
/* Start/stop a context */
/************************/
static void xbt_context_stop(int retvalue);
static void __xbt_context_yield(xbt_context_t context);

static void *__context_wrapper(void *c)
{
  xbt_context_t context = current_context;

#ifdef CONTEXT_THREADS
  context = (xbt_context_t) c;
  context->thread = xbt_os_thread_self();

  DEBUG3("**[ctx:%p;self:%p]** Lock creation_mutex %p ****", context,
	 (void *) xbt_os_thread_self(), creation_mutex);
  xbt_os_mutex_lock(creation_mutex);
  xbt_os_mutex_lock(context->mutex);

  DEBUG4
      ("**[ctx:%p;self:%p]** Releasing the creator (creation_cond %p,%p) ****",
       context, (void *) xbt_os_thread_self(), creation_cond,
       creation_mutex);
  xbt_os_cond_signal(creation_cond);
  xbt_os_mutex_unlock(creation_mutex);

  DEBUG4("**[ctx:%p;self:%p]** Going to Jail on lock %p and cond %p ****",
	 context, (void *) xbt_os_thread_self(), context->mutex,
	 context->cond);
  xbt_os_cond_wait(context->cond, context->mutex);

  DEBUG3("**[ctx:%p;self:%p]** Unlocking individual %p ****",
	 context, (void *) xbt_os_thread_self(), context->mutex);
  xbt_os_mutex_unlock(context->mutex);

#endif

  if (context->startup_func)
    context->startup_func(context->startup_arg);

  DEBUG0("Calling the main function");

  xbt_context_stop((context->code) (context->argc, context->argv));
  return NULL;
}
/** 
 * \param context the context to start
 * 
 * Calling this function prepares \a context to be run. It will 
   however run effectively only when calling #xbt_context_schedule
 */
void xbt_context_start(xbt_context_t context)
{
#ifdef CONTEXT_THREADS
  /* Launch the thread */
   
  DEBUG3("**[ctx:%p;self:%p]** Locking creation_mutex %p ****", context,
	 xbt_os_thread_self(), creation_mutex);
  xbt_os_mutex_lock(creation_mutex);

  DEBUG2("**[ctx:%p;self:%p]** Thread create ****", context,
	 xbt_os_thread_self());
  context->thread = xbt_os_thread_create(context->name,__context_wrapper, context);
  DEBUG3("**[ctx:%p;self:%p]** Thread created : %p ****", context,
	 xbt_os_thread_self(), context->thread);

  DEBUG4
      ("**[ctx:%p;self:%p]** Going to jail on creation_cond/mutex (%p,%p) ****",
       context, xbt_os_thread_self(), creation_cond, creation_mutex);
  xbt_os_cond_wait(creation_cond, creation_mutex);
  DEBUG3("**[ctx:%p;self:%p]** Unlocking creation %p ****", context,
	 xbt_os_thread_self(), creation_mutex);
  xbt_os_mutex_unlock(creation_mutex);
#else
  makecontext(&(context->uc), (void (*)(void)) __context_wrapper, 1,
	      context);
#endif
  return;
}

/* Stops current context: calls user's cleanup function, kills os thread, and yields back to maestro */
static void xbt_context_stop(int retvalue) {
  DEBUG1("--------- %p is exiting ---------", current_context);

  if (current_context->cleanup_func) {
    DEBUG0("Calling cleanup function");
    current_context->cleanup_func(current_context->cleanup_arg);
  }

  DEBUG0("Putting context in the to_destroy set");
  xbt_swag_remove(current_context, context_living);
  xbt_swag_insert(current_context, context_to_destroy);

  DEBUG0("Yielding");

#ifdef CONTEXT_THREADS
  DEBUG2("[%p] **** Locking %p ****", current_context, current_context->mutex);
  xbt_os_mutex_lock(current_context->mutex);
  DEBUG1("[%p] **** Releasing the prisonner ****", current_context);
  xbt_os_cond_signal(current_context->cond);
  DEBUG2("[%p] **** Unlocking individual %p ****", current_context,
	 current_context->mutex);
  xbt_os_mutex_unlock(current_context->mutex);
  DEBUG1("[%p] **** Exiting ****", current_context);
  xbt_os_thread_exit(NULL);	// We should provide return value in case other wants it
#else
  __xbt_context_yield(current_context);
#endif
  THROW_IMPOSSIBLE;
}



/** Garbage collection
 *
 * Should be called some time to time to free the memory allocated for contexts
 * that have finished executing their main functions.
 */
void xbt_context_empty_trash(void)
{
  xbt_context_t context = NULL;
  DEBUG1("Emptying trashbin (%d contexts to free)",
	 xbt_swag_size(context_to_destroy));
  while ((context = xbt_swag_extract(context_to_destroy)))
    xbt_context_free(context);
}

/*********************/
/* context switching */
/*********************/

static void __xbt_context_yield(xbt_context_t context)
{
#ifdef CONTEXT_THREADS
  xbt_context_t self;
#endif  
xbt_assert0(current_context, "You have to call context_init() first.");	
  xbt_assert0(context,"Invalid argument");

  if (current_context == context) {
    DEBUG1("--------- current_context (%p) is yielding back to maestro ---------",
	   context);
  } else {
    DEBUG2("--------- current_context (%p) is yielding to context(%p) ---------",
       current_context, context);
  }

#ifdef CONTEXT_THREADS
  self = current_context;
  DEBUG2("[%p] **** Locking ctx %p ****", self, context);
  xbt_os_mutex_lock(context->mutex);
  DEBUG1("[%p] **** Updating current_context ****", self);
  current_context = context;
  DEBUG1("[%p] **** Releasing the prisonner ****", self);
  xbt_os_cond_signal(context->cond);
  DEBUG3("[%p] **** Going to jail on individual %p/%p ****", self,
	 context->cond, context->mutex);
  xbt_os_cond_wait(context->cond, context->mutex);
  DEBUG2("[%p] **** Unlocking individual %p ****", self, context->mutex);
  xbt_os_mutex_unlock(context->mutex);
  DEBUG1("[%p] **** Updating current_context ****", self);
  current_context = self;

#else				/* use SUSv2 contexts */
  VOIRP(current_context);
  VOIRP(current_context->save);

  VOIRP(context);
  VOIRP(context->save);

  int return_value = 0;

  if (context->save == NULL) {
    DEBUG1("[%p] **** Yielding to somebody else ****", current_context);
    DEBUG2("Saving current_context value (%p) to context(%p)->save",
	     current_context, context);
    context->save = current_context;
    DEBUG1("current_context becomes  context(%p) ", context);
    current_context = context;
    DEBUG1
	  ("Current position memorized (context->save). Jumping to context (%p)",
	   context);
    return_value = swapcontext(&(context->save->uc), &(context->uc));
    xbt_assert0((return_value == 0), "Context swapping failure");
    DEBUG1("I am (%p). Coming back\n", context);
  } else {
    xbt_context_t old_context = context->save;
    DEBUG1("[%p] **** Back ! ****", context);
    DEBUG2("Setting current_context (%p) to context(%p)->save",
	   current_context, context);
    current_context = context->save;
    DEBUG1("Setting context(%p)->save to NULL", context);
    context->save = NULL;
    DEBUG2("Current position memorized (%p). Jumping to context (%p)",
	   context, old_context);
    return_value = swapcontext(&(context->uc), &(old_context->uc));
    xbt_assert0((return_value == 0), "Context swapping failure");
    DEBUG1("I am (%p). Coming back\n", context);
  }
#endif
  if (current_context->iwannadie)
    xbt_context_stop(1);

  return;
}





/** 
 * Calling this function makes the current context yield. The context
 * that scheduled it returns from xbt_context_schedule as if nothing
 * had happened.
 * 
 * Only the processes can call this function, giving back the control
 * to the maestro
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
 * 
 * Only the maestro can call this function to run a given process.
 */
void xbt_context_schedule(xbt_context_t context)
{
  DEBUG1("Scheduling %p", context);
  xbt_assert0((current_context == init_context),
	      "You are not supposed to run this function here!");
  __xbt_context_yield(context);
}


/** 
 * \param context poor victim
 *
 * This function simply kills \a context... scarry isn't it ?
 */
void xbt_context_kill(xbt_context_t context)
{
  DEBUG1("Killing %p", context);

  context->iwannadie = 1;
  DEBUG1("Scheduling %p", context);
  __xbt_context_yield(context);
  DEBUG1("End of Scheduling %p", context);

  return;
}

/* Java cruft I'm gonna kill in the next cleanup round */
void  xbt_context_set_jprocess(xbt_context_t context, void *jp){}
void* xbt_context_get_jprocess(xbt_context_t context){return NULL;}
void  xbt_context_set_jenv(xbt_context_t context,void* je){}
void* xbt_context_get_jenv(xbt_context_t context){return NULL;}

/* @} */
