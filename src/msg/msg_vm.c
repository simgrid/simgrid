/* Copyright (c) 2012. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// QUESTIONS:
// 1./ check how and where a new VM is added to the list of the hosts
// 2./ Diff between SIMIX_Actions and SURF_Actions
// => SIMIX_actions : point synchro entre processus de niveau (theoretically speaking I do not have to create such SIMIX_ACTION
// =>  Surf_Actions

// TODO
//	MSG_TRACE can be revisited in order to use  the host
//	To implement a mixed model between workstation and vm_workstation,
//     please give a look at surf_model_private_t model_private at SURF Level and to the share resource functions
//     double (*share_resources) (double now);
//	For the action into the vm workstation model, we should be able to leverage the usual one (and if needed, look at
// 		the workstation model.

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_vm, msg,
                                "Cloud-oriented parts of the MSG API");


/* **** ******** GENERAL ********* **** */

/** \ingroup m_vm_management
 * \brief Returns the value of a given vm property
 *
 * \param vm a vm
 * \param name a property name
 * \return value of a property (or NULL if property not set)
 */

const char *MSG_vm_get_property_value(msg_vm_t vm, const char *name)
{
  return MSG_host_get_property_value(vm, name);
}

/** \ingroup m_vm_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this host
 *
 * \param vm a vm
 * \return a dict containing the properties
 */
xbt_dict_t MSG_vm_get_properties(msg_vm_t vm)
{
  xbt_assert((vm != NULL), "Invalid parameters (vm is NULL)");

  return (simcall_host_get_properties(vm));
}

/** \ingroup m_host_management
 * \brief Change the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \param value what to change the property to
 * \param free_ctn the freeing function to use to kill the value on need
 */
void MSG_vm_set_property_value(msg_vm_t vm, const char *name, void *value, void_f_pvoid_t free_ctn)
{
  xbt_dict_set(MSG_host_get_properties(vm), name, value, free_ctn);
}

/** \ingroup msg_vm_management
 * \brief Finds a msg_vm_t using its name.
 *
 * This is a name directory service
 * \param name the name of a vm.
 * \return the corresponding vm
 *
 * Please note that a VM is a specific host. Hence, you should give a different name
 * for each VM/PM.
 */

msg_vm_t MSG_vm_get_by_name(const char *name)
{
	return MSG_get_host_by_name(name);
}

/** \ingroup m_vm_management
 *
 * \brief Return the name of the #msg_host_t.
 *
 * This functions checks whether \a host is a valid pointer or not and return
   its name.
 */
const char *MSG_vm_get_name(msg_vm_t vm)
{
  return MSG_host_get_name(vm);
}


/* **** Check state of a VM **** */
static inline int __MSG_vm_is_state(msg_vm_t vm, e_surf_vm_state_t state)
{
  return simcall_vm_get_state(vm) == state;
}

/** @brief Returns whether the given VM has just reated, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_created(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_CREATED);
}

/** @brief Returns whether the given VM is currently running
 *  @ingroup msg_VMs
 */
int MSG_vm_is_running(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_RUNNING);
}

/** @brief Returns whether the given VM is currently migrating
 *  @ingroup msg_VMs
 */
int MSG_vm_is_migrating(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_MIGRATING);
}

/** @brief Returns whether the given VM is currently suspended, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_suspended(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_SUSPENDED);
}

/** @brief Returns whether the given VM is being saved (FIXME: live saving or not?).
 *  @ingroup msg_VMs
 */
int MSG_vm_is_saving(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_SAVING);
}

/** @brief Returns whether the given VM has been saved, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_saved(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_SAVED);
}

/** @brief Returns whether the given VM is being restored, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_restoring(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_RESTORING);
}



/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/* **** ******** MSG vm actions ********* **** */

/** @brief Create a new VM with specified parameters.
 *  @ingroup msg_VMs*
 *
 */
msg_vm_t MSG_vm_create(msg_host_t ind_pm, const char *name,
	                                     int ncpus, int ramsize, int net_cap, char *disk_path, int disksize)
{
  msg_vm_t vm = MSG_vm_create_core(ind_pm, name);

  {
    s_ws_params_t params;
    memset(&params, 0, sizeof(params));
    params.ramsize = ramsize;
    params.overcommit = 0;
    simcall_host_set_params(vm, &params);
  }

  /* TODO: We will revisit the disk support later. */

  return vm;
}


