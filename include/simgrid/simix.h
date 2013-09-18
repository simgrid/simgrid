/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SIMIX_H
#define _SIMIX_SIMIX_H

#include "xbt/misc.h"
#include "xbt/fifo.h"
#include "xbt/dict.h"
#include "xbt/function_types.h"
#include "xbt/parmap.h"
#include "xbt/swag.h"

SG_BEGIN_DECL()

/**************************** Scalar Values **********************************/

typedef union u_smx_scalar u_smx_scalar_t;

/* ******************************** Host ************************************ */
/** @brief Host datatype
    @ingroup simix_host_management

    A <em>location</em> (or <em>host</em>) is any possible place where
    a process may run. Thus it is represented as a <em>physical
    resource with computing capabilities</em>, some <em>mailboxes</em>
    to enable running process to communicate with remote ones, and
    some <em>private data</em> that can be only accessed by local
    process.

    \see m_host_management
  @{ */
typedef xbt_dictelm_t smx_host_t;
typedef struct s_smx_host_priv *smx_host_priv_t;
typedef enum {
  SIMIX_WAITING,
  SIMIX_READY,
  SIMIX_RUNNING,
  SIMIX_DONE,
  SIMIX_CANCELED,
  SIMIX_FAILED,
  SIMIX_SRC_HOST_FAILURE,
  SIMIX_DST_HOST_FAILURE,
  SIMIX_SRC_TIMEOUT,
  SIMIX_DST_TIMEOUT,
  SIMIX_LINK_FAILURE
} e_smx_state_t;
/** @} */


typedef struct s_smx_timer* smx_timer_t;

/* ******************************** Synchro ************************************ */
/**
 * \ingroup simix_synchro_management
 */
typedef struct s_smx_mutex *smx_mutex_t;
/**
 * \ingroup simix_synchro_management
 */
typedef struct s_smx_cond *smx_cond_t;
/**
 * \ingroup simix_synchro_management
 */
typedef struct s_smx_sem *smx_sem_t;

/********************************** File *************************************/

typedef struct s_smx_file *smx_file_t;

/********************************** Storage *************************************/
typedef xbt_dictelm_t smx_storage_t;
typedef struct s_smx_storage_priv *smx_storage_priv_t;

/********************************** Action *************************************/
typedef struct s_smx_action *smx_action_t; /* FIXME: replace by specialized action handlers */



/* ****************************** Process *********************************** */
/** @brief Process datatype
    @ingroup simix_process_management

    A processt may be defined as a <em>code</em>, with some <em>private
    data</em>, executing in a <em>location</em>.
    \see m_process_management
  @{ */
typedef struct s_smx_process *smx_process_t;
/** @} */


/*
 * Type of function that creates a process.
 * The function must accept the following parameters:
 * void* process: the process created will be stored there
 * const char *name: a name for the object. It is for user-level information and can be NULL
 * xbt_main_func_t code: is a function describing the behavior of the process
 * void *data: data a pointer to any data one may want to attach to the new object.
 * smx_host_t host: the location where the new process is executed
 * int argc, char **argv: parameters passed to code
 * xbt_dict_t pros: properties
 */
typedef void (*smx_creation_func_t) ( /* process */ smx_process_t*,
                                      /* name */ const char*,
                                      /* code */ xbt_main_func_t,
                                      /* userdata */ void*,
                                      /* hostname */ const char*,
                                      /* kill_time */ double,
                                      /* argc */ int,
                                      /* argv */ char**,
                                      /* props */ xbt_dict_t,
                                      /* auto_restart */ int);


/******************************* Networking ***********************************/
/**
 * \ingroup simix_rdv_management
 */
typedef struct s_smx_rvpoint *smx_rdv_t;

XBT_PUBLIC(void*) SIMIX_comm_get_src_data(smx_action_t action);
XBT_PUBLIC(void*) SIMIX_comm_get_dst_data(smx_action_t action);

/******************************** Context *************************************/
typedef struct s_smx_context *smx_context_t;
typedef struct s_smx_context_factory *smx_context_factory_t;

/* Process creation/destruction callbacks */
typedef void (*void_pfn_smxprocess_t) (smx_process_t);
/* Process kill */
typedef void (*void_pfn_smxprocess_t_smxprocess_t) (smx_process_t, smx_process_t);
/* for auto-restart function */
typedef void (*void_pfn_smxhost_t) (smx_host_t);

