/* Copyright (c) 2004-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_H
#define MSG_H

#include "xbt.h"

#include "msg/datatypes.h"

#include "simgrid/simix.h"

SG_BEGIN_DECL()

/** @brief Return code of most MSG functions
    @ingroup msg_simulation
    @{ */
/* Keep these code as binary values: java bindings manipulate | of these values */
typedef enum {
  MSG_OK = 0,                 /**< @brief Everything is right. Keep on going this way ! */
  MSG_TIMEOUT = 1,            /**< @brief nothing good happened before the timer you provided elapsed */
  MSG_TRANSFER_FAILURE = 2,   /**< @brief There has been a problem during you task
      transfer. Either the network is down or the remote host has been
      shutdown. */
  MSG_HOST_FAILURE = 4,       /**< @brief System shutdown. The host on which you are
      running has just been rebooted. Free your datastructures and
      return now !*/
  MSG_TASK_CANCELED = 8      /**< @brief Canceled task. This task has been canceled by somebody!*/
} msg_error_t;
/** @} */

/************************** Global ******************************************/
XBT_PUBLIC(void) MSG_config(const char *key, const char *value);
/** \ingroup msg_simulation
 *  \brief Initialize the MSG internal data.
 *  \hideinitializer
 *
 *  It also check that the link-time and compile-time versions of SimGrid do
 *  match, so you should use this version instead of the #MSG_init_nocheck
 *  function that does the same initializations, but without this check.
 *
 *  We allow to link against compiled versions that differ in the patch level.
 */
#define MSG_init(argc,argv)  {                                          \
    int ver_major,ver_minor,ver_patch;                                  \
    sg_version(&ver_major,&ver_minor,&ver_patch);                       \
    if ((ver_major != SIMGRID_VERSION_MAJOR) ||                         \
        (ver_minor != SIMGRID_VERSION_MINOR)) {                         \
      fprintf(stderr,"FATAL ERROR: Your program was compiled with SimGrid version %d.%d.%d, and then linked against SimGrid %d.%d.%d. Please fix this.\n", \
              SIMGRID_VERSION_MAJOR,SIMGRID_VERSION_MINOR,SIMGRID_VERSION_PATCH,ver_major,ver_minor,ver_patch); \
    }                                                                   \
    MSG_init_nocheck(argc,argv);                                        \
  }

XBT_PUBLIC(void) MSG_init_nocheck(int *argc, char **argv);
XBT_PUBLIC(msg_error_t) MSG_main(void);
XBT_PUBLIC(void) MSG_function_register(const char *name,
                                       xbt_main_func_t code);
XBT_PUBLIC(void) MSG_function_register_default(xbt_main_func_t code);
XBT_PUBLIC(xbt_main_func_t) MSG_get_registered_function(const char *name);
XBT_PUBLIC(void) MSG_launch_application(const char *file);
/*Bypass the parser */
XBT_PUBLIC(void) MSG_set_function(const char *host_id,
                                  const char *function_name,
                                  xbt_dynar_t arguments);

XBT_PUBLIC(double) MSG_get_clock(void);
XBT_PUBLIC(unsigned long int) MSG_get_sent_msg(void);


/************************** File handling ***********************************/
XBT_PUBLIC(double) MSG_file_read(void* ptr, size_t size, size_t nmemb, msg_file_t stream);
XBT_PUBLIC(size_t) MSG_file_write(const void* ptr, size_t size, size_t nmemb, msg_file_t stream);
XBT_PUBLIC(msg_file_t) MSG_file_open(const char* mount, const char* path, const char* mode);
XBT_PUBLIC(int) MSG_file_close(msg_file_t fp);
XBT_PUBLIC(int) MSG_file_stat(msg_file_t fd, s_msg_stat_t *buf);
XBT_PUBLIC(void) MSG_file_free_stat(s_msg_stat_t *stat);

