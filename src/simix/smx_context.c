/* a fast and simple context switching library                              */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "xbt/log.h"
#include "xbt/swag.h"
#include "xbt/xbt_os_thread.h"
#include "src/simix/private.h"
#include "simix/context.h"
#include "gras_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_context, simix,
                                "Context switching mecanism");

char* smx_context_factory_name = NULL; /* factory name specified by --cfg=contexts/factory:value */
smx_ctx_factory_initializer_t smx_factory_initializer_to_use = NULL;
int smx_context_stack_size = 128 * 1024;

#ifdef HAVE_THREAD_LOCAL_STORAGE
__thread smx_context_t smx_current_context;
#else
smx_context_t smx_current_context; /* define it anyway, will be used in non-parallel mode */
static xbt_os_thread_key_t smx_current_context_key = 0;
#endif

static int smx_parallel_contexts = 1;
static int smx_parallel_threshold = 2;

/** 
 * This function is called by SIMIX_global_init() to initialize the context module.
 */
void SIMIX_context_mod_init(void)
{
  if (!simix_global->context_factory) {
    /* select the context factory to use to create the contexts */
    if (smx_factory_initializer_to_use) {
      smx_factory_initializer_to_use(&simix_global->context_factory);
    }
    else { /* use the factory specified by --cfg=contexts/factory:value */

    if (smx_context_factory_name == NULL) {
        /* use the default factory */
	#ifdef HAVE_RAWCTX
    	SIMIX_ctx_raw_factory_init(&simix_global->context_factory);
	#elif CONTEXT_UCONTEXT
		SIMIX_ctx_sysv_factory_init(&simix_global->context_factory);
	#else
		SIMIX_ctx_thread_factory_init(&simix_global->context_factory);
	#endif
    }
    else if (!strcmp(smx_context_factory_name, "ucontext")) {
        /* use ucontext */
#ifdef CONTEXT_UCONTEXT
        SIMIX_ctx_sysv_factory_init(&simix_global->context_factory);
#else
        xbt_die("The context factory 'ucontext' unavailable on your system");
#endif
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
        XBT_ERROR("Invalid context factory specified. Valid factories on this machine:");
#ifdef HAVE_RAWCTX
        XBT_ERROR("  raw: high performance context factory implemented specifically for SimGrid");
#else
        XBT_ERROR("  (raw contextes are disabled at compilation time on this machine -- check configure logs for details)");
#endif
#ifdef CONTEXT_UCONTEXT
        XBT_ERROR("  ucontext: classical system V contextes (implemented with makecontext, swapcontext and friends)");
#else
        XBT_ERROR("  (ucontext is disabled at compilation time on this machine -- check configure logs for details)");
#endif
        XBT_ERROR("  thread: slow portability layer using system threads (pthreads on UNIX, CreateThread() on windows)");
        xbt_die("Please use a valid factory.");
      }
    }
  }

#if defined(CONTEXT_THREADS) && !defined(HAVE_THREAD_LOCAL_STORAGE)
  /* the __thread storage class is not available on this platform:
   * use getspecific/setspecific instead to store the current context in each thread */
  xbt_os_thread_key_create(&smx_current_context_key);
#endif
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
    finalize_factory(&simix_global->context_factory);
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
XBT_INLINE void SIMIX_context_set_nthreads(int nb_threads) {

  xbt_assert(nb_threads > 0, "Invalid number of parallel threads: %d", nb_threads);

  if (nb_threads > 1) {
#ifndef CONTEXT_THREADS
    THROWF(arg_error, 0, "No thread support for parallel context execution");
#endif
  }

  smx_parallel_contexts = nb_threads;
}

/**
 * \brief Returns the number of parallel threads used
 * for the user contexts.
 * \return the number of threads (1 means no parallelism)
 */
XBT_INLINE int SIMIX_context_get_nthreads(void) {
  return smx_parallel_contexts;
}

/**
 * \brief Returns whether some parallel threads are used
 * for the user contexts.
 * \return 1 if parallelism is used
 */
XBT_INLINE int SIMIX_context_is_parallel(void) {
  return smx_parallel_contexts > 1;
}

/**
 * \brief Sets the threshold above which user processes are run in parallel.
 *
 * If the number of threads is set to 1, there is no parallelism and this
 * threshold has no effect.
 *
 * \param threshold when the number of user processes ready to run is above
 * this threshold, they are run in parallel
 */
XBT_INLINE void SIMIX_context_set_parallel_threshold(int threshold) {
  smx_parallel_threshold = threshold;
}

/**
 * \brief Returns the threshold above which user processes are run in parallel.
 *
 * If the number of threads is set to 1, there is no parallelism and this
 * threshold has no effect.
 *
 * \return when the number of user processes ready to run is above
 * this threshold, they are run in parallel
 */
XBT_INLINE int SIMIX_context_get_parallel_threshold(void) {
  return smx_parallel_threshold;
}

/**
 * \brief Returns the current context of this thread.
 * \return the current context of this thread
 */
XBT_INLINE smx_context_t SIMIX_context_get_current(void)
{
#ifdef HAVE_THREAD_LOCAL_STORAGE
  return smx_current_context;
#else
  return xbt_os_thread_get_specific(smx_current_context_key);
#endif
}

/**
 * \brief Sets the current context of this thread.
 * \param context the context to set
 */
XBT_INLINE void SIMIX_context_set_current(smx_context_t context)
{
#ifdef HAVE_THREAD_LOCAL_STORAGE
  smx_current_context = context;
#else
  xbt_os_thread_set_specific(smx_current_context_key, context);
#endif
}

