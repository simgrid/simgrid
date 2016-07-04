/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_H
#define MSG_H

#include "xbt.h"
#include "xbt/lib.h"
#include "simgrid/forward.h"
#include "simgrid/simix.h"

SG_BEGIN_DECL()

/* ******************************** Mailbox ************************************ */

/** @brief Mailbox datatype
 *  @ingroup msg_task_usage
 *
 * Object representing a communication rendez-vous point, on which
 * the sender finds the receiver it wants to communicate with. As a
 * MSG user, you will only rarely manipulate any of these objects
 * directly, since most of the public interface (such as
 * #MSG_task_send and friends) hide this object behind a string
 * alias. That mean that you don't provide the mailbox on which you
 * want to send your task, but only the name of this mailbox. */
typedef smx_mailbox_t msg_mailbox_t;

/* ******************************** Environment ************************************ */
typedef simgrid_As *msg_as_t;

/* ******************************** Host ************************************ */

/** @brief Host datatype.
    @ingroup m_host_management

    A <em>location</em> (or <em>host</em>) is any possible place where
    a process may run. Thus it is represented as a <em>physical
    resource with computing capabilities</em>, some <em>mailboxes</em>
    to enable running process to communicate with remote ones, and
    some <em>private data</em> that can be only accessed by local
    process.
 */
typedef sg_host_t msg_host_t;

typedef struct s_msg_host_priv {
  int        dp_enabled;
  xbt_dict_t dp_objs;
  double     dp_updated_by_deleted_tasks;
  int        is_migrating;

  xbt_dict_t affinity_mask_db;
  xbt_dynar_t file_descriptor_table;
} s_msg_host_priv_t;

/* ******************************** Task ************************************ */

typedef struct simdata_task *simdata_task_t;

typedef struct msg_task {
  char *name;                   /**< @brief task name if any */
  simdata_task_t simdata;       /**< @brief simulator data */
  void *data;                   /**< @brief user data */
  long long int counter;        /* task unique identifier for instrumentation */
  char *category;               /* task category for instrumentation */
} s_msg_task_t;

/** @brief Task datatype.
    @ingroup m_task_management

    A <em>task</em> may then be defined by a <em>computing
    amount</em>, a <em>message size</em> and some <em>private
    data</em>.
 */
typedef struct msg_task *msg_task_t;

/* ******************************** VM ************************************* */
typedef msg_host_t msg_vm_t;

/* ******************************** File ************************************ */

/** @brief Opaque object describing a File in MSG.
 *  @ingroup msg_file */
typedef xbt_dictelm_t msg_file_t;
typedef s_xbt_dictelm_t s_msg_file_t;

extern int MSG_FILE_LEVEL;
typedef struct simdata_file *simdata_file_t;

struct msg_file_priv  {
  char *fullpath;
  sg_size_t size;
  char* mount_point;
  char* storageId;
  char* storage_type;
  char* content_type;
  int desc_id;
  void *data;
  simdata_file_t simdata;
};
typedef struct msg_file_priv s_msg_file_priv_t;
typedef struct msg_file_priv* msg_file_priv_t;

static inline msg_file_priv_t MSG_file_priv(msg_file_t file){
  return (msg_file_priv_t )xbt_lib_get_level(file, MSG_FILE_LEVEL);
}

/* ******************************** Storage ************************************ */
/* TODO: PV: to comment */

extern int MSG_STORAGE_LEVEL;

/** @brief Storage datatype.
 *  @ingroup msg_storage_management
 *
 *  You should consider this as an opaque object.
 */
typedef xbt_dictelm_t msg_storage_t;
typedef s_xbt_dictelm_t s_msg_storage_t;

struct msg_storage_priv  {
  const char *hostname;
  void *data;
};
typedef struct msg_storage_priv  s_msg_storage_priv_t;
typedef struct msg_storage_priv* msg_storage_priv_t;

static inline msg_storage_priv_t MSG_storage_priv(msg_storage_t storage){
  return (msg_storage_priv_t )xbt_lib_get_level(storage, MSG_STORAGE_LEVEL);
}

/**
 * \brief @brief Communication action.
 * \ingroup msg_task_usage
 *
 * Object representing an ongoing communication between processes. Such beast is usually obtained by using #MSG_task_isend, #MSG_task_irecv or friends.
 */
