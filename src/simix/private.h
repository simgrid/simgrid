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
#include "xbt/ex_interface.h"

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
       ex_ctx_t *exception;
       int blocked : 1;
       int suspended : 1;
       int iwannadie : 1;
       smx_mutex_t mutex;       /* mutex on which the process is blocked  */
       smx_cond_t cond;         /* cond on which the process is blocked  */
       smx_action_t waiting_action;
       xbt_dict_t properties;
       void *data;              /* kept for compatibility, it should be replaced with moddata */

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

void SIMIX_create_maestro_process(void);
void SIMIX_process_empty_trash(void);
void SIMIX_process_schedule(smx_process_t process);
void SIMIX_process_yield(void);
ex_ctx_t *SIMIX_process_get_exception(void);
void SIMIX_process_exception_terminate(xbt_ex_t * e);

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

/******************************* Networking ***********************************/

/** @brief Rendez-vous point datatype */
typedef struct s_smx_rvpoint {
  char *name;
  smx_mutex_t read;
  smx_mutex_t write;
  xbt_fifo_t comm_fifo;  
} s_smx_rvpoint_t;

typedef struct s_smx_comm {
  smx_comm_type_t type;
  smx_host_t src_host;
  smx_host_t dst_host;
  smx_rdv_t rdv;
  smx_cond_t cond;
  smx_action_t act;
  void *data;
  size_t data_size;
  void *dest_buff;
  size_t *dest_buff_size;
  double rate;
  double task_size;
  int refcount;
} s_smx_comm_t;

/********************************* Action *************************************/

typedef enum {ready, ongoing, done, failed} smx_action_state_t;

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
  xbt_main_func_t code; \
  int argc; \
  char **argv; \
  void_f_pvoid_t cleanup_func; \
  void *cleanup_arg; \

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
typedef smx_context_t (*smx_pfn_context_factory_create_context_t) 
                      (xbt_main_func_t, int, char**, void_f_pvoid_t, void*);

/* this function finalize the specified context factory */
typedef int (*smx_pfn_context_factory_finalize_t) (smx_context_factory_t*);

/* function used to destroy the specified context */
typedef void (*smx_pfn_context_free_t) (smx_context_t);

/* function used to start the specified context */
typedef void (*smx_pfn_context_start_t) (smx_context_t);

/* function used to stop the current context */
typedef void (*smx_pfn_context_stop_t) (smx_context_t);

/* function used to suspend the current context */
typedef void (*smx_pfn_context_suspend_t) (smx_context_t context);

/* function used to resume the current context */
typedef void (*smx_pfn_context_resume_t) (smx_context_t old_context, 
                                          smx_context_t new_context);

/* interface of the context factories */
typedef struct s_smx_context_factory {
  smx_pfn_context_factory_create_context_t create_context;
  smx_pfn_context_factory_finalize_t finalize;
  smx_pfn_context_free_t free;
  smx_pfn_context_start_t start;
  smx_pfn_context_stop_t stop;
  smx_pfn_context_suspend_t suspend;
  smx_pfn_context_resume_t resume;
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

/* ****************************** */
/* context manipulation functions */
/* ****************************** */

/* Scenario for the end of a context:
 *
 * CASE 1: death after end of the main function
 *   the context_wrapper, called internally by the context module, calls 
 *   SIMIX_context_stop after user code stops, smx_context_stop calls user 
 *   cleanup_func if any (in context settings), add current process to trashbin
 *   and yields back to maestro.
 *   From time to time, maestro calls SIMIX_context_empty_trash, which destroy
 *   all the process and context data structures, and frees the memory 
 *
 * CASE 2: brutal death
 *   SIMIX_process_kill (from any process) set process->iwannadie = 1 and then
 *   schedules the process. Then the process is awaken in the middle of the
 *   SIMIX_process_yield function, and at the end of it, it checks that
 *   iwannadie == 1, and call SIMIX_context_stop(same than first case afterward)
 */

/**
 * \brief creates a new context for a user level process
 * \param code a main function
 * \param argc the number of arguments of the main function
 * \param argv the vector of arguments of the main function
 * \param cleanup_func the function to call when the context stops
 * \param cleanup_arg the argument of the cleanup_func function
 */
static inline smx_context_t SIMIX_context_new(xbt_main_func_t code, int argc,
                                              char** argv,
                                              void_f_pvoid_t cleanup_func,
                                              void* cleanup_arg)
{
  return (*(simix_global->context_factory->create_context))
           (code, argc, argv, cleanup_func, cleanup_arg);
}

/**
 * \brief destroy a context 
 * \param context the context to destroy
 * Argument must be stopped first -- runs in maestro context
 */
static inline void SIMIX_context_free(smx_context_t context)
{
  (*(simix_global->context_factory->free)) (context);
}

/**
 * \brief prepares aa context to be run
 * \param context the context to start
 * It will however run effectively only when calling #SIMIX_process_schedule
 */
static inline void SIMIX_context_start(smx_context_t context)
{
  (*(simix_global->context_factory->start)) (context);
}

/**
 * \brief stops the execution of a context
 * \param context to stop
 */
static inline void SIMIX_context_stop(smx_context_t context)
{
  (*(simix_global->context_factory->stop)) (context);
}

/**
 \brief resumes the execution of a context
 \param old_context the actual context from which is resuming
 \param new_context the context to resume
 */
static inline void SIMIX_context_resume(smx_context_t old_context,
                                        smx_context_t new_context)
{
  (*(simix_global->context_factory->resume)) (old_context, new_context);
}

/**
 \brief suspends a context and return the control back to the one which
        scheduled it
 \param context the context to be suspended (it must be the running one)
 */
static inline void SIMIX_context_suspend(smx_context_t context)
{
  (*(simix_global->context_factory->suspend)) (context);
}

#endif
