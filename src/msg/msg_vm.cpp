/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* TODO:
 * 1. add the support of trace
 * 2. use parallel tasks to simulate CPU overhead and remove the very
 *    experimental code generating micro computation tasks
 */



#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "simgrid/host.h"

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
  return vm->properties();
}

/** \ingroup m_host_management
 * \brief Change the value of a given host property
 *
 * \param vm a vm
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
  return MSG_host_by_name(name);
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

/** @brief Returns whether the given VM has just created, not running.
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
  msg_host_priv_t priv = sg_host_msg(vm);
  return priv->is_migrating;
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
 *  @param pm        Physical machine that will host the VM
 *  @param name      [TODO]
 *  @param ncpus     [TODO]
 *  @param ramsize   [TODO]
 *  @param net_cap   Maximal bandwidth that the VM can consume (in MByte/s)
 *  @param disk_path (unused) Path to the image that boots
 *  @param disksize  (unused) will represent the size of the VM (will be used during migrations)
 *  @param mig_netspeed Amount of Mbyte/s allocated to the migration (cannot be larger than net_cap). Use 0 if unsure.
 *  @param dp_intensity Dirty page percentage according to migNetSpeed, [0-100]. Use 0 if unsure.
 *
 */
msg_vm_t MSG_vm_create(msg_host_t pm, const char *name,
                       int ncpus, int ramsize,
                       int net_cap, char *disk_path, int disksize,
                       int mig_netspeed, int dp_intensity)
{
  /* For the moment, intensity_rate is the percentage against the migration
   * bandwidth */
  double host_speed = MSG_host_get_speed(pm);
  double update_speed = ((double)dp_intensity/100) * mig_netspeed;

  msg_vm_t vm = MSG_vm_create_core(pm, name);
  s_vm_params_t params;
  memset(&params, 0, sizeof(params));
  params.ramsize = (sg_size_t)ramsize * 1024 * 1024;
  //params.overcommit = 0;
  params.devsize = 0;
  params.skip_stage2 = 0;
  params.max_downtime = 0.03;
  params.dp_rate = (update_speed * 1024 * 1024) / host_speed;
  params.dp_cap = params.ramsize * 0.9; // assume working set memory is 90% of ramsize
  params.mig_speed = (double)mig_netspeed * 1024 * 1024; // mig_speed

  //XBT_INFO("dp rate %f migspeed : %f intensity mem : %d, updatespeed %f, hostspeed %f",params.dp_rate, params.mig_speed, dp_intensity, update_speed, host_speed);
  vm->setParameters(&params);

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
    simgrid::s4u::Host* ind_host_tmp =
      (simgrid::s4u::Host*) xbt_dict_get_or_null(host_list, name);
    if (ind_host_tmp != nullptr && sg_host_simix(ind_host_tmp) != nullptr) {
      XBT_ERROR("host %s already exits", name);
      return nullptr;
    }
  }

  /* Note: ind_vm and vm_workstation point to the same elm object. */
  
  /* Ask the SIMIX layer to create the surf vm resource */
  sg_host_t ind_vm_workstation =  (sg_host_t) simcall_vm_create(name, ind_pm);

  msg_vm_t ind_vm = (msg_vm_t) __MSG_host_create(ind_vm_workstation);

  XBT_DEBUG("A new VM (%s) has been created", name);

  TRACE_msg_vm_create(name, ind_pm);

  return ind_vm;
}

/** @brief Destroy a VM. Destroy the VM object from the simulation.
 *  @ingroup msg_VMs
 */
void MSG_vm_destroy(msg_vm_t vm)
{
  if (MSG_vm_is_migrating(vm))
    THROWF(vm_error, 0, "VM(%s) is migrating", sg_host_get_name(vm));

  /* First, terminate all processes on the VM if necessary */
  if (MSG_vm_is_running(vm))
      simcall_vm_shutdown(vm);

  if (!MSG_vm_is_created(vm)) {
    XBT_CRITICAL("shutdown the given VM before destroying it");
    DIE_IMPOSSIBLE;
  }

  /* Then, destroy the VM object */
  simcall_vm_destroy(vm);

  TRACE_msg_vm_end(vm);
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

  TRACE_msg_vm_start(vm);
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

  // TRACE_msg_vm_(vm);
}



/* We have two mailboxes. mbox is used to transfer migration data between
 * source and destination PMs. mbox_ctl is used to detect the completion of a
 * migration. The names of these mailboxes must not conflict with others. */
