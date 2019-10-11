/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_H
#define SIMGRID_SIMIX_H

#include <simgrid/forward.h>
#include <simgrid/host.h>
#include <xbt/ex.h>
#include <xbt/parmap.h>
#ifdef __cplusplus
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
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

    @see m_host_management
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

/* ****************************** Process *********************************** */

typedef enum {
  SMX_EXIT_SUCCESS = 0,
  SMX_EXIT_FAILURE = 1
} smx_process_exit_status_t;
/** @} */

/******************************* Networking ***********************************/
extern unsigned smx_context_stack_size;
extern unsigned smx_context_guard_size;

SG_BEGIN_DECL()

XBT_PUBLIC smx_actor_t SIMIX_process_from_PID(aid_t PID);

/* parallelism */
XBT_PUBLIC int SIMIX_context_is_parallel();
XBT_PUBLIC int SIMIX_context_get_nthreads();
XBT_PUBLIC void SIMIX_context_set_nthreads(int nb_threads);
XBT_PUBLIC int SIMIX_context_get_parallel_threshold();
XBT_PUBLIC void SIMIX_context_set_parallel_threshold(int threshold);
XBT_PUBLIC e_xbt_parmap_mode_t SIMIX_context_get_parallel_mode();
XBT_PUBLIC void SIMIX_context_set_parallel_mode(e_xbt_parmap_mode_t mode);
XBT_PUBLIC int SIMIX_is_maestro();

/********************************** Global ************************************/
/* Initialization and exit */
XBT_PUBLIC void SIMIX_global_init(int* argc, char** argv);

/* Set to execute in the maestro
 *
 * If no maestro code is registered (the default), the main thread
 * is assumed to be the maestro. */
XBT_PUBLIC void SIMIX_set_maestro(void (*code)(void*), void* data);

/* Simulation execution */
XBT_PUBLIC void SIMIX_run();
XBT_PUBLIC double SIMIX_get_clock();

XBT_PUBLIC smx_timer_t SIMIX_timer_set(double date, void (*function)(void*), void* arg);
XBT_PUBLIC void SIMIX_timer_remove(smx_timer_t timer);
XBT_PUBLIC double SIMIX_timer_next();
XBT_PUBLIC double SIMIX_timer_get_date(smx_timer_t timer);

XBT_PUBLIC void SIMIX_display_process_status();
SG_END_DECL()

/******************************** Deployment **********************************/
SG_BEGIN_DECL()
XBT_PUBLIC void SIMIX_function_register_default(xbt_main_func_t code);

XBT_PUBLIC void SIMIX_init_application();
XBT_PUBLIC void SIMIX_process_set_function(const char* process_host, const char* process_function,
                                           xbt_dynar_t arguments, double process_start_time, double process_kill_time);
SG_END_DECL()

#ifdef __cplusplus
XBT_PUBLIC void SIMIX_function_register(const std::string& name, void (*code)(std::vector<std::string>));
XBT_PUBLIC void SIMIX_function_register(const std::string& name, xbt_main_func_t code);
XBT_PUBLIC void SIMIX_launch_application(const std::string& file);
#endif

/********************************* Process ************************************/
SG_BEGIN_DECL()
XBT_PUBLIC int SIMIX_process_count();
XBT_PUBLIC smx_actor_t SIMIX_process_self();
XBT_PUBLIC const char* SIMIX_process_self_get_name();
XBT_PUBLIC void SIMIX_process_self_set_data(void* data);
XBT_PUBLIC void* SIMIX_process_self_get_data();
SG_END_DECL()

#ifdef __cplusplus
XBT_PUBLIC void SIMIX_process_on_exit(smx_actor_t process, const std::function<void(bool /*failed*/)>& fun);
#endif

/****************************** Communication *********************************/
#ifdef __cplusplus
XBT_PUBLIC void SIMIX_comm_set_copy_data_callback(void (*callback)(simgrid::kernel::activity::CommImpl*, void*,
                                                                   size_t));
XBT_PUBLIC void SIMIX_comm_copy_pointer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff,
                                                 size_t buff_size);
XBT_PUBLIC void SIMIX_comm_copy_buffer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff,
                                                size_t buff_size);
