/* context_base - Code factorization accross context switching implementations */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "xbt/function_types.h"
#include "smx_context_private.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(bindings);

void smx_ctx_base_factory_init(smx_context_factory_t * factory)
{
  /* instantiate the context factory */
  *factory = xbt_new0(s_smx_context_factory_t, 1);

  (*factory)->create_context = NULL;
  (*factory)->finalize = smx_ctx_base_factory_finalize;
  (*factory)->free = smx_ctx_base_free;
  (*factory)->stop = smx_ctx_base_stop;
  (*factory)->suspend = NULL;
  (*factory)->resume = NULL;

  (*factory)->name = "base context factory";
}

int smx_ctx_base_factory_finalize(smx_context_factory_t * factory)
{
  free(*factory);
  *factory = NULL;
  return 0;
}

smx_context_t
smx_ctx_base_factory_create_context_sized(size_t size,
                                          xbt_main_func_t code, int argc,
                                          char **argv,
                                          void_f_pvoid_t cleanup_func,
                                          void *cleanup_arg)
{
  smx_context_t context = xbt_malloc0(size);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if (code) {
    context->cleanup_func = cleanup_func;
    context->cleanup_arg = cleanup_arg;
    context->argc = argc;
    context->argv = argv;
    context->code = code;
  }

  return context;
}

void smx_ctx_base_free(smx_context_t context)
{
  int i;

  if (context) {

    /* free argv */
    if (context->argv) {
      for (i = 0; i < context->argc; i++)
        if (context->argv[i])
          free(context->argv[i]);

      free(context->argv);
    }

    /* free structure */
    free(context);
  }
}

void smx_ctx_base_stop(smx_context_t context)
{

  if (context->cleanup_func)
    (*(context->cleanup_func)) (context->cleanup_arg);

}