static inline char *get_mig_mbox_src_dst(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  const char *vm_name = sg_host_get_name(vm);
  const char *src_pm_name = sg_host_get_name(src_pm);
  const char *dst_pm_name = sg_host_get_name(dst_pm);

  return bprintf("__mbox_mig_src_dst:%s(%s-%s)", vm_name, src_pm_name, dst_pm_name);
}

static inline char *get_mig_mbox_ctl(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  const char *vm_name = sg_host_get_name(vm);
  const char *src_pm_name = sg_host_get_name(src_pm);
  const char *dst_pm_name = sg_host_get_name(dst_pm);

  return bprintf("__mbox_mig_ctl:%s(%s-%s)", vm_name, src_pm_name, dst_pm_name);
}

static inline char *get_mig_process_tx_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  const char *vm_name = sg_host_get_name(vm);
  const char *src_pm_name = sg_host_get_name(src_pm);
  const char *dst_pm_name = sg_host_get_name(dst_pm);

  return bprintf("__pr_mig_tx:%s(%s-%s)", vm_name, src_pm_name, dst_pm_name);
}

static inline char *get_mig_process_rx_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  const char *vm_name = sg_host_get_name(vm);
  const char *src_pm_name = sg_host_get_name(src_pm);
  const char *dst_pm_name = sg_host_get_name(dst_pm);

  return bprintf("__pr_mig_rx:%s(%s-%s)", vm_name, src_pm_name, dst_pm_name);
}

static inline char *get_mig_task_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm, int stage)
{
  const char *vm_name = sg_host_get_name(vm);
  const char *src_pm_name = sg_host_get_name(src_pm);
  const char *dst_pm_name = sg_host_get_name(dst_pm);

  return bprintf("__task_mig_stage%d:%s(%s-%s)", stage, vm_name, src_pm_name, dst_pm_name);
}


struct migration_session {
  msg_vm_t vm;
  msg_host_t src_pm;
  msg_host_t dst_pm;

  /* The miration_rx process uses mbox_ctl to let the caller of do_migration()
   * know the completion of the migration. */
  char *mbox_ctl;
  /* The migration_rx and migration_tx processes use mbox to transfer migration
   * data. */
  char *mbox;
};


static int migration_rx_fun(int argc, char *argv[])
{
  XBT_DEBUG("mig: rx_start");

  // The structure has been created in the do_migration function and should only be freed in the same place ;)
  struct migration_session *ms = (migration_session *) MSG_process_get_data(MSG_process_self());

  s_vm_params_t params;
  ms->vm->parameters(&params);

  int need_exit = 0;

  char *finalize_task_name = get_mig_task_name(ms->vm, ms->src_pm, ms->dst_pm, 3);

  int ret = 0;
  for (;;) {
    msg_task_t task = NULL;
    ret = MSG_task_recv(&task, ms->mbox);
    {
      if (ret != MSG_OK) {
        // An error occured, clean the code and return
        // The owner did not change, hence the task should be only destroyed on the other side
        xbt_free(finalize_task_name);
        return 0;
      }
    }

    if (strcmp(task->name, finalize_task_name) == 0)
      need_exit = 1;

    MSG_task_destroy(task);

    if (need_exit)
      break;
  }

  // Here Stage 1, 2  and 3 have been performed.
  // Hence complete the migration

  // Copy the reference to the vm (if SRC crashes now, do_migration will free ms)
  // This is clearly ugly but I (Adrien) need more time to do something cleaner (actually we should copy the whole ms structure at the begining and free it at the end of each function)
   msg_vm_t vm = ms->vm;
   msg_host_t src_pm = ms->src_pm;
   msg_host_t dst_pm = ms-> dst_pm;
   msg_host_priv_t priv = sg_host_msg(vm);

// // TODO: we have an issue, if the DST node is turning off during the three next calls, then the VM is in an inconsistent state
// // I should check with Takahiro in order to make this portion of code atomic
//  /* deinstall the current affinity setting for the CPU */
//  simcall_vm_set_affinity(vm, src_pm, 0);
//
//  /* Update the vm location */
//  simcall_vm_migrate(vm, dst_pm);
// 
//  /* Resume the VM */
//  simcall_vm_resume(vm);
//
   simcall_vm_migratefrom_resumeto(vm, src_pm, dst_pm); 

  /* install the affinity setting of the VM on the destination pm */
  {

    unsigned long affinity_mask = (unsigned long)(uintptr_t) xbt_dict_get_or_null_ext(priv->affinity_mask_db, (char *)dst_pm, sizeof(msg_host_t));
    simcall_vm_set_affinity(vm, dst_pm, affinity_mask);
    XBT_DEBUG("set affinity(0x%04lx@%s) for %s", affinity_mask, MSG_host_get_name(dst_pm), MSG_host_get_name(vm));
  }

  {

   // Now the VM is running on the new host (the migration is completed) (even if the SRC crash)
   msg_host_priv_t priv = sg_host_msg(vm);
   priv->is_migrating = 0;
   XBT_DEBUG("VM(%s) moved from PM(%s) to PM(%s)",
      sg_host_get_name(ms->vm), sg_host_get_name(ms->src_pm),
      sg_host_get_name(ms->dst_pm));
   TRACE_msg_vm_change_host(ms->vm, ms->src_pm, ms->dst_pm);
  }
  // Inform the SRC that the migration has been correctly performed
  {
    char *task_name = get_mig_task_name(ms->vm, ms->src_pm, ms->dst_pm, 4);
    msg_task_t task = MSG_task_create(task_name, 0, 0, NULL);
    msg_error_t ret = MSG_task_send(task, ms->mbox_ctl);
    // xbt_assert(ret == MSG_OK);
    if(ret == MSG_HOST_FAILURE){
      // The DST has crashed, this is a problem has the VM since we are not sure whether SRC is considering that the VM has been correctly migrated on the DST node
      // TODO What does it mean ? What should we do ?
      MSG_task_destroy(task);
    } else if(ret == MSG_TRANSFER_FAILURE){
      // The SRC has crashed, this is not a problem has the VM has been correctly migrated on the DST node
      MSG_task_destroy(task);
    }

    xbt_free(task_name);
  }


  xbt_free(finalize_task_name);

  XBT_DEBUG("mig: rx_done");

  return 0;
}