XBT_PUBLIC(int) MSG_file_unlink(msg_file_t fd);
XBT_PUBLIC(xbt_dict_t) MSG_file_ls(const char *mount, const char *path);

/************************** AS Router handling ************************************/
XBT_PUBLIC(const char *) MSG_as_router_get_property_value(const char* asr, const char *name);
XBT_PUBLIC(xbt_dict_t) MSG_as_router_get_properties(const char* asr);
XBT_PUBLIC(void) MSG_as_router_set_property_value(const char* asr, const char *name, char *value,void_f_pvoid_t free_ctn);

/************************** Host handling ***********************************/
XBT_PUBLIC(msg_error_t) MSG_host_set_data(msg_host_t host, void *data);
XBT_PUBLIC(void *) MSG_host_get_data(msg_host_t host);
XBT_PUBLIC(const char *) MSG_host_get_name(msg_host_t host);
XBT_PUBLIC(msg_host_t) MSG_host_self(void);
XBT_PUBLIC(int) MSG_get_host_msgload(msg_host_t host);
/* int MSG_get_msgload(void); This function lacks specification; discard it */
XBT_PUBLIC(double) MSG_get_host_speed(msg_host_t h);
XBT_PUBLIC(int) MSG_host_is_avail(msg_host_t h);
XBT_PUBLIC(void) __MSG_host_priv_free(msg_host_priv_t priv);
XBT_PUBLIC(void) __MSG_host_destroy(msg_host_t host);

/*property handlers*/
XBT_PUBLIC(xbt_dict_t) MSG_host_get_properties(msg_host_t host);
XBT_PUBLIC(const char *) MSG_host_get_property_value(msg_host_t host,
                                                     const char *name);
XBT_PUBLIC(void) MSG_host_set_property_value(msg_host_t host,
                                             const char *name, char *value,
                                             void_f_pvoid_t free_ctn);


XBT_PUBLIC(void) MSG_create_environment(const char *file);

XBT_PUBLIC(msg_host_t) MSG_get_host_by_name(const char *name);
XBT_PUBLIC(xbt_dynar_t) MSG_hosts_as_dynar(void);
XBT_PUBLIC(int) MSG_get_host_number(void);

XBT_PUBLIC(void) MSG_host_get_params(msg_host_t ind_pm, ws_params_t params);
XBT_PUBLIC(void) MSG_host_set_params(msg_host_t ind_pm, ws_params_t params);

/************************** Process handling *********************************/
XBT_PUBLIC(msg_process_t) MSG_process_create(const char *name,
                                           xbt_main_func_t code,
                                           void *data, msg_host_t host);
XBT_PUBLIC(msg_process_t) MSG_process_create_with_arguments(const char *name,
                                                          xbt_main_func_t
                                                          code, void *data,
                                                          msg_host_t host,
                                                          int argc,
                                                          char **argv);
XBT_PUBLIC(msg_process_t) MSG_process_create_with_environment(const char
                                                            *name,
                                                            xbt_main_func_t
                                                            code,
                                                            void *data,
                                                            msg_host_t host,
                                                            int argc,
                                                            char **argv,
                                                            xbt_dict_t
                                                            properties);
XBT_PUBLIC(void) MSG_process_kill(msg_process_t process);
XBT_PUBLIC(int) MSG_process_killall(int reset_PIDs);

XBT_PUBLIC(msg_error_t) MSG_process_migrate(msg_process_t process, msg_host_t host);

XBT_PUBLIC(void *) MSG_process_get_data(msg_process_t process);
XBT_PUBLIC(msg_error_t) MSG_process_set_data(msg_process_t process,
                                             void *data);
