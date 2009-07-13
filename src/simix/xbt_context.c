/* a fast and simple context switching library                              */

/* Copyright (c) 2004-2008 the SimGrid team.                                */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "xbt/log.h"
#include "xbt/swag.h"
#include "xbt_context_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_context, xbt,
                                "Context switching mecanism");

/* the context associated with the current process 				*/
xbt_context_t current_context = NULL;

/* the context associated with the maestro						*/
xbt_context_t maestro_context = NULL;

/* this list contains the contexts to destroy					*/
xbt_swag_t context_to_destroy = NULL;

/* this list contains the contexts in use						*/
xbt_swag_t context_living = NULL;

/* the context factory used to create the appropriate context
 * each context implementation define its own context factory
 * a context factory is responsable of the creation of the context
 * associated with the maestro and of all the context based on
 * the selected implementation.
 *
 * for example, the context switch based on java thread use the
 * java implementation of the context and the java factory build
 * the context depending of this implementation.
 */
static xbt_context_factory_t context_factory = NULL;

/**
 * This function is call by the xbt_init() function to initialize the context module.
 */
void xbt_context_mod_init(void)
{
  if (!context_factory) {
    /* select context factory to use to create the context(depends of the macro definitions) */

#ifdef CONTEXT_THREADS
    /* context switch based os thread */
    xbt_ctx_thread_factory_init(&context_factory);
#elif !defined(WIN32)
    /* context switch based ucontext */
    xbt_ctx_sysv_factory_init(&context_factory);
#else
    /* context switch is not allowed on Windows */
#error ERROR [__FILE__, line __LINE__]: no context based implementation specified.
#endif

    /* maestro context specialisation (this create the maestro with the good implementation */
    (*(context_factory->create_maestro_context)) (&maestro_context);

    /* the current context is the context of the maestro */
    current_context = maestro_context;

    /* the current context doesn't want to die */
    current_context->iwannadie = 0;

    /* intantiation of the lists containing the contexts to destroy and the contexts in use */
    context_to_destroy =
      xbt_swag_new(xbt_swag_offset(*current_context, hookup));
    context_living = xbt_swag_new(xbt_swag_offset(*current_context, hookup));

    /* insert the current context in the list of the contexts in use */
    xbt_swag_insert(current_context, context_living);

  }
}

/**
 * This function is call by the xbt_exit() function to finalize the context module.
 */
void xbt_context_mod_exit(void)
{
  if (context_factory) {
    xbt_context_t context = NULL;
    xbt_pfn_context_factory_finalize_t finalize_factory;

    /* finalize the context factory */
    finalize_factory = context_factory->finalize;

    (*finalize_factory) (&context_factory);

    /* destroy all contexts in the list of contexts to destroy */
    xbt_context_empty_trash();

    /* remove the context of the scheduler from the list of the contexts in use */
    xbt_swag_remove(maestro_context, context_living);

    /*
     * kill all the contexts in use :
     * the killed contexts are added in the list of the contexts to destroy
     */
    while ((context = xbt_swag_extract(context_living)))
      (*(context->kill)) (context);


    /* destroy all contexts in the list of contexts to destroy */
    xbt_context_empty_trash();

    free(maestro_context);
    maestro_context = current_context = NULL;

    /* destroy the lists */
    xbt_swag_free(context_to_destroy);
    xbt_swag_free(context_living);
  }
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
xbt_context_t
xbt_context_new(const char *name,
                xbt_main_func_t code,
                void_f_pvoid_t startup_func,
                void *startup_arg,
                void_f_pvoid_t cleanup_func,
                void *cleanup_arg, int argc, char *argv[]
  )
{
  /* use the appropriate context factory to create the appropriate context */
  xbt_context_t context =
    (*(context_factory->create_context)) (name, code, startup_func,
                                          startup_arg, cleanup_func,
                                          cleanup_arg, argc, argv);

  /* add the context in the list of the contexts in use */
  xbt_swag_insert(context, context_living);

  return context;
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
void xbt_context_free(xbt_context_t context)
{
  (*(context->free)) (context);
}


void xbt_context_kill(xbt_context_t context)
{
  (*(context->kill)) (context);
}

/**
 * \param context the context to start
 *
 * Calling this function prepares \a context to be run. It will
   however run effectively only when calling #xbt_context_schedule
 */
void xbt_context_start(xbt_context_t context)
{
  (*(context->start)) (context);
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
  (*(current_context->yield)) ();
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
  (*(context->schedule)) (context);
}

void xbt_context_stop(int exit_code)
{

  (*(current_context->stop)) (exit_code);
}

int xbt_context_select_factory(const char *name)
{
  /* if a factory is already instantiated (xbt_context_mod_init() was called) */
  if (NULL != context_factory) {
    /* if the desired factory is different of the current factory, call xbt_context_mod_exit() */
    if (strcmp(context_factory->name, name))
      xbt_context_mod_exit();
    else
      /* the same context factory is requested return directly */
      return 0;
  }

  /* get the desired factory */
  xbt_context_init_factory_by_name(&context_factory, name);

  /* maestro context specialisation */
  (*(context_factory->create_maestro_context)) (&maestro_context);

  /* the current context is the context of the maestro */
  current_context = maestro_context;

  /* the current context doesn't want to die */
  current_context->iwannadie = 0;

  /* intantiation of the lists containing the contexts to destroy and the contexts in use */
  context_to_destroy =
    xbt_swag_new(xbt_swag_offset(*current_context, hookup));
  context_living = xbt_swag_new(xbt_swag_offset(*current_context, hookup));

  /* insert the current context in the list of the contexts in use */
  xbt_swag_insert(current_context, context_living);

  return 0;
}

void
xbt_context_init_factory_by_name(xbt_context_factory_t * factory,
                                 const char *name)
{
  if (!strcmp(name, "java"))
#ifdef HAVE_JAVA     
    xbt_ctx_java_factory_init(factory);
#else
    THROW0(not_found_error, 0, "Factory 'Java' does not exist: Java support was not compiled in the SimGrid library");
#endif /* HAVE_JAVA */
   
  else if (!strcmp(name, "thread"))
#ifdef CONTEXT_THREADS
    xbt_ctx_thread_factory_init(factory);
#else
    THROW0(not_found_error, 0, "Factory 'thread' does not exist: thread support was not compiled in the SimGrid library");
#endif /* CONTEXT_THREADS */
   
  else if (!strcmp(name, "sysv"))
#ifndef WIN32
    xbt_ctx_sysv_factory_init(factory);
#else
    THROW0(not_found_error, 0, "Factory 'sysv' does not exist: no System V thread support under Windows");
#endif
   
  else
    THROW1(not_found_error, 0, "Factory '%s' does not exist", name);

}

/** Garbage collection
 *
 * Should be called some time to time to free the memory allocated for contexts
 * that have finished executing their main functions.
 */
void xbt_context_empty_trash(void)
{
  xbt_context_t context = NULL;

  while ((context = xbt_swag_extract(context_to_destroy)))
    (*(context->free)) (context);
}