static void reset_dirty_pages(msg_vm_t vm)
{
  msg_host_priv_t priv = sg_host_msg(vm);

  char *key = NULL;
  xbt_dict_cursor_t cursor = NULL;
  dirty_page_t dp = NULL;
  xbt_dict_foreach(priv->dp_objs, cursor, key, dp) {
    double remaining = MSG_task_get_flops_amount(dp->task);
    dp->prev_clock = MSG_get_clock();
    dp->prev_remaining = remaining;

    // XBT_INFO("%s@%s remaining %f", key, sg_host_name(vm), remaining);
  }
}

static void start_dirty_page_tracking(msg_vm_t vm)
{
  msg_host_priv_t priv = sg_host_msg(vm);
  priv->dp_enabled = 1;

  reset_dirty_pages(vm);
}

static void stop_dirty_page_tracking(msg_vm_t vm)
{
  msg_host_priv_t priv = sg_host_msg(vm);
  priv->dp_enabled = 0;
}

static double get_computed(char *key, msg_vm_t vm, dirty_page_t dp, double remaining, double clock)
{
  double computed = dp->prev_remaining - remaining;
  double duration = clock - dp->prev_clock;

  XBT_DEBUG("%s@%s: computed %f ops (remaining %f -> %f) in %f secs (%f -> %f)",
      key, sg_host_get_name(vm), computed, dp->prev_remaining, remaining, duration, dp->prev_clock, clock);

  return computed;
}

static double lookup_computed_flop_counts(msg_vm_t vm, int stage_for_fancy_debug, int stage2_round_for_fancy_debug)
{
  msg_host_priv_t priv = sg_host_msg(vm);
  double total = 0;

  char *key = NULL;
  xbt_dict_cursor_t cursor = NULL;
  dirty_page_t dp = NULL;
  xbt_dict_foreach(priv->dp_objs, cursor, key, dp) {
    double remaining = MSG_task_get_flops_amount(dp->task);

    double clock = MSG_get_clock();

    // total += calc_updated_pages(key, vm, dp, remaining, clock);
    total += get_computed(key, vm, dp, remaining, clock);

    dp->prev_remaining = remaining;
    dp->prev_clock = clock;
  }

  total += priv->dp_updated_by_deleted_tasks;

  XBT_DEBUG("mig-stage%d.%d: computed %f flop_counts (including %f by deleted tasks)",
      stage_for_fancy_debug,
      stage2_round_for_fancy_debug,
      total, priv->dp_updated_by_deleted_tasks);



  priv->dp_updated_by_deleted_tasks = 0;


  return total;
}

