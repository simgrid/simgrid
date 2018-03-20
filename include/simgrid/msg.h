/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MSG_H
#define SIMGRID_MSG_H

#include <simgrid/forward.h>
#include <simgrid/host.h>
#include <simgrid/instr.h>
#include <simgrid/plugins/live_migration.h>
#include <simgrid/storage.h>
#include <simgrid/vm.h>
#include <simgrid/zone.h>
#include <xbt/base.h>
#include <xbt/dict.h>
#include <xbt/dynar.h>

#ifdef __cplusplus
#include <map>
#include <simgrid/simix.h>
namespace simgrid {
namespace msg {
class Comm;
}
}
typedef simgrid::msg::Comm sg_msg_Comm;
#else
typedef struct msg_Comm sg_msg_Comm;
#endif

SG_BEGIN_DECL()

/* *************************** Network Zones ******************************** */
#define msg_as_t msg_netzone_t /* portability macro */
typedef sg_netzone_t msg_netzone_t;

#define MSG_zone_get_root() sg_zone_get_root()
#define MSG_zone_get_name(zone) sg_zone_get_name(zone)
#define MSG_zone_get_by_name(name) sg_zone_get_by_name(name)
#define MSG_zone_get_sons(zone, whereto) sg_zone_get_sons(zone, whereto)
#define MSG_zone_get_property_value(zone, name) sg_zone_get_property_value(zone, name)
#define MSG_zone_set_property_value(zone, name, value) sg_zone_set_property_value(zone, name, value)
#define MSG_zone_get_hosts(zone, whereto) sg_zone_get_hosts(zone, whereto)

/* ******************************** Hosts ************************************ */
typedef sg_host_t msg_host_t;

#define MSG_get_host_number() sg_host_count()
#define MSG_get_host_by_name(n) sg_host_by_name(n) /* Rewrite the old name into the new one transparently */

#define MSG_hosts_as_dynar() sg_hosts_as_dynar()

#define MSG_host_by_name(name) sg_host_by_name(name)
#define MSG_host_get_name(host) sg_host_get_name(host)
#define MSG_host_get_data(host) sg_host_user(host)
#define MSG_host_set_data(host, data) sg_host_user_set(host, data)
#define MSG_host_get_mounted_storage_list(host) sg_host_get_mounted_storage_list(host)
#define MSG_host_get_attached_storage_list(host) host_get_attached_storage_list(host)
#define MSG_host_get_speed(host) sg_host_speed(host)
#define MSG_host_get_power_peak_at(host, pstate_index) sg_host_get_pstate_speed(host, pstate_index)
#define MSG_host_get_core_number(host) sg_host_core_count(host)
#define MSG_host_self() sg_host_self()
#define MSG_host_get_nb_pstates(host) sg_host_get_nb_pstates(host)
#define MSG_host_get_pstate(h) sg_host_get_pstate(h)
#define MSG_host_set_pstate(h, pstate) sg_host_set_pstate(h, pstate)
#define MSG_host_on(h) sg_host_turn_on(h)
#define MSG_host_off(h) sg_host_turn_off(h)
#define MSG_host_is_on(h) sg_host_is_on(h)
#define MSG_host_is_off(h) sg_host_is_off(h)
#define MSG_host_get_properties(host) sg_host_get_properties(host)
#define MSG_host_get_property_value(host, name) sg_host_get_property_value(host, name)
#define MSG_host_set_property_value(host, name, value) sg_host_set_property_value(host, name, value)
#define MSG_host_get_process_list(host, whereto) sg_host_get_actor_list(host, whereto)

XBT_ATTRIB_DEPRECATED_v320("Use MSG_host_get_speed(): v3.20 will drop MSG_host_get_current_power_peak() "
                           "completely.") static inline double MSG_host_get_current_power_peak(msg_host_t host)
{
  return sg_host_speed(host);
}

/* ******************************** VMs ************************************* */
typedef sg_vm_t msg_vm_t;

#define MSG_vm_create_core(vm, name) sg_vm_create_core(vm, name)
#define MSG_vm_create_multicore(vm, name, coreAmount) sg_vm_create_multicore(vm, name, coreAmount)
XBT_ATTRIB_DEPRECATED_v322("Use sg_vm_create_migratable() from the live migration plugin: "
                           "v3.22 will drop MSG_vm_create() completely.") XBT_PUBLIC sg_vm_t
    MSG_vm_create(sg_host_t ind_pm, const char* name, int coreAmount, int ramsize, int mig_netspeed, int dp_intensity);