/* The following function pointer types describe the interface that any context
   factory should implement */


typedef smx_context_t(*smx_pfn_context_factory_create_context_t)
  (xbt_main_func_t, int, char **, void_pfn_smxprocess_t, void* data);
typedef int (*smx_pfn_context_factory_finalize_t) (smx_context_factory_t*);
typedef void (*smx_pfn_context_free_t) (smx_context_t);
typedef void (*smx_pfn_context_start_t) (smx_context_t);
typedef void (*smx_pfn_context_stop_t) (smx_context_t);
typedef void (*smx_pfn_context_suspend_t) (smx_context_t context);
typedef void (*smx_pfn_context_runall_t) (void);
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

/* Hack: let msg load directly the right factory */
typedef void (*smx_ctx_factory_initializer_t)(smx_context_factory_t*);
XBT_PUBLIC(smx_ctx_factory_initializer_t) smx_factory_initializer_to_use;
extern char* smx_context_factory_name;
extern int smx_context_stack_size;
extern int smx_context_stack_size_was_set;

/* *********************** */
/* Context type definition */
/* *********************** */
/* the following function pointers types describe the interface that all context
   concepts must implement */
/* each context type derive from this structure, so they must contain this structure
 * at their beginning -- OOP in C :/ */
typedef struct s_smx_context {
  s_xbt_swag_hookup_t hookup;
  xbt_main_func_t code;
  void_pfn_smxprocess_t cleanup_func;
  void *data;   /* Here SIMIX stores the smx_process_t containing the context */
  char **argv;
  int argc;
  unsigned iwannadie:1;
} s_smx_ctx_base_t;

/* methods of this class */
XBT_PUBLIC(void) smx_ctx_base_factory_init(smx_context_factory_t *factory);
XBT_PUBLIC(int) smx_ctx_base_factory_finalize(smx_context_factory_t *factory);

XBT_PUBLIC(smx_context_t)
smx_ctx_base_factory_create_context_sized(size_t size,
                                          xbt_main_func_t code, int argc,
                                          char **argv,
                                          void_pfn_smxprocess_t cleanup,
                                          void* data);
XBT_PUBLIC(void) smx_ctx_base_free(smx_context_t context);
XBT_PUBLIC(void) smx_ctx_base_stop(smx_context_t context);
XBT_PUBLIC(smx_context_t) smx_ctx_base_self(void);
XBT_PUBLIC(void) *smx_ctx_base_get_data(smx_context_t context);

XBT_PUBLIC(xbt_dynar_t) SIMIX_process_get_runnable(void);
XBT_PUBLIC(smx_process_t) SIMIX_process_from_PID(int PID);
XBT_PUBLIC(xbt_dynar_t) SIMIX_processes_as_dynar(void);

/* parallelism */
XBT_PUBLIC(int) SIMIX_context_is_parallel(void);
XBT_PUBLIC(int) SIMIX_context_get_nthreads(void);
XBT_PUBLIC(void) SIMIX_context_set_nthreads(int nb_threads);
XBT_PUBLIC(int) SIMIX_context_get_parallel_threshold(void);
XBT_PUBLIC(void) SIMIX_context_set_parallel_threshold(int threshold);
XBT_PUBLIC(e_xbt_parmap_mode_t) SIMIX_context_get_parallel_mode(void);
XBT_PUBLIC(void) SIMIX_context_set_parallel_mode(e_xbt_parmap_mode_t mode);



/********************************** Global ************************************/
/* Initialization and exit */
XBT_PUBLIC(void) SIMIX_global_init(int *argc, char **argv);


XBT_PUBLIC(void) SIMIX_function_register_process_cleanup(void_pfn_smxprocess_t function);
XBT_PUBLIC(void) SIMIX_function_register_process_create(smx_creation_func_t function);
XBT_PUBLIC(void) SIMIX_function_register_process_kill(void_pfn_smxprocess_t_smxprocess_t function);

/* Simulation execution */
XBT_PUBLIC(void) SIMIX_run(void);    
XBT_PUBLIC(double) SIMIX_get_clock(void);