// TODO Is this code redundant with the information provided by
// msg_process_t MSG_process_create(const char *name, xbt_main_func_t code, void *data, msg_host_t host)
void MSG_host_add_task(msg_host_t host, msg_task_t task)
{
  msg_host_priv_t priv = sg_host_msg(host);
  double remaining = MSG_task_get_flops_amount(task);
  char *key = bprintf("%s-%p", task->name, task);

  dirty_page_t dp = xbt_new0(s_dirty_page, 1);
  dp->task = task;

  /* It should be okay that we add a task onto a migrating VM. */
  if (priv->dp_enabled) {
    dp->prev_clock = MSG_get_clock();
    dp->prev_remaining = remaining;
  }

  xbt_assert(xbt_dict_get_or_null(priv->dp_objs, key) == NULL);
  xbt_dict_set(priv->dp_objs, key, dp, NULL);
  XBT_DEBUG("add %s on %s (remaining %f, dp_enabled %d)", key, sg_host_get_name(host), remaining, priv->dp_enabled);

  xbt_free(key);
}

void MSG_host_del_task(msg_host_t host, msg_task_t task)
{
  msg_host_priv_t priv = sg_host_msg(host);

  char *key = bprintf("%s-%p", task->name, task);

  dirty_page_t dp = (dirty_page_t) xbt_dict_get_or_null(priv->dp_objs, key);
  xbt_assert(dp->task == task);

  /* If we are in the middle of dirty page tracking, we record how much
   * computation has been done until now, and keep the information for the
   * lookup_() function that will called soon. */
  if (priv->dp_enabled) {
    double remaining = MSG_task_get_flops_amount(task);
    double clock = MSG_get_clock();
    // double updated = calc_updated_pages(key, host, dp, remaining, clock);
    double updated = get_computed(key, host, dp, remaining, clock);

    priv->dp_updated_by_deleted_tasks += updated;
  }

  xbt_dict_remove(priv->dp_objs, key);
  xbt_free(dp);

  XBT_DEBUG("del %s on %s", key, sg_host_get_name(host));

  xbt_free(key);
}




static sg_size_t send_migration_data(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm,
    sg_size_t size, char *mbox, int stage, int stage2_round, double mig_speed, double timeout)
{
  sg_size_t sent = 0;
  char *task_name = get_mig_task_name(vm, src_pm, dst_pm, stage);
  msg_task_t task = MSG_task_create(task_name, 0, (double)size, NULL);

  /* TODO: clean up */

  double clock_sta = MSG_get_clock();

  msg_error_t ret;
  if (mig_speed > 0)
    ret = MSG_task_send_with_timeout_bounded(task, mbox, timeout, mig_speed);
  else
    ret = MSG_task_send(task, mbox);

  xbt_free(task_name);

  if (ret == MSG_OK) {
    sent = size;
  } else if (ret == MSG_TIMEOUT) {
    sg_size_t remaining = (sg_size_t)MSG_task_get_remaining_communication(task);
    sent = size - remaining;
    XBT_VERB("timeout (%lf s) in sending_migration_data, remaining %llu bytes of %llu",
        timeout, remaining, size);
  }

  /* FIXME: why try-and-catch is used here? */
  if(ret == MSG_HOST_FAILURE){
    //XBT_DEBUG("SRC host failed during migration of %s (stage %d)", sg_host_name(vm), stage);
    MSG_task_destroy(task);
    THROWF(host_error, 0, "SRC host failed during migration of %s (stage %d)", sg_host_get_name(vm), stage);
  }else if(ret == MSG_TRANSFER_FAILURE){
    //XBT_DEBUG("DST host failed during migration of %s (stage %d)", sg_host_name(vm), stage);
    MSG_task_destroy(task);
    THROWF(host_error, 0, "DST host failed during migration of %s (stage %d)", sg_host_get_name(vm), stage);
  }

  double clock_end = MSG_get_clock();
  double duration = clock_end - clock_sta;
  double actual_speed = size / duration;

  if (stage == 2)
    XBT_DEBUG("mig-stage%d.%d: sent %llu duration %f actual_speed %f (target %f)", stage, stage2_round, size, duration, actual_speed, mig_speed);
  else
    XBT_DEBUG("mig-stage%d: sent %llu duration %f actual_speed %f (target %f)", stage, size, duration, actual_speed, mig_speed);

  return sent;
}