#define MSG_vm_is_created(vm) sg_vm_is_created(vm)
#define MSG_vm_is_running(vm) sg_vm_is_running(vm)
#define MSG_vm_is_suspended(vm) sg_vm_is_suspended(vm)

#define MSG_vm_get_name(vm) sg_vm_get_name(vm)
#define MSG_vm_set_ramsize(vm, size) sg_vm_set_ramsize(vm, size)
#define MSG_vm_get_ramsize(vm) sg_vm_get_ramsize(vm)
#define MSG_vm_set_bound(vm, bound) sg_vm_set_bound(vm, bound)
#define MSG_vm_get_pm(vm) sg_vm_get_pm(vm)

#define MSG_vm_start(vm) sg_vm_start(vm)
#define MSG_vm_suspend(vm) sg_vm_suspend(vm)
#define MSG_vm_resume(vm) sg_vm_resume(vm)
#define MSG_vm_shutdown(vm) sg_vm_shutdown(vm)
#define MSG_vm_destroy(vm) sg_vm_destroy(vm)

/* ******************************** Storage ************************************ */
typedef sg_storage_t msg_storage_t;

#define MSG_storage_get_name(storage) sg_storage_get_name(storage)
#define MSG_storage_get_by_name(name) sg_storage_get_by_name(name)
#define MSG_storage_get_properties(storage) sg_storage_get_properties(storage)
#define MSG_storage_set_property_value(storage, name, value) sg_storage_set_property_value(storage, name, value)
#define MSG_storage_get_property_value(storage, name) sg_storage_get_property_value(storage, name)
#define MSG_storages_as_dynar() sg_storages_as_dynar()
#define MSG_storage_set_data(storage, data) sg_storage_set_data(storage, data)
#define MSG_storage_get_data(storage) sg_storage_get_data(storage)
#define MSG_storage_get_host(storage) sg_storage_get_host(storage)
#define MSG_storage_read(storage, size) sg_storage_read(storage, size)
#define MSG_storage_write(storage, size) sg_storage_write(storage, size)

/* ******************************** File ************************************ */
typedef sg_file_t msg_file_t;
XBT_PUBLIC_DATA int sg_storage_max_file_descriptors;

/**
 * \brief @brief Communication action.
 * \ingroup msg_task_usage
 *
 * Object representing an ongoing communication between processes. Such beast is usually obtained by using #MSG_task_isend, #MSG_task_irecv or friends.
 */
typedef sg_msg_Comm* msg_comm_t;

/* ******************************** Task ************************************ */

typedef struct s_simdata_task_t* simdata_task_t;

typedef struct msg_task {
  char* name;             /**< @brief task name if any */
  simdata_task_t simdata; /**< @brief simulator data */
  void* data;             /**< @brief user data */
  long long int counter;  /* task unique identifier for instrumentation */
  char* category;         /* task category for instrumentation */
} s_msg_task_t;

/** @brief Task datatype.
    @ingroup m_task_management

    A <em>task</em> may then be defined by a <em>computing
    amount</em>, a <em>message size</em> and some <em>private
    data</em>.
 */

typedef struct msg_task* msg_task_t;

/** \brief Default value for an uninitialized #msg_task_t.
    \ingroup m_task_management
*/
#define MSG_TASK_UNINITIALIZED NULL

/* ****************************** Process *********************************** */

/** @brief Process datatype.
    @ingroup m_process_management

    A process may be defined as a <em>code</em>, with some
    <em>private data</em>, executing in a <em>location</em>.

    You should not access directly to the fields of the pointed
    structure, but always use the provided API to interact with
    processes.
 */
typedef s4u_Actor* msg_process_t;

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
XBT_PUBLIC void MSG_config(const char* key, const char* value);
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
#define MSG_init(argc,argv)  do {                                                          \
  sg_version_check(SIMGRID_VERSION_MAJOR,SIMGRID_VERSION_MINOR,SIMGRID_VERSION_PATCH);\
    MSG_init_nocheck(argc,argv);                                                        \
  } while (0)