/** @brief Create a new VM object. The VM is not yet started. The resource of the VM is allocated upon MSG_vm_start().
 *  @ingroup msg_VMs*
 *
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
msg_vm_t MSG_vm_create_core(msg_host_t ind_pm, const char *name)
{
  /* make sure the VM of the same name does not exit */
  {
    void *ind_host_tmp = xbt_lib_get_elm_or_null(host_lib, name);
    if (ind_host_tmp) {
      XBT_ERROR("host %s already exits", name);
      return NULL;
    }
  }

  /* Note: ind_vm and vm_workstation point to the same elm object. */
  msg_vm_t ind_vm = NULL;
  void *ind_vm_workstation =  NULL;

  /* Ask the SIMIX layer to create the surf vm resource */
  ind_vm_workstation = simcall_vm_create(name, ind_pm);
  ind_vm = (msg_vm_t) __MSG_host_create(ind_vm_workstation);

  XBT_DEBUG("A new VM (%s) has been created", name);

  #ifdef HAVE_TRACING
  TRACE_msg_vm_create(name, ind_pm);
  #endif

  return ind_vm;
}


/** @brief Start a vm (i.e., boot the guest operating system)
 *  @ingroup msg_VMs
 *
 *  If the VM cannot be started, an exception is generated.
 *
 */
void MSG_vm_start(msg_vm_t vm)
{
  simcall_vm_start(vm);

  #ifdef HAVE_TRACING
  TRACE_msg_vm_start(vm);
  #endif
}



/** @brief Immediately kills all processes within the given VM. Any memory that they allocated will be leaked.
 *  @ingroup msg_VMs
 *
 * FIXME: No extra delay occurs. If you want to simulate this too, you want to
 * use a #MSG_process_sleep() or something. I'm not quite sure.
 */
void MSG_vm_shutdown(msg_vm_t vm)
{
  /* msg_vm_t equals to msg_host_t */
  simcall_vm_shutdown(vm);

  // #ifdef HAVE_TRACING
  // TRACE_msg_vm_(vm);
  // #endif
}


static int migration_rx_fun(int argc, char *argv[])
{
  const char *pr_name = MSG_process_get_name(MSG_process_self());
  const char *host_name = MSG_host_get_name(MSG_host_self());
  int need_exit = 0;

  xbt_assert(argc == 3);
  const char *mbox = argv[1];
  const char *mbox_ctl = argv[2];

  for (;;) {
    msg_task_t task = NULL;
    MSG_task_recv(&task, mbox);

    if (strcmp(task->name, "finalize") == 0)
      need_exit = 1;

    MSG_task_destroy(task);

    if (need_exit)
      break;
  }


  {
    msg_task_t task = MSG_task_create("quit_migration", 0, 0, NULL);
    msg_error_t ret = MSG_task_send(task, mbox_ctl);
    xbt_assert(ret == MSG_OK);
  }

  XBT_INFO("%s@%s done", pr_name, host_name);

  return 0;
}

static int migration_tx_fun(int argc, char *argv[])
{
  const char *pr_name = MSG_process_get_name(MSG_process_self());
  const char *host_name = MSG_host_get_name(MSG_host_self());

  xbt_assert(argc == 3);
  const char *mbox = argv[1];
  long ramsize = atol(argv[2]);

  char *task_name = bprintf("task-%s", mbox);
  msg_task_t task = MSG_task_create(task_name, 0, ramsize, NULL);
  msg_error_t ret = MSG_task_send(task, mbox);
  xbt_assert(ret == MSG_OK);

  xbt_free(task_name);

  {
    msg_task_t task = MSG_task_create("finalize", 0, 0, NULL);
    msg_error_t ret = MSG_task_send(task, mbox);
    xbt_assert(ret == MSG_OK);
  }

  XBT_INFO("%s@%s done", pr_name, host_name);

  return 0;
}

static void create_dummy_task(msg_vm_t vm, msg_host_t old_pm, msg_host_t new_pm, long ramsize)
{
  if (ramsize == 0)
    XBT_WARN("migrate a VM, but ramsize is zero");

  /* We have two mailboxes. mbox is used to transfer migration data between
   * source and destiantion PMs. mbox_ctl is used to detect the completion of a
   * migration. The names of these mailboxes must not conflict with others. */

  char *suffix = bprintf("mig-%s(%s-%s)", vm->key, old_pm->key, new_pm->key);
  char *mbox = bprintf("__MBOX:%s", suffix);
  char *mbox_ctl = bprintf("__MBOX:%s:CTL", suffix);

  {
    const char *pr_name = "mig_tx";
    int nargvs = 4;
    char **argv = xbt_new(char *, nargvs);
    argv[0] = xbt_strdup(pr_name);
    argv[1] = xbt_strdup(mbox);
    argv[2] = bprintf("%ld", ramsize);
    argv[3] = NULL;

    msg_process_t pr = MSG_process_create_with_arguments(pr_name, migration_tx_fun, NULL, old_pm, nargvs - 1, argv);
  }

  {
    const char *pr_name = "mig_rx";
    int nargvs = 4;
    char **argv = xbt_new(char *, nargvs);
    argv[0] = xbt_strdup(pr_name);
    argv[1] = xbt_strdup(mbox);
    argv[2] = xbt_strdup(mbox_ctl);
    argv[3] = NULL;

    msg_process_t pr = MSG_process_create_with_arguments(pr_name, migration_rx_fun, NULL, new_pm, nargvs - 1, argv);
  }

  /* wait until the migration have finished */
  {
    msg_task_t task = NULL;
    msg_error_t ret = MSG_task_recv(&task, mbox_ctl);
    xbt_assert(ret == MSG_OK);
    xbt_assert(strcmp(task->name, "quit_migration") == 0);
  }

  xbt_free(suffix);
  xbt_free(mbox);
  xbt_free(mbox_ctl);
}


