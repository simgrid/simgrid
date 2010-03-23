/* a fast and simple context switching library                              */

/* Copyright (c) 2004-2008 the SimGrid team.                                */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_PRIVATE_H
#define _XBT_CONTEXT_PRIVATE_H

/*#include "xbt/sysdep.h"*/
#include "simix/private.h"
#include "xbt/swag.h"

SG_BEGIN_DECL()

/* *********************** */
/* Context type definition */
/* *********************** */
/* the following function pointers types describe the interface that all context
   concepts must implement */

/* each context type must contain this macro at its begining -- OOP in C :/ */
#define SMX_CTX_BASE_T \
  s_xbt_swag_hookup_t hookup; \
  ex_ctx_t *exception; \
  xbt_main_func_t code; \

/* all other context types derive from this structure */
typedef struct s_smx_context {
  SMX_CTX_BASE_T;
} s_smx_context_t;

/* *********************** */
/* factory type definition */
/* *********************** */

/* The following function pointer types describe the interface that any context 
   factory should implement */

/* function used to create a new context */
typedef int (*smx_pfn_context_factory_create_context_t) (smx_process_t *, xbt_main_func_t);

/* function used to create the context for the maestro process */
typedef int (*smx_pfn_context_factory_create_maestro_context_t) (smx_process_t*);

/* this function finalize the specified context factory */
typedef int (*smx_pfn_context_factory_finalize_t) (smx_context_factory_t*);

/* function used to destroy the specified context */
typedef void (*smx_pfn_context_free_t) (smx_process_t);

/* function used to kill the specified context */
typedef void (*smx_pfn_context_kill_t) (smx_process_t);

/* function used to resume the specified context */
typedef void (*smx_pfn_context_schedule_t) (smx_process_t);

/* function used to yield the specified context */
typedef void (*smx_pfn_context_yield_t) (void);

/* function used to start the specified context */
typedef void (*smx_pfn_context_start_t) (smx_process_t);

/* function used to stop the current context */
typedef void (*smx_pfn_context_stop_t) (int);

/* interface of the context factories */
typedef struct s_smx_context_factory {
  smx_pfn_context_factory_create_maestro_context_t create_maestro_context;
  smx_pfn_context_factory_create_context_t create_context;
  smx_pfn_context_factory_finalize_t finalize;
  smx_pfn_context_free_t free;
  smx_pfn_context_kill_t kill;
  smx_pfn_context_schedule_t schedule;
  smx_pfn_context_yield_t yield;
  smx_pfn_context_stop_t stop;
  const char *name;
} s_smx_context_factory_t;

/* Selects a context factory associated with the name specified by the parameter name.
 * If successful the function returns 0. Otherwise the function returns the error code.
 */
int SIMIX_context_select_factory(const char *name);

/* Initializes a context factory from the name specified by the parameter name.
 * If the factory cannot be found, an exception is raised.
 */
void SIMIX_context_init_factory_by_name(smx_context_factory_t * factory, const char *name);

/* All factories init */
void SIMIX_ctx_thread_factory_init(smx_context_factory_t * factory);

void SIMIX_ctx_sysv_factory_init(smx_context_factory_t * factory);

void SIMIX_ctx_java_factory_init(smx_context_factory_t * factory);

SG_END_DECL()
#endif /* !_XBT_CONTEXT_PRIVATE_H */
