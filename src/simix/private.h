/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_PRIVATE_H
#define _SIMIX_PRIVATE_H

#include "simix/simix.h"
#include "surf/surf.h"
#include "xbt/fifo.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/config.h"
#include "xbt/function_types.h"
#include "xbt/ex_interface.h"
#include "instr/private.h"
#include "process_private.h"
#include "host_private.h"
#include "network_private.h"
#include "smurf_private.h"
#include "synchro_private.h"

extern int _surf_do_model_check;

/********************************** Simix Global ******************************/

typedef struct s_smx_context_factory *smx_context_factory_t;

typedef struct s_smx_global {
  smx_context_factory_t context_factory;
  xbt_dict_t host;
  xbt_swag_t process_to_run;
  xbt_swag_t process_list;
  xbt_swag_t process_to_destroy;
  smx_process_t maestro_process;
  xbt_dict_t registered_functions;
  smx_creation_func_t create_process_function;
  void_f_pvoid_t kill_process_function;
  void_pfn_smxprocess_t cleanup_process_function;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  xbt_dict_t latency_limited_dict;
#endif
} s_smx_global_t, *smx_global_t;

extern smx_global_t simix_global;


/*********************************** Time ************************************/

/** @brief Timer datatype */
typedef struct s_smx_timer {
  double date;
  void* func;
  void* args;
} s_smx_timer_t;

/********************************* Action *************************************/

typedef enum {
  SIMIX_ACTION_EXECUTE,
  SIMIX_ACTION_PARALLEL_EXECUTE,
  SIMIX_ACTION_COMMUNICATE,
  SIMIX_ACTION_SLEEP,
  SIMIX_ACTION_SYNCHRO,
  SIMIX_ACTION_IO
} e_smx_action_type_t;

typedef enum {
  SIMIX_COMM_SEND,
  SIMIX_COMM_RECEIVE,
  SIMIX_COMM_READY,
  SIMIX_COMM_DONE
} e_smx_comm_type_t;

/** @brief Action datatype */
typedef struct s_smx_action {

  e_smx_action_type_t type;          /* Type of SIMIX action*/
  e_smx_state_t state;               /* State of the action */
  char *name;                        /* Action name if any */
  xbt_fifo_t request_list;           /* List of requests on this action */

  /* Data specific to each action type */
  union {

    struct {
      smx_host_t host;                /* The host where the execution takes place */
      surf_action_t surf_exec;        /* The Surf execution action encapsulated */
    } execution; /* Possibly parallel execution */

    struct {
      e_smx_comm_type_t type;         /* Type of the communication (SIMIX_COMM_SEND or SIMIX_COMM_RECEIVE) */
      smx_rdv_t rdv;                  /* Rendez-vous where the comm is queued */
      int refcount;                   /* Number of processes involved in the cond */

      /* Surf action data */
      surf_action_t surf_comm;        /* The Surf communication action encapsulated */
      surf_action_t src_timeout;      /* Surf's actions to instrument the timeouts */
      surf_action_t dst_timeout;      /* Surf's actions to instrument the timeouts */
      smx_process_t src_proc;
      smx_process_t dst_proc;
      double rate;
      double task_size;

      /* Data to be transfered */
      void *src_buff;
      void *dst_buff;
      size_t src_buff_size;
      size_t *dst_buff_size;
      char copied;

      void *data;                     /* User data associated to communication */
    } comm;    

    struct {
      smx_host_t host;                /* The host that is sleeping */
      surf_action_t surf_sleep;       /* The Surf sleeping action encapsulated */
    } sleep;

    struct {
      surf_action_t sleep;
    } synchro;

  };

#ifdef HAVE_TRACING
  long long int counter;              /* simix action unique identifier for instrumentation */
  char *category;                     /* simix action category for instrumentation */
#endif
} s_smx_action_t;

/* FIXME: check if we can delete this function */
static XBT_INLINE e_smx_state_t SIMIX_action_map_state(e_surf_action_state_t state)
{
  switch (state) {
    case SURF_ACTION_READY:
      return SIMIX_READY;
    case SURF_ACTION_RUNNING:
      return SIMIX_RUNNING;
    case SURF_ACTION_FAILED:
      return SIMIX_FAILED;
    case SURF_ACTION_DONE:
      return SIMIX_DONE;
    default:
      xbt_die("Unexpected SURF action state");
  }
}

/******************************** Context *************************************/

/* The following function pointer types describe the interface that any context
   factory should implement */


typedef smx_context_t(*smx_pfn_context_factory_create_context_t)
  (xbt_main_func_t, int, char **, void_pfn_smxprocess_t, void* data);
typedef int (*smx_pfn_context_factory_finalize_t) (smx_context_factory_t*);
typedef void (*smx_pfn_context_free_t) (smx_context_t);
typedef void (*smx_pfn_context_start_t) (smx_context_t);
typedef void (*smx_pfn_context_stop_t) (smx_context_t);
typedef void (*smx_pfn_context_suspend_t) (smx_context_t context);
typedef void (*smx_pfn_context_runall_t) (xbt_swag_t processes);
typedef smx_context_t (*smx_pfn_context_self_t) (void);
typedef void* (*smx_pfn_context_get_data_t) (smx_context_t context);

/* interface of the context factories */
typedef struct s_smx_context_factory {
  const char *name;
  smx_pfn_context_factory_create_context_t create_context;
  smx_pfn_context_factory_finalize_t finalize;
  smx_pfn_context_free_t free;
  smx_pfn_context_stop_t stop;
  smx_pfn_context_suspend_t suspend;
  smx_pfn_context_runall_t runall;
  smx_pfn_context_self_t self;
  smx_pfn_context_get_data_t get_data;
} s_smx_context_factory_t;


void SIMIX_context_mod_init(void);
void SIMIX_context_mod_exit(void);


/* All factories init */
void SIMIX_ctx_thread_factory_init(smx_context_factory_t *factory);
void SIMIX_ctx_sysv_factory_init(smx_context_factory_t *factory);

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
                                                  void_pfn_smxprocess_t cleanup_func,
                                                  smx_process_t simix_process)
{

  return (*(simix_global->context_factory->create_context))
      (code, argc, argv, cleanup_func, simix_process);
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
 \brief suspends a context and return the control back to the one which
        scheduled it
 \param context the context to be suspended (it must be the running one)
 */
static XBT_INLINE void SIMIX_context_suspend(smx_context_t context)
{
  (*(simix_global->context_factory->suspend)) (context);
}

/**
 \brief executes all the processes (in parallel if possible)
 \param processes the swag of processes to execute
 */
static XBT_INLINE void SIMIX_context_runall(xbt_swag_t processes)
{
  (*(simix_global->context_factory->runall)) (processes);
}

/**
 \brief returns the current running context 
 */
static XBT_INLINE smx_context_t SIMIX_context_self(void)
{
  if (simix_global->context_factory == NULL) {
    return NULL;
  }

  return (*(simix_global->context_factory->self))();
}

<<<<<<< HEAD
/**
 \brief returns the data associated to a context
 \param context The context
 \return The data
 */
static XBT_INLINE void* SIMIX_context_get_data(smx_context_t context)
{
  return (*(simix_global->context_factory->get_data))(context);
}


=======
>>>>>>> Clean indentation and coding style in SIMIX
#endif