/** @brief Migrate the VM to the given host.
 *  @ingroup msg_VMs
 *
 * FIXME: No migration cost occurs. If you want to simulate this too, you want to use a
 * MSG_task_send() before or after, depending on whether you want to do cold or hot
 * migration.
 */
void MSG_vm_migrate(msg_vm_t vm, msg_host_t new_pm)
{
  /* some thoughts:
   * - One approach is ...
   *   We first create a new VM (i.e., destination VM) on the destination
   *   physical host. The destination VM will receive the state of the source
   *   VM over network. We will finally destroy the source VM.
   *   - This behavior is similar to the way of migration in the real world.
   *     Even before a migration is completed, we will see a destination VM,
   *     consuming resources.
   *   - We have to relocate all processes. The existing process migraion code
   *     will work for this?
   *   - The name of the VM is a somewhat unique ID in the code. It is tricky
   *     for the destination VM?
   *
   * - Another one is ...
   *   We update the information of the given VM to place it to the destination
   *   physical host.
   *
   * The second one would be easier.
   *   
   */

  msg_host_t old_pm = simcall_vm_get_pm(vm);

  s_ws_params_t params;
  simcall_host_get_params(vm, &params);
  long ramsize = params.ramsize;

  create_dummy_task(vm, old_pm, new_pm, ramsize);

  simcall_vm_migrate(vm, new_pm);

  XBT_DEBUG("VM(%s) moved from PM(%s) to PM(%s)", vm->key, old_pm->key, new_pm->key);

  #ifdef HAVE_TRACING
  TRACE_msg_vm_change_host(vm, old_pm, new_pm);
  #endif
}


/** @brief Immediately suspend the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * This function stops the exection of the VM. All the processes on this VM
 * will pause. The state of the VM is perserved. We can later resume it again.
 *
 * No suspension cost occurs.
 */
void MSG_vm_suspend(msg_vm_t vm)
{
  simcall_vm_suspend(vm);

  XBT_DEBUG("vm_suspend done");

  #ifdef HAVE_TRACING
  TRACE_msg_vm_suspend(vm);
  #endif
}


/** @brief Resume the execution of the VM. All processes on the VM run again.
 *  @ingroup msg_VMs
 *
 * No resume cost occurs.
 */
void MSG_vm_resume(msg_vm_t vm)
{
  simcall_vm_resume(vm);

  #ifdef HAVE_TRACING
  TRACE_msg_vm_resume(vm);
  #endif
}


/** @brief Immediately save the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * This function stops the exection of the VM. All the processes on this VM
 * will pause. The state of the VM is perserved. We can later resume it again.
 *
 * FIXME: No suspension cost occurs. If you want to simulate this too, you want to
 * use a \ref MSG_file_write() before or after, depending on the exact semantic
 * of VM save to you.
 */
void MSG_vm_save(msg_vm_t vm)
{
  simcall_vm_save(vm);
  #ifdef HAVE_TRACING
  TRACE_msg_vm_save(vm);
  #endif
}

/** @brief Restore the execution of the VM. All processes on the VM run again.
 *  @ingroup msg_VMs
 *
 * FIXME: No restore cost occurs. If you want to simulate this too, you want to
 * use a \ref MSG_file_read() before or after, depending on the exact semantic
 * of VM restore to you.
 */
void MSG_vm_restore(msg_vm_t vm)
{
  simcall_vm_restore(vm);

  #ifdef HAVE_TRACING
  TRACE_msg_vm_restore(vm);
  #endif
}


/** @brief Destroy a VM. Destroy the VM object from the simulation.
 *  @ingroup msg_VMs
 */
void MSG_vm_destroy(msg_vm_t vm)
{
  /* First, terminate all processes on the VM if necessary */
  if (MSG_vm_is_running(vm))
      simcall_vm_shutdown(vm);

  if (!MSG_vm_is_created(vm)) {
    XBT_CRITICAL("shutdown the given VM before destroying it");
    DIE_IMPOSSIBLE;
  }

  /* Then, destroy the VM object */
  simcall_vm_destroy(vm);

  __MSG_host_destroy(vm);

  #ifdef HAVE_TRACING
  TRACE_msg_vm_end(vm);
  #endif
}


/** @brief Get the physical host of a givne VM.
 *  @ingroup msg_VMs
 */
msg_host_t MSG_vm_get_pm(msg_vm_t vm)
{
  return simcall_vm_get_pm(vm);
}
