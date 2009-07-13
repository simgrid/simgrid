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

/**
 * This function is call by SIMIX_global_init() to initialize the context module.
 */
void SIMIX_context_mod_init(void)
{
  if (!simix_global->context_factory) {
    /* select context factory to use to create the context(depends of the macro definitions) */

#ifdef CONTEXT_THREADS
    /* context switch based os thread */
    SIMIX_ctx_thread_factory_init(&simix_global->context_factory);
#elif !defined(WIN32)
    /* context switch based ucontext */
    SIMIX_ctx_sysv_factory_init(&simix_global->context_factory);
#else
    /* context switch is not allowed on Windows */
#error ERROR [__FILE__, line __LINE__]: no context based implementation specified.
#endif
  }
}

/**
 * This function is call by SIMIX_clean() to finalize the context module.
 */
void SIMIX_context_mod_exit(void)
{
  if (simix_global->context_factory) {
    smx_process_t process = NULL;
    smx_pfn_context_factory_finalize_t finalize_factory;

    /* kill all the processes (except maestro)
     * the killed processes are added in the list of the processes to destroy */
    
    while ((process = xbt_swag_extract(simix_global->process_list)))
      (*(simix_global->context_factory->kill)) (process);

    /* destroy all processes in the list of process to destroy */
    SIMIX_context_empty_trash();

    /* finalize the context factory */
    finalize_factory = simix_global->context_factory->finalize;
    (*finalize_factory) (&simix_global->context_factory);
  }
}

/*******************************/
/* Object creation/destruction */
/*******************************/
/**
 * \param smx_process the simix process that contains this context
 * \param code a main function
 */
int SIMIX_context_new(smx_process_t *process, xbt_main_func_t code)
{
  /* use the appropriate context factory to create the appropriate context */
    return (*(simix_global->context_factory->create_context)) (process, code);
}


int SIMIX_context_create_maestro(smx_process_t *process)
{
  return (*(simix_global->context_factory->create_maestro_context)) (process);
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
void SIMIX_context_free(smx_process_t process)
{
  (*(simix_global->context_factory->free)) (process);
}

void SIMIX_context_kill(smx_process_t process)
{
  (*(simix_global->context_factory->kill)) (process);
}

/**
 * \param context the context to start
 *
 * Calling this function prepares \a process to be run. It will
   however run effectively only when calling #SIMIX_context_schedule
 */
void SIMIX_context_start(smx_process_t process)
{
  (*(simix_global->context_factory->start)) (process);
}

/**
 * Calling this function makes the current process yield. The process
 * that scheduled it returns from SIMIX_context_schedule as if nothing
 * had happened.
 *
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
void SIMIX_context_yield(void)
{
  (*(simix_global->context_factory->yield)) ();
}

/**
 * \param process to be scheduled
 *
 * Calling this function blocks the current process and schedule \a process.
 * When \a process would call SIMIX_context_yield, it will return
 * to this function as if nothing had happened.
 *
 * Only the maestro can call this function to run a given process.
 */
void SIMIX_context_schedule(smx_process_t process)
{
  (*(simix_global->context_factory->schedule)) (process);
}

void SIMIX_context_stop(int exit_code)
{
  (*(simix_global->context_factory->stop)) (exit_code);
}

int SIMIX_context_select_factory(const char *name)
{
  /* if a factory is already instantiated (SIMIX_context_mod_init() was called) */
  if (simix_global->context_factory != NULL) {
    /* if the desired factory is different of the current factory, call SIMIX_context_mod_exit() */
    if (strcmp(simix_global->context_factory->name, name))
      SIMIX_context_mod_exit();
    else
      /* the same context factory is requested return directly */
      return 0;
  }

  /* get the desired factory */
  SIMIX_context_init_factory_by_name(&simix_global->context_factory, name);

  /* maestro process specialisation */
  (*(simix_global->context_factory->create_maestro_context)) (&simix_global->maestro_process);

  /* the current process is the process of the maestro */
  simix_global->current_process = simix_global->maestro_process;

  /* the current context doesn't want to die */
  simix_global->current_process->iwannadie = 0;

  /* insert the current context in the list of the contexts in use */
  xbt_swag_insert(simix_global->current_process, simix_global->process_list);

  return 0;
}

void
SIMIX_context_init_factory_by_name(smx_context_factory_t * factory,
                                   const char *name)
{
  if (!strcmp(name, "java"))
#ifdef HAVE_JAVA     
    SIMIX_ctx_java_factory_init(factory);
#else
    THROW0(not_found_error, 0, "Factory 'Java' does not exist: Java support was not compiled in the SimGrid library");
#endif /* HAVE_JAVA */
   
  else if (!strcmp(name, "thread"))
#ifdef CONTEXT_THREADS
    SIMIX_ctx_thread_factory_init(factory);
#else
    THROW0(not_found_error, 0, "Factory 'thread' does not exist: thread support was not compiled in the SimGrid library");
#endif /* CONTEXT_THREADS */
   
  else if (!strcmp(name, "sysv"))
#if !defined(WIN32) && !defined(CONTEXT_THREADS)
    SIMIX_ctx_sysv_factory_init(factory);
#else
    THROW0(not_found_error, 0, "Factory 'sysv' does not exist: no System V thread support under Windows");
#endif   
  else
    THROW1(not_found_error, 0, "Factory '%s' does not exist", name);
}

/** Garbage collection
 *
 * Should be called some time to time to free the memory allocated for processes
 * that have finished (or killed).
 */
void SIMIX_context_empty_trash(void)
{ 
  smx_process_t process = NULL;
  int i;  

  while ((process = xbt_swag_extract(simix_global->process_to_destroy))){

    free(process->name);
    process->name = NULL;
  
    if (process->argv) {
      for (i = 0; i < process->argc; i++)
        if (process->argv[i])
          free(process->argv[i]);

      free(process->argv);
    }
  
    free(process);
  }
}