XBT_PUBLIC(void) MSG_process_set_data_cleanup(void_f_pvoid_t data_cleanup);
XBT_PUBLIC(msg_host_t) MSG_process_get_host(msg_process_t process);
XBT_PUBLIC(msg_process_t) MSG_process_from_PID(int PID);
XBT_PUBLIC(int) MSG_process_get_PID(msg_process_t process);
XBT_PUBLIC(int) MSG_process_get_PPID(msg_process_t process);
XBT_PUBLIC(const char *) MSG_process_get_name(msg_process_t process);
XBT_PUBLIC(int) MSG_process_self_PID(void);
XBT_PUBLIC(int) MSG_process_self_PPID(void);
XBT_PUBLIC(msg_process_t) MSG_process_self(void);
XBT_PUBLIC(xbt_dynar_t) MSG_processes_as_dynar(void);
XBT_PUBLIC(int) MSG_process_get_number(void);

XBT_PUBLIC(msg_error_t) MSG_process_set_kill_time(msg_process_t process, double kill_time);

/*property handlers*/
XBT_PUBLIC(xbt_dict_t) MSG_process_get_properties(msg_process_t process);
XBT_PUBLIC(const char *) MSG_process_get_property_value(msg_process_t
                                                        process,
                                                        const char *name);

XBT_PUBLIC(msg_error_t) MSG_process_suspend(msg_process_t process);
XBT_PUBLIC(msg_error_t) MSG_process_resume(msg_process_t process);
XBT_PUBLIC(int) MSG_process_is_suspended(msg_process_t process);
XBT_PUBLIC(void) MSG_process_on_exit(int_f_pvoid_t fun, void *data);
XBT_PUBLIC(void) MSG_process_auto_restart_set(msg_process_t process, int auto_restart);

XBT_PUBLIC(msg_process_t) MSG_process_restart(msg_process_t process);

/************************** Task handling ************************************/
XBT_PUBLIC(msg_task_t) MSG_task_create(const char *name,
                                     double compute_duration,
                                     double message_size, void *data);
XBT_PUBLIC(msg_gpu_task_t) MSG_gpu_task_create(const char *name,
                                     double compute_duration,
                                     double dispatch_latency,
                                     double collect_latency);
XBT_PUBLIC(msg_task_t) MSG_parallel_task_create(const char *name,
                                              int host_nb,
                                              const msg_host_t * host_list,
                                              double *computation_amount,
                                              double *communication_amount,
                                              void *data);
XBT_PUBLIC(void *) MSG_task_get_data(msg_task_t task);
XBT_PUBLIC(void) MSG_task_set_data(msg_task_t task, void *data);
XBT_PUBLIC(void) MSG_task_set_copy_callback(void (*callback) (
    msg_task_t task, msg_process_t src, msg_process_t dst));
XBT_PUBLIC(msg_process_t) MSG_task_get_sender(msg_task_t task);
XBT_PUBLIC(msg_host_t) MSG_task_get_source(msg_task_t task);
XBT_PUBLIC(const char *) MSG_task_get_name(msg_task_t task);
XBT_PUBLIC(void) MSG_task_set_name(msg_task_t task, const char *name);
XBT_PUBLIC(msg_error_t) MSG_task_cancel(msg_task_t task);
XBT_PUBLIC(msg_error_t) MSG_task_destroy(msg_task_t task);

XBT_PUBLIC(msg_error_t) MSG_task_receive_from_host(msg_task_t * task, const char *alias,
                                       msg_host_t host);
XBT_PUBLIC(msg_error_t) MSG_task_receive_from_host_bounded(msg_task_t * task, const char *alias,
                                       msg_host_t host, double rate);

XBT_PUBLIC(msg_error_t) MSG_task_execute(msg_task_t task);
XBT_PUBLIC(msg_error_t) MSG_parallel_task_execute(msg_task_t task);
XBT_PUBLIC(void) MSG_task_set_priority(msg_task_t task, double priority);

XBT_PUBLIC(msg_error_t) MSG_process_sleep(double nb_sec);

XBT_PUBLIC(double) MSG_task_get_compute_duration(msg_task_t task);
XBT_PUBLIC(void) MSG_task_set_compute_duration(msg_task_t task,
                                               double compute_duration);