static sg_size_t get_updated_size(double computed, double dp_rate, double dp_cap)
{
  double updated_size = computed * dp_rate;
  XBT_DEBUG("updated_size %f dp_rate %f", updated_size, dp_rate);
  if (updated_size > dp_cap) {
    // XBT_INFO("mig-stage2.%d: %f bytes updated, but cap it with the working set size %f", stage2_round, updated_size, dp_cap);
    updated_size = dp_cap;
  }

  return (sg_size_t) updated_size;
}

static double send_stage1(struct migration_session *ms,
    sg_size_t ramsize, double mig_speed, double dp_rate, double dp_cap)
{

  // const long chunksize = (sg_size_t)1024 * 1024 * 100;
  const sg_size_t chunksize = (sg_size_t)1024 * 1024 * 100000;
  sg_size_t remaining = ramsize;
  double computed_total = 0;

  while (remaining > 0) {
    sg_size_t datasize = chunksize;
    if (remaining < chunksize)
      datasize = remaining;

    remaining -= datasize;
    send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, datasize, ms->mbox, 1, 0, mig_speed, -1);
    double computed = lookup_computed_flop_counts(ms->vm, 1, 0);
    computed_total += computed;
  }

  return computed_total;
}



static double get_threshold_value(double bandwidth, double max_downtime)
{
  return max_downtime * bandwidth;
}

