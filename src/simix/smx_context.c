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
