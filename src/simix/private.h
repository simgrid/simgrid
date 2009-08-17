/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_PRIVATE_H
#define SIMIX_PRIVATE_H

#include <stdio.h>
#include "simix/simix.h"
#include "surf/surf.h"
#include "xbt/fifo.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/config.h"
#include "xbt/function_types.h"

/******************************** Datatypes ***********************************/


/*********************************** Host *************************************/

/** @brief Host datatype
    @ingroup m_datatypes_management_details */
typedef struct s_smx_host {
  char *name;              /**< @brief host name if any */
  void *host;              /* SURF modeling */
  xbt_swag_t process_list;
  void *data;              /**< @brief user data */
} s_smx_host_t;

/********************************** Simix Global ******************************/

typedef struct s_smx_context_factory *smx_context_factory_t;

typedef struct SIMIX_Global {
  smx_context_factory_t context_factory;
  xbt_dict_t host;
  xbt_swag_t process_to_run;
  xbt_swag_t process_list;
  xbt_swag_t process_to_destroy;
  smx_process_t current_process;
  smx_process_t maestro_process;
  xbt_dict_t registered_functions;
  smx_creation_func_t create_process_function;
  void_f_pvoid_t kill_process_function;
  void_f_pvoid_t cleanup_process_function;
} s_SIMIX_Global_t, *SIMIX_Global_t;

extern SIMIX_Global_t simix_global;

/******************************** Process *************************************/

typedef struct s_smx_context *smx_context_t;

/** @brief Process datatype
    @ingroup m_datatypes_management_details @{ */
     typedef struct s_smx_process {
       s_xbt_swag_hookup_t process_hookup;
       s_xbt_swag_hookup_t synchro_hookup;
       s_xbt_swag_hookup_t host_proc_hookup;
       s_xbt_swag_hookup_t destroy_hookup;

       char *name;              /**< @brief process name if any */
       smx_host_t smx_host;     /* the host on which the process is running */
       smx_context_t context;   /* the context that executes the scheduler function */
       int argc;                /* arguments number if any */
       char **argv;             /* arguments table if any */
       int blocked : 1;
       int suspended : 1;
       int iwannadie : 1;
       smx_mutex_t mutex;       /* mutex on which the process is blocked  */
       smx_cond_t cond;         /* cond on which the process is blocked  */
       xbt_dict_t properties;
       void *data;              /* kept for compatibility, it should be replaced with moddata */
       void_f_pvoid_t cleanup_func;
       void *cleanup_arg;

     } s_smx_process_t;
/** @} */

typedef struct s_smx_process_arg {
  const char *name;
  xbt_main_func_t code;
  void *data;
  char *hostname;
  int argc;
  char **argv;
  double kill_time;
  xbt_dict_t properties;
} s_smx_process_arg_t, *smx_process_arg_t;

void SIMIX_process_empty_trash(void);
void __SIMIX_process_schedule(smx_process_t process);
void __SIMIX_process_yield(void);

/*************************** Mutex and Conditional ****************************/

typedef struct s_smx_mutex {

  /* KEEP IT IN SYNC WITH src/xbt_sg_thread.c::struct s_xbt_mutex */
  xbt_swag_t sleeping;          /* list of sleeping process */
  int refcount;
  /* KEEP IT IN SYNC WITH src/xbt_sg_thread.c::struct s_xbt_mutex */

} s_smx_mutex_t;

typedef struct s_smx_cond {

  /* KEEP IT IN SYNC WITH src/xbt_sg_thread.c::struct s_xbt_cond */
  xbt_swag_t sleeping;          /* list of sleeping process */
  smx_mutex_t mutex;
  xbt_fifo_t actions;           /* list of actions */
  /* KEEP IT IN SYNC WITH src/xbt_sg_thread.c::struct s_xbt_cond */

} s_smx_cond_t;

/********************************* Action *************************************/

/** @brief Action datatype
    @ingroup m_datatypes_management_details */
typedef struct s_smx_action {
  char *name;              /**< @brief action name if any */
  xbt_fifo_t cond_list;    /*< conditional variables that must be signaled when the action finish. */
  void *data;              /**< @brief user data */
  int refcount;            /**< @brief reference counter */
  surf_action_t surf_action;    /* SURF modeling of computation  */
  smx_host_t source;
} s_smx_action_t;