XBT_PUBLIC void MSG_init_nocheck(int* argc, char** argv);
XBT_PUBLIC msg_error_t MSG_main();
XBT_PUBLIC void MSG_function_register(const char* name, xbt_main_func_t code);
XBT_PUBLIC void MSG_function_register_default(xbt_main_func_t code);
XBT_PUBLIC void MSG_create_environment(const char* file);
XBT_PUBLIC void MSG_launch_application(const char* file);
/*Bypass the parser */
XBT_PUBLIC void MSG_set_function(const char* host_id, const char* function_name, xbt_dynar_t arguments);

XBT_PUBLIC double MSG_get_clock();
XBT_PUBLIC unsigned long int MSG_get_sent_msg();

/************************** Process handling *********************************/
XBT_PUBLIC msg_process_t MSG_process_create(const char* name, xbt_main_func_t code, void* data, msg_host_t host);
XBT_PUBLIC msg_process_t MSG_process_create_with_arguments(const char* name, xbt_main_func_t code, void* data,
                                                           msg_host_t host, int argc, char** argv);
XBT_PUBLIC msg_process_t MSG_process_create_with_environment(const char* name, xbt_main_func_t code, void* data,
                                                             msg_host_t host, int argc, char** argv,
                                                             xbt_dict_t properties);

XBT_PUBLIC msg_process_t MSG_process_attach(const char* name, void* data, msg_host_t host, xbt_dict_t properties);
XBT_PUBLIC void MSG_process_detach();

XBT_PUBLIC void MSG_process_kill(msg_process_t process);
XBT_PUBLIC int MSG_process_killall();
XBT_PUBLIC msg_error_t MSG_process_migrate(msg_process_t process, msg_host_t host);
XBT_PUBLIC void MSG_process_yield();

XBT_PUBLIC void* MSG_process_get_data(msg_process_t process);
XBT_PUBLIC msg_error_t MSG_process_set_data(msg_process_t process, void* data);
XBT_PUBLIC void MSG_process_set_data_cleanup(void_f_pvoid_t data_cleanup);
XBT_PUBLIC msg_host_t MSG_process_get_host(msg_process_t process);
XBT_PUBLIC msg_process_t MSG_process_from_PID(int PID);
XBT_PUBLIC int MSG_process_get_PID(msg_process_t process);
XBT_PUBLIC int MSG_process_get_PPID(msg_process_t process);
XBT_PUBLIC const char* MSG_process_get_name(msg_process_t process);
XBT_PUBLIC int MSG_process_self_PID();
XBT_PUBLIC int MSG_process_self_PPID();
XBT_PUBLIC const char* MSG_process_self_name();
XBT_PUBLIC msg_process_t MSG_process_self();
XBT_PUBLIC xbt_dynar_t MSG_processes_as_dynar();
XBT_PUBLIC int MSG_process_get_number();

XBT_PUBLIC msg_error_t MSG_process_set_kill_time(msg_process_t process, double kill_time);

/*property handlers*/
XBT_PUBLIC xbt_dict_t MSG_process_get_properties(msg_process_t process);
XBT_PUBLIC const char* MSG_process_get_property_value(msg_process_t process, const char* name);

XBT_PUBLIC msg_error_t MSG_process_suspend(msg_process_t process);
XBT_PUBLIC msg_error_t MSG_process_resume(msg_process_t process);
XBT_PUBLIC int MSG_process_is_suspended(msg_process_t process);
XBT_PUBLIC void MSG_process_on_exit(int_f_pvoid_pvoid_t fun, void* data);
XBT_PUBLIC void MSG_process_auto_restart_set(msg_process_t process, int auto_restart);

XBT_PUBLIC void MSG_process_daemonize(msg_process_t process);
XBT_PUBLIC msg_process_t MSG_process_restart(msg_process_t process);
XBT_PUBLIC void MSG_process_ref(msg_process_t process);
XBT_PUBLIC void MSG_process_unref(msg_process_t process);

/************************** Task handling ************************************/
XBT_PUBLIC msg_task_t MSG_task_create(const char* name, double flops_amount, double bytes_amount, void* data);
XBT_PUBLIC msg_task_t MSG_parallel_task_create(const char* name, int host_nb, const msg_host_t* host_list,
                                               double* flops_amount, double* bytes_amount, void* data);
