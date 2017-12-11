/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SIMIX_H
#define _SIMIX_SIMIX_H

#include "simgrid/datatypes.h"
#include "simgrid/forward.h"
#include "simgrid/host.h"
#include "xbt/ex.h"
#include "xbt/parmap.h"
#ifdef __cplusplus
#include <map>
#endif

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
typedef enum {
  SIMIX_WAITING,
  SIMIX_READY,
  SIMIX_RUNNING,
  SIMIX_DONE,
  SIMIX_CANCELED,
  SIMIX_FAILED,
  SIMIX_SRC_HOST_FAILURE,
  SIMIX_DST_HOST_FAILURE,
  SIMIX_TIMEOUT,
  SIMIX_SRC_TIMEOUT,
  SIMIX_DST_TIMEOUT,
  SIMIX_LINK_FAILURE
} e_smx_state_t;
/** @} */

/* ******************************** Synchro ************************************ */

/**
 * \ingroup simix_synchro_management
 */
typedef struct s_smx_cond_t* smx_cond_t;
/**
 * \ingroup simix_synchro_management
 */
typedef struct s_smx_sem_t* smx_sem_t;

/* ****************************** Process *********************************** */

typedef enum {
  SMX_EXIT_SUCCESS = 0,
  SMX_EXIT_FAILURE = 1
} smx_process_exit_status_t;
/** @} */

/******************************* Networking ***********************************/

/* Process creation/destruction callbacks */
typedef void (*void_pfn_smxprocess_t) (smx_actor_t);

extern unsigned smx_context_stack_size;
extern int smx_context_stack_size_was_set;
extern unsigned smx_context_guard_size;
extern int smx_context_guard_size_was_set;

SG_BEGIN_DECL()

XBT_PUBLIC(smx_actor_t) SIMIX_process_from_PID(aid_t PID);

/* parallelism */
XBT_PUBLIC(int) SIMIX_context_is_parallel();
XBT_PUBLIC(int) SIMIX_context_get_nthreads();
XBT_PUBLIC(void) SIMIX_context_set_nthreads(int nb_threads);
XBT_PUBLIC(int) SIMIX_context_get_parallel_threshold();
XBT_PUBLIC(void) SIMIX_context_set_parallel_threshold(int threshold);
XBT_PUBLIC(e_xbt_parmap_mode_t) SIMIX_context_get_parallel_mode();
XBT_PUBLIC(void) SIMIX_context_set_parallel_mode(e_xbt_parmap_mode_t mode);
XBT_PUBLIC(int) SIMIX_is_maestro();


/********************************** Global ************************************/
/* Initialization and exit */
XBT_PUBLIC(void) SIMIX_global_init(int *argc, char **argv);

/* Set to execute in the maestro
 *
 * If no maestro code is registered (the default), the main thread
 * is assumed to be the maestro. */
XBT_PUBLIC(void) SIMIX_set_maestro(void (*code)(void*), void* data);

XBT_PUBLIC(void) SIMIX_function_register_process_cleanup(void_pfn_smxprocess_t function);
XBT_PUBLIC(void) SIMIX_function_register_process_kill(void_pfn_smxprocess_t function);

/* Simulation execution */
XBT_PUBLIC(void) SIMIX_run();
XBT_PUBLIC(double) SIMIX_get_clock();

/* Timer functions FIXME: should these be public? */
typedef struct s_smx_timer_t* smx_timer_t;

XBT_PUBLIC(smx_timer_t) SIMIX_timer_set(double date, void (*function)(void*), void *arg);
XBT_PUBLIC(void) SIMIX_timer_remove(smx_timer_t timer);
XBT_PUBLIC(double) SIMIX_timer_next();
XBT_PUBLIC(double) SIMIX_timer_get_date(smx_timer_t timer);

XBT_PUBLIC(void) SIMIX_display_process_status();

/******************************* Environment **********************************/
XBT_PUBLIC(void) SIMIX_create_environment(const char *file);

/******************************** Deployment **********************************/

XBT_PUBLIC(void) SIMIX_function_register(const char *name, xbt_main_func_t code);
XBT_PUBLIC(void) SIMIX_function_register_default(xbt_main_func_t code);
XBT_PUBLIC(void) SIMIX_init_application();
XBT_PUBLIC(void) SIMIX_launch_application(const char *file);