/* Timer functions FIXME: should these be public? */
XBT_PUBLIC(void) SIMIX_timer_set(double date, void *function, void *arg);
XBT_PUBLIC(double) SIMIX_timer_next(void);

XBT_PUBLIC(void) SIMIX_display_process_status(void);

/******************************* Environment **********************************/
XBT_PUBLIC(void) SIMIX_create_environment(const char *file);

/******************************** Deployment **********************************/

XBT_PUBLIC(void) SIMIX_function_register(const char *name, xbt_main_func_t code);
XBT_PUBLIC(void) SIMIX_function_register_default(xbt_main_func_t code);
XBT_PUBLIC(xbt_main_func_t) SIMIX_get_registered_function(const char *name);
XBT_PUBLIC(void) SIMIX_init_application(void);
XBT_PUBLIC(void) SIMIX_launch_application(const char *file);

XBT_PUBLIC(void) SIMIX_process_set_function(const char* process_host,
                                            const char *process_function,
                                            xbt_dynar_t arguments,
                                            double process_start_time,
                                            double process_kill_time);

/*********************************** Host *************************************/
//XBT_PUBLIC(xbt_dict_t) SIMIX_host_get_dict(u_smx_scalar_t *args);
XBT_PUBLIC(smx_host_t) SIMIX_host_get_by_name(const char *name);
XBT_PUBLIC(smx_host_t) SIMIX_host_self(void);
XBT_PUBLIC(const char*) SIMIX_host_self_get_name(void);
XBT_PUBLIC(const char*) SIMIX_host_get_name(smx_host_t host); /* FIXME: make private: only the name of SIMIX_host_self() should be public without request */
XBT_PUBLIC(void) SIMIX_host_self_set_data(void *data);
XBT_PUBLIC(void*) SIMIX_host_self_get_data(void);
XBT_PUBLIC(void*) SIMIX_host_get_data(smx_host_t host);
XBT_PUBLIC(void) SIMIX_host_set_data(smx_host_t host, void *data);
XBT_PUBLIC(xbt_dynar_t) SIMIX_host_get_storage_list(smx_host_t host);
XBT_PUBLIC(const char*) SIMIX_storage_get_name(smx_host_t host);