XBT_PUBLIC(void) MSG_task_set_data_size(msg_task_t task,
                                        double data_size);

XBT_PUBLIC(double) MSG_task_get_remaining_computation(msg_task_t task);
XBT_PUBLIC(double) MSG_task_get_remaining_communication(msg_task_t task);
XBT_PUBLIC(int) MSG_task_is_latency_bounded(msg_task_t task);
XBT_PUBLIC(double) MSG_task_get_data_size(msg_task_t task);


XBT_PUBLIC(msg_error_t)
    MSG_task_receive_ext(msg_task_t * task, const char *alias, double timeout,
                     msg_host_t host);

XBT_PUBLIC(msg_error_t)
    MSG_task_receive_with_timeout(msg_task_t * task, const char *alias,
                              double timeout);

XBT_PUBLIC(msg_error_t)
    MSG_task_receive(msg_task_t * task, const char *alias);
#define MSG_task_recv(t,a) MSG_task_receive(t,a)



XBT_PUBLIC(msg_error_t)
    MSG_task_receive_ext_bounded(msg_task_t * task, const char *alias, double timeout,
                     msg_host_t host, double rate);

XBT_PUBLIC(msg_error_t)
    MSG_task_receive_with_timeout_bounded(msg_task_t * task, const char *alias,
                              double timeout, double rate);

XBT_PUBLIC(msg_error_t)
    MSG_task_receive_bounded(msg_task_t * task, const char *alias,double rate);
#define MSG_task_recv_bounded(t,a,r) MSG_task_receive_bounded(t,a,r)

XBT_PUBLIC(msg_comm_t) MSG_task_isend(msg_task_t task, const char *alias);
XBT_PUBLIC(msg_comm_t) MSG_task_isend_bounded(msg_task_t task, const char *alias, double maxrate);
XBT_PUBLIC(msg_comm_t) MSG_task_isend_with_matching(msg_task_t task,
                                                    const char *alias,
                                                    int (*match_fun)(void*,void*, smx_action_t),
                                                    void *match_data);

XBT_PUBLIC(void) MSG_task_dsend(msg_task_t task, const char *alias, void_f_pvoid_t cleanup);
XBT_PUBLIC(void) MSG_task_dsend_bounded(msg_task_t task, const char *alias, void_f_pvoid_t cleanup, double maxrate);
XBT_PUBLIC(msg_comm_t) MSG_task_irecv(msg_task_t * task, const char *alias);
XBT_PUBLIC(msg_comm_t) MSG_task_irecv_bounded(msg_task_t * task, const char *alias, double rate);
XBT_PUBLIC(int) MSG_comm_test(msg_comm_t comm);
XBT_PUBLIC(int) MSG_comm_testany(xbt_dynar_t comms);
XBT_PUBLIC(void) MSG_comm_destroy(msg_comm_t comm);
XBT_PUBLIC(msg_error_t) MSG_comm_wait(msg_comm_t comm, double timeout);
XBT_PUBLIC(void) MSG_comm_waitall(msg_comm_t * comm, int nb_elem,
                                  double timeout);
XBT_PUBLIC(int) MSG_comm_waitany(xbt_dynar_t comms);
XBT_PUBLIC(msg_task_t) MSG_comm_get_task(msg_comm_t comm);
XBT_PUBLIC(msg_error_t) MSG_comm_get_status(msg_comm_t comm);

XBT_PUBLIC(int) MSG_task_listen(const char *alias);

XBT_PUBLIC(int) MSG_task_listen_from_host(const char *alias,
                                          msg_host_t host);

XBT_PUBLIC(msg_error_t)
    MSG_task_send_with_timeout(msg_task_t task, const char *alias,
                           double timeout);

XBT_PUBLIC(msg_error_t)
    MSG_task_send_with_timeout_bounded(msg_task_t task, const char *alias,
                           double timeout, double maxrate);

