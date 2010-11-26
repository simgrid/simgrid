/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_PRIVATE_H
#define SIMIX_PRIVATE_H

#include "simix/simix.h"
#include "surf/surf.h"
#include "xbt/fifo.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/config.h"
#include "xbt/function_types.h"
#include "xbt/ex_interface.h"
#include "instr/private.h"

/******************************** Datatypes ***********************************/


/*********************************** Host *************************************/

/** @brief Host datatype
    @ingroup m_datatypes_management_details */
typedef struct s_smx_host {
  char *name;              /**< @brief host name if any */
  void *host;                   /* SURF modeling */
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
  xbt_dict_t msg_sizes; /* pimple to get an histogram of message sizes in the simulation */
#ifdef HAVE_LATENCY_BOUND_TRACKING
  xbt_dict_t latency_limited_dict;
#endif
} s_SIMIX_Global_t, *SIMIX_Global_t;

extern SIMIX_Global_t simix_global;

/******************************** Process *************************************/

/** @brief Process datatype 
    @ingroup m_datatypes_management_details @{ */
typedef struct s_smx_process {
  s_xbt_swag_hookup_t process_hookup;
  s_xbt_swag_hookup_t synchro_hookup;   /* process_to_run or mutex->sleeping and co */
  s_xbt_swag_hookup_t host_proc_hookup;
  s_xbt_swag_hookup_t destroy_hookup;

  char *name;                   /**< @brief process name if any */
  smx_host_t smx_host;          /* the host on which the process is running */
  smx_context_t context;        /* the context that executes the scheduler function */
  ex_ctx_t *exception;
  int blocked:1;
  int suspended:1;
  int iwannadie:1;
  smx_mutex_t mutex;            /* mutex on which the process is blocked  */
  smx_cond_t cond;              /* cond on which the process is blocked  */
  smx_sem_t sem;                /* semaphore on which the process is blocked  */
  smx_action_t waiting_action;
  xbt_dict_t properties;
  void *data;                   /* kept for compatibility, it should be replaced with moddata */

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
ex_ctx_t *SIMIX_process_get_exception(void);
void SIMIX_process_exception_terminate(xbt_ex_t * e);

/******************************* Networking ***********************************/

/** @brief Rendez-vous point datatype */
typedef struct s_smx_rvpoint {
  char *name;
  smx_mutex_t read;
  smx_mutex_t write;
  xbt_fifo_t comm_fifo;
  void *data;
} s_smx_rvpoint_t;

typedef struct s_smx_comm {


  smx_comm_type_t type;         /* Type of the communication (comm_send,comm_recv) */
  smx_rdv_t rdv;                /* Rendez-vous where the comm is queued */
  smx_sem_t sem;                /* Semaphore associated to the surf simulation */
  int refcount;                 /* Number of processes involved in the cond */

  /* Surf action data */
  smx_process_t src_proc;
  smx_process_t dst_proc;
  smx_action_t src_timeout;
  smx_action_t dst_timeout;
  smx_action_t act;
  double rate;
  double task_size;

  /* Data to be transfered */
  void *src_buff;
  void *dst_buff;
  size_t src_buff_size;
  size_t *dst_buff_size;
  char copied;

  void *data;                   /* User data associated to communication */
} s_smx_comm_t;

void SIMIX_network_copy_data(smx_comm_t comm);
smx_comm_t SIMIX_communication_new(smx_comm_type_t type);
static XBT_INLINE void SIMIX_communication_use(smx_comm_t comm);
static XBT_INLINE void SIMIX_communication_wait_for_completion(smx_comm_t
                                                               comm,
                                                               double
                                                               timeout);
static XBT_INLINE void SIMIX_rdv_push(smx_rdv_t rdv, smx_comm_t comm);
static XBT_INLINE void SIMIX_rdv_remove(smx_rdv_t rdv, smx_comm_t comm);

/********************************* Action *************************************/

typedef enum { ready, ongoing, done, failed } smx_action_state_t;

/** @brief Action datatype
    @ingroup m_datatypes_management_details */
typedef struct s_smx_action {
  char *name;              /**< @brief action name if any */
  xbt_fifo_t cond_list;         /*< conditional variables that must be signaled when the action finish. */
  xbt_fifo_t sem_list;          /*< semaphores that must be signaled when the action finish. */
  void *data;              /**< @brief user data */
  int refcount;            /**< @brief reference counter */
  surf_action_t surf_action;    /* SURF modeling of computation  */
  smx_host_t source;
#ifdef HAVE_TRACING
  long long int counter;        /* simix action unique identifier for instrumentation */
  char *category;               /* simix action category for instrumentation */
#endif
} s_smx_action_t;

/************************** Configuration support *****************************/

extern int _simix_init_status;  /* 0: beginning of time; FIXME: KILLME ?
                                   1: pre-inited (cfg_set created);
                                   2: inited (running) */

#define SIMIX_CHECK_HOST()  xbt_assert0(surf_workstation_model->extension.workstation. \
				  get_state(SIMIX_host_self()->host)==SURF_RESOURCE_ON,\
                                  "Host failed, you cannot call this function.")