XBT_PUBLIC(void) SIMIX_process_set_function(const char* process_host,
                                            const char *process_function,
                                            xbt_dynar_t arguments,
                                            double process_start_time,
                                            double process_kill_time);

/*********************************** Host *************************************/
/* Functions for running a process in main()
 *
 *  1. create the maestro process
 *  2. attach (create a context and wait for maestro to give control back to you)
 *  3. do you process job
 *  4. detach (this waits for the simulation to terminate)
 */

XBT_PUBLIC(void) SIMIX_maestro_create(void (*code)(void*), void* data);
#ifdef __cplusplus
XBT_PUBLIC(smx_actor_t)
SIMIX_process_attach(const char* name, void* data, const char* hostname, std::map<std::string, std::string>* properties,
                     smx_actor_t parent_process);
#endif
XBT_PUBLIC(void) SIMIX_process_detach();

/*********************************** Host *************************************/
XBT_PUBLIC(void) SIMIX_host_off(sg_host_t host, smx_actor_t issuer);
XBT_PUBLIC(void) SIMIX_host_self_set_data(void *data);
XBT_PUBLIC(void*) SIMIX_host_self_get_data();

/********************************* Process ************************************/
XBT_PUBLIC(int) SIMIX_process_count();
XBT_PUBLIC(smx_actor_t) SIMIX_process_self();
XBT_PUBLIC(const char*) SIMIX_process_self_get_name();
XBT_PUBLIC(void) SIMIX_process_self_set_data(void *data);
XBT_PUBLIC(void*) SIMIX_process_self_get_data();
XBT_PUBLIC(int) SIMIX_process_has_pending_comms(smx_actor_t process);
XBT_PUBLIC(void) SIMIX_process_on_exit_runall(smx_actor_t process);
XBT_PUBLIC(void) SIMIX_process_on_exit(smx_actor_t process, int_f_pvoid_pvoid_t fun, void *data);

SG_END_DECL()

/****************************** Communication *********************************/
XBT_PUBLIC(void) SIMIX_comm_set_copy_data_callback(void (*callback) (smx_activity_t, void*, size_t));
XBT_PUBLIC(void) SIMIX_comm_copy_pointer_callback(smx_activity_t comm, void* buff, size_t buff_size);
XBT_PUBLIC(void) SIMIX_comm_copy_buffer_callback(smx_activity_t comm, void* buff, size_t buff_size);

XBT_PUBLIC(smx_activity_t) SIMIX_comm_get_send_match(smx_mailbox_t mbox, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(int) SIMIX_comm_has_send_match(smx_mailbox_t mbox, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(int) SIMIX_comm_has_recv_match(smx_mailbox_t mbox, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(void) SIMIX_comm_finish(smx_activity_t synchro);

/******************************************************************************/
/*                            SIMIX simcalls                                  */
/******************************************************************************/
/* These functions are a system call-like interface to the simulation kernel. */
/* They can also be called from maestro's context, and they are thread safe.  */
/******************************************************************************/

XBT_PUBLIC(void) simcall_call(smx_actor_t process);

/******************************* Host simcalls ********************************/
XBT_PUBLIC(void) simcall_host_set_data(sg_host_t host, void *data);
XBT_PUBLIC(smx_activity_t)
simcall_execution_start(const char* name, double flops_amount, double priority, double bound, sg_host_t host);
XBT_PUBLIC(smx_activity_t)
simcall_execution_parallel_start(const char* name, int host_nb, sg_host_t* host_list, double* flops_amount,
                                 double* bytes_amount, double rate, double timeout);
XBT_PUBLIC(void) simcall_execution_cancel(smx_activity_t execution);
XBT_PUBLIC(void) simcall_execution_set_priority(smx_activity_t execution, double priority);
XBT_PUBLIC(void) simcall_execution_set_bound(smx_activity_t execution, double bound);
XBT_PUBLIC(e_smx_state_t) simcall_execution_wait(smx_activity_t execution);
XBT_PUBLIC(e_smx_state_t) simcall_execution_test(smx_activity_t execution);

/**************************** Process simcalls ********************************/
SG_BEGIN_DECL()
/* Constructor and Destructor */
#ifdef __cplusplus
XBT_PUBLIC(smx_actor_t)
simcall_process_create(const char* name, xbt_main_func_t code, void* data, sg_host_t host, int argc, char** argv,
                       std::map<std::string, std::string>* properties);
#endif

XBT_PUBLIC(void) simcall_process_killall(int reset_pid);
XBT_PUBLIC(void) SIMIX_process_throw(smx_actor_t process, xbt_errcat_t cat, int value, const char *msg);


/* Process handling */
XBT_PUBLIC(void) simcall_process_cleanup(smx_actor_t process);
XBT_PUBLIC(void) simcall_process_suspend(smx_actor_t process);

/* Getters and Setters */
XBT_PUBLIC(int) simcall_process_count();
XBT_PUBLIC(void) simcall_process_set_data(smx_actor_t process, void *data);
XBT_PUBLIC(void) simcall_process_set_kill_time(smx_actor_t process, double kill_time);
XBT_PUBLIC(void) simcall_process_on_exit(smx_actor_t process, int_f_pvoid_pvoid_t fun, void *data);
XBT_PUBLIC(void) simcall_process_join(smx_actor_t process, double timeout);
/* Sleep control */
XBT_PUBLIC(e_smx_state_t) simcall_process_sleep(double duration);
SG_END_DECL()

/************************** Comunication simcalls *****************************/

#ifdef __cplusplus
XBT_PUBLIC(void)
simcall_comm_send(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                  size_t src_buff_size, int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                  void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data, double timeout);

XBT_PUBLIC(smx_activity_t)
simcall_comm_isend(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                   size_t src_buff_size, int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*clean_fun)(void*), void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data,
                   int detached);

XBT_PUBLIC(void)
simcall_comm_recv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                  int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                  void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data, double timeout, double rate);

XBT_PUBLIC(smx_activity_t)
simcall_comm_irecv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                   int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data, double rate);