/************************** Configuration support *****************************/

extern int _simix_init_status;  /* 0: beginning of time; FIXME: KILLME ?
                                   1: pre-inited (cfg_set created);
                                   2: inited (running) */

#define SIMIX_CHECK_HOST()  xbt_assert0(surf_workstation_model->extension.workstation. \
				  get_state(SIMIX_host_self()->host)==SURF_RESOURCE_ON,\
                                  "Host failed, you cannot call this function.")

smx_host_t __SIMIX_host_create(const char *name, void *workstation, void *data);
void __SIMIX_host_destroy(void *host);
void __SIMIX_cond_wait(smx_cond_t cond);
void __SIMIX_cond_display_actions(smx_cond_t cond);
void __SIMIX_action_display_conditions(smx_action_t action);
void __SIMIX_create_maestro_process(void);

/******************************** Context *************************************/

void SIMIX_context_mod_init(void);

void SIMIX_context_mod_exit(void);

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

/* Each context implementation define its own context factory
 * A context factory is responsable of the creation and manipulation of the 
 * execution context of all the simulated processes (and maestro) using the
 * selected implementation.
 *
 * For example, the context switch based on java thread use the
 * java implementation of the context and the java factory to build and control
 * the contexts depending on this implementation.

 * The following function pointer types describe the interface that any context 
 * factory should implement.
 */

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
  smx_pfn_context_start_t start;
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

/* ******************************* */
/* contexts manipulation functions */
/* ******************************* */

/**
 * \param smx_process the simix maestro process that contains this context
 */
static inline int SIMIX_context_create_maestro(smx_process_t *process)
{
  return (*(simix_global->context_factory->create_maestro_context)) (process);
}

/**
 * \param smx_process the simix process that contains this context
 * \param code a main function
 */
static inline int SIMIX_context_new(smx_process_t *process, xbt_main_func_t code)
{
    return (*(simix_global->context_factory->create_context)) (process, code);
}

/* Scenario for the end of a context:
 *
 * CASE 1: death after end of function
 *   __context_wrapper, called by os thread, calls smx_context_stop after user code stops
 *   smx_context_stop calls user cleanup_func if any (in context settings),
 *                    add current to trashbin
 *                    yields back to maestro (destroy os thread on need)
 *   From time to time, maestro calls smx_context_empty_trash,
 *       which maps smx_context_free on the content
 *   smx_context_free frees some more memory,
 *                    joins os thread
 *
 * CASE 2: brutal death
 *   smx_context_kill (from any context)
 *                    set context->wannadie to 1
 *                    yields to the context
 *   the context is awaken in the middle of __yield.
 *   At the end of it, it checks that wannadie == 1, and call smx_context_stop
 *   (same than first case afterward)
 */
static inline void SIMIX_context_kill(smx_process_t process)
{
  (*(simix_global->context_factory->kill)) (process);
}

/* Argument must be stopped first -- runs in maestro context */
static inline void SIMIX_context_free(smx_process_t process)
{
  (*(simix_global->context_factory->free)) (process);
}

/**
 * \param context the context to start
 *
 * Calling this function prepares \a process to be run. It will
   however run effectively only when calling #SIMIX_context_schedule
 */
static inline void SIMIX_context_start(smx_process_t process)
{
  (*(simix_global->context_factory->start)) (process);
}

/**
 * Calling this function makes the current process yield. The process
 * that scheduled it returns from SIMIX_context_schedule as if nothing
 * had happened.
 *
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
static inline void SIMIX_context_yield(void)
{
  (*(simix_global->context_factory->yield)) ();
}

/**
 * \param process to be scheduled
 *
 * Calling this function blocks the current process and schedule \a process.
 * When \a process would call SIMIX_context_yield, it will return
 * to this function as if nothing had happened.
 *
 * Only the maestro can call this function to run a given process.
 */
static inline void SIMIX_context_schedule(smx_process_t process)
{
  (*(simix_global->context_factory->schedule)) (process);
}

static inline void SIMIX_context_stop(int exit_code)
{
  (*(simix_global->context_factory->stop)) (exit_code);
}

#endif
