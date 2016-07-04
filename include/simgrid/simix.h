/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
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
#include "simgrid/datatypes.h"
#include "simgrid/host.h"

#ifdef __cplusplus

namespace simgrid {
namespace simix {

  /** @brief Process datatype
      @ingroup simix_process_management

      A process may be defined as a <em>code</em>, with some <em>private
      data</em>, executing in a <em>location</em>.
      \see m_process_management
    @{ */
  class Process;
  class Context;
  class ContextFactory;
  class Mutex;
  class Mailbox;
}
}

typedef simgrid::simix::Context *smx_context_t;
typedef simgrid::simix::Process *smx_process_t;
typedef simgrid::simix::Mutex   *smx_mutex_t;
typedef simgrid::simix::Mailbox *smx_mailbox_t;

#else

typedef struct s_smx_context *smx_context_t;
typedef struct s_smx_process *smx_process_t;
typedef struct s_smx_mutex   *smx_mutex_t;
typedef struct s_smx_mailbox *smx_mailbox_t;

#endif

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

/* ******************************** Synchro ************************************ */

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

/* ****************************** Process *********************************** */

typedef enum {
  SMX_EXIT_SUCCESS = 0,
  SMX_EXIT_FAILURE = 1
} smx_process_exit_status_t;
/** @} */

/******************************* Networking ***********************************/

/* Process creation/destruction callbacks */
typedef void (*void_pfn_smxprocess_t) (smx_process_t);
/* for auto-restart function */
typedef void (*void_pfn_sghost_t) (sg_host_t);

extern int smx_context_stack_size;
extern int smx_context_stack_size_was_set;
extern int smx_context_guard_size;
extern int smx_context_guard_size_was_set;

SG_BEGIN_DECL()

XBT_PUBLIC(xbt_dynar_t) SIMIX_process_get_runnable();
XBT_PUBLIC(smx_process_t) SIMIX_process_from_PID(int PID);
XBT_PUBLIC(xbt_dynar_t) SIMIX_processes_as_dynar(void);

/* parallelism */
XBT_PUBLIC(int) SIMIX_context_is_parallel(void);
XBT_PUBLIC(int) SIMIX_context_get_nthreads(void);
XBT_PUBLIC(void) SIMIX_context_set_nthreads(int nb_threads);
XBT_PUBLIC(int) SIMIX_context_get_parallel_threshold();
XBT_PUBLIC(void) SIMIX_context_set_parallel_threshold(int threshold);
XBT_PUBLIC(e_xbt_parmap_mode_t) SIMIX_context_get_parallel_mode(void);
XBT_PUBLIC(void) SIMIX_context_set_parallel_mode(e_xbt_parmap_mode_t mode);
XBT_PUBLIC(int) SIMIX_is_maestro(void);


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
typedef struct s_smx_timer* smx_timer_t;

XBT_PUBLIC(smx_timer_t) SIMIX_timer_set(double date, void (*function)(void*), void *arg);
XBT_PUBLIC(void) SIMIX_timer_remove(smx_timer_t timer);
XBT_PUBLIC(double) SIMIX_timer_next(void);
XBT_PUBLIC(double) SIMIX_timer_get_date(smx_timer_t timer);

XBT_PUBLIC(void) SIMIX_display_process_status();

/******************************* Environment **********************************/
XBT_PUBLIC(void) SIMIX_create_environment(const char *file);

/******************************** Deployment **********************************/

XBT_PUBLIC(void) SIMIX_function_register(const char *name, xbt_main_func_t code);
XBT_PUBLIC(void) SIMIX_function_register_default(xbt_main_func_t code);
XBT_PUBLIC(void) SIMIX_init_application(void);
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
XBT_PUBLIC(smx_process_t) SIMIX_process_attach(
  const char* name,
  void *data,
  const char* hostname,
  xbt_dict_t properties,
  smx_process_t parent_process);
XBT_PUBLIC(void) SIMIX_process_detach();

/*********************************** Host *************************************/
XBT_PUBLIC(sg_host_t) SIMIX_host_self(void);
XBT_PUBLIC(const char*) SIMIX_host_self_get_name(void);
XBT_PUBLIC(void) SIMIX_host_on(sg_host_t host);
XBT_PUBLIC(void) SIMIX_host_off(sg_host_t host, smx_process_t issuer);
XBT_PUBLIC(void) SIMIX_host_self_set_data(void *data);
XBT_PUBLIC(void*) SIMIX_host_self_get_data(void);

