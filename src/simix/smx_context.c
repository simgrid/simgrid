/* a fast and simple context switching library                              */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "xbt/log.h"
#include "xbt/swag.h"
#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_context, simix,
                                "Context switching mecanism");

const char *xbt_ctx_factory_to_use = NULL;
typedef void (*SIMIX_ctx_factory_initializer_t)(smx_context_factory_t *);
SIMIX_ctx_factory_initializer_t factory_initializer_to_use = NULL;

/** 
 * This function is called by SIMIX_global_init() to initialize the context module.
 */
void SIMIX_context_mod_init(void)
{
  if (!simix_global->context_factory) {
    /* select context factory to use to create the context(depends of the macro definitions) */
    if (factory_initializer_to_use) {
      (*factory_initializer_to_use)(&(simix_global->context_factory));
    } else {
#ifdef CONTEXT_THREADS /* Use os threads (either pthreads or windows ones) */
      SIMIX_ctx_thread_factory_init(&simix_global->context_factory);
#elif defined(CONTEXT_UCONTEXT) /* use ucontext */
      SIMIX_ctx_sysv_factory_init(&simix_global->context_factory);
#else
#error ERROR [__FILE__, line __LINE__]: no context implementation specified.
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