/********************************* Process ************************************/
XBT_PUBLIC(int) SIMIX_process_count(void);
XBT_PUBLIC(smx_process_t) SIMIX_process_self(void);
XBT_PUBLIC(const char*) SIMIX_process_self_get_name(void);
XBT_PUBLIC(void) SIMIX_process_self_set_data(smx_process_t self, void *data);
XBT_PUBLIC(void*) SIMIX_process_self_get_data(smx_process_t self);
XBT_PUBLIC(smx_context_t) SIMIX_process_get_context(smx_process_t);
XBT_PUBLIC(void) SIMIX_process_set_context(smx_process_t p,smx_context_t c);
XBT_PUBLIC(int) SIMIX_process_has_pending_comms(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_on_exit_runall(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_on_exit(smx_process_t process, int_f_pvoid_t fun, void *data);

/****************************** Communication *********************************/
XBT_PUBLIC(void) SIMIX_comm_set_copy_data_callback(void (*callback) (smx_action_t, void*, size_t));
XBT_PUBLIC(void) SIMIX_comm_copy_pointer_callback(smx_action_t comm, void* buff, size_t buff_size);
XBT_PUBLIC(void) SIMIX_comm_copy_buffer_callback(smx_action_t comm, void* buff, size_t buff_size);

XBT_PUBLIC(smx_action_t) SIMIX_comm_get_send_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(int) SIMIX_comm_has_send_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(int) SIMIX_comm_has_recv_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(void) SIMIX_comm_finish(smx_action_t action);

/*********************************** File *************************************/
XBT_PUBLIC(void*) SIMIX_file_get_data(smx_file_t fd);
XBT_PUBLIC(void) SIMIX_file_set_data(smx_file_t fd, void *data);

/******************************************************************************/
/*                            SIMIX simcalls                                  */
/******************************************************************************/
/* These functions are a system call-like interface to the simulation kernel. */
/* They can also be called from maestro's context, and they are thread safe.  */
/******************************************************************************/

/******************************* Host simcalls ********************************/
/* TODO use handlers and keep smx_host_t hidden from higher levels */
XBT_PUBLIC(smx_host_t) simcall_host_get_by_name(const char *name);
XBT_PUBLIC(const char *) simcall_host_get_name(smx_host_t host);
XBT_PUBLIC(xbt_dict_t) simcall_host_get_properties(smx_host_t host);
XBT_PUBLIC(int) simcall_host_get_core(smx_host_t host);
XBT_PUBLIC(xbt_swag_t) simcall_host_get_process_list(smx_host_t host);
XBT_PUBLIC(double) simcall_host_get_speed(smx_host_t host);
XBT_PUBLIC(double) simcall_host_get_available_speed(smx_host_t host);
/* Two possible states, 1 - CPU ON and 0 CPU OFF */
XBT_PUBLIC(int) simcall_host_get_state(smx_host_t host);
XBT_PUBLIC(void *) simcall_host_get_data(smx_host_t host);

XBT_PUBLIC(void) simcall_host_set_data(smx_host_t host, void *data);

XBT_PUBLIC(double) simcall_host_get_current_power_peak(smx_host_t host);
XBT_PUBLIC(double) simcall_host_get_power_peak_at(smx_host_t host, int pstate_index);
XBT_PUBLIC(int) simcall_host_get_nb_pstates(smx_host_t host);
XBT_PUBLIC(void) simcall_host_set_power_peak_at(smx_host_t host, int pstate_index);
XBT_PUBLIC(double) simcall_host_get_consumed_energy(smx_host_t host);

XBT_PUBLIC(smx_action_t) simcall_host_execute(const char *name, smx_host_t host,
                                                double computation_amount,
                                                double priority);
XBT_PUBLIC(smx_action_t) simcall_host_parallel_execute(const char *name,
                                                     int host_nb,
                                                     smx_host_t *host_list,
                                                     double *computation_amount,
                                                     double *communication_amount,
                                                     double amount,
                                                     double rate);
XBT_PUBLIC(void) simcall_host_execution_destroy(smx_action_t execution);
XBT_PUBLIC(void) simcall_host_execution_cancel(smx_action_t execution);
XBT_PUBLIC(double) simcall_host_execution_get_remains(smx_action_t execution);
XBT_PUBLIC(e_smx_state_t) simcall_host_execution_get_state(smx_action_t execution);
XBT_PUBLIC(void) simcall_host_execution_set_priority(smx_action_t execution, double priority);
XBT_PUBLIC(e_smx_state_t) simcall_host_execution_wait(smx_action_t execution);
XBT_PUBLIC(xbt_dynar_t) simcall_host_get_storage_list(smx_host_t host);

/**************************** Process simcalls ********************************/
/* Constructor and Destructor */
XBT_PUBLIC(void) simcall_process_create(smx_process_t *process,
                                          const char *name,
                                          xbt_main_func_t code,
                                          void *data,
                                          const char *hostname,
                                          double kill_time,
                                          int argc, char **argv,
                                          xbt_dict_t properties,
                                          int auto_restart);

XBT_PUBLIC(void) simcall_process_kill(smx_process_t process);
XBT_PUBLIC(void) simcall_process_killall(int reset_pid);

/* Process handling */
XBT_PUBLIC(void) simcall_process_cleanup(smx_process_t process);
XBT_PUBLIC(void) simcall_process_change_host(smx_process_t process,
                 smx_host_t dest);
XBT_PUBLIC(void) simcall_process_suspend(smx_process_t process);
XBT_PUBLIC(void) simcall_process_resume(smx_process_t process);

/* Getters and Setters */
XBT_PUBLIC(int) simcall_process_count(void);
XBT_PUBLIC(void *) simcall_process_get_data(smx_process_t process);
XBT_PUBLIC(void) simcall_process_set_data(smx_process_t process, void *data);
XBT_PUBLIC(smx_host_t) simcall_process_get_host(smx_process_t process);
XBT_PUBLIC(const char *) simcall_process_get_name(smx_process_t process);
XBT_PUBLIC(int) simcall_process_get_PID(smx_process_t process);
XBT_PUBLIC(int) simcall_process_get_PPID(smx_process_t process);
XBT_PUBLIC(int) simcall_process_is_suspended(smx_process_t process);
XBT_PUBLIC(xbt_dict_t) simcall_process_get_properties(smx_process_t host);
XBT_PUBLIC(void) simcall_process_set_kill_time(smx_process_t process, double kill_time);
XBT_PUBLIC(void) simcall_process_on_exit(smx_process_t process, int_f_pvoid_t fun, void *data);
XBT_PUBLIC(void) simcall_process_auto_restart_set(smx_process_t process, int auto_restart);
XBT_PUBLIC(smx_process_t) simcall_process_restart(smx_process_t process);
/* Sleep control */
XBT_PUBLIC(e_smx_state_t) simcall_process_sleep(double duration);

/************************** Comunication simcalls *****************************/
/***** Rendez-vous points *****/

XBT_PUBLIC(smx_rdv_t) simcall_rdv_create(const char *name);
XBT_PUBLIC(void) simcall_rdv_destroy(smx_rdv_t rvp);
XBT_PUBLIC(smx_rdv_t) simcall_rdv_get_by_name(const char *name);
XBT_PUBLIC(int) simcall_rdv_comm_count_by_host(smx_rdv_t rdv, smx_host_t host);
XBT_PUBLIC(smx_action_t) simcall_rdv_get_head(smx_rdv_t rdv);
XBT_PUBLIC(smx_process_t) simcall_rdv_get_receiver(smx_rdv_t rdv);
XBT_PUBLIC(void) simcall_rdv_set_receiver(smx_rdv_t rdv , smx_process_t process);

XBT_PUBLIC(xbt_dict_t) SIMIX_get_rdv_points(void);

/***** Communication simcalls *****/

XBT_PUBLIC(void) simcall_comm_send(smx_rdv_t rdv, double task_size,
                                     double rate, void *src_buff,
                                     size_t src_buff_size,
                                     int (*match_fun)(void *, void *, smx_action_t),
                                     void *data, double timeout);

XBT_PUBLIC(smx_action_t) simcall_comm_isend(smx_rdv_t rdv, double task_size,
                                              double rate, void *src_buff,
                                              size_t src_buff_size,
                                              int (*match_fun)(void *, void *, smx_action_t),
                                              void (*clean_fun)(void *),
                                              void *data, int detached);

XBT_PUBLIC(void) simcall_comm_recv(smx_rdv_t rdv, void *dst_buff,
                                     size_t * dst_buff_size,
                                     int (*match_fun)(void *, void *, smx_action_t),
                                     void *data, double timeout);

XBT_PUBLIC(smx_action_t) simcall_comm_irecv(smx_rdv_t rdv, void *dst_buff,
                                              size_t * dst_buff_size,
                                              int (*match_fun)(void *, void *, smx_action_t),
                                              void *data);

XBT_PUBLIC(void) simcall_comm_recv_bounded(smx_rdv_t rdv, void *dst_buff,
                                     size_t * dst_buff_size,
                                     int (*match_fun)(void *, void *, smx_action_t),
                                     void *data, double timeout, double rate);

XBT_PUBLIC(smx_action_t) simcall_comm_irecv_bounded(smx_rdv_t rdv, void *dst_buff,
                                              size_t * dst_buff_size,
                                              int (*match_fun)(void *, void *, smx_action_t),
                                              void *data, double rate);

XBT_PUBLIC(void) simcall_comm_destroy(smx_action_t comm);
XBT_PUBLIC(smx_action_t) simcall_comm_iprobe(smx_rdv_t rdv, int src, int tag,
                                int (*match_fun)(void *, void *, smx_action_t), void *data);
XBT_PUBLIC(void) simcall_comm_cancel(smx_action_t comm);

/* FIXME: waitany is going to be a vararg function, and should take a timeout */
XBT_PUBLIC(unsigned int) simcall_comm_waitany(xbt_dynar_t comms);
XBT_PUBLIC(void) simcall_comm_wait(smx_action_t comm, double timeout);
XBT_PUBLIC(int) simcall_comm_test(smx_action_t comm);
XBT_PUBLIC(int) simcall_comm_testany(xbt_dynar_t comms);

/* Getters and setters */
XBT_PUBLIC(double) simcall_comm_get_remains(smx_action_t comm);
XBT_PUBLIC(e_smx_state_t) simcall_comm_get_state(smx_action_t comm);
XBT_PUBLIC(void *) simcall_comm_get_src_data(smx_action_t comm);
XBT_PUBLIC(void *) simcall_comm_get_dst_data(smx_action_t comm);
XBT_PUBLIC(smx_process_t) simcall_comm_get_src_proc(smx_action_t comm);
XBT_PUBLIC(smx_process_t) simcall_comm_get_dst_proc(smx_action_t comm);

#ifdef HAVE_LATENCY_BOUND_TRACKING
XBT_PUBLIC(int) simcall_comm_is_latency_bounded(smx_action_t comm);
#endif

#ifdef HAVE_TRACING
/************************** Tracing handling **********************************/
XBT_PUBLIC(void) simcall_set_category(smx_action_t action, const char *category);
#endif

/************************** Synchro simcalls **********************************/

XBT_PUBLIC(smx_mutex_t) simcall_mutex_init(void);
XBT_PUBLIC(void) simcall_mutex_destroy(smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_mutex_lock(smx_mutex_t mutex);
XBT_PUBLIC(int) simcall_mutex_trylock(smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_mutex_unlock(smx_mutex_t mutex);

XBT_PUBLIC(smx_cond_t) simcall_cond_init(void);
XBT_PUBLIC(void) simcall_cond_destroy(smx_cond_t cond);
XBT_PUBLIC(void) simcall_cond_signal(smx_cond_t cond);
XBT_PUBLIC(void) simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_cond_wait_timeout(smx_cond_t cond,
                                         smx_mutex_t mutex,
                                         double max_duration);
XBT_PUBLIC(void) simcall_cond_broadcast(smx_cond_t cond);

XBT_PUBLIC(smx_sem_t) simcall_sem_init(int capacity);
XBT_PUBLIC(void) simcall_sem_destroy(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_release(smx_sem_t sem);
XBT_PUBLIC(int) simcall_sem_would_block(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_acquire(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_acquire_timeout(smx_sem_t sem,
                                             double max_duration);
XBT_PUBLIC(int) simcall_sem_get_capacity(smx_sem_t sem);

/*****************************   File   **********************************/
XBT_PUBLIC(void *) simcall_file_get_data(smx_file_t fd);
XBT_PUBLIC(void) simcall_file_set_data(smx_file_t fd, void *data);
XBT_PUBLIC(size_t) simcall_file_read(size_t size, smx_file_t fd);
XBT_PUBLIC(size_t) simcall_file_write(size_t size, smx_file_t fd);
XBT_PUBLIC(smx_file_t) simcall_file_open(const char* storage, const char* path);
XBT_PUBLIC(int) simcall_file_close(smx_file_t fd);
XBT_PUBLIC(int) simcall_file_unlink(smx_file_t fd);
XBT_PUBLIC(xbt_dict_t) simcall_file_ls(const char* mount, const char* path);
XBT_PUBLIC(size_t) simcall_file_get_size(smx_file_t fd);
XBT_PUBLIC(xbt_dynar_t) simcall_file_get_info(smx_file_t fd);

/*****************************   Storage   **********************************/
XBT_PUBLIC(size_t) simcall_storage_get_free_size (const char* name);
XBT_PUBLIC(size_t) simcall_storage_get_used_size (const char* name);
XBT_PUBLIC(xbt_dict_t) simcall_storage_get_properties(smx_storage_t storage);
XBT_PUBLIC(void*) SIMIX_storage_get_data(smx_storage_t storage);
XBT_PUBLIC(void) SIMIX_storage_set_data(smx_storage_t storage, void *data);

/************************** AS router   **********************************/
XBT_PUBLIC(xbt_dict_t) SIMIX_asr_get_properties(const char *name);
/************************** AS router simcalls ***************************/
XBT_PUBLIC(xbt_dict_t) simcall_asr_get_properties(const char *name);

/************************** MC simcalls   **********************************/
XBT_PUBLIC(void *) simcall_mc_snapshot(void);
XBT_PUBLIC(int) simcall_mc_compare_snapshots(void *s1, void *s2);
XBT_PUBLIC(int) simcall_mc_random(int min, int max);

/************************** New API simcalls **********************************/
/* TUTORIAL: New API                                                          */
/******************************************************************************/
XBT_PUBLIC(int) simcall_new_api_fct(const char* param1, double param2);

SG_END_DECL()
#endif                          /* _SIMIX_SIMIX_H */