XBT_PUBLIC(smx_activity_t)
simcall_comm_iprobe(smx_mailbox_t mbox, int type, int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                    void* data);
#endif
XBT_PUBLIC(void) simcall_comm_cancel(smx_activity_t comm);

/* FIXME: waitany is going to be a vararg function, and should take a timeout */
XBT_PUBLIC(unsigned int) simcall_comm_waitany(xbt_dynar_t comms, double timeout);
XBT_PUBLIC(void) simcall_comm_wait(smx_activity_t comm, double timeout);
XBT_PUBLIC(int) simcall_comm_test(smx_activity_t comm);
XBT_PUBLIC(int) simcall_comm_testany(smx_activity_t* comms, size_t count);

/************************** Tracing handling **********************************/
XBT_PUBLIC(void) simcall_set_category(smx_activity_t synchro, const char *category);

/************************** Synchro simcalls **********************************/
SG_BEGIN_DECL()
XBT_PUBLIC(smx_mutex_t) simcall_mutex_init();
XBT_PUBLIC(smx_mutex_t) SIMIX_mutex_ref(smx_mutex_t mutex);
XBT_PUBLIC(void) SIMIX_mutex_unref(smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_mutex_lock(smx_mutex_t mutex);
XBT_PUBLIC(int) simcall_mutex_trylock(smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_mutex_unlock(smx_mutex_t mutex);

XBT_PUBLIC(smx_cond_t) simcall_cond_init();
XBT_PUBLIC(void) SIMIX_cond_unref(smx_cond_t cond);
XBT_PUBLIC(smx_cond_t) SIMIX_cond_ref(smx_cond_t cond);
XBT_PUBLIC(void) simcall_cond_signal(smx_cond_t cond);
XBT_PUBLIC(void) simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_cond_wait_timeout(smx_cond_t cond, smx_mutex_t mutex, double max_duration);
XBT_PUBLIC(void) simcall_cond_broadcast(smx_cond_t cond);

XBT_PUBLIC(void) SIMIX_sem_destroy(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_acquire(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_acquire_timeout(smx_sem_t sem, double max_duration);

/*****************************   Storage   **********************************/
XBT_PUBLIC(sg_size_t) simcall_storage_read(surf_storage_t st, sg_size_t size);
XBT_PUBLIC(sg_size_t) simcall_storage_write(surf_storage_t fd, sg_size_t size);
/************************** MC simcalls   **********************************/
XBT_PUBLIC(int) simcall_mc_random(int min, int max);

SG_END_DECL()

#endif                          /* _SIMIX_SIMIX_H */