static int migration_tx_fun(int argc, char *argv[])
{
  XBT_DEBUG("mig: tx_start");

  // Note that the ms structure has been allocated in do_migration and hence should be freed in the same function ;)
  migration_session *ms = 
    (migration_session *) MSG_process_get_data(MSG_process_self());

  s_vm_params_t params;
  ms->vm->parameters(&params);
  const sg_size_t ramsize   = params.ramsize;
  const sg_size_t devsize   = params.devsize;
  const int skip_stage1     = params.skip_stage1;
  int skip_stage2           = params.skip_stage2;
  const double dp_rate      = params.dp_rate;
  const double dp_cap       = params.dp_cap;
  const double mig_speed    = params.mig_speed;
  double max_downtime       = params.max_downtime;

  /* hard code it temporally. Fix Me */
#define MIGRATION_TIMEOUT_DO_NOT_HARDCODE_ME 10000000.0
  double mig_timeout = MIGRATION_TIMEOUT_DO_NOT_HARDCODE_ME;

  double remaining_size = (double) (ramsize + devsize);
  double threshold = 0.0;

  /* check parameters */
  if (ramsize == 0)
    XBT_WARN("migrate a VM, but ramsize is zero");

  if (max_downtime == 0) {
    XBT_WARN("use the default max_downtime value 30ms");
    max_downtime = 0.03;
  }

  /* Stage1: send all memory pages to the destination. */
  XBT_DEBUG("mig-stage1: remaining_size %f", remaining_size);
  start_dirty_page_tracking(ms->vm);

  double computed_during_stage1 = 0;
  if (!skip_stage1) {
    double clock_prev_send = MSG_get_clock();

    TRY {
      /* At stage 1, we do not need timeout. We have to send all the memory
       * pages even though the duration of this tranfer exceeds the timeout
       * value. */
      XBT_VERB("Stage 1: Gonna send %llu", ramsize);
      sg_size_t sent = send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, ramsize, ms->mbox, 1, 0, mig_speed, -1);
      remaining_size -= sent;
      computed_during_stage1 = lookup_computed_flop_counts(ms->vm, 1, 0);

      if (sent < ramsize) {
        XBT_VERB("mig-stage1: timeout, force moving to stage 3");
        skip_stage2 = 1;
      } else if (sent > ramsize)
        XBT_CRITICAL("bug");

    } CATCH_ANONYMOUS {
      //hostfailure (if you want to know whether this is the SRC or the DST please check directly in send_migration_data code)
      // Stop the dirty page tracking an return (there is no memory space to release)
      stop_dirty_page_tracking(ms->vm);
      return 0;
    }

    double clock_post_send = MSG_get_clock();
    mig_timeout -= (clock_post_send - clock_prev_send);
    if (mig_timeout < 0) {
      XBT_VERB("The duration of stage 1 exceeds the timeout value (%lf > %lf), skip stage 2",
          (clock_post_send - clock_prev_send), MIGRATION_TIMEOUT_DO_NOT_HARDCODE_ME);
      skip_stage2 = 1;
    }

    /* estimate bandwidth */
    double bandwidth = ramsize / (clock_post_send - clock_prev_send);
    threshold = get_threshold_value(bandwidth, max_downtime);
    XBT_DEBUG("actual bandwidth %f (MB/s), threshold %f", bandwidth / 1024 / 1024, threshold);
  }


  /* Stage2: send update pages iteratively until the size of remaining states
   * becomes smaller than the threshold value. */
 if (! skip_stage2) {

  int stage2_round = 0;
  for (;;) {

    sg_size_t updated_size = 0;
    if (stage2_round == 0) {
      /* just after stage1, nothing has been updated. But, we have to send the
       * data updated during stage1 */
      updated_size = get_updated_size(computed_during_stage1, dp_rate, dp_cap);
    } else {
      double computed = lookup_computed_flop_counts(ms->vm, 2, stage2_round);
      updated_size = get_updated_size(computed, dp_rate, dp_cap);
    }

    XBT_DEBUG("mig-stage 2:%d updated_size %llu computed_during_stage1 %f dp_rate %f dp_cap %f",
        stage2_round, updated_size, computed_during_stage1, dp_rate, dp_cap);


    /* Check whether the remaining size is below the threshold value. If so,
     * move to stage 3. */
    remaining_size += updated_size;
    XBT_DEBUG("mig-stage2.%d: remaining_size %f (%s threshold %f)", stage2_round,
        remaining_size, (remaining_size < threshold) ? "<" : ">", threshold);
    if (remaining_size < threshold)
      break;


    sg_size_t sent = 0;
    double clock_prev_send = MSG_get_clock();
    TRY {
      XBT_DEBUG("Stage 2, gonna send %llu", updated_size);
      sent = send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, updated_size, ms->mbox, 2, stage2_round, mig_speed, mig_timeout);
    } CATCH_ANONYMOUS {
      //hostfailure (if you want to know whether this is the SRC or the DST please check directly in send_migration_data code)
      // Stop the dirty page tracking an return (there is no memory space to release)
      stop_dirty_page_tracking(ms->vm);
      return 0;
    }
    double clock_post_send = MSG_get_clock();

    if (sent == updated_size) {
      /* timeout did not happen */
      double bandwidth = updated_size / (clock_post_send - clock_prev_send);
      threshold = get_threshold_value(bandwidth, max_downtime);
      XBT_DEBUG("actual bandwidth %f, threshold %f", bandwidth / 1024 / 1024, threshold);
      remaining_size -= sent;
      stage2_round += 1;
      mig_timeout -= (clock_post_send - clock_prev_send);
      xbt_assert(mig_timeout > 0);

    } else if (sent < updated_size) {
      /* When timeout happens, we move to stage 3. The size of memory pages
       * updated before timeout must be added to the remaining size. */
      XBT_VERB("mig-stage2.%d: timeout, force moving to stage 3. sent %llu / %llu, eta %lf",
          stage2_round, sent, updated_size, (clock_post_send - clock_prev_send));
      remaining_size -= sent;

      double computed = lookup_computed_flop_counts(ms->vm, 2, stage2_round);
      updated_size = get_updated_size(computed, dp_rate, dp_cap);
      remaining_size += updated_size;
      break;

    } else
      XBT_CRITICAL("bug");
  }
 }

  /* Stage3: stop the VM and copy the rest of states. */
  XBT_DEBUG("mig-stage3: remaining_size %f", remaining_size);
  simcall_vm_suspend(ms->vm);
  stop_dirty_page_tracking(ms->vm);

  TRY {
    XBT_DEBUG("Stage 3: Gonna send %f", remaining_size);
    send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, (sg_size_t)remaining_size, ms->mbox, 3, 0, mig_speed, -1);
  } CATCH_ANONYMOUS {
    //hostfailure (if you want to know whether this is the SRC or the DST please check directly in send_migration_data code)
    // Stop the dirty page tracking an return (there is no memory space to release)
    simcall_vm_resume(ms->vm);
    return 0;
  }

  // At that point the Migration is considered valid for the SRC node but remind that the DST side should relocate effectively the VM on the DST node.
  XBT_DEBUG("mig: tx_done");

  return 0;
}



