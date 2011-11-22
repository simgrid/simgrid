/* Functions of sysv context mecanism: lua inherites them                   */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_SYSV_PRIVATE_H
#define _XBT_CONTEXT_SYSV_PRIVATE_H

#include "xbt/swag.h"
#include "simix/context.h"
#include "portable.h"

SG_BEGIN_DECL()

/* lower this if you want to reduce the memory consumption  */
#include "context_sysv_config.h"        /* loads context system definitions */
#ifdef _XBT_WIN32
#include <win32_ucontext.h>     /* context relative declarations */
#else
#include <ucontext.h>           /* context relative declarations */
#endif

typedef struct s_smx_ctx_sysv {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  ucontext_t uc;                /* the ucontext that executes the code */
#ifdef HAVE_VALGRIND_VALGRIND_H
  unsigned int valgrind_stack_id;       /* the valgrind stack id */
#endif
  char stack[0];                /* the thread stack (must remain the last element of the structure) */
} s_smx_ctx_sysv_t, *smx_ctx_sysv_t;

void SIMIX_ctx_sysv_factory_init(smx_context_factory_t *factory);
int smx_ctx_sysv_factory_finalize(smx_context_factory_t *factory);

smx_context_t
smx_ctx_sysv_create_context_sized(size_t structure_size,
                                  xbt_main_func_t code, int argc,
                                  char **argv,
                                  void_pfn_smxprocess_t cleanup_func,
                                  void *data);
void smx_ctx_sysv_free(smx_context_t context);
int smx_ctx_sysv_get_thread_id(void);
smx_context_t smx_ctx_sysv_self_parallel(void);

SG_END_DECL()

#endif                          /* !_XBT_CONTEXT_SYSV_PRIVATE_H */
