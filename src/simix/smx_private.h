/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_PRIVATE_H
#define _SIMIX_PRIVATE_H

#include "simgrid/simix.h"
#include "surf/surf.h"
#include "xbt/base.h"
#include "xbt/fifo.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/mallocator.h"
#include "xbt/config.h"
#include "xbt/xbt_os_time.h"
#include "xbt/function_types.h"
#include "xbt/ex_interface.h"
#include "instr/instr_private.h"
#include "smx_process_private.h"
#include "smx_host_private.h"
#include "smx_io_private.h"
#include "smx_network_private.h"
#include "popping_private.h"
#include "smx_synchro_private.h"

SG_BEGIN_DECL()

/* Define only for SimGrid benchmarking purposes */
//#define TIME_BENCH_PER_SR     /* this aims at measuring the time spent in each scheduling round per each thread. The code is thus run in sequential to bench separately each SSR */
//#define TIME_BENCH_AMDAHL     /* this aims at measuring the porting of time that could be parallelized at maximum (to get the optimal speedup by applying the amdahl law). */
//#define ADAPTIVE_THRESHOLD    /* this is to enable the adaptive threshold algorithm in raw contexts*/
//#define TIME_BENCH_ENTIRE_SRS /* more general benchmark than TIME_BENCH_PER_SR. It aims to measure the total time spent in a whole scheduling round (including synchro costs)*/

#ifdef TIME_BENCH_PER_SR
XBT_PRIVATE void smx_ctx_raw_new_sr(void);
#endif
/********************************** Simix Global ******************************/
typedef struct s_smx_global {
  smx_context_factory_t context_factory;
  xbt_dynar_t process_to_run;
  xbt_dynar_t process_that_ran;
  xbt_swag_t process_list;
  xbt_swag_t process_to_destroy;
  smx_process_t maestro_process;
  xbt_dict_t registered_functions;
  smx_creation_func_t create_process_function;
  void_pfn_smxprocess_t_smxprocess_t kill_process_function;
  /** Callback used when killing a SMX_process */
  void_pfn_smxprocess_t cleanup_process_function;
  xbt_mallocator_t synchro_mallocator;
  void_pfn_sghost_t autorestart;

#ifdef TIME_BENCH_AMDAHL
  xbt_os_timer_t timer_seq; /* used to bench the sequential and parallel parts of the simulation, if requested to */
  xbt_os_timer_t timer_par;
#endif

  xbt_os_mutex_t mutex;
} s_smx_global_t, *smx_global_t;

XBT_PUBLIC_DATA(smx_global_t) simix_global;
extern XBT_PRIVATE unsigned long simix_process_maxpid;

XBT_PUBLIC(void) SIMIX_clean(void);

/******************************** Exceptions *********************************/
/** @brief Ask to the provided simix process to raise the provided exception */
#define SMX_EXCEPTION(issuer, cat, val, msg)                            \
  if (1) {                                                              \
    smx_process_t _smx_throw_issuer = (issuer); /* evaluate only once */\
    THROW_PREPARE(_smx_throw_issuer->running_ctx, (cat), (val), xbt_strdup(msg)); \
    _smx_throw_issuer->doexception = 1;                                 \
  } else ((void)0)

#define SMX_THROW() RETHROW

/* ******************************** File ************************************ */
typedef struct s_smx_file {
  surf_file_t surf_file;
  void* data;                   /**< @brief user data */
} s_smx_file_t;

/*********************************** Time ************************************/

/** @brief Timer datatype */
typedef struct s_smx_timer {
  double date;
  void* func;
  void* args;
} s_smx_timer_t;

/********************************* synchro *************************************/

typedef enum {
  SIMIX_SYNC_EXECUTE,
  SIMIX_SYNC_PARALLEL_EXECUTE,
  SIMIX_SYNC_COMMUNICATE,
  SIMIX_SYNC_JOIN,
  SIMIX_SYNC_SLEEP,
  SIMIX_SYNC_SYNCHRO,
  SIMIX_SYNC_IO,
} e_smx_synchro_type_t;

typedef enum {
  SIMIX_COMM_SEND,
  SIMIX_COMM_RECEIVE,
  SIMIX_COMM_READY,
  SIMIX_COMM_DONE
} e_smx_comm_type_t;

typedef enum {
  SIMIX_IO_OPEN,
  SIMIX_IO_WRITE,
  SIMIX_IO_READ,
  SIMIX_IO_STAT
} e_smx_io_type_t;