XBT_PUBLIC(msg_error_t)
    MSG_task_send(msg_task_t task, const char *alias);

XBT_PUBLIC(msg_error_t)
    MSG_task_send_bounded(msg_task_t task, const char *alias, double rate);

XBT_PUBLIC(int) MSG_task_listen_from(const char *alias);

XBT_PUBLIC(void) MSG_task_set_category (msg_task_t task, const char *category);
XBT_PUBLIC(const char *) MSG_task_get_category (msg_task_t task);

/************************** Task handling ************************************/
XBT_PUBLIC(msg_error_t)
    MSG_mailbox_get_task_ext(msg_mailbox_t mailbox, msg_task_t * task,
                         msg_host_t host, double timeout);

XBT_PUBLIC(msg_error_t)
    MSG_mailbox_get_task_ext_bounded(msg_mailbox_t mailbox, msg_task_t *task,
                                     msg_host_t host, double timeout, double rate);

XBT_PUBLIC(msg_error_t)
    MSG_mailbox_put_with_timeout(msg_mailbox_t mailbox, msg_task_t task,
                             double timeout);

XBT_PUBLIC(void) MSG_mailbox_set_async(const char *alias);


/************************** Action handling **********************************/
XBT_PUBLIC(msg_error_t) MSG_action_trace_run(char *path);

#ifdef MSG_USE_DEPRECATED

typedef msg_error_t MSG_error_t;

#define MSG_global_init(argc, argv)      MSG_init(argc,argv)
#define MSG_global_init_args(argc, argv) MSG_init(argc,argv)

/* these are the functions which are deprecated. Do not use them, they may get removed in future releases */
XBT_PUBLIC(msg_host_t *) MSG_get_host_table(void);

#define MSG_TIMEOUT_FAILURE MSG_TIMEOUT
#define MSG_TASK_CANCELLED MSG_TASK_CANCELED
#define MSG_mailbox_put_with_time_out(mailbox, task, timeout) \
        MSG_mailbox_put_with_timeout(mailbox, task, timeout)

#define MSG_process_change_host(h) MSG_process_migrate(MSG_process_self(),h);
XBT_PUBLIC(msg_error_t) MSG_get_errno(void);

XBT_PUBLIC(msg_error_t) MSG_clean(void);

XBT_PUBLIC(msg_error_t) MSG_task_get(msg_task_t * task, m_channel_t channel);
XBT_PUBLIC(msg_error_t) MSG_task_get_with_timeout(msg_task_t * task,
                                                  m_channel_t channel,
                                                  double max_duration);
XBT_PUBLIC(msg_error_t) MSG_task_get_from_host(msg_task_t * task,
                                               int channel, msg_host_t host);
XBT_PUBLIC(msg_error_t) MSG_task_get_ext(msg_task_t * task, int channel,
                                         double max_duration,
                                         msg_host_t host);
XBT_PUBLIC(msg_error_t) MSG_task_put(msg_task_t task, msg_host_t dest,
                                     m_channel_t channel);
XBT_PUBLIC(msg_error_t) MSG_task_put_bounded(msg_task_t task,
                                             msg_host_t dest,
                                             m_channel_t channel,
                                             double max_rate);
XBT_PUBLIC(msg_error_t) MSG_task_put_with_timeout(msg_task_t task,
                                                  msg_host_t dest,
                                                  m_channel_t channel,
                                                  double max_duration);
XBT_PUBLIC(int) MSG_task_Iprobe(m_channel_t channel);
XBT_PUBLIC(int) MSG_task_probe_from(m_channel_t channel);
XBT_PUBLIC(int) MSG_task_probe_from_host(int channel, msg_host_t host);

XBT_PUBLIC(msg_error_t) MSG_set_channel_number(int number);
XBT_PUBLIC(int) MSG_get_channel_number(void);
#endif