smx_host_t __SIMIX_host_create(const char *name, void *workstation,
                               void *data);
void __SIMIX_host_destroy(void *host);
void __SIMIX_cond_wait(smx_cond_t cond);
void __SIMIX_cond_display_actions(smx_cond_t cond);
void __SIMIX_action_display_conditions(smx_action_t action);

/******************************** Context *************************************/

/* The following function pointer types describe the interface that any context
   factory should implement */

typedef smx_context_t(*smx_pfn_context_factory_create_context_t)
 (xbt_main_func_t, int, char **, void_f_pvoid_t, void *);
typedef int (*smx_pfn_context_factory_finalize_t) (smx_context_factory_t
                                                   *);
typedef void (*smx_pfn_context_free_t) (smx_context_t);
typedef void (*smx_pfn_context_start_t) (smx_context_t);
typedef void (*smx_pfn_context_stop_t) (smx_context_t);
typedef void (*smx_pfn_context_suspend_t) (smx_context_t context);
typedef void (*smx_pfn_context_resume_t) (smx_context_t new_context);

/* interface of the context factories */
typedef struct s_smx_context_factory {
  smx_pfn_context_factory_create_context_t create_context;
  smx_pfn_context_factory_finalize_t finalize;
  smx_pfn_context_free_t free;
  smx_pfn_context_stop_t stop;
  smx_pfn_context_suspend_t suspend;
  smx_pfn_context_resume_t resume;
  const char *name;
} s_smx_context_factory_t;


void SIMIX_context_mod_init(void);

void SIMIX_context_mod_exit(void);


/* All factories init */
void SIMIX_ctx_thread_factory_init(smx_context_factory_t * factory);
void SIMIX_ctx_sysv_factory_init(smx_context_factory_t * factory);

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
static XBT_INLINE smx_context_t SIMIX_context_new(xbt_main_func_t code,
                                                  int argc, char **argv,
                                                  void_f_pvoid_t
                                                  cleanup_func,
                                                  void *cleanup_arg)
{

  return (*(simix_global->context_factory->create_context))
      (code, argc, argv, cleanup_func, cleanup_arg);
}

/**
 * \brief destroy a context 
 * \param context the context to destroy
 * Argument must be stopped first -- runs in maestro context
 */
static XBT_INLINE void SIMIX_context_free(smx_context_t context)
{
  (*(simix_global->context_factory->free)) (context);
}

/**
 * \brief stops the execution of a context
 * \param context to stop
 */
static XBT_INLINE void SIMIX_context_stop(smx_context_t context)
{
  (*(simix_global->context_factory->stop)) (context);
}

/**
 \brief resumes the execution of a context
 \param old_context the actual context from which is resuming
 \param new_context the context to resume
 */
static XBT_INLINE void SIMIX_context_resume(smx_context_t new_context)
{
  (*(simix_global->context_factory->resume)) (new_context);
}

/**
 \brief suspends a context and return the control back to the one which
        scheduled it
 \param context the context to be suspended (it must be the running one)
 */
static XBT_INLINE void SIMIX_context_suspend(smx_context_t context)
{
  (*(simix_global->context_factory->suspend)) (context);
}

#endif
