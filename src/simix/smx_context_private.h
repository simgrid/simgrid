/* a fast and simple context switching library                              */

/* Copyright (c) 2004-2008 the SimGrid team.                                */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_PRIVATE_H
#define _XBT_CONTEXT_PRIVATE_H

#include "xbt/swag.h"
#include "simix/private.h"

SG_BEGIN_DECL()

/* *********************** */
/* Context type definition */
/* *********************** */
/* the following function pointers types describe the interface that all context
   concepts must implement */
/* the following function pointers types describe the interface that all context
   concepts must implement */

/* each context type derive from this structure, so they must contain this structure
 * at their begining -- OOP in C :/ */
typedef struct s_smx_context {
  s_xbt_swag_hookup_t hookup;
  xbt_main_func_t code;
  int argc;
  char **argv;
  void_f_pvoid_t cleanup_func;
  void *cleanup_arg;
} s_smx_ctx_base_t;
/* methods of this class */
void smx_ctx_base_factory_init(smx_context_factory_t * factory);
int smx_ctx_base_factory_finalize(smx_context_factory_t * factory);

smx_context_t
smx_ctx_base_factory_create_context_sized(size_t size,
    xbt_main_func_t code, int argc, char** argv,
    void_f_pvoid_t cleanup_func, void* cleanup_arg);
void smx_ctx_base_free(smx_context_t context);
void smx_ctx_base_stop(smx_context_t context);

/* Functions of sysv context mecanism: lua inherites them */
smx_context_t
smx_ctx_sysv_create_context_sized(size_t size,
    xbt_main_func_t code, int argc, char** argv,
    void_f_pvoid_t cleanup_func, void* cleanup_arg);
void smx_ctx_sysv_free(smx_context_t context);
void smx_ctx_sysv_stop(smx_context_t context);
void smx_ctx_sysv_suspend(smx_context_t context);
void smx_ctx_sysv_resume(smx_context_t new_context);

SG_END_DECL()
#endif /* !_XBT_CONTEXT_PRIVATE_H */