XBT_PUBLIC void* MSG_task_get_data(msg_task_t task);
XBT_PUBLIC void MSG_task_set_data(msg_task_t task, void* data);
XBT_PUBLIC void MSG_task_set_copy_callback(void (*callback)(msg_task_t task, msg_process_t src, msg_process_t dst));
XBT_PUBLIC msg_process_t MSG_task_get_sender(msg_task_t task);
XBT_PUBLIC msg_host_t MSG_task_get_source(msg_task_t task);
XBT_PUBLIC const char* MSG_task_get_name(msg_task_t task);
XBT_PUBLIC void MSG_task_set_name(msg_task_t task, const char* name);
XBT_PUBLIC msg_error_t MSG_task_cancel(msg_task_t task);
XBT_PUBLIC msg_error_t MSG_task_destroy(msg_task_t task);

XBT_PUBLIC msg_error_t MSG_task_execute(msg_task_t task);
XBT_PUBLIC msg_error_t MSG_parallel_task_execute(msg_task_t task);
XBT_PUBLIC msg_error_t MSG_parallel_task_execute_with_timeout(msg_task_t task, double timeout);
XBT_PUBLIC void MSG_task_set_priority(msg_task_t task, double priority);
XBT_PUBLIC void MSG_task_set_bound(msg_task_t task, double bound);

XBT_PUBLIC msg_error_t MSG_process_join(msg_process_t process, double timeout);
XBT_PUBLIC msg_error_t MSG_process_sleep(double nb_sec);

XBT_PUBLIC void MSG_task_set_flops_amount(msg_task_t task, double flops_amount);
XBT_PUBLIC double MSG_task_get_flops_amount(msg_task_t task);
XBT_PUBLIC double MSG_task_get_remaining_work_ratio(msg_task_t task);
XBT_PUBLIC void MSG_task_set_bytes_amount(msg_task_t task, double bytes_amount);

XBT_PUBLIC double MSG_task_get_remaining_communication(msg_task_t task);
XBT_PUBLIC double MSG_task_get_bytes_amount(msg_task_t task);

XBT_PUBLIC msg_error_t MSG_task_receive_ext(msg_task_t* task, const char* alias, double timeout, msg_host_t host);

XBT_PUBLIC msg_error_t MSG_task_receive_with_timeout(msg_task_t* task, const char* alias, double timeout);

XBT_PUBLIC msg_error_t MSG_task_receive(msg_task_t* task, const char* alias);
#define MSG_task_recv(t,a) MSG_task_receive(t,a)

XBT_PUBLIC msg_error_t MSG_task_receive_ext_bounded(msg_task_t* task, const char* alias, double timeout,
                                                    msg_host_t host, double rate);

XBT_PUBLIC msg_error_t MSG_task_receive_with_timeout_bounded(msg_task_t* task, const char* alias, double timeout,
                                                             double rate);
XBT_PUBLIC msg_error_t MSG_task_receive_bounded(msg_task_t* task, const char* alias, double rate);
#define MSG_task_recv_bounded(t,a,r) MSG_task_receive_bounded(t,a,r)

XBT_PUBLIC msg_comm_t MSG_task_isend(msg_task_t task, const char* alias);
XBT_PUBLIC msg_comm_t MSG_task_isend_bounded(msg_task_t task, const char* alias, double maxrate);
XBT_ATTRIB_DEPRECATED_v320(
    "This function will be removed from SimGrid v3.20. If you really need this function, please speak up quickly.")
    XBT_PUBLIC msg_comm_t MSG_task_isend_with_matching(msg_task_t task, const char* alias,
                                                       int (*match_fun)(void*, void*, void*), void* match_data);

