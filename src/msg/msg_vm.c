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

  /* TODO: Limit net capability, take into account disk considerations. */

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



/* We have two mailboxes. mbox is used to transfer migration data between
 * source and destiantion PMs. mbox_ctl is used to detect the completion of a
 * migration. The names of these mailboxes must not conflict with others. */
static inline char *get_mig_mbox_src_dst(const char *vm_name, const char *src_pm_name, const char *dst_pm_name)
{
  return bprintf("__mbox_mig_src_dst:%s(%s-%s)", vm_name, src_pm_name, dst_pm_name);
}

static inline char *get_mig_mbox_ctl(const char *vm_name, const char *src_pm_name, const char *dst_pm_name)
{
  return bprintf("__mbox_mig_ctl:%s(%s-%s)", vm_name, src_pm_name, dst_pm_name);
}

static inline char *get_mig_process_tx_name(const char *vm_name, const char *src_pm_name, const char *dst_pm_name)
{
  return bprintf("__pr_mig_tx:%s(%s-%s)", vm_name, src_pm_name, dst_pm_name);
}

static inline char *get_mig_process_rx_name(const char *vm_name, const char *src_pm_name, const char *dst_pm_name)
{
  return bprintf("__pr_mig_rx:%s(%s-%s)", vm_name, src_pm_name, dst_pm_name);
}

static inline char *get_mig_task_name(const char *vm_name, const char *src_pm_name, const char *dst_pm_name, int stage)
{
  return bprintf("__task_mig_stage%d:%s(%s-%s)", stage, vm_name, src_pm_name, dst_pm_name);
}

static int migration_rx_fun(int argc, char *argv[])
{
  const char *pr_name = MSG_process_get_name(MSG_process_self());
  const char *host_name = MSG_host_get_name(MSG_host_self());

  XBT_DEBUG("mig: rx_start");

  xbt_assert(argc == 4);
  const char *vm_name = argv[1];
  const char *src_pm_name  = argv[2];
  const char *dst_pm_name  = argv[3];
  msg_vm_t vm = MSG_get_host_by_name(vm_name);
  msg_vm_t dst_pm = MSG_get_host_by_name(dst_pm_name);

  int need_exit = 0;

  char *mbox = get_mig_mbox_src_dst(vm_name, src_pm_name, dst_pm_name);
  char *mbox_ctl = get_mig_mbox_ctl(vm_name, src_pm_name, dst_pm_name);
  char *finalize_task_name = get_mig_task_name(vm_name, src_pm_name, dst_pm_name, 3);

  for (;;) {
    msg_task_t task = NULL;
    MSG_task_recv(&task, mbox);

    if (strcmp(task->name, finalize_task_name) == 0)
      need_exit = 1;

    MSG_task_destroy(task);

    if (need_exit)
      break;
  }


  simcall_vm_migrate(vm, dst_pm);
  simcall_vm_resume(vm);

  {
    char *task_name = get_mig_task_name(vm_name, src_pm_name, dst_pm_name, 4);

    msg_task_t task = MSG_task_create(task_name, 0, 0, NULL);
    msg_error_t ret = MSG_task_send(task, mbox_ctl);
    xbt_assert(ret == MSG_OK);

    xbt_free(task_name);
  }


  xbt_free(mbox);
  xbt_free(mbox_ctl);
  xbt_free(finalize_task_name);

  XBT_DEBUG("mig: rx_done");

  return 0;
}


typedef struct dirty_page {
  double prev_clock;
  double prev_remaining;
  msg_task_t task;
} s_dirty_page, *dirty_page_t;


static void reset_dirty_pages(msg_vm_t vm)
{
  msg_host_priv_t priv = msg_host_resource_priv(vm);

  char *key = NULL;
  xbt_dict_cursor_t cursor = NULL;
  dirty_page_t dp = NULL;
  xbt_dict_foreach(priv->dp_objs, cursor, key, dp) {
    double remaining = MSG_task_get_remaining_computation(dp->task);
    dp->prev_clock = MSG_get_clock();
    dp->prev_remaining = remaining;

    // XBT_INFO("%s@%s remaining %f", key, sg_host_name(vm), remaining);
  }
}