static int do_migration(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  struct migration_session *ms = xbt_new(struct migration_session, 1);
  ms->vm = vm;
  ms->src_pm = src_pm;
  ms->dst_pm = dst_pm;
  ms->mbox_ctl = get_mig_mbox_ctl(vm, src_pm, dst_pm);
  ms->mbox = get_mig_mbox_src_dst(vm, src_pm, dst_pm);
 

  char *pr_rx_name = get_mig_process_rx_name(vm, src_pm, dst_pm);
  char *pr_tx_name = get_mig_process_tx_name(vm, src_pm, dst_pm);

//  msg_process_t tx_process, rx_process; 
//  MSG_process_create(pr_rx_name, migration_rx_fun, ms, dst_pm);
//  MSG_process_create(pr_tx_name, migration_tx_fun, ms, src_pm);
#if 1
 {
 char **argv = xbt_new(char *, 2);
 argv[0] = pr_rx_name;
 argv[1] = NULL;
/*rx_process = */ MSG_process_create_with_arguments(pr_rx_name, migration_rx_fun, ms, dst_pm, 1, argv);
 }
 {
 char **argv = xbt_new(char *, 2);
 argv[0] = pr_tx_name;
 argv[1] = NULL;
/* tx_process = */MSG_process_create_with_arguments(pr_tx_name, migration_tx_fun, ms, src_pm, 1, argv);
 }
#endif

  /* wait until the migration have finished or on error has occured */
  {
    XBT_DEBUG("wait for reception of the final ACK (i.e. migration has been correctly performed");
    msg_task_t task = NULL;
    msg_error_t ret = MSG_TIMEOUT;
    while (ret == MSG_TIMEOUT && MSG_host_is_on(dst_pm)) //Wait while you receive the message o
     ret = MSG_task_receive_with_timeout(&task, ms->mbox_ctl, 4);

    xbt_free(ms->mbox_ctl);
    xbt_free(ms->mbox);
    xbt_free(ms);
   
    //xbt_assert(ret == MSG_OK);
    if(ret == MSG_HOST_FAILURE){
        // Note that since the communication failed, the owner did not change and the task should be destroyed on the other side.
        // Hence, just throw the execption
        XBT_ERROR("SRC crashes, throw an exception (m-control)");
        //MSG_process_kill(tx_process); // Adrien, I made a merge on Nov 28th 2014, I'm not sure whether this line is required or not 
        return -1; 
    } 
    else if((ret == MSG_TRANSFER_FAILURE) || (ret == MSG_TIMEOUT)){ // MSG_TIMEOUT here means that MSG_host_is_avail() returned false.
        XBT_ERROR("DST crashes, throw an exception (m-control)");
        return -2;  
    }

   
    char *expected_task_name = get_mig_task_name(vm, src_pm, dst_pm, 4);
    xbt_assert(strcmp(task->name, expected_task_name) == 0);
    xbt_free(expected_task_name);
    MSG_task_destroy(task);
    return 0;
  }
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

  msg_host_t old_pm = (msg_host_t) simcall_vm_get_pm(vm);

  if(MSG_host_is_off(old_pm))
    THROWF(vm_error, 0, "SRC host(%s) seems off, cannot start a migration", sg_host_get_name(old_pm));
 
  if(MSG_host_is_off(new_pm))
    THROWF(vm_error, 0, "DST host(%s) seems off, cannot start a migration", sg_host_get_name(new_pm));
  
  if (!MSG_vm_is_running(vm))
    THROWF(vm_error, 0, "VM(%s) is not running", sg_host_get_name(vm));

  if (MSG_vm_is_migrating(vm))
    THROWF(vm_error, 0, "VM(%s) is already migrating", sg_host_get_name(vm));

  msg_host_priv_t priv = sg_host_msg(vm);
  priv->is_migrating = 1;

  {
  
    int ret = do_migration(vm, old_pm, new_pm);
    if (ret == -1){
     priv->is_migrating = 0;
     THROWF(host_error, 0, "SRC host failed during migration");
    }
    else if(ret == -2){
     priv->is_migrating = 0;
     THROWF(host_error, 0, "DST host failed during migration");
    }
  }

  // This part is done in the RX code, to handle the corner case where SRC can crash just at the end of the migration process
  // In that case, the VM has been already assigned to the DST node.
  //XBT_DEBUG("VM(%s) moved from PM(%s) to PM(%s)", vm->key, old_pm->key, new_pm->key);
  //TRACE_msg_vm_change_host(vm, old_pm, new_pm);
}


/** @brief Immediately suspend the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * This function stops the execution of the VM. All the processes on this VM
 * will pause. The state of the VM is preserved. We can later resume it again.
 *
 * No suspension cost occurs.
 */