/********************************* Process ************************************/
XBT_PUBLIC(smx_process_t) SIMIX_process_ref(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_unref(smx_process_t process);
XBT_PUBLIC(int) SIMIX_process_count(void);
XBT_PUBLIC(smx_process_t) SIMIX_process_self(void);
XBT_PUBLIC(const char*) SIMIX_process_self_get_name(void);
XBT_PUBLIC(void) SIMIX_process_self_set_data(void *data);
XBT_PUBLIC(void*) SIMIX_process_self_get_data(void);
XBT_PUBLIC(smx_context_t) SIMIX_process_get_context(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_set_context(smx_process_t p,smx_context_t c);
XBT_PUBLIC(int) SIMIX_process_has_pending_comms(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_on_exit_runall(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_on_exit(smx_process_t process, int_f_pvoid_pvoid_t fun, void *data);

/****************************** Communication *********************************/
XBT_PUBLIC(void) SIMIX_comm_set_copy_data_callback(void (*callback) (smx_synchro_t, void*, size_t));
XBT_PUBLIC(void) SIMIX_comm_copy_pointer_callback(smx_synchro_t comm, void* buff, size_t buff_size);
XBT_PUBLIC(void) SIMIX_comm_copy_buffer_callback(smx_synchro_t comm, void* buff, size_t buff_size);

XBT_PUBLIC(smx_synchro_t) SIMIX_comm_get_send_match(smx_mailbox_t mbox, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(int) SIMIX_comm_has_send_match(smx_mailbox_t mbox, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(int) SIMIX_comm_has_recv_match(smx_mailbox_t mbox, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(void) SIMIX_comm_finish(smx_synchro_t synchro);

/******************************************************************************/
/*                            SIMIX simcalls                                  */
/******************************************************************************/
/* These functions are a system call-like interface to the simulation kernel. */
/* They can also be called from maestro's context, and they are thread safe.  */
/******************************************************************************/

XBT_PUBLIC(void) simcall_call(smx_process_t process);

/******************************* Host simcalls ********************************/
XBT_PUBLIC(void) simcall_host_set_data(sg_host_t host, void *data);

XBT_PUBLIC(smx_synchro_t) simcall_execution_start(const char *name,
                                                double flops_amount,
                                                double priority, double bound, unsigned long affinity_mask);
XBT_PUBLIC(smx_synchro_t) simcall_execution_parallel_start(const char *name,
                                                     int host_nb,
                                                     sg_host_t *host_list,
                                                     double *flops_amount,
                                                     double *bytes_amount,
                                                     double amount,
                                                     double rate);
XBT_PUBLIC(void) simcall_execution_cancel(smx_synchro_t execution);
XBT_PUBLIC(void) simcall_execution_set_priority(smx_synchro_t execution, double priority);
XBT_PUBLIC(void) simcall_execution_set_bound(smx_synchro_t execution, double bound);
XBT_PUBLIC(void) simcall_execution_set_affinity(smx_synchro_t execution, sg_host_t host, unsigned long mask);
XBT_PUBLIC(e_smx_state_t) simcall_execution_wait(smx_synchro_t execution);

/******************************* VM simcalls ********************************/
// Create the vm_workstation at the SURF level
XBT_PUBLIC(void*) simcall_vm_create(const char *name, sg_host_t host);
XBT_PUBLIC(int) simcall_vm_get_state(sg_host_t vm);
XBT_PUBLIC(void) simcall_vm_start(sg_host_t vm);
XBT_PUBLIC(void) simcall_vm_migrate(sg_host_t vm, sg_host_t dst_pm);
XBT_PUBLIC(void *) simcall_vm_get_pm(sg_host_t vm);
XBT_PUBLIC(void) simcall_vm_set_bound(sg_host_t vm, double bound);
XBT_PUBLIC(void) simcall_vm_set_affinity(sg_host_t vm, sg_host_t pm, unsigned long mask);
XBT_PUBLIC(void) simcall_vm_resume(sg_host_t vm);
XBT_PUBLIC(void) simcall_vm_migratefrom_resumeto(sg_host_t vm, sg_host_t src_pm, sg_host_t dst_pm);
XBT_PUBLIC(void) simcall_vm_save(sg_host_t vm);
XBT_PUBLIC(void) simcall_vm_restore(sg_host_t vm);
XBT_PUBLIC(void) simcall_vm_suspend(sg_host_t vm);
XBT_PUBLIC(void) simcall_vm_destroy(sg_host_t vm);
XBT_PUBLIC(void) simcall_vm_shutdown(sg_host_t vm);

/**************************** Process simcalls ********************************/
/* Constructor and Destructor */
XBT_PUBLIC(smx_process_t) simcall_process_create(const char *name,
                                          xbt_main_func_t code,
                                          void *data,
                                          const char *hostname,
                                          double kill_time,
                                          int argc, char **argv,
                                          xbt_dict_t properties,
                                          int auto_restart);

XBT_PUBLIC(void) simcall_process_kill(smx_process_t process);
XBT_PUBLIC(void) simcall_process_killall(int reset_pid);
XBT_PUBLIC(void) SIMIX_process_throw(smx_process_t process, xbt_errcat_t cat, int value, const char *msg);


/* Process handling */
XBT_PUBLIC(void) simcall_process_cleanup(smx_process_t process);
XBT_PUBLIC(void) simcall_process_suspend(smx_process_t process);
XBT_PUBLIC(void) simcall_process_resume(smx_process_t process);

/* Getters and Setters */
XBT_PUBLIC(int) simcall_process_count(void);
XBT_PUBLIC(void *) simcall_process_get_data(smx_process_t process);
XBT_PUBLIC(void) simcall_process_set_data(smx_process_t process, void *data);
XBT_PUBLIC(void) simcall_process_set_host(smx_process_t process, sg_host_t dest);
XBT_PUBLIC(sg_host_t) simcall_process_get_host(smx_process_t process);
XBT_PUBLIC(const char *) simcall_process_get_name(smx_process_t process);
XBT_PUBLIC(int) simcall_process_get_PID(smx_process_t process);
XBT_PUBLIC(int) simcall_process_get_PPID(smx_process_t process);
XBT_PUBLIC(int) simcall_process_is_suspended(smx_process_t process);
XBT_PUBLIC(xbt_dict_t) simcall_process_get_properties(smx_process_t host);
XBT_PUBLIC(void) simcall_process_set_kill_time(smx_process_t process, double kill_time);
XBT_PUBLIC(double) simcall_process_get_kill_time(smx_process_t process);
XBT_PUBLIC(void) simcall_process_on_exit(smx_process_t process, int_f_pvoid_pvoid_t fun, void *data);
XBT_PUBLIC(void) simcall_process_auto_restart_set(smx_process_t process, int auto_restart);
XBT_PUBLIC(smx_process_t) simcall_process_restart(smx_process_t process);
XBT_PUBLIC(void) simcall_process_join(smx_process_t process, double timeout);
/* Sleep control */
XBT_PUBLIC(e_smx_state_t) simcall_process_sleep(double duration);

/************************** Comunication simcalls *****************************/
/***** Rendez-vous points *****/

XBT_PUBLIC(smx_mailbox_t) simcall_mbox_create(const char *name);
XBT_PUBLIC(smx_mailbox_t) simcall_mbox_get_by_name(const char *name);
XBT_PUBLIC(smx_synchro_t) simcall_mbox_front(smx_mailbox_t mbox);
XBT_PUBLIC(void) simcall_mbox_set_receiver(smx_mailbox_t mbox , smx_process_t process);

/***** Communication simcalls *****/

XBT_PUBLIC(void) simcall_comm_send(smx_process_t sender, smx_mailbox_t mbox, double task_size,
                                     double rate, void *src_buff,
                                     size_t src_buff_size,
                                     int (*match_fun)(void *, void *, smx_synchro_t),
                                     void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                                     void *data, double timeout);

XBT_PUBLIC(smx_synchro_t) simcall_comm_isend(smx_process_t sender, smx_mailbox_t mbox,
                                              double task_size,
                                              double rate, void *src_buff,
                                              size_t src_buff_size,
                                              int (*match_fun)(void *, void *, smx_synchro_t),
                                              void (*clean_fun)(void *),
                                              void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                                              void *data, int detached);

XBT_PUBLIC(void) simcall_comm_recv(smx_process_t receiver, smx_mailbox_t mbox, void *dst_buff,
                                   size_t * dst_buff_size,
                                   int (*match_fun)(void *, void *, smx_synchro_t),
                                   void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                                   void *data, double timeout, double rate);

XBT_PUBLIC(smx_synchro_t) simcall_comm_irecv(smx_process_t receiver, smx_mailbox_t mbox, void *dst_buff,
                                            size_t * dst_buff_size,
                                            int (*match_fun)(void *, void *, smx_synchro_t),
                                            void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                                            void *data, double rate);

XBT_PUBLIC(smx_synchro_t) simcall_comm_iprobe(smx_mailbox_t mbox, int type, int src, int tag,
                                int (*match_fun)(void *, void *, smx_synchro_t), void *data);
XBT_PUBLIC(void) simcall_comm_cancel(smx_synchro_t comm);

/* FIXME: waitany is going to be a vararg function, and should take a timeout */
XBT_PUBLIC(unsigned int) simcall_comm_waitany(xbt_dynar_t comms);
XBT_PUBLIC(void) simcall_comm_wait(smx_synchro_t comm, double timeout);
XBT_PUBLIC(int) simcall_comm_test(smx_synchro_t comm);
XBT_PUBLIC(int) simcall_comm_testany(xbt_dynar_t comms);

/************************** Tracing handling **********************************/
XBT_PUBLIC(void) simcall_set_category(smx_synchro_t synchro, const char *category);

/************************** Synchro simcalls **********************************/
XBT_PUBLIC(smx_mutex_t) simcall_mutex_init(void);
XBT_PUBLIC(smx_mutex_t) SIMIX_mutex_ref(smx_mutex_t mutex);
XBT_PUBLIC(void) SIMIX_mutex_unref(smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_mutex_lock(smx_mutex_t mutex);
XBT_PUBLIC(int) simcall_mutex_trylock(smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_mutex_unlock(smx_mutex_t mutex);

XBT_PUBLIC(smx_cond_t) simcall_cond_init(void);
XBT_PUBLIC(void) SIMIX_cond_unref(smx_cond_t cond);
XBT_PUBLIC(smx_cond_t) SIMIX_cond_ref(smx_cond_t cond);
XBT_PUBLIC(void) simcall_cond_signal(smx_cond_t cond);
XBT_PUBLIC(void) simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_cond_wait_timeout(smx_cond_t cond,
                                         smx_mutex_t mutex,
                                         double max_duration);
XBT_PUBLIC(void) simcall_cond_broadcast(smx_cond_t cond);

XBT_PUBLIC(smx_sem_t) simcall_sem_init(int capacity);
XBT_PUBLIC(void) SIMIX_sem_destroy(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_release(smx_sem_t sem);
XBT_PUBLIC(int) simcall_sem_would_block(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_acquire(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_acquire_timeout(smx_sem_t sem,
                                             double max_duration);
XBT_PUBLIC(int) simcall_sem_get_capacity(smx_sem_t sem);

/*****************************   File   **********************************/
XBT_PUBLIC(void *) simcall_file_get_data(smx_file_t fd);
XBT_PUBLIC(void) simcall_file_set_data(smx_file_t fd, void *data);
XBT_PUBLIC(sg_size_t) simcall_file_read(smx_file_t fd, sg_size_t size, sg_host_t host);
XBT_PUBLIC(sg_size_t) simcall_file_write(smx_file_t fd, sg_size_t size, sg_host_t host);
XBT_PUBLIC(smx_file_t) simcall_file_open(const char* fullpath, sg_host_t host);
XBT_PUBLIC(int) simcall_file_close(smx_file_t fd, sg_host_t host);
XBT_PUBLIC(int) simcall_file_unlink(smx_file_t fd, sg_host_t host);
XBT_PUBLIC(sg_size_t) simcall_file_get_size(smx_file_t fd);
XBT_PUBLIC(xbt_dynar_t) simcall_file_get_info(smx_file_t fd);
XBT_PUBLIC(sg_size_t) simcall_file_tell(smx_file_t fd);
XBT_PUBLIC(int) simcall_file_seek(smx_file_t fd, sg_offset_t offset, int origin);
XBT_PUBLIC(int) simcall_file_move(smx_file_t fd, const char* fullpath);
/*****************************   Storage   **********************************/
XBT_PUBLIC(sg_size_t) simcall_storage_get_free_size (smx_storage_t storage);
XBT_PUBLIC(sg_size_t) simcall_storage_get_used_size (smx_storage_t storage);
XBT_PUBLIC(xbt_dict_t) simcall_storage_get_properties(smx_storage_t storage);
XBT_PUBLIC(void*) SIMIX_storage_get_data(smx_storage_t storage);
XBT_PUBLIC(void) SIMIX_storage_set_data(smx_storage_t storage, void *data);
XBT_PUBLIC(xbt_dict_t) SIMIX_storage_get_content(smx_storage_t storage);
XBT_PUBLIC(xbt_dict_t) simcall_storage_get_content(smx_storage_t storage);
XBT_PUBLIC(const char*) SIMIX_storage_get_name(smx_storage_t storage);
XBT_PUBLIC(sg_size_t) SIMIX_storage_get_size(smx_storage_t storage);
XBT_PUBLIC(const char*) SIMIX_storage_get_host(smx_storage_t storage);
/************************** AS router   **********************************/
XBT_PUBLIC(xbt_dict_t) SIMIX_asr_get_properties(const char *name);
/************************** AS router simcalls ***************************/
XBT_PUBLIC(xbt_dict_t) simcall_asr_get_properties(const char *name);

/************************** MC simcalls   **********************************/
XBT_PUBLIC(int) simcall_mc_random(int min, int max);

SG_END_DECL()
#endif                          /* _SIMIX_SIMIX_H */
