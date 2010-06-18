/* a fast and simple context switching library                              */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "xbt/log.h"
#include "xbt/swag.h"
#include "private.h"

#ifdef HAVE_LUA
#include <lauxlib.h>
#endif

#ifdef HAVE_RUBY
 void SIMIX_ctx_ruby_factory_init(smx_context_factory_t *factory);
#endif 
 
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_context, simix, "Context switching mecanism");

const char *xbt_ctx_factory_to_use = NULL;

/** 
 * This function is call by SIMIX_global_init() to initialize the context module.
 */

void SIMIX_context_mod_init(void)
{
  if (!simix_global->context_factory) {
  /* select context factory to use to create the context(depends of the macro definitions) */
    if (xbt_ctx_factory_to_use) {
      SIMIX_context_select_factory(xbt_ctx_factory_to_use);
    } else {
#ifdef CONTEXT_THREADS
    /* context switch based os thread */
    SIMIX_ctx_thread_factory_init(&simix_global->context_factory);
#elif !defined(_XBT_WIN32)
    /* context switch based ucontext */
    SIMIX_ctx_sysv_factory_init(&simix_global->context_factory);
#else
    /* context switch is not allowed on Windows */
#error ERROR [__FILE__, line __LINE__]: no context based implementation specified.
#endif
    }
  }
}

/**
 * This function is call by SIMIX_clean() to finalize the context module.
 */
void SIMIX_context_mod_exit(void)
{
  if (simix_global->context_factory) {
    smx_pfn_context_factory_finalize_t finalize_factory;
    
    /* finalize the context factory */
    finalize_factory = simix_global->context_factory->finalize;
    (*finalize_factory) (&simix_global->context_factory);
  }
}

/**
 * This function is used to change the context factory.
 * Warning: it destroy all the existing processes (even for maestro), and it
 * will create a new maestro process using the new context factory.
 */
int SIMIX_context_select_factory(const char *name)
{
  /* if a context factory is already instantiated and it is different from the
     newly selected one, then kill all the processes, exit the context module
     and initialize the new factory.
  */
  

  if (simix_global->context_factory != NULL) {
    if (strcmp(simix_global->context_factory->name, name)){

      SIMIX_process_killall();
      
      /* kill maestro process */
      SIMIX_context_free(simix_global->maestro_process->context);
      free(simix_global->maestro_process);  
      simix_global->maestro_process = NULL;

      SIMIX_context_mod_exit();
    }
    else
      /* the same context factory is requested return directly */
      return 0;
  }
  
  /* init the desired factory */
  SIMIX_context_init_factory_by_name(&simix_global->context_factory, name);

  SIMIX_create_maestro_process ();


  
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
  #if !defined(_XBT_WIN32) && !defined(CONTEXT_THREADS)
      SIMIX_ctx_sysv_factory_init(factory);
  #else
      THROW0(not_found_error, 0, "Factory 'sysv' does not exist: no System V thread support under Windows");
  #endif
    else if (!strcmp(name, "lua"))
#ifdef HAVE_LUA
      SIMIX_ctx_lua_factory_init(factory);
#else

    THROW0(not_found_error, 0, "Factory 'lua' does not exist: Lua support was not compiled in the SimGrid library");
#endif /* HAVE_LUA */

    else if (!strcmp(name,"ruby"))
#ifdef HAVE_RUBY
    SIMIX_ctx_ruby_factory_init(factory);
#else
     THROW0(not_found_error, 0, "Factory 'ruby' does not exist: Ruby support was not compiled in the SimGrid library");
#endif
  else
    THROW1(not_found_error, 0, "Factory '%s' does not exist", name);
}