void MSG_vm_suspend(msg_vm_t vm)
{
  if (MSG_vm_is_migrating(vm))
    THROWF(vm_error, 0, "VM(%s) is migrating", sg_host_get_name(vm));

  simcall_vm_suspend(vm);

  XBT_DEBUG("vm_suspend done");

  TRACE_msg_vm_suspend(vm);
}


/** @brief Resume the execution of the VM. All processes on the VM run again.
 *  @ingroup msg_VMs
 *
 * No resume cost occurs.
 */
void MSG_vm_resume(msg_vm_t vm)
{
  simcall_vm_resume(vm);

  TRACE_msg_vm_resume(vm);
}


/** @brief Immediately save the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * This function stops the execution of the VM. All the processes on this VM
 * will pause. The state of the VM is preserved. We can later resume it again.
 *
 * FIXME: No suspension cost occurs. If you want to simulate this too, you want to
 * use a \ref MSG_file_write() before or after, depending on the exact semantic
 * of VM save to you.
 */
void MSG_vm_save(msg_vm_t vm)
{
  if (MSG_vm_is_migrating(vm))
    THROWF(vm_error, 0, "VM(%s) is migrating", sg_host_get_name(vm));

  simcall_vm_save(vm);
  TRACE_msg_vm_save(vm);
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

  TRACE_msg_vm_restore(vm);
}


/** @brief Get the physical host of a given VM.
 *  @ingroup msg_VMs
 */
msg_host_t MSG_vm_get_pm(msg_vm_t vm)
{
  return (msg_host_t) simcall_vm_get_pm(vm);
}


/** @brief Set a CPU bound for a given VM.
 *  @ingroup msg_VMs
 *
 * 1.
 * Note that in some cases MSG_task_set_bound() may not intuitively work for VMs.
 *
 * For example,
 *  On PM0, there are Task1 and VM0.
 *  On VM0, there is Task2.
 * Now we bound 75% to Task1\@PM0 and bound 25% to Task2\@VM0.
 * Then,
 *  Task1\@PM0 gets 50%.
 *  Task2\@VM0 gets 25%.
 * This is NOT 75% for Task1\@PM0 and 25% for Task2\@VM0, respectively.
 *
 * This is because a VM has the dummy CPU action in the PM layer. Putting a
 * task on the VM does not affect the bound of the dummy CPU action. The bound
 * of the dummy CPU action is unlimited.
 *
 * There are some solutions for this problem. One option is to update the bound
 * of the dummy CPU action automatically. It should be the sum of all tasks on
 * the VM. But, this solution might be costly, because we have to scan all tasks
 * on the VM in share_resource() or we have to trap both the start and end of
 * task execution.
 *
 * The current solution is to use MSG_vm_set_bound(), which allows us to
 * directly set the bound of the dummy CPU action.
 *
 *
 * 2.
 * Note that bound == 0 means no bound (i.e., unlimited). But, if a host has
 * multiple CPU cores, the CPU share of a computation task (or a VM) never
 * exceeds the capacity of a CPU core.
 */
void MSG_vm_set_bound(msg_vm_t vm, double bound)
{
  simcall_vm_set_bound(vm, bound);
}


/** @brief Set the CPU affinity of a given VM.
 *  @ingroup msg_VMs
 *
 * This function changes the CPU affinity of a given VM. Usage is the same as
 * MSG_task_set_affinity(). See the MSG_task_set_affinity() for details.
 */
void MSG_vm_set_affinity(msg_vm_t vm, msg_host_t pm, unsigned long mask)
{
  msg_host_priv_t priv = sg_host_msg(vm);

  if (mask == 0)
    xbt_dict_remove_ext(priv->affinity_mask_db, (char *) pm, sizeof(pm));
  else
    xbt_dict_set_ext(priv->affinity_mask_db, (char *) pm, sizeof(pm), (void *)(uintptr_t) mask, NULL);

  msg_host_t pm_now = MSG_vm_get_pm(vm);
  if (pm_now == pm) {
    XBT_DEBUG("set affinity(0x%04lx@%s) for %s", mask, MSG_host_get_name(pm), MSG_host_get_name(vm));
    simcall_vm_set_affinity(vm, pm, mask);
  } else
    XBT_DEBUG("set affinity(0x%04lx@%s) for %s (not active now)", mask, MSG_host_get_name(pm), MSG_host_get_name(vm));
}