static void start_dirty_page_tracking(msg_vm_t vm)
{
  msg_host_priv_t priv = msg_host_resource_priv(vm);
  priv->dp_enabled = 1;

  reset_dirty_pages(vm);
}

static void stop_dirty_page_tracking(msg_vm_t vm)
{
  msg_host_priv_t priv = msg_host_resource_priv(vm);
  priv->dp_enabled = 0;
}

#if 0
/* It might be natural that we define dp_rate for each task. But, we will also
 * have to care about how each task behavior affects the memory update behavior
 * at the operating system level. It may not be easy to model it with a simple algorithm. */
double calc_updated_pages(char *key, msg_vm_t vm, dirty_page_t dp, double remaining, double clock)
{
    double computed = dp->prev_remaining - remaining;
    double duration = clock - dp->prev_clock;
    double updated = dp->task->dp_rate * computed;

    XBT_INFO("%s@%s: computated %f ops (remaining %f -> %f) in %f secs (%f -> %f)",
        key, sg_host_name(vm), computed, dp->prev_remaining, remaining, duration, dp->prev_clock, clock);
    XBT_INFO("%s@%s: updated %f bytes, %f Mbytes/s",
        key, sg_host_name(vm), updated, updated / duration / 1000 / 1000);

    return updated;
}
#endif

double get_computed(char *key, msg_vm_t vm, dirty_page_t dp, double remaining, double clock)
{
  double computed = dp->prev_remaining - remaining;
  double duration = clock - dp->prev_clock;

  XBT_DEBUG("%s@%s: computated %f ops (remaining %f -> %f) in %f secs (%f -> %f)",
      key, sg_host_name(vm), computed, dp->prev_remaining, remaining, duration, dp->prev_clock, clock);

  return computed;
}

static double lookup_computed_flop_counts(msg_vm_t vm, int stage2_round_for_fancy_debug)
{
  msg_host_priv_t priv = msg_host_resource_priv(vm);
  double total = 0;

  char *key = NULL;
  xbt_dict_cursor_t cursor = NULL;
  dirty_page_t dp = NULL;
  xbt_dict_foreach(priv->dp_objs, cursor, key, dp) {
    double remaining = MSG_task_get_remaining_computation(dp->task);
    double clock = MSG_get_clock();

    // total += calc_updated_pages(key, vm, dp, remaining, clock);
    total += get_computed(key, vm, dp, remaining, clock);

    dp->prev_remaining = remaining;
    dp->prev_clock = clock;
  }

  total += priv->dp_updated_by_deleted_tasks;

  XBT_INFO("mig-stage2.%d: computed %f flop_counts (including %f by deleted tasks)",
      stage2_round_for_fancy_debug,
      total, priv->dp_updated_by_deleted_tasks);



  priv->dp_updated_by_deleted_tasks = 0;


  return total;
}

// TODO Is this code redundant with the information provided by
// msg_process_t MSG_process_create(const char *name, xbt_main_func_t code, void *data, msg_host_t host)
void MSG_host_add_task(msg_host_t host, msg_task_t task)
{
  msg_host_priv_t priv = msg_host_resource_priv(host);
  double remaining = MSG_task_get_remaining_computation(task);
  char *key = bprintf("%s-%lld", task->name, task->counter);

  dirty_page_t dp = xbt_new0(s_dirty_page, 1);
  dp->task = task;

  /* It should be okay that we add a task onto a migrating VM. */
  if (priv->dp_enabled) {
    dp->prev_clock = MSG_get_clock();
    dp->prev_remaining = remaining;
  }

  xbt_assert(xbt_dict_get_or_null(priv->dp_objs, key) == NULL);
  xbt_dict_set(priv->dp_objs, key, dp, NULL);
  XBT_DEBUG("add %s on %s (remaining %f, dp_enabled %d)", key, sg_host_name(host), remaining, priv->dp_enabled);

  xbt_free(key);
}

