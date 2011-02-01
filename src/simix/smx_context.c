/* a fast and simple context switching library                              */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "xbt/log.h"
#include "xbt/swag.h"
#include "private.h"
#include "simix/context.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_context, simix,
                                "Context switching mecanism");

char* smx_context_factory_name = NULL; /* factory name specified by --cfg=contexts/factory:value */
smx_ctx_factory_initializer_t smx_factory_initializer_to_use = NULL;
int smx_context_stack_size = 128 * 1024;
smx_context_t smx_current_context;
static int smx_parallel_contexts = 1;

/** 
 * This function is called by SIMIX_global_init() to initialize the context module.
 */
void SIMIX_context_mod_init(void)
{
  if (!simix_global->context_factory) {
    /* select the context factory to use to create the contexts */
    if (smx_factory_initializer_to_use) {
      (*smx_factory_initializer_to_use)(&(simix_global->context_factory));
    }
    else { /* use the factory specified by --cfg=contexts/factory:value */
      if (smx_context_factory_name == NULL || !strcmp(smx_context_factory_name, "ucontext")) {
        /* use ucontext */
        SIMIX_ctx_sysv_factory_init(&simix_global->context_factory);
      }
      else if (!strcmp(smx_context_factory_name, "thread")) {
	/* use os threads (either pthreads or windows ones) */
        SIMIX_ctx_thread_factory_init(&simix_global->context_factory);
      }
      else if (!strcmp(smx_context_factory_name, "raw")) {
	/* use raw contexts */
	SIMIX_ctx_raw_factory_init(&simix_global->context_factory);
      }
      else {
        xbt_die("Invalid context factory specified");
      }
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
  xbt_dict_remove((xbt_dict_t) _surf_cfg_set,"contexts/factory");
}

/**
 * \brief Sets the number of parallel threads to use
 * for the user contexts.
 *
 * This function should be called before initializing SIMIX.
 * A value of 1 means no parallelism.
 * If the value is greater than 1, the thread support must be enabled.
 *
 * \param nb_threads the number of threads to use
 */
void SIMIX_context_set_nthreads(int nb_threads) {

  xbt_assert1(nb_threads > 0, "Invalid number of parallel threads: %d", nb_threads);
  smx_parallel_contexts = nb_threads;
}

/**
 * \brief Returns the number of parallel threads used
 * for the user contexts.
 * \return the number of threads (1 means no parallelism)
 */
int SIMIX_context_get_nthreads() {
  return smx_parallel_contexts;
}

/**
 * \brief Returns whether some parallel threads are used
 * for the user contexts.
 * \return 1 if parallelism is used
 */
int SIMIX_context_is_parallel() {
  return smx_parallel_contexts > 1;
}

