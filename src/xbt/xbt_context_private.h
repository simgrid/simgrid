/* a fast and simple context switching library                              */

/* Copyright (c) 2004-2008 the SimGrid team.                                */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_PRIVATE_H
#define _XBT_CONTEXT_PRIVATE_H

#include "xbt/sysdep.h"
#include "xbt/context.h"
#include "xbt/swag.h"

SG_BEGIN_DECL()

/* *********************** */
/* Context type definition */
/* *********************** */
/* the following function pointers describe the interface that all context concepts must implement */
     typedef void (*xbt_pfn_context_free_t) (xbt_context_t);    /* pointer type to the function used to destroy the specified context   */

     typedef void (*xbt_pfn_context_kill_t) (xbt_context_t);    /* pointer type to the function used to kill the specified context              */

     typedef void (*xbt_pfn_context_schedule_t) (xbt_context_t);        /* pointer type to the function used to resume the specified context    */

     typedef void (*xbt_pfn_context_yield_t) (void);    /* pointer type to the function used to yield the specified context             */

     typedef void (*xbt_pfn_context_start_t) (xbt_context_t);   /* pointer type to the function used to start the specified context             */

     typedef void (*xbt_pfn_context_stop_t) (int);      /* pointer type to the function used to stop the current context                */

/* each context type must contain this macro at its begining -- OOP in C :/ */
#define XBT_CTX_BASE_T \
  s_xbt_swag_hookup_t hookup; \
  char *name; \
  void_f_pvoid_t cleanup_func; \
  void *cleanup_arg; \
  ex_ctx_t *exception; \
  int iwannadie; \
  xbt_main_func_t code; \
  int argc; \
  char **argv; \
  void_f_pvoid_t startup_func; \
  void *startup_arg; \
  xbt_pfn_context_free_t free; \
  xbt_pfn_context_kill_t kill; \
  xbt_pfn_context_schedule_t schedule; \
  xbt_pfn_context_yield_t yield; \
  xbt_pfn_context_start_t start; \
  xbt_pfn_context_stop_t stop

/* all other context types derive from this structure */
     typedef struct s_xbt_context {
       XBT_CTX_BASE_T;
     } s_xbt_context_t;

/* ****************** */
/* Globals definition */
/* ****************** */

/* Important guys */
     extern xbt_context_t current_context;

     extern xbt_context_t maestro_context;

/* All dudes lists */
     extern xbt_swag_t context_living;

     extern xbt_swag_t context_to_destroy;

/* *********************** */
/* factory type definition */
/* *********************** */
     typedef struct s_xbt_context_factory *xbt_context_factory_t;

/* this function describes the interface that all context factory must implement */
     typedef xbt_context_t(*xbt_pfn_context_factory_create_context_t) (const
                                                                       char *,
                                                                       xbt_main_func_t,
                                                                       void_f_pvoid_t,
                                                                       void *,
                                                                       void_f_pvoid_t,
                                                                       void *,
                                                                       int,
                                                                       char
                                                                       **);
     typedef
     int (*xbt_pfn_context_factory_create_maestro_context_t) (xbt_context_t
                                                              *);

/* this function finalize the specified context factory */
     typedef int (*xbt_pfn_context_factory_finalize_t) (xbt_context_factory_t
                                                        *);

/* this interface is used by the xbt context module to create the appropriate concept */
     typedef struct s_xbt_context_factory {
       xbt_pfn_context_factory_create_maestro_context_t create_maestro_context; /* create the context of the maestro    */
       xbt_pfn_context_factory_create_context_t create_context; /* create a new context                 */
       xbt_pfn_context_factory_finalize_t finalize;     /* finalize the context factory         */
       const char *name;        /* the name of the context factory      */

     } s_xbt_context_factory_t;

/**
 * This function select a context factory associated with the name specified by
 * the parameter name.
 * If successful the function returns 0. Otherwise the function returns the error
 * code.
 */
     int
       xbt_context_select_factory(const char *name);

/**
 * This function initialize a context factory from the name specified by the parameter
 * name.
 * If the factory cannot be found, an exception is raised.
 */
     void



      
       xbt_context_init_factory_by_name(xbt_context_factory_t * factory,
                                        const char *name);


/* All factories init */
     void xbt_ctx_thread_factory_init(xbt_context_factory_t * factory);

     void xbt_ctx_sysv_factory_init(xbt_context_factory_t * factory);

     void xbt_ctx_java_factory_init(xbt_context_factory_t * factory);





SG_END_DECL()
#endif /* !_XBT_CONTEXT_PRIVATE_H */