typedef struct msg_comm *msg_comm_t;

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
typedef smx_process_t msg_process_t;

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
#define MSG_init(argc,argv)  do {                                                          \
  sg_version_check(SIMGRID_VERSION_MAJOR,SIMGRID_VERSION_MINOR,SIMGRID_VERSION_PATCH);\
    MSG_init_nocheck(argc,argv);                                                        \
  } while (0)

XBT_PUBLIC(void) MSG_init_nocheck(int *argc, char **argv);
XBT_PUBLIC(msg_error_t) MSG_main(void);
XBT_PUBLIC(void) MSG_function_register(const char *name,
                                       xbt_main_func_t code);
XBT_PUBLIC(void) MSG_function_register_default(xbt_main_func_t code);
XBT_PUBLIC(void) MSG_launch_application(const char *file);
/*Bypass the parser */
XBT_PUBLIC(void) MSG_set_function(const char *host_id,
                                  const char *function_name,
                                  xbt_dynar_t arguments);

XBT_PUBLIC(double) MSG_get_clock(void);
XBT_PUBLIC(unsigned long int) MSG_get_sent_msg(void);

/************************** Environment ***********************************/
XBT_PUBLIC(msg_as_t) MSG_environment_get_routing_root(void);
XBT_PUBLIC(const char *) MSG_environment_as_get_name(msg_as_t as);
XBT_PUBLIC(msg_as_t) MSG_environment_as_get_by_name(const char * name);
XBT_PUBLIC(xbt_dict_t) MSG_environment_as_get_routing_sons(msg_as_t as);
XBT_PUBLIC(const char *) MSG_environment_as_get_property_value(msg_as_t as, const char *name);
XBT_PUBLIC(xbt_dynar_t) MSG_environment_as_get_hosts(msg_as_t as);

/************************** File handling ***********************************/
XBT_PUBLIC(sg_size_t) MSG_file_read(msg_file_t fd, sg_size_t size);
XBT_PUBLIC(sg_size_t) MSG_file_write(msg_file_t fd, sg_size_t size);
XBT_PUBLIC(msg_file_t) MSG_file_open(const char* fullpath, void* data);
XBT_PUBLIC(void*) MSG_file_get_data(msg_file_t fd);
XBT_PUBLIC(msg_error_t) MSG_file_set_data(msg_file_t fd, void * data);
XBT_PUBLIC(int) MSG_file_close(msg_file_t fd);
XBT_PUBLIC(sg_size_t) MSG_file_get_size(msg_file_t fd);
XBT_PUBLIC(void) MSG_file_dump(msg_file_t fd);
XBT_PUBLIC(msg_error_t) MSG_file_unlink(msg_file_t fd);
XBT_PUBLIC(msg_error_t) MSG_file_seek(msg_file_t fd, sg_offset_t offset, int origin);
XBT_PUBLIC(sg_size_t) MSG_file_tell (msg_file_t fd);
XBT_PUBLIC(void) __MSG_file_get_info(msg_file_t fd);
XBT_PUBLIC(void) __MSG_file_priv_free(msg_file_priv_t priv);
XBT_PUBLIC(const char *) MSG_file_get_name(msg_file_t file);
XBT_PUBLIC(msg_error_t) MSG_file_move(msg_file_t fd, const char* fullpath);
XBT_PUBLIC(msg_error_t) MSG_file_rcopy(msg_file_t fd, msg_host_t host, const char* fullpath);
XBT_PUBLIC(msg_error_t) MSG_file_rmove(msg_file_t fd, msg_host_t host, const char* fullpath);
/************************** Storage handling ***********************************/
XBT_PUBLIC(const char *) MSG_storage_get_name(msg_storage_t storage);
XBT_PUBLIC(sg_size_t) MSG_storage_get_free_size(msg_storage_t storage);
XBT_PUBLIC(sg_size_t) MSG_storage_get_used_size(msg_storage_t storage);
XBT_PUBLIC(msg_storage_t) MSG_storage_get_by_name(const char *name);
XBT_PUBLIC(xbt_dict_t) MSG_storage_get_properties(msg_storage_t storage);
XBT_PUBLIC(void) MSG_storage_set_property_value(msg_storage_t storage, const char *name, char *value,void_f_pvoid_t free_ctn);
XBT_PUBLIC(const char *)MSG_storage_get_property_value(msg_storage_t storage, const char *name);
XBT_PUBLIC(xbt_dynar_t) MSG_storages_as_dynar(void);
XBT_PUBLIC(msg_error_t) MSG_storage_set_data(msg_storage_t host, void *data);
XBT_PUBLIC(void *) MSG_storage_get_data(msg_storage_t storage);
XBT_PUBLIC(xbt_dict_t) MSG_storage_get_content(msg_storage_t storage);
XBT_PUBLIC(sg_size_t) MSG_storage_get_size(msg_storage_t storage);
XBT_PUBLIC(msg_error_t) MSG_storage_file_move(msg_file_t fd, msg_host_t dest, char* mount, char* fullname);
XBT_PUBLIC(const char *) MSG_storage_get_host(msg_storage_t storage);
/************************** AS Router handling ************************************/
XBT_PUBLIC(const char *) MSG_as_router_get_property_value(const char* asr, const char *name);
XBT_PUBLIC(xbt_dict_t) MSG_as_router_get_properties(const char* asr);
XBT_PUBLIC(void) MSG_as_router_set_property_value(const char* asr, const char *name, char *value,void_f_pvoid_t free_ctn);

