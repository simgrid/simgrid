/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

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

/******************************* Networking ***********************************/
extern unsigned smx_context_stack_size;
extern unsigned smx_context_guard_size;

SG_BEGIN_DECL

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

XBT_ATTRIB_DEPRECATED_v329("Please use simgrid::simix::Timer::set()") XBT_PUBLIC smx_timer_t
    SIMIX_timer_set(double date, void (*function)(void*), void* arg);
XBT_ATTRIB_DEPRECATED_v329("Please use simgrid::simix::Timer::remove()") XBT_PUBLIC
    void SIMIX_timer_remove(smx_timer_t timer);
XBT_ATTRIB_DEPRECATED_v329("Please use simgrid::simix::Timer::next()") XBT_PUBLIC double SIMIX_timer_next();
XBT_ATTRIB_DEPRECATED_v329("Please use simgrid::simix::Timer::get_date()") XBT_PUBLIC
    double SIMIX_timer_get_date(smx_timer_t timer);

XBT_ATTRIB_DEPRECATED_v329("Please use simix_global->display_all_actor_status()") XBT_PUBLIC
    void SIMIX_display_process_status();
SG_END_DECL

/******************************** Deployment **********************************/
SG_BEGIN_DECL
XBT_ATTRIB_DEPRECATED_v329("Please use simgrid_register_default() or Engine::register_default()") XBT_PUBLIC
    void SIMIX_function_register_default(xbt_main_func_t code);
XBT_ATTRIB_DEPRECATED_v329("This function will be removed in 3.29") XBT_PUBLIC void SIMIX_init_application();

XBT_PUBLIC void SIMIX_process_set_function(const char* process_host, const char* process_function,
                                           xbt_dynar_t arguments, double process_start_time, double process_kill_time);
SG_END_DECL

#ifdef __cplusplus
XBT_ATTRIB_DEPRECATED_v329("Please use simgrid_register_function() or Engine::register_function()") XBT_PUBLIC
    void SIMIX_function_register(const std::string& name, void (*code)(std::vector<std::string>));
XBT_ATTRIB_DEPRECATED_v329("Please use simgrid_register_function() or Engine::register_function()") XBT_PUBLIC
    void SIMIX_function_register(const std::string& name, xbt_main_func_t code);
XBT_ATTRIB_DEPRECATED_v329("Please use simgrid_load_deployment() or Engine::load_deployment()") XBT_PUBLIC
    void SIMIX_launch_application(const std::string& file);
#endif

/********************************* Process ************************************/
SG_BEGIN_DECL
XBT_ATTRIB_DEPRECATED_v329("Please use simgrid_get_actor_count()") XBT_PUBLIC int SIMIX_process_count();
XBT_PUBLIC smx_actor_t SIMIX_process_self();
XBT_PUBLIC const char* SIMIX_process_self_get_name();
XBT_ATTRIB_DEPRECATED_v329("This function will be removed in 3.29") XBT_PUBLIC
    void SIMIX_process_self_set_data(void* data);
XBT_ATTRIB_DEPRECATED_v329("This function will be removed in 3.29") XBT_PUBLIC void* SIMIX_process_self_get_data();
SG_END_DECL

#ifdef __cplusplus
XBT_ATTRIB_DEPRECATED_v329("This function will be removed in 3.29") XBT_PUBLIC
    void SIMIX_process_on_exit(smx_actor_t process, const std::function<void(bool /*failed*/)>& fun);
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
XBT_ATTRIB_DEPRECATED_v330("Please use s4u::Exec::wait_for()") XBT_PUBLIC e_smx_state_t
    simcall_execution_wait(simgrid::kernel::activity::ActivityImpl* execution, double timeout);
XBT_ATTRIB_DEPRECATED_v330("Please use s4u::Exec::wait_for()") XBT_PUBLIC e_smx_state_t
    simcall_execution_wait(const simgrid::kernel::activity::ActivityImplPtr& execution, double timeout);
XBT_PUBLIC unsigned int simcall_execution_waitany_for(simgrid::kernel::activity::ExecImpl* execs[], size_t count,
                                                      double timeout);
XBT_ATTRIB_DEPRECATED_v330("Please use s4u::Exec::test()") XBT_PUBLIC
    bool simcall_execution_test(simgrid::kernel::activity::ActivityImpl* execution);
XBT_ATTRIB_DEPRECATED_v330("Please use s4u::Exec::test()") XBT_PUBLIC
    bool simcall_execution_test(const simgrid::kernel::activity::ActivityImplPtr& execution);

#endif

/**************************** Process simcalls ********************************/
SG_BEGIN_DECL
XBT_ATTRIB_DEPRECATED_v329("This function will be removed in 3.29") void simcall_process_set_data(smx_actor_t process,
                                                                                                  void* data);
XBT_ATTRIB_DEPRECATED_v328("Please use sg_actor_suspend()") XBT_PUBLIC
    void simcall_process_suspend(smx_actor_t process);