/** @brief synchro datatype */
typedef struct s_smx_synchro {

  e_smx_synchro_type_t type;          /* Type of SIMIX synchro */
  e_smx_state_t state;               /* State of the synchro */
  char *name;                        /* synchro name if any */
  xbt_fifo_t simcalls;               /* List of simcalls waiting for this synchro */

  /* Data specific to each synchro type */
  union {

    struct {
      sg_host_t host;                /* The host where the execution takes place */
      surf_action_t surf_exec;        /* The Surf execution action encapsulated */
    } execution; /* Possibly parallel execution */

    struct {
      e_smx_comm_type_t type;         /* Type of the communication (SIMIX_COMM_SEND or SIMIX_COMM_RECEIVE) */
      smx_rdv_t rdv;                  /* Rendez-vous where the comm is queued */

#ifdef HAVE_MC
      smx_rdv_t rdv_cpy;              /* Copy of the rendez-vous where the comm is queued, MC needs it for DPOR
                                         (comm.rdv set to NULL when the communication is removed from the mailbox
                                         (used as garbage collector)) */
#endif
      int refcount;                   /* Number of processes involved in the cond */
      int detached;                   /* If detached or not */

      void (*clean_fun)(void*);       /* Function to clean the detached src_buf if something goes wrong */
      int (*match_fun)(void*,void*,smx_synchro_t);  /* Filter function used by the other side. It is used when
                                         looking if a given communication matches my needs. For that, myself must match the
                                         expectations of the other side, too. See  */
      void (*copy_data_fun) (smx_synchro_t, void*, size_t);

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
      unsigned copied:1;              /* whether the data were already copied */

      void* src_data;                 /* User data associated to communication */
      void* dst_data;
    } comm;

    struct {
      sg_host_t host;                /* The host that is sleeping */
      surf_action_t surf_sleep;       /* The Surf sleeping action encapsulated */
    } sleep;

    struct {
      surf_action_t sleep;
    } synchro;

    struct {
      sg_host_t host;
      surf_action_t surf_io;
    } io;
  };

#ifdef HAVE_LATENCY_BOUND_TRACKING
  int latency_limited;
#endif

  char *category;                     /* simix action category for instrumentation */
} s_smx_synchro_t;

XBT_PRIVATE void SIMIX_context_mod_init(void);
XBT_PRIVATE void SIMIX_context_mod_exit(void);

#ifndef WIN32
XBT_PUBLIC_DATA(char sigsegv_stack[SIGSTKSZ]);
#endif

/* We are using the bottom of the stack to save some information, like the
 * valgrind_stack_id. Define smx_context_usable_stack_size to give the remaining
 * size for the stack. */
#ifdef HAVE_VALGRIND_VALGRIND_H
# define smx_context_usable_stack_size                                  \
  (smx_context_stack_size - sizeof(unsigned int)) /* for valgrind_stack_id */
#else
# define smx_context_usable_stack_size smx_context_stack_size
#endif

XBT_PRIVATE void *SIMIX_context_stack_new(void);
XBT_PRIVATE void SIMIX_context_stack_delete(void *stack);

XBT_PRIVATE void SIMIX_context_set_current(smx_context_t context);
XBT_PRIVATE smx_context_t SIMIX_context_get_current(void);

/* All factories init */

XBT_PRIVATE void SIMIX_ctx_thread_factory_init(smx_context_factory_t *factory);
XBT_PRIVATE void SIMIX_ctx_sysv_factory_init(smx_context_factory_t *factory);
XBT_PRIVATE void SIMIX_ctx_raw_factory_init(smx_context_factory_t *factory);
XBT_PRIVATE void SIMIX_ctx_boost_factory_init(smx_context_factory_t *factory);

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
  if (!simix_global)
    xbt_die("simix is not initialized, please call MSG_init first");
  return simix_global->context_factory->create_context(code,
                                                       argc, argv,
                                                       cleanup_func,
                                                       simix_process);
}

/**
 * \brief destroy a context
 * \param context the context to destroy
 * Argument must be stopped first -- runs in maestro context
 */
static XBT_INLINE void SIMIX_context_free(smx_context_t context)
{
  simix_global->context_factory->free(context);
}

/**
 * \brief stops the execution of a context
 * \param context to stop
 */
static XBT_INLINE void SIMIX_context_stop(smx_context_t context)
{
  simix_global->context_factory->stop(context);
}

/**
 \brief suspends a context and return the control back to the one which
        scheduled it
 \param context the context to be suspended (it must be the running one)
 */
static XBT_INLINE void SIMIX_context_suspend(smx_context_t context)
{
  simix_global->context_factory->suspend(context);
}

/**
 \brief Executes all the processes to run (in parallel if possible).
 */
static XBT_INLINE void SIMIX_context_runall(void)
{
  if (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    simix_global->context_factory->runall();
  }
}

/**
 \brief returns the current running context
 */
static XBT_INLINE smx_context_t SIMIX_context_self(void)
{
  if (simix_global && simix_global->context_factory) {
    return simix_global->context_factory->self();
  }
  return NULL;
}

/**
 \brief returns the SIMIX process associated to a context
 \param context The context
 \return The SIMIX process
 */
static XBT_INLINE smx_process_t SIMIX_context_get_process(smx_context_t context)
{
  return simix_global->context_factory->get_process(context);
}

XBT_PUBLIC(int) SIMIX_process_get_maxpid(void);

XBT_PRIVATE void SIMIX_post_create_environment(void);

SG_END_DECL()

#endif