/************************** Host handling ***********************************/
XBT_PUBLIC(msg_host_t) MSG_host_by_name(const char *name);
#define MSG_get_host_by_name(n) MSG_host_by_name(n) /* Rewrite the old name into the new one transparently */
XBT_PUBLIC(msg_error_t) MSG_host_set_data(msg_host_t host, void *data);
XBT_PUBLIC(void *) MSG_host_get_data(msg_host_t host);
/** \ingroup m_host_management
 *
 * \brief Return the name of the #msg_host_t. */
#define MSG_host_get_name(host) sg_host_get_name(host)
XBT_PUBLIC(void) MSG_host_on(msg_host_t host);
XBT_PUBLIC(void) MSG_host_off(msg_host_t host);
XBT_PUBLIC(msg_host_t) MSG_host_self(void);
XBT_PUBLIC(double) MSG_host_get_speed(msg_host_t h);
XBT_PUBLIC(int) MSG_host_get_core_number(msg_host_t h);
XBT_PUBLIC(xbt_swag_t) MSG_host_get_process_list(msg_host_t h);
XBT_PUBLIC(int) MSG_host_is_on(msg_host_t h);
XBT_PUBLIC(int) MSG_host_is_off(msg_host_t h);

// deprecated
XBT_PUBLIC(double) MSG_get_host_speed(msg_host_t h);


XBT_PUBLIC(double) MSG_host_get_power_peak_at(msg_host_t h, int pstate);
XBT_PUBLIC(double) MSG_host_get_current_power_peak(msg_host_t h);
XBT_PUBLIC(int)    MSG_host_get_nb_pstates(msg_host_t h);
#define MSG_host_get_pstate(h)         sg_host_get_pstate(h)
#define MSG_host_set_pstate(h, pstate) sg_host_set_pstate(h, pstate)
XBT_PUBLIC(xbt_dynar_t) MSG_hosts_as_dynar(void);
XBT_PUBLIC(int) MSG_get_host_number(void);
XBT_PUBLIC(void) MSG_host_get_params(msg_host_t ind_pm, vm_params_t params);
XBT_PUBLIC(void) MSG_host_set_params(msg_host_t ind_pm, vm_params_t params);
XBT_PUBLIC(xbt_dict_t) MSG_host_get_mounted_storage_list(msg_host_t host);
XBT_PUBLIC(xbt_dynar_t) MSG_host_get_attached_storage_list(msg_host_t host);
XBT_PUBLIC(xbt_dict_t) MSG_host_get_storage_content(msg_host_t host);

/*property handlers*/
XBT_PUBLIC(xbt_dict_t) MSG_host_get_properties(msg_host_t host);
XBT_PUBLIC(const char *) MSG_host_get_property_value(msg_host_t host,
                                                     const char *name);
XBT_PUBLIC(void) MSG_host_set_property_value(msg_host_t host,
                                             const char *name, char *value,
                                             void_f_pvoid_t free_ctn);


XBT_PUBLIC(void) MSG_create_environment(const char *file);

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
XBT_PUBLIC(msg_process_t) MSG_process_attach(
  const char *name, void *data,
  msg_host_t host, xbt_dict_t properties);
XBT_PUBLIC(void) MSG_process_detach(void);

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
XBT_PUBLIC(void) MSG_process_on_exit(int_f_pvoid_pvoid_t fun, void *data);
XBT_PUBLIC(void) MSG_process_auto_restart_set(msg_process_t process, int auto_restart);