void MSG_host_del_task(msg_host_t host, msg_task_t task)
{
  msg_host_priv_t priv = msg_host_resource_priv(host);

  char *key = bprintf("%s-%lld", task->name, task->counter);

  dirty_page_t dp = xbt_dict_get_or_null(priv->dp_objs, key);
  xbt_assert(dp->task == task);

  /* If we are in the middle of dirty page tracking, we record how much
   * computaion has been done until now, and keep the information for the
   * lookup_() function that will called soon. */
  if (priv->dp_enabled) {
    double remaining = MSG_task_get_remaining_computation(task);
    double clock = MSG_get_clock();
    // double updated = calc_updated_pages(key, host, dp, remaining, clock);
    double updated = get_computed(key, host, dp, remaining, clock);

    priv->dp_updated_by_deleted_tasks += updated;
  }

  xbt_dict_remove(priv->dp_objs, key);
  xbt_free(dp);

  XBT_DEBUG("del %s on %s", key, sg_host_name(host));

  xbt_free(key);
}


static void send_migration_data(const char *vm_name, const char *src_pm_name, const char *dst_pm_name,
    double size, char *mbox, int stage, int stage2_round, double mig_speed)
{
  char *task_name = get_mig_task_name(vm_name, src_pm_name, dst_pm_name, stage);
  msg_task_t task = MSG_task_create(task_name, 0, size, NULL);

  msg_error_t ret;
  if (mig_speed > 0)
    ret = MSG_task_send_bounded(task, mbox, mig_speed);
  else
    ret = MSG_task_send(task, mbox);
  xbt_assert(ret == MSG_OK);

  if (stage == 2)
    XBT_INFO("mig-stage%d.%d: sent %f", stage, stage2_round, size);
  else
    XBT_INFO("mig-stage%d: sent %f", stage, size);

  xbt_free(task_name);
}


static int migration_tx_fun(int argc, char *argv[])
{
  const char *pr_name = MSG_process_get_name(MSG_process_self());
  const char *host_name = MSG_host_get_name(MSG_host_self());

  XBT_DEBUG("mig: tx_start");

  xbt_assert(argc == 4);
  const char *vm_name = argv[1];
  const char *src_pm_name  = argv[2];
  const char *dst_pm_name  = argv[3];
  msg_vm_t vm = MSG_get_host_by_name(vm_name);


  s_ws_params_t params;
  simcall_host_get_params(vm, &params);
  const long ramsize        = params.ramsize;
  const long devsize        = params.devsize;
  const int skip_stage2     = params.skip_stage2;
  const double max_downtime = params.max_downtime;
  const double dp_rate      = params.dp_rate;
  const double dp_cap       = params.dp_cap;
  const double mig_speed    = params.mig_speed;
  double remaining_size = ramsize + devsize;
  double threshold = max_downtime * 125 * 1000 * 1000;


  if (ramsize == 0)
    XBT_WARN("migrate a VM, but ramsize is zero");

  char *mbox = get_mig_mbox_src_dst(vm_name, src_pm_name, dst_pm_name);

  XBT_INFO("mig-stage1: remaining_size %f", remaining_size);

  /* Stage1: send all memory pages to the destination. */
  start_dirty_page_tracking(vm);

  send_migration_data(vm_name, src_pm_name, dst_pm_name, ramsize, mbox, 1, 0, mig_speed);

  remaining_size -= ramsize;



  /* Stage2: send update pages iteratively until the size of remaining states
   * becomes smaller than the threshold value. */
  if (skip_stage2)
    goto stage3;
  if (max_downtime == 0) {
    XBT_WARN("no max_downtime parameter, skip stage2");
    goto stage3;
  }


  int stage2_round = 0;
  for (;;) {
    // long updated_size = lookup_dirty_pages(vm);
    double updated_size = lookup_computed_flop_counts(vm, stage2_round) * dp_rate;
    if (updated_size > dp_cap) {
      XBT_INFO("mig-stage2.%d: %f bytes updated, but cap it with the working set size %f",
          stage2_round, updated_size, dp_cap);
      updated_size = dp_cap;
    }

    remaining_size += updated_size;

    XBT_INFO("mig-stage2.%d: remaining_size %f (%s threshold %f)", stage2_round,
        remaining_size, (remaining_size < threshold) ? "<" : ">", threshold);

    if (remaining_size < threshold)
      break;

    send_migration_data(vm_name, src_pm_name, dst_pm_name, updated_size, mbox, 2, stage2_round, mig_speed);

    remaining_size -= updated_size;
    stage2_round += 1;
  }


stage3:
  /* Stage3: stop the VM and copy the rest of states. */
  XBT_INFO("mig-stage3: remaining_size %f", remaining_size);
  simcall_vm_suspend(vm);
  stop_dirty_page_tracking(vm);

  send_migration_data(vm_name, src_pm_name, dst_pm_name, remaining_size, mbox, 3, 0, mig_speed);

  xbt_free(mbox);

  XBT_DEBUG("mig: tx_done");

  return 0;
}