XBT_PUBLIC void MSG_task_dsend(msg_task_t task, const char* alias, void_f_pvoid_t cleanup);
XBT_PUBLIC void MSG_task_dsend_bounded(msg_task_t task, const char* alias, void_f_pvoid_t cleanup, double maxrate);
XBT_PUBLIC msg_comm_t MSG_task_irecv(msg_task_t* task, const char* alias);
XBT_PUBLIC msg_comm_t MSG_task_irecv_bounded(msg_task_t* task, const char* alias, double rate);
XBT_PUBLIC int MSG_comm_test(msg_comm_t comm);
XBT_PUBLIC int MSG_comm_testany(xbt_dynar_t comms);
XBT_PUBLIC void MSG_comm_destroy(msg_comm_t comm);
XBT_PUBLIC msg_error_t MSG_comm_wait(msg_comm_t comm, double timeout);
XBT_PUBLIC void MSG_comm_waitall(msg_comm_t* comm, int nb_elem, double timeout);
XBT_PUBLIC int MSG_comm_waitany(xbt_dynar_t comms);
XBT_PUBLIC msg_task_t MSG_comm_get_task(msg_comm_t comm);
XBT_PUBLIC msg_error_t MSG_comm_get_status(msg_comm_t comm);

XBT_PUBLIC int MSG_task_listen(const char* alias);
XBT_PUBLIC msg_error_t MSG_task_send_with_timeout(msg_task_t task, const char* alias, double timeout);
XBT_PUBLIC msg_error_t MSG_task_send_with_timeout_bounded(msg_task_t task, const char* alias, double timeout,
                                                          double maxrate);
XBT_PUBLIC msg_error_t MSG_task_send(msg_task_t task, const char* alias);
XBT_PUBLIC msg_error_t MSG_task_send_bounded(msg_task_t task, const char* alias, double rate);
XBT_PUBLIC int MSG_task_listen_from(const char* alias);
XBT_PUBLIC void MSG_task_set_category(msg_task_t task, const char* category);
XBT_PUBLIC const char* MSG_task_get_category(msg_task_t task);

/************************** Mailbox handling ************************************/

/* @brief MSG_mailbox_set_async - set a mailbox as eager
 * Sets the mailbox to a permanent receiver mode. Messages sent to this mailbox will then be sent just after the send
 * is issued, without waiting for the corresponding receive.
 * This call should be done before issuing any receive, and on the receiver's side only
 * @param alias    The alias of the mailbox to modify.
 */
XBT_PUBLIC void MSG_mailbox_set_async(const char* alias);

/************************** Action handling **********************************/
XBT_PUBLIC msg_error_t MSG_action_trace_run(char* path);
XBT_PUBLIC void MSG_action_init();
XBT_PUBLIC void MSG_action_exit();

/** @brief Opaque type representing a semaphore
 *  @ingroup msg_synchro
 *  @hideinitializer
 */
typedef struct s_smx_sem_t* msg_sem_t; // Yeah that's a rename of the smx_sem_t which doesnt require smx_sem_t to be
                                       // declared here
XBT_PUBLIC msg_sem_t MSG_sem_init(int initial_value);
XBT_PUBLIC void MSG_sem_acquire(msg_sem_t sem);
XBT_PUBLIC msg_error_t MSG_sem_acquire_timeout(msg_sem_t sem, double timeout);
XBT_PUBLIC void MSG_sem_release(msg_sem_t sem);
XBT_PUBLIC int MSG_sem_get_capacity(msg_sem_t sem);
XBT_PUBLIC void MSG_sem_destroy(msg_sem_t sem);
XBT_PUBLIC int MSG_sem_would_block(msg_sem_t sem);

/** @brief Opaque type representing a barrier identifier
 *  @ingroup msg_synchro
 *  @hideinitializer
 */

#define MSG_BARRIER_SERIAL_PROCESS -1
typedef struct s_msg_bar_t* msg_bar_t;
XBT_PUBLIC msg_bar_t MSG_barrier_init(unsigned int count);
XBT_PUBLIC void MSG_barrier_destroy(msg_bar_t bar);
XBT_PUBLIC int MSG_barrier_wait(msg_bar_t bar);

/* ****************************************************************************************** */
/* Used only by the bindings -- unclean pimple, please ignore if you're not writing a binding */
XBT_PUBLIC smx_context_t MSG_process_get_smx_ctx(msg_process_t process);

SG_END_DECL()

#ifdef __cplusplus
XBT_PUBLIC msg_process_t MSG_process_create_from_stdfunc(const char* name, std::function<void()> code, void* data,
                                                         msg_host_t host,
                                                         std::map<std::string, std::string>* properties);
#endif

#endif