#endif

/******************************************************************************/
/*                            SIMIX simcalls                                  */
/******************************************************************************/
/* These functions are a system call-like interface to the simulation kernel. */
/* They can also be called from maestro's context, and they are thread safe.  */
/******************************************************************************/

/******************************* Host simcalls ********************************/
#ifdef __cplusplus
XBT_PUBLIC e_smx_state_t simcall_execution_wait(const smx_activity_t& execution);
XBT_PUBLIC unsigned int simcall_execution_waitany_for(simgrid::kernel::activity::ExecImpl* execs[], size_t count,
                                                      double timeout);
XBT_PUBLIC bool simcall_execution_test(const smx_activity_t& execution);
#endif

/**************************** Process simcalls ********************************/
SG_BEGIN_DECL()
void simcall_process_set_data(smx_actor_t process, void* data);
XBT_ATTRIB_DEPRECATED_v328("Please use Actor::suspend()") XBT_PUBLIC void simcall_process_suspend(smx_actor_t process);

XBT_ATTRIB_DEPRECATED_v328("Please use Actor::join()") XBT_PUBLIC
    void simcall_process_join(smx_actor_t process, double timeout);

/* Sleep control */
XBT_PUBLIC e_smx_state_t simcall_process_sleep(double duration);
SG_END_DECL()

/************************** Comunication simcalls *****************************/

#ifdef __cplusplus
XBT_PUBLIC void simcall_comm_send(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                                  size_t src_buff_size,
                                  int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                  void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                  void* data, double timeout);

XBT_PUBLIC smx_activity_t simcall_comm_isend(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate,
                                             void* src_buff, size_t src_buff_size,
                                             int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                             void (*clean_fun)(void*),
                                             void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                             void* data, bool detached);

XBT_PUBLIC void simcall_comm_recv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                                  int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                  void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                  void* data, double timeout, double rate);

XBT_PUBLIC smx_activity_t simcall_comm_irecv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff,
                                             size_t* dst_buff_size,
                                             int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                             void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                             void* data, double rate);

XBT_PUBLIC smx_activity_t simcall_comm_iprobe(smx_mailbox_t mbox, int type,
                                              int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                              void* data);

/* FIXME: waitany is going to be a vararg function, and should take a timeout */
XBT_PUBLIC unsigned int simcall_comm_waitany(smx_activity_t comms[], size_t count, double timeout);
XBT_PUBLIC unsigned int simcall_comm_waitany(simgrid::kernel::activity::CommImpl* comms[], size_t count,
                                             double timeout);
XBT_PUBLIC void simcall_comm_wait(const smx_activity_t& comm, double timeout);
XBT_PUBLIC bool simcall_comm_test(const smx_activity_t& comm);
XBT_PUBLIC int simcall_comm_testany(smx_activity_t comms[], size_t count);
XBT_PUBLIC int simcall_comm_testany(simgrid::kernel::activity::CommImpl* comms[], size_t count);
#endif

/************************** Synchro simcalls **********************************/
SG_BEGIN_DECL()
XBT_PUBLIC smx_mutex_t simcall_mutex_init();
XBT_PUBLIC void simcall_mutex_lock(smx_mutex_t mutex);
XBT_PUBLIC int simcall_mutex_trylock(smx_mutex_t mutex);
XBT_PUBLIC void simcall_mutex_unlock(smx_mutex_t mutex);

XBT_PUBLIC smx_cond_t simcall_cond_init();
XBT_PUBLIC void simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex);
XBT_PUBLIC int simcall_cond_wait_timeout(smx_cond_t cond, smx_mutex_t mutex, double max_duration);

XBT_PUBLIC void simcall_sem_acquire(smx_sem_t sem);
XBT_PUBLIC int simcall_sem_acquire_timeout(smx_sem_t sem, double max_duration);
SG_END_DECL()

/*****************************   Io   **************************************/
#ifdef __cplusplus
XBT_PUBLIC e_smx_state_t simcall_io_wait(const smx_activity_t& io);
#endif
/************************** MC simcalls   **********************************/
SG_BEGIN_DECL()
XBT_PUBLIC int simcall_mc_random(int min, int max);
SG_END_DECL()

#endif
