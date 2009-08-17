/* a fast and simple context switching library                              */

/* Copyright (c) 2004-2008 the SimGrid team.                                */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "xbt/log.h"
#include "xbt/swag.h"
#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smx_context, simix, "Context switching mecanism");


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
    smx_pfn_context_factory_finalize_t finalize_factory;

    /* if there are living processes then kill them (except maestro) */
    if(simix_global->process_list != NULL)
      SIMIX_process_killall();
    
    /* finalize the context factory */
    finalize_factory = simix_global->context_factory->finalize;
    (*finalize_factory) (&simix_global->context_factory);
  }
}

/**
 * This function is used to change the context factory.
 * Warning: it destroy all the existing contexts
 */
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

/**
 * Initializes a context factory given by its name
 */
void SIMIX_context_init_factory_by_name(smx_context_factory_t * factory,
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