static void do_migration(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  char *mbox_ctl = get_mig_mbox_ctl(sg_host_name(vm), sg_host_name(src_pm), sg_host_name(dst_pm));

  {
    char *pr_name = get_mig_process_rx_name(sg_host_name(vm), sg_host_name(src_pm), sg_host_name(dst_pm));
    int nargvs = 5;
    char **argv = xbt_new(char *, nargvs);
    argv[0] = xbt_strdup(pr_name);
    argv[1] = xbt_strdup(sg_host_name(vm));
    argv[2] = xbt_strdup(sg_host_name(src_pm));
    argv[3] = xbt_strdup(sg_host_name(dst_pm));
    argv[4] = NULL;

    msg_process_t pr = MSG_process_create_with_arguments(pr_name, migration_rx_fun, NULL, dst_pm, nargvs - 1, argv);

    xbt_free(pr_name);
  }

  {
    char *pr_name = get_mig_process_tx_name(sg_host_name(vm), sg_host_name(src_pm), sg_host_name(dst_pm));
    int nargvs = 5;
    char **argv = xbt_new(char *, nargvs);
    argv[0] = xbt_strdup(pr_name);
    argv[1] = xbt_strdup(sg_host_name(vm));
    argv[2] = xbt_strdup(sg_host_name(src_pm));
    argv[3] = xbt_strdup(sg_host_name(dst_pm));
    argv[4] = NULL;

    msg_process_t pr = MSG_process_create_with_arguments(pr_name, migration_tx_fun, NULL, src_pm, nargvs - 1, argv);

    xbt_free(pr_name);
  }

  /* wait until the migration have finished */
  {
    msg_task_t task = NULL;
    msg_error_t ret = MSG_task_recv(&task, mbox_ctl);
    xbt_assert(ret == MSG_OK);

    char *expected_task_name = get_mig_task_name(sg_host_name(vm), sg_host_name(src_pm), sg_host_name(dst_pm), 4);
    xbt_assert(strcmp(task->name, expected_task_name) == 0);
    xbt_free(expected_task_name);
  }

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

  if (simcall_vm_get_state(vm) != SURF_VM_STATE_RUNNING)
    THROWF(vm_error, 0, "VM(%s) is not running", sg_host_name(vm));

  do_migration(vm, old_pm, new_pm);



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




/** @brief Get the physical host of a given VM.
 *  @ingroup msg_VMs
 */
msg_host_t MSG_vm_get_pm(msg_vm_t vm)
{
  return simcall_vm_get_pm(vm);
}