XBT_PUBLIC(msg_process_t) MSG_process_restart(msg_process_t process);

/************************** Task handling ************************************/
XBT_PUBLIC(msg_task_t) MSG_task_create(const char *name,
                                     double flops_amount,
                                     double bytes_amount, void *data);
XBT_PUBLIC(msg_task_t) MSG_parallel_task_create(const char *name,
                                              int host_nb,
                                              const msg_host_t * host_list,
                                              double *flops_amount,
                                              double *bytes_amount,
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

XBT_PUBLIC(msg_error_t) MSG_task_execute(msg_task_t task);
XBT_PUBLIC(msg_error_t) MSG_parallel_task_execute(msg_task_t task);
XBT_PUBLIC(void) MSG_task_set_priority(msg_task_t task, double priority);
XBT_PUBLIC(void) MSG_task_set_bound(msg_task_t task, double bound);
XBT_PUBLIC(void) MSG_task_set_affinity(msg_task_t task, msg_host_t host, unsigned long mask);

XBT_PUBLIC(msg_error_t) MSG_process_join(msg_process_t process, double timeout);
XBT_PUBLIC(msg_error_t) MSG_process_sleep(double nb_sec);

XBT_PUBLIC(void) MSG_task_set_flops_amount(msg_task_t task,
                                               double flops_amount);
XBT_PUBLIC(double) MSG_task_get_flops_amount(msg_task_t task);
XBT_PUBLIC(void) MSG_task_set_bytes_amount(msg_task_t task,
                                        double bytes_amount);

XBT_PUBLIC(double) MSG_task_get_remaining_communication(msg_task_t task);
XBT_PUBLIC(int) MSG_task_is_latency_bounded(msg_task_t task);
XBT_PUBLIC(double) MSG_task_get_bytes_amount(msg_task_t task);


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

XBT_PUBLIC(msg_error_t) MSG_task_receive_with_timeout_bounded(msg_task_t * task, const char *alias,  double timeout, double rate);
XBT_PUBLIC(msg_error_t) MSG_task_receive_bounded(msg_task_t * task, const char *alias,double rate);
#define MSG_task_recv_bounded(t,a,r) MSG_task_receive_bounded(t,a,r)

XBT_PUBLIC(msg_comm_t) MSG_task_isend(msg_task_t task, const char *alias);
XBT_PUBLIC(msg_comm_t) MSG_task_isend_bounded(msg_task_t task, const char *alias, double maxrate);
XBT_PUBLIC(msg_comm_t) MSG_task_isend_with_matching(msg_task_t task, const char *alias,
    int (*match_fun)(void*,void*, smx_synchro_t), void *match_data);

XBT_PUBLIC(void) MSG_task_dsend(msg_task_t task, const char *alias, void_f_pvoid_t cleanup);
XBT_PUBLIC(void) MSG_task_dsend_bounded(msg_task_t task, const char *alias, void_f_pvoid_t cleanup, double maxrate);
XBT_PUBLIC(msg_comm_t) MSG_task_irecv(msg_task_t * task, const char *alias);
XBT_PUBLIC(msg_comm_t) MSG_task_irecv_bounded(msg_task_t * task, const char *alias, double rate);
XBT_PUBLIC(int) MSG_comm_test(msg_comm_t comm);
XBT_PUBLIC(int) MSG_comm_testany(xbt_dynar_t comms);
XBT_PUBLIC(void) MSG_comm_destroy(msg_comm_t comm);
XBT_PUBLIC(msg_error_t) MSG_comm_wait(msg_comm_t comm, double timeout);
XBT_PUBLIC(void) MSG_comm_waitall(msg_comm_t * comm, int nb_elem, double timeout);
XBT_PUBLIC(int) MSG_comm_waitany(xbt_dynar_t comms);
XBT_PUBLIC(msg_task_t) MSG_comm_get_task(msg_comm_t comm);
XBT_PUBLIC(msg_error_t) MSG_comm_get_status(msg_comm_t comm);

XBT_PUBLIC(int) MSG_task_listen(const char *alias);
XBT_PUBLIC(msg_error_t) MSG_task_send_with_timeout(msg_task_t task, const char *alias, double timeout);
XBT_PUBLIC(msg_error_t) MSG_task_send_with_timeout_bounded(msg_task_t task, const char *alias, double timeout, double maxrate);
XBT_PUBLIC(msg_error_t) MSG_task_send(msg_task_t task, const char *alias);
XBT_PUBLIC(msg_error_t) MSG_task_send_bounded(msg_task_t task, const char *alias, double rate);
XBT_PUBLIC(int) MSG_task_listen_from(const char *alias);
XBT_PUBLIC(void) MSG_task_set_category (msg_task_t task, const char *category);
XBT_PUBLIC(const char *) MSG_task_get_category (msg_task_t task);

/************************** Mailbox handling ************************************/
/* @brief MSG_mailbox_new - create a new mailbox.
 * Creates a new mailbox identified by the key specified by the parameter alias and add it in the global dictionary.
 * @param  alias  The alias of the mailbox to create.
 * @return        The newly created mailbox.
 */
XBT_PUBLIC(msg_mailbox_t) MSG_mailbox_new(const char *alias);

/* @brief MSG_mailbox_get_by_alias - get a mailbox from its alias.
 * Returns the mailbox associated with the key specified by the parameter alias. If the mailbox does not exists,
 * the function creates it.
 * @param   alias    The alias of the mailbox to return.
 * @return           The mailbox associated with the alias specified as parameter or a new one if the key doesn't match.
 */
XBT_PUBLIC(msg_mailbox_t) MSG_mailbox_get_by_alias(const char *alias);

/* @brief MSG_mailbox_is_empty - test if a mailbox is empty.
 * Tests if a mailbox is empty (contains no msg task).
 * @param   mailbox  The mailbox to get test.
 * @return           1 if the mailbox is empty, 0 otherwise.
 */
XBT_PUBLIC(int) MSG_mailbox_is_empty(msg_mailbox_t mailbox);

/* @brief MSG_mailbox_set_async - set a mailbox as eager
 * Sets the mailbox to a permanent receiver mode. Messages sent to this mailbox will then be sent just after the send
 * is issued, without waiting for the corresponding receive.
 * This call should be done before issuing any receive, and on the receiver's side only
 * @param alias    The alias of the mailbox to modify.
 */
XBT_PUBLIC(void) MSG_mailbox_set_async(const char *alias);

/* @brief MSG_mailbox_get_head - get the task at the head of a mailbox.
 * Returns the task at the head of the mailbox. This function does not remove the task from the mailbox.
 * @param   mailbox  The mailbox concerned by the operation.
 * @return           The task at the head of the mailbox.
 */
XBT_PUBLIC(msg_task_t) MSG_mailbox_front(msg_mailbox_t mailbox);

XBT_PUBLIC(msg_error_t) MSG_mailbox_get_task_ext(msg_mailbox_t mailbox, msg_task_t * task, msg_host_t host,
                                                 double timeout);
XBT_PUBLIC(msg_error_t) MSG_mailbox_get_task_ext_bounded(msg_mailbox_t mailbox, msg_task_t *task, msg_host_t host,
                                                         double timeout, double rate);

/************************** Action handling **********************************/
XBT_PUBLIC(msg_error_t) MSG_action_trace_run(char *path);
XBT_PUBLIC(void) MSG_action_init(void);
XBT_PUBLIC(void) MSG_action_exit(void);

/** @brief Opaque type representing a semaphore
 *  @ingroup msg_synchro
 *  @hideinitializer
 */
typedef struct s_smx_sem *msg_sem_t; // Yeah that's a rename of the smx_sem_t which doesnt require smx_sem_t to be declared here
XBT_PUBLIC(msg_sem_t) MSG_sem_init(int initial_value);
XBT_PUBLIC(void) MSG_sem_acquire(msg_sem_t sem);
XBT_PUBLIC(msg_error_t) MSG_sem_acquire_timeout(msg_sem_t sem, double timeout);
XBT_PUBLIC(void) MSG_sem_release(msg_sem_t sem);
XBT_PUBLIC(void) MSG_sem_get_capacity(msg_sem_t sem);
XBT_PUBLIC(void) MSG_sem_destroy(msg_sem_t sem);
XBT_PUBLIC(int) MSG_sem_would_block(msg_sem_t sem);

/** @brief Opaque type representing a barrier identifier
 *  @ingroup msg_synchro
 *  @hideinitializer
 */

#define MSG_BARRIER_SERIAL_PROCESS -1
typedef struct s_xbt_bar *msg_bar_t;
XBT_PUBLIC(msg_bar_t) MSG_barrier_init( unsigned int count);
XBT_PUBLIC(void) MSG_barrier_destroy(msg_bar_t bar);
XBT_PUBLIC(int) MSG_barrier_wait(msg_bar_t bar);

/** @brief Opaque type describing a Virtual Machine.
 *  @ingroup msg_VMs
 *
 * All this is highly experimental and the interface will probably change in the future.
 * Please don't depend on this yet (although testing is welcomed if you feel so).
 * Usual lack of guaranty of any kind applies here, and is even increased.
 *
 */

XBT_PUBLIC(int) MSG_vm_is_created(msg_vm_t vm);
XBT_PUBLIC(int) MSG_vm_is_running(msg_vm_t vm);
XBT_PUBLIC(int) MSG_vm_is_migrating(msg_vm_t vm);

XBT_PUBLIC(int) MSG_vm_is_suspended(msg_vm_t vm);
XBT_PUBLIC(int) MSG_vm_is_saving(msg_vm_t vm);
XBT_PUBLIC(int) MSG_vm_is_saved(msg_vm_t vm);
XBT_PUBLIC(int) MSG_vm_is_restoring(msg_vm_t vm);


XBT_PUBLIC(const char*) MSG_vm_get_name(msg_vm_t vm);

// TODO add VDI later
XBT_PUBLIC(msg_vm_t) MSG_vm_create_core(msg_host_t location, const char *name);
XBT_PUBLIC(msg_vm_t) MSG_vm_create(msg_host_t ind_pm, const char *name,
    int core_nb, int mem_cap, int net_cap, char *disk_path, int disk_size, int mig_netspeed, int dp_intensity);

XBT_PUBLIC(void) MSG_vm_destroy(msg_vm_t vm);

XBT_PUBLIC(void) MSG_vm_start(msg_vm_t vm);

/* Shutdown the guest operating system. */
XBT_PUBLIC(void) MSG_vm_shutdown(msg_vm_t vm);

XBT_PUBLIC(void) MSG_vm_migrate(msg_vm_t vm, msg_host_t destination);

/* Suspend the execution of the VM, but keep its state on memory. */
XBT_PUBLIC(void) MSG_vm_suspend(msg_vm_t vm);
XBT_PUBLIC(void) MSG_vm_resume(msg_vm_t vm);

/* Save the VM state to a disk. */
XBT_PUBLIC(void) MSG_vm_save(msg_vm_t vm);
XBT_PUBLIC(void) MSG_vm_restore(msg_vm_t vm);

XBT_PUBLIC(msg_host_t) MSG_vm_get_pm(msg_vm_t vm);
XBT_PUBLIC(void) MSG_vm_set_bound(msg_vm_t vm, double bound);
XBT_PUBLIC(void) MSG_vm_set_affinity(msg_vm_t vm, msg_host_t pm, unsigned long mask);

/* TODO: do we need this? */
// XBT_PUBLIC(xbt_dynar_t) MSG_vms_as_dynar(void);

/*
void* MSG_process_get_property(msg_process_t, char* key)
void MSG_process_set_property(msg_process_t, char* key, void* data)
void MSG_vm_set_property(msg_vm_t, char* key, void* data)

void MSG_vm_setMemoryUsed(msg_vm_t vm, double size);
void MSG_vm_setCpuUsed(msg_vm_t vm, double inducedLoad);
  // inducedLoad: a percentage (>100 if it loads more than one core;
  //                            <100 if it's not CPU intensive)
  // Required contraints:
  //   HOST_Power >= CpuUsedVm (\forall VM) + CpuUsedTask (\forall Task)
  //   VM_coreAmount >= Load of all tasks
*/

  /*
xbt_dynar_t<msg_vm_t> MSG_vm_get_list_from_host(msg_host_t)
xbt_dynar_t<msg_vm_t> MSG_vm_get_list_from_hosts(msg_dynar_t<msg_host_t>)
+ filtering functions on dynars
*/
#include "simgrid/instr.h"



/* ****************************************************************************************** */
/* Used only by the bindings -- unclean pimple, please ignore if you're not writing a binding */
XBT_PUBLIC(smx_context_t) MSG_process_get_smx_ctx(msg_process_t process);


/* Functions renamed in 3.14 */
#define MSG_mailbox_get_head(m) MSG_mailbox_front(m)


SG_END_DECL()
#endif