XBT_ATTRIB_DEPRECATED_v328("Please use sg_actor_join()") XBT_PUBLIC
    void simcall_process_join(smx_actor_t process, double timeout);

XBT_ATTRIB_DEPRECATED_v329("Please use sg_actor_sleep_for()") XBT_PUBLIC e_smx_state_t
    simcall_process_sleep(double duration);
SG_END_DECL

/************************** Communication simcalls ****************************/

#ifdef __cplusplus
XBT_PUBLIC void simcall_comm_send(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                                  size_t src_buff_size,
                                  bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                  void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                  void* data, double timeout);

XBT_PUBLIC simgrid::kernel::activity::ActivityImplPtr
simcall_comm_isend(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                   size_t src_buff_size, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*clean_fun)(void*), void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                   void* data, bool detached);

XBT_PUBLIC void simcall_comm_recv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                                  bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                  void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                  void* data, double timeout, double rate);

XBT_PUBLIC simgrid::kernel::activity::ActivityImplPtr
simcall_comm_irecv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                   bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data, double rate);

XBT_ATTRIB_DEPRECATED_v330("Please use Mailbox::iprobe()") XBT_PUBLIC simgrid::kernel::activity::ActivityImplPtr
    simcall_comm_iprobe(smx_mailbox_t mbox, int type,
                        bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*), void* data);

XBT_ATTRIB_DEPRECATED_v330("Please use a CommImpl*[] for first parameter") XBT_PUBLIC
    unsigned int simcall_comm_waitany(simgrid::kernel::activity::ActivityImplPtr comms[], size_t count, double timeout);
XBT_PUBLIC unsigned int simcall_comm_waitany(simgrid::kernel::activity::CommImpl* comms[], size_t count,
                                             double timeout);
XBT_PUBLIC void simcall_comm_wait(simgrid::kernel::activity::ActivityImpl* comm, double timeout);
XBT_PUBLIC bool simcall_comm_test(simgrid::kernel::activity::ActivityImpl* comm);
XBT_ATTRIB_DEPRECATED_v330("Please use a CommImpl*[] for first parameter") XBT_PUBLIC
    int simcall_comm_testany(simgrid::kernel::activity::ActivityImplPtr comms[], size_t count);
XBT_PUBLIC int simcall_comm_testany(simgrid::kernel::activity::CommImpl* comms[], size_t count);

XBT_ATTRIB_DEPRECATED_v330("Please use a ActivityImpl* for first parameter") inline void simcall_comm_wait(
    const simgrid::kernel::activity::ActivityImplPtr& comm, double timeout)
{
  simcall_comm_wait(comm.get(), timeout);
}
XBT_ATTRIB_DEPRECATED_v330("Please use a ActivityImpl* for first parameter") inline bool simcall_comm_test(
    const simgrid::kernel::activity::ActivityImplPtr& comm)
{
  return simcall_comm_test(comm.get());
}
#endif

/************************** Synchro simcalls **********************************/
SG_BEGIN_DECL
XBT_ATTRIB_DEPRECATED_v330("Please use sg_mutex_init()") XBT_PUBLIC smx_mutex_t simcall_mutex_init();
XBT_PUBLIC void simcall_mutex_lock(smx_mutex_t mutex);
XBT_PUBLIC int simcall_mutex_trylock(smx_mutex_t mutex);
XBT_PUBLIC void simcall_mutex_unlock(smx_mutex_t mutex);

XBT_ATTRIB_DEPRECATED_v330("Please use sg_cond_init()") XBT_PUBLIC smx_cond_t simcall_cond_init();
XBT_PUBLIC void simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex);
XBT_PUBLIC int simcall_cond_wait_timeout(smx_cond_t cond, smx_mutex_t mutex, double max_duration);

XBT_PUBLIC void simcall_sem_acquire(smx_sem_t sem);
XBT_PUBLIC int simcall_sem_acquire_timeout(smx_sem_t sem, double max_duration);
SG_END_DECL

/*****************************   Io   **************************************/
#ifdef __cplusplus
XBT_ATTRIB_DEPRECATED_v330("Please use s4u::Io::wait_for()") XBT_PUBLIC e_smx_state_t
    simcall_io_wait(simgrid::kernel::activity::ActivityImpl* io, double timeout);
XBT_ATTRIB_DEPRECATED_v330("Please use s4u::Io::wait_for()") XBT_PUBLIC e_smx_state_t
    simcall_io_wait(const simgrid::kernel::activity::ActivityImplPtr& io, double timeout);
XBT_ATTRIB_DEPRECATED_v330("Please use s4u::Io::test()") XBT_PUBLIC
    bool simcall_io_test(simgrid::kernel::activity::ActivityImpl* io);
XBT_ATTRIB_DEPRECATED_v330("Please use s4u::Io::test()") XBT_PUBLIC
    bool simcall_io_test(const simgrid::kernel::activity::ActivityImplPtr& io);
#endif
/************************** MC simcalls   **********************************/
SG_BEGIN_DECL
XBT_PUBLIC int simcall_mc_random(int min, int max);
SG_END_DECL

#endif