/** @brief Opaque type describing a Virtual Machine.
 *  @ingroup msg_VMs
 *
 * All this is highly experimental and the interface will probably change in the future.
 * Please don't depend on this yet (although testing is welcomed if you feel so).
 * Usual lack of guaranty of any kind applies here, and is even increased.
 *
 */

XBT_PUBLIC(int) MSG_vm_is_created(msg_vm_t);
XBT_PUBLIC(int) MSG_vm_is_running(msg_vm_t);
XBT_PUBLIC(int) MSG_vm_is_migrating(msg_vm_t);

XBT_PUBLIC(int) MSG_vm_is_suspended(msg_vm_t);
XBT_PUBLIC(int) MSG_vm_is_saving(msg_vm_t);
XBT_PUBLIC(int) MSG_vm_is_saved(msg_vm_t);
XBT_PUBLIC(int) MSG_vm_is_restoring(msg_vm_t);


XBT_PUBLIC(const char*) MSG_vm_get_name(msg_vm_t);

// TODO add VDI later
XBT_PUBLIC(msg_vm_t) MSG_vm_create_core(msg_host_t location, const char *name);
XBT_PUBLIC(msg_vm_t) MSG_vm_create(msg_host_t ind_pm, const char *name,
    int core_nb, int mem_cap, int net_cap, char *disk_path, int disk_size);

XBT_PUBLIC(void) MSG_vm_destroy(msg_vm_t vm);

XBT_PUBLIC(void) MSG_vm_start(msg_vm_t);

/* Shutdown the guest operating system. */
XBT_PUBLIC(void) MSG_vm_shutdown(msg_vm_t vm);

XBT_PUBLIC(void) MSG_vm_migrate(msg_vm_t vm, msg_host_t destination);

/* Suspend the execution of the VM, but keep its state on memory. */
XBT_PUBLIC(void) MSG_vm_suspend(msg_vm_t vm);
XBT_PUBLIC(void) MSG_vm_resume(msg_vm_t vm);

/* Save the VM state to a disk. */
XBT_PUBLIC(void) MSG_vm_save(msg_vm_t vm);
XBT_PUBLIC(void) MSG_vm_restore(msg_vm_t vm);

msg_host_t MSG_vm_get_pm(msg_vm_t vm);

/* TODO: do we need this? */
// XBT_PUBLIC(xbt_dynar_t) MSG_vms_as_dynar(void);

/*
void* MSG_process_get_property(msg_process_t, char* key)
void MSG_process_set_property(msg_process_t, char* key, void* data)
void MSG_vm_set_property(msg_vm_t, char* key, void* data)

void MSG_vm_setMemoryUsed(msg_vm_t vm, double size);
void MSG_vm_setCpuUsed(msg_vm_t vm, double inducedLoad);
  // inducedLoad: un pourcentage (>100 si ca charge plus d'un coeur;
  //                              <100 si c'est pas CPU intensive)
  // Contraintes à poser:
  //   HOST_Power >= CpuUsedVm (\forall VM) + CpuUsedTask (\forall Task)
  //   VM_coreAmount >= Load de toutes les tasks
*/

  /*
xbt_dynar_t<msg_vm_t> MSG_vm_get_list_from_host(msg_host_t)
xbt_dynar_t<msg_vm_t> MSG_vm_get_list_from_hosts(msg_dynar_t<msg_host_t>)
+ des fonctions de filtrage sur les dynar
*/
#include "instr/instr.h"



/* ****************************************************************************************** */
/* Used only by the bindings -- unclean pimple, please ignore if you're not writing a binding */
XBT_PUBLIC(smx_context_t) MSG_process_get_smx_ctx(msg_process_t process);

/* ****************************************************************************************** */
/* TUTORIAL: New API                                                                        */
/* Declare all functions for the API                                                          */
/* ****************************************************************************************** */
XBT_PUBLIC(int) MSG_new_API_fct(const char* param1, double param2);

SG_END_DECL()
#endif
