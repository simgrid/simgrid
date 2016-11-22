/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* TODO:
 * 1. add the support of trace
 * 2. use parallel tasks to simulate CPU overhead and remove the experimental code generating micro computation tasks
 */

#include <xbt/ex.hpp>

#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/plugins/vm/VmHostExt.hpp"
#include "src/simix/ActorImpl.hpp"
#include <simgrid/s4u/VirtualMachine.hpp>
#include <simgrid/s4u/host.hpp>

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "simgrid/host.h"

#include "src/simix/smx_host_private.h" /* don't ask me why the VM functions are in there (FIXME:KILLME) */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_vm, msg, "Cloud-oriented parts of the MSG API");


/* **** ******** GENERAL ********* **** */

/** \ingroup m_vm_management
 * \brief Set the parameters of a given host
 *
 * \param vm a vm
 * \param params a parameter object
 */
void MSG_vm_set_params(msg_vm_t vm, vm_params_t params)
{
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->setParameters(params);
}

/** \ingroup m_vm_management
 * \brief Get the parameters of a given host
 *
 * \param host a host
 * \param params a prameter object
 */
void MSG_vm_get_params(msg_vm_t vm, vm_params_t params)
{
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->parameters(params);
}

/* **** Check state of a VM **** */
static inline int __MSG_vm_is_state(msg_vm_t vm, e_surf_vm_state_t state)
{
  return static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getState() == state;
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
  return static_cast<simgrid::s4u::VirtualMachine*>(vm)->isMigrating();
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
 */
msg_vm_t MSG_vm_create(msg_host_t pm, const char* name, int ncpus, int ramsize, int net_cap, char* disk_path,
                       int disksize, int mig_netspeed, int dp_intensity)
{
  simgrid::vm::VmHostExt::ensureVmExtInstalled();

  /* For the moment, intensity_rate is the percentage against the migration bandwidth */
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

  //XBT_INFO("dp rate %f migspeed : %f intensity mem : %d, updatespeed %f, hostspeed %f",params.dp_rate,
  //         params.mig_speed, dp_intensity, update_speed, host_speed);
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->setParameters(&params);

  return vm;
}

/** @brief Create a new VM object. The VM is not yet started. The resource of the VM is allocated upon MSG_vm_start().
 *  @ingroup msg_VMs*
 *
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
msg_vm_t MSG_vm_create_core(msg_host_t pm, const char* name)
{
  xbt_assert(sg_host_by_name(name) == nullptr,
             "Cannot create a VM named %s: this name is already used by an host or a VM", name);

  return new simgrid::s4u::VirtualMachine(name, pm);
}

/** @brief Destroy a VM. Destroy the VM object from the simulation.
 *  @ingroup msg_VMs
 */
void MSG_vm_destroy(msg_vm_t vm)
{
  if (MSG_vm_is_migrating(vm))
    THROWF(vm_error, 0, "Cannot destroy VM '%s', which is migrating.", vm->cname());

  /* First, terminate all processes on the VM if necessary */
  if (MSG_vm_is_running(vm))
    MSG_vm_shutdown(vm);

  xbt_assert(MSG_vm_is_created(vm), "shutdown the given VM before destroying it");

  /* Then, destroy the VM object */
  simgrid::simix::kernelImmediate([vm]() {
    vm->destroy();
  });

  if (TRACE_msg_vm_is_enabled()) {
    container_t container = PJ_container_get(vm->name().c_str());
    PJ_container_remove_from_parent(container);
    PJ_container_free(container);
  }
}

/** @brief Start a vm (i.e., boot the guest operating system)
 *  @ingroup msg_VMs
 *
 *  If the VM cannot be started (because of memory overprovisionning), an exception is generated.
 */
void MSG_vm_start(msg_vm_t vm)
{
  simgrid::simix::kernelImmediate([vm]() {
    simgrid::vm::VmHostExt::ensureVmExtInstalled();

    simgrid::s4u::VirtualMachine* typedVM = static_cast<simgrid::s4u::VirtualMachine*>(vm);
    simgrid::s4u::Host* pm                = typedVM->pimpl_vm_->getPm();
    if (pm->extension<simgrid::vm::VmHostExt>() == nullptr)
      pm->extension_set(new simgrid::vm::VmHostExt());

    long pm_ramsize   = pm->extension<simgrid::vm::VmHostExt>()->ramsize;
    int pm_overcommit = pm->extension<simgrid::vm::VmHostExt>()->overcommit;
    long vm_ramsize   = typedVM->getRamsize();

    if (pm_ramsize && !pm_overcommit) { /* Only verify that we don't overcommit on need */
      /* Retrieve the memory occupied by the VMs on that host. Yep, we have to traverse all VMs of all hosts for that */
      long total_ramsize_of_vms = 0;
      for (simgrid::s4u::VirtualMachine* ws_vm : simgrid::surf::VirtualMachineImpl::allVms_)
        if (pm == ws_vm->pimpl_vm_->getPm())
          total_ramsize_of_vms += ws_vm->pimpl_vm_->getRamsize();

      if (vm_ramsize > pm_ramsize - total_ramsize_of_vms) {
        XBT_WARN("cannnot start %s@%s due to memory shortage: vm_ramsize %ld, free %ld, pm_ramsize %ld (bytes).",
                 sg_host_get_name(vm), sg_host_get_name(pm), vm_ramsize, pm_ramsize - total_ramsize_of_vms, pm_ramsize);
        THROWF(vm_error, 0, "Memory shortage on host '%s', VM '%s' cannot be started", pm->cname(), vm->cname());
      }
    }

    typedVM->pimpl_vm_->setState(SURF_VM_STATE_RUNNING);
  });

  if (TRACE_msg_vm_is_enabled()) {
    container_t vm_container = PJ_container_get(vm->name().c_str());
    type_t type              = PJ_type_get("MSG_VM_STATE", vm_container->type);
    val_t value              = PJ_value_get_or_new("start", "0 0 1", type); // start is blue
    new_pajePushState(MSG_get_clock(), vm_container, type, value);
  }
}

/** @brief Immediately kills all processes within the given VM. Any memory that they allocated will be leaked.
 *  @ingroup msg_VMs
 *
 * FIXME: No extra delay occurs. If you want to simulate this too, you want to use a #MSG_process_sleep() or something.
 *        I'm not quite sure.
 */
void MSG_vm_shutdown(msg_vm_t vm)
{
  simcall_vm_shutdown(vm);

  // Make sure that the processes in the VM are killed in this scheduling round before processing
  // (eg with the VM destroy)
  MSG_process_sleep(0.);
}

static inline char *get_mig_process_tx_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  return bprintf("__pr_mig_tx:%s(%s-%s)", vm->cname(), src_pm->cname(), dst_pm->cname());
}

static inline char *get_mig_process_rx_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  return bprintf("__pr_mig_rx:%s(%s-%s)", vm->cname(), src_pm->cname(), dst_pm->cname());
}

static inline char *get_mig_task_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm, int stage)
{
  return bprintf("__task_mig_stage%d:%s(%s-%s)", stage, vm->cname(), src_pm->cname(), dst_pm->cname());
}

struct migration_session {
  msg_vm_t vm;
  msg_host_t src_pm;
  msg_host_t dst_pm;

  /* The miration_rx process uses mbox_ctl to let the caller of do_migration()
   * know the completion of the migration. */
  char *mbox_ctl;
  /* The migration_rx and migration_tx processes use mbox to transfer migration data. */
  char *mbox;
};

static int migration_rx_fun(int argc, char *argv[])
{
  XBT_DEBUG("mig: rx_start");

  // The structure has been created in the do_migration function and should only be freed in the same place ;)
  struct migration_session *ms = (migration_session *) MSG_process_get_data(MSG_process_self());

  bool received_finalize = false;

  char *finalize_task_name = get_mig_task_name(ms->vm, ms->src_pm, ms->dst_pm, 3);
  while (!received_finalize) {
    msg_task_t task = nullptr;
    int ret         = MSG_task_recv(&task, ms->mbox);

    if (ret != MSG_OK) {
      // An error occurred, clean the code and return
      // The owner did not change, hence the task should be only destroyed on the other side
      xbt_free(finalize_task_name);
      return 0;
    }

    if (strcmp(task->name, finalize_task_name) == 0)
      received_finalize = 1;

    MSG_task_destroy(task);
  }
  xbt_free(finalize_task_name);

  // Here Stage 1, 2  and 3 have been performed.
  // Hence complete the migration

  // Copy the reference to the vm (if SRC crashes now, do_migration will free ms)
  // This is clearly ugly but I (Adrien) need more time to do something cleaner (actually we should copy the whole ms
  // structure at the beginning and free it at the end of each function)
  simgrid::s4u::VirtualMachine* vm = static_cast<simgrid::s4u::VirtualMachine*>(ms->vm);
  msg_host_t src_pm                = ms->src_pm;
  msg_host_t dst_pm                = ms->dst_pm;

  // Make sure that we cannot get interrupted between the migrate and the resume to not end in an inconsistent state
  simgrid::simix::kernelImmediate([vm, src_pm, dst_pm]() {
    /* Update the vm location */
    /* precopy migration makes the VM temporally paused */
    xbt_assert(static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getState() == SURF_VM_STATE_SUSPENDED);

    /* jump to vm_ws_xigrate(). this will update the vm location. */
    static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->migrate(dst_pm);

    /* Resume the VM */
    SIMIX_vm_resume(vm);
  });

  {
    // Now the VM is running on the new host (the migration is completed) (even if the SRC crash)
    vm->pimpl_vm_->isMigrating = false;
    XBT_DEBUG("VM(%s) moved from PM(%s) to PM(%s)", ms->vm->cname(), ms->src_pm->cname(), ms->dst_pm->cname());

    if (TRACE_msg_vm_is_enabled()) {
      static long long int counter = 0;
      char key[INSTR_DEFAULT_STR_SIZE];
      snprintf(key, INSTR_DEFAULT_STR_SIZE, "%lld", counter++);

      // start link
      container_t msg = PJ_container_get(vm->name().c_str());
      type_t type     = PJ_type_get("MSG_VM_LINK", PJ_type_get_root());
      new_pajeStartLink(MSG_get_clock(), PJ_container_get_root(), type, msg, "M", key);

      // destroy existing container of this vm
      container_t existing_container = PJ_container_get(vm->name().c_str());
      PJ_container_remove_from_parent(existing_container);
      PJ_container_free(existing_container);

      // create new container on the new_host location
      PJ_container_new(vm->cname(), INSTR_MSG_VM, PJ_container_get(sg_host_get_name(ms->dst_pm)));

      // end link
      msg  = PJ_container_get(vm->name().c_str());
      type = PJ_type_get("MSG_VM_LINK", PJ_type_get_root());
      new_pajeEndLink(MSG_get_clock(), PJ_container_get_root(), type, msg, "M", key);
    }
  }
  // Inform the SRC that the migration has been correctly performed
  {
    char *task_name = get_mig_task_name(ms->vm, ms->src_pm, ms->dst_pm, 4);
    msg_task_t task = MSG_task_create(task_name, 0, 0, nullptr);
    msg_error_t ret = MSG_task_send(task, ms->mbox_ctl);
    // xbt_assert(ret == MSG_OK);
    if(ret == MSG_HOST_FAILURE){
      // The DST has crashed, this is a problem has the VM since we are not sure whether SRC is considering that the VM
      // has been correctly migrated on the DST node
      // TODO What does it mean ? What should we do ?
      MSG_task_destroy(task);
    } else if(ret == MSG_TRANSFER_FAILURE){
      // The SRC has crashed, this is not a problem has the VM has been correctly migrated on the DST node
      MSG_task_destroy(task);
    }
    xbt_free(task_name);
  }

  XBT_DEBUG("mig: rx_done");
  return 0;
}

static void start_dirty_page_tracking(msg_vm_t vm)
{
  simgrid::surf::VirtualMachineImpl* pimpl = static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_;

  pimpl->dp_enabled = 1;
  if (!pimpl->dp_objs)
    return;

  char *key = nullptr;
  xbt_dict_cursor_t cursor = nullptr;
  dirty_page_t dp = nullptr;
  xbt_dict_foreach (pimpl->dp_objs, cursor, key, dp) {
    double remaining = MSG_task_get_flops_amount(dp->task);
    dp->prev_clock = MSG_get_clock();
    dp->prev_remaining = remaining;

    // XBT_INFO("%s@%s remaining %f", key, sg_host_name(vm), remaining);
  }
}

static void stop_dirty_page_tracking(msg_vm_t vm)
{
  static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->dp_enabled = 0;
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
  simgrid::surf::VirtualMachineImpl* pimpl = static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_;
  double total = 0;

  char *key = nullptr;
  xbt_dict_cursor_t cursor = nullptr;
  dirty_page_t dp = nullptr;
  xbt_dict_foreach (pimpl->dp_objs, cursor, key, dp) {
    double remaining = MSG_task_get_flops_amount(dp->task);

    double clock = MSG_get_clock();

    // total += calc_updated_pages(key, vm, dp, remaining, clock);
    total += get_computed(key, vm, dp, remaining, clock);

    dp->prev_remaining = remaining;
    dp->prev_clock = clock;
  }

  total += pimpl->dp_updated_by_deleted_tasks;

  XBT_DEBUG("mig-stage%d.%d: computed %f flop_counts (including %f by deleted tasks)", stage_for_fancy_debug,
            stage2_round_for_fancy_debug, total, pimpl->dp_updated_by_deleted_tasks);

  pimpl->dp_updated_by_deleted_tasks = 0;

  return total;
}

// TODO Is this code redundant with the information provided by
// msg_process_t MSG_process_create(const char *name, xbt_main_func_t code, void *data, msg_host_t host)
/** @brief take care of the dirty page tracking, in case we're adding a task to a migrating VM */
void MSG_host_add_task(msg_host_t host, msg_task_t task)
{
  simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host);
  if (vm == nullptr)
    return;
  simgrid::surf::VirtualMachineImpl* pimpl = static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_;

  double remaining = MSG_task_get_flops_amount(task);
  char *key = bprintf("%s-%p", task->name, task);

  dirty_page_t dp = xbt_new0(s_dirty_page, 1);
  dp->task = task;
  if (pimpl->dp_enabled) {
    dp->prev_clock = MSG_get_clock();
    dp->prev_remaining = remaining;
  }
  if (!pimpl->dp_objs)
    pimpl->dp_objs = xbt_dict_new();
  xbt_assert(xbt_dict_get_or_null(pimpl->dp_objs, key) == nullptr);
  xbt_dict_set(pimpl->dp_objs, key, dp, nullptr);
  XBT_DEBUG("add %s on %s (remaining %f, dp_enabled %d)", key, sg_host_get_name(host), remaining, pimpl->dp_enabled);

  xbt_free(key);
}

void MSG_host_del_task(msg_host_t host, msg_task_t task)
{
  simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host);
  if (vm == nullptr)
    return;
  simgrid::surf::VirtualMachineImpl* pimpl = static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_;

  char *key = bprintf("%s-%p", task->name, task);
  dirty_page_t dp = (dirty_page_t)(pimpl->dp_objs ? xbt_dict_get_or_null(pimpl->dp_objs, key) : NULL);
  xbt_assert(dp->task == task);

  /* If we are in the middle of dirty page tracking, we record how much computation has been done until now, and keep
   * the information for the lookup_() function that will called soon. */
  if (pimpl->dp_enabled) {
    double remaining = MSG_task_get_flops_amount(task);
    double clock = MSG_get_clock();
    // double updated = calc_updated_pages(key, host, dp, remaining, clock);
    double updated = get_computed(key, host, dp, remaining, clock);

    pimpl->dp_updated_by_deleted_tasks += updated;
  }
  if (pimpl->dp_objs)
    xbt_dict_remove(pimpl->dp_objs, key);
  xbt_free(dp);

  XBT_DEBUG("del %s on %s", key, sg_host_get_name(host));
  xbt_free(key);
}

static sg_size_t send_migration_data(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm, sg_size_t size, char* mbox,
                                     int stage, int stage2_round, double mig_speed, double timeout)
{
  sg_size_t sent = 0;
  char *task_name = get_mig_task_name(vm, src_pm, dst_pm, stage);
  msg_task_t task = MSG_task_create(task_name, 0, (double)size, nullptr);

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
    XBT_VERB("timeout (%lf s) in sending_migration_data, remaining %llu bytes of %llu", timeout, remaining, size);
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
    XBT_DEBUG("mig-stage%d.%d: sent %llu duration %f actual_speed %f (target %f)", stage, stage2_round, size, duration,
              actual_speed, mig_speed);
  else
    XBT_DEBUG("mig-stage%d: sent %llu duration %f actual_speed %f (target %f)", stage, size, duration, actual_speed,
              mig_speed);

  return sent;
}

static sg_size_t get_updated_size(double computed, double dp_rate, double dp_cap)
{
  double updated_size = computed * dp_rate;
  XBT_DEBUG("updated_size %f dp_rate %f", updated_size, dp_rate);
  if (updated_size > dp_cap) {
    // XBT_INFO("mig-stage2.%d: %f bytes updated, but cap it with the working set size %f", stage2_round, updated_size,
    //          dp_cap);
    updated_size = dp_cap;
  }

  return (sg_size_t) updated_size;
}

static int migration_tx_fun(int argc, char *argv[])
{
  XBT_DEBUG("mig: tx_start");

  // Note that the ms structure has been allocated in do_migration and hence should be freed in the same function ;)
  migration_session *ms = (migration_session *) MSG_process_get_data(MSG_process_self());

  s_vm_params_t params;
  static_cast<simgrid::s4u::VirtualMachine*>(ms->vm)->parameters(&params);
  const sg_size_t ramsize   = params.ramsize;
  const sg_size_t devsize   = params.devsize;
  const int skip_stage1     = params.skip_stage1;
  int skip_stage2           = params.skip_stage2;
  const double dp_rate      = params.dp_rate;
  const double dp_cap       = params.dp_cap;
  const double mig_speed    = params.mig_speed;
  double max_downtime       = params.max_downtime;

  double mig_timeout = 10000000.0;

  double remaining_size = (double) (ramsize + devsize);
  double threshold = 0.0;

  /* check parameters */
  if (ramsize == 0)
    XBT_WARN("migrate a VM, but ramsize is zero");

  if (max_downtime <= 0) {
    XBT_WARN("use the default max_downtime value 30ms");
    max_downtime = 0.03;
  }

  /* Stage1: send all memory pages to the destination. */
  XBT_DEBUG("mig-stage1: remaining_size %f", remaining_size);
  start_dirty_page_tracking(ms->vm);

  double computed_during_stage1 = 0;
  if (!skip_stage1) {
    double clock_prev_send = MSG_get_clock();

    try {
      /* At stage 1, we do not need timeout. We have to send all the memory pages even though the duration of this
       * transfer exceeds the timeout value. */
      XBT_VERB("Stage 1: Gonna send %llu", ramsize);
      sg_size_t sent = send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, ramsize, ms->mbox, 1, 0, mig_speed, -1);
      remaining_size -= sent;
      computed_during_stage1 = lookup_computed_flop_counts(ms->vm, 1, 0);

      if (sent < ramsize) {
        XBT_VERB("mig-stage1: timeout, force moving to stage 3");
        skip_stage2 = 1;
      } else if (sent > ramsize)
        XBT_CRITICAL("bug");

    }
    catch (xbt_ex& e) {
      //hostfailure (if you want to know whether this is the SRC or the DST check directly in send_migration_data code)
      // Stop the dirty page tracking an return (there is no memory space to release)
      stop_dirty_page_tracking(ms->vm);
      return 0;
    }

    double clock_post_send = MSG_get_clock();
    mig_timeout -= (clock_post_send - clock_prev_send);
    if (mig_timeout < 0) {
      XBT_VERB("The duration of stage 1 exceeds the timeout value, skip stage 2");
      skip_stage2 = 1;
    }

    /* estimate bandwidth */
    double bandwidth = ramsize / (clock_post_send - clock_prev_send);
    threshold        = bandwidth * max_downtime;
    XBT_DEBUG("actual bandwidth %f (MB/s), threshold %f", bandwidth / 1024 / 1024, threshold);
  }


  /* Stage2: send update pages iteratively until the size of remaining states becomes smaller than threshold value. */
  if (!skip_stage2) {

    int stage2_round = 0;
    for (;;) {

      sg_size_t updated_size = 0;
      if (stage2_round == 0) {
        /* just after stage1, nothing has been updated. But, we have to send the data updated during stage1 */
        updated_size = get_updated_size(computed_during_stage1, dp_rate, dp_cap);
      } else {
        double computed = lookup_computed_flop_counts(ms->vm, 2, stage2_round);
        updated_size    = get_updated_size(computed, dp_rate, dp_cap);
      }

      XBT_DEBUG("mig-stage 2:%d updated_size %llu computed_during_stage1 %f dp_rate %f dp_cap %f", stage2_round,
                updated_size, computed_during_stage1, dp_rate, dp_cap);

      /* Check whether the remaining size is below the threshold value. If so, move to stage 3. */
      remaining_size += updated_size;
      XBT_DEBUG("mig-stage2.%d: remaining_size %f (%s threshold %f)", stage2_round, remaining_size,
                (remaining_size < threshold) ? "<" : ">", threshold);
      if (remaining_size < threshold)
        break;

      sg_size_t sent         = 0;
      double clock_prev_send = MSG_get_clock();
      try {
        XBT_DEBUG("Stage 2, gonna send %llu", updated_size);
        sent = send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, updated_size, ms->mbox, 2, stage2_round, mig_speed,
                                   mig_timeout);
      } catch (xbt_ex& e) {
        // hostfailure (if you want to know whether this is the SRC or the DST check directly in send_migration_data
        // code)
        // Stop the dirty page tracking an return (there is no memory space to release)
        stop_dirty_page_tracking(ms->vm);
        return 0;
      }
      double clock_post_send = MSG_get_clock();

      if (sent == updated_size) {
        /* timeout did not happen */
        double bandwidth = updated_size / (clock_post_send - clock_prev_send);
        threshold        = bandwidth * max_downtime;
        XBT_DEBUG("actual bandwidth %f, threshold %f", bandwidth / 1024 / 1024, threshold);
        remaining_size -= sent;
        stage2_round += 1;
        mig_timeout -= (clock_post_send - clock_prev_send);
        xbt_assert(mig_timeout > 0);

      } else if (sent < updated_size) {
        /* When timeout happens, we move to stage 3. The size of memory pages
         * updated before timeout must be added to the remaining size. */
        XBT_VERB("mig-stage2.%d: timeout, force moving to stage 3. sent %llu / %llu, eta %lf", stage2_round, sent,
                 updated_size, (clock_post_send - clock_prev_send));
        remaining_size -= sent;

        double computed = lookup_computed_flop_counts(ms->vm, 2, stage2_round);
        updated_size    = get_updated_size(computed, dp_rate, dp_cap);
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

  try {
    XBT_DEBUG("Stage 3: Gonna send %f", remaining_size);
    send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, (sg_size_t)remaining_size, ms->mbox, 3, 0, mig_speed, -1);
  }
  catch(xbt_ex& e) {
    //hostfailure (if you want to know whether this is the SRC or the DST check directly in send_migration_data code)
    // Stop the dirty page tracking an return (there is no memory space to release)
    simcall_vm_resume(ms->vm);
    return 0;
  }

  // At that point the Migration is considered valid for the SRC node but remind that the DST side should relocate
  // effectively the VM on the DST node.
  XBT_DEBUG("mig: tx_done");

  return 0;
}

/** @brief Migrate the VM to the given host.
 *  @ingroup msg_VMs
 */
void MSG_vm_migrate(msg_vm_t vm, msg_host_t dst_pm)
{
  /* some thoughts:
   * - One approach is ...
   *   We first create a new VM (i.e., destination VM) on the destination   physical host. The destination VM will
   *   receive the state of the source
   *   VM over network. We will finally destroy the source VM.
   *   - This behavior is similar to the way of migration in the real world.
   *     Even before a migration is completed, we will see a destination VM, consuming resources.
   *   - We have to relocate all processes. The existing process migration code will work for this?
   *   - The name of the VM is a somewhat unique ID in the code. It is tricky for the destination VM?
   *
   * - Another one is ...
   *   We update the information of the given VM to place it to the destination physical host.
   *
   * The second one would be easier.
   */

  simgrid::s4u::VirtualMachine* typedVm    = static_cast<simgrid::s4u::VirtualMachine*>(vm);
  simgrid::surf::VirtualMachineImpl* pimpl = typedVm->pimpl_vm_;
  msg_host_t src_pm                        = pimpl->getPm();

  if (src_pm->isOff())
    THROWF(vm_error, 0, "Cannot migrate VM '%s' from host '%s', which is offline.", vm->cname(), src_pm->cname());
  if (dst_pm->isOff())
    THROWF(vm_error, 0, "Cannot migrate VM '%s' to host '%s', which is offline.", vm->cname(), dst_pm->cname());
  if (!MSG_vm_is_running(vm))
    THROWF(vm_error, 0, "Cannot migrate VM '%s' that is not running yet.", vm->cname());
  if (typedVm->isMigrating())
    THROWF(vm_error, 0, "Cannot migrate VM '%s' that is already migrating.", vm->cname());

  pimpl->isMigrating = true;

  struct migration_session *ms = xbt_new(struct migration_session, 1);
  ms->vm = vm;
  ms->src_pm = src_pm;
  ms->dst_pm = dst_pm;

  /* We have two mailboxes. mbox is used to transfer migration data between source and destination PMs. mbox_ctl is used
   * to detect the completion of a migration. The names of these mailboxes must not conflict with others. */
  ms->mbox_ctl = bprintf("__mbox_mig_ctl:%s(%s-%s)", vm->cname(), src_pm->cname(), dst_pm->cname());
  ms->mbox     = bprintf("__mbox_mig_src_dst:%s(%s-%s)", vm->cname(), src_pm->cname(), dst_pm->cname());

  char *pr_rx_name = get_mig_process_rx_name(vm, src_pm, dst_pm);
  char *pr_tx_name = get_mig_process_tx_name(vm, src_pm, dst_pm);

  char** argv = xbt_new(char*, 2);
  argv[0]     = pr_rx_name;
  argv[1]     = nullptr;
  MSG_process_create_with_arguments(pr_rx_name, migration_rx_fun, ms, dst_pm, 1, argv);

  argv        = xbt_new(char*, 2);
  argv[0]     = pr_tx_name;
  argv[1]     = nullptr;
  MSG_process_create_with_arguments(pr_tx_name, migration_tx_fun, ms, src_pm, 1, argv);

  /* wait until the migration have finished or on error has occurred */
  XBT_DEBUG("wait for reception of the final ACK (i.e. migration has been correctly performed");
  msg_task_t task = nullptr;
  msg_error_t ret = MSG_TIMEOUT;
  while (ret == MSG_TIMEOUT && dst_pm->isOn()) // The rx will tell me when he gots the VM
    ret = MSG_task_receive_with_timeout(&task, ms->mbox_ctl, 4);

  pimpl->isMigrating = false;

  xbt_free(ms->mbox_ctl);
  xbt_free(ms->mbox);
  xbt_free(ms);

  if (ret == MSG_HOST_FAILURE) {
    // Note that since the communication failed, the owner did not change and the task should be destroyed on the
    // other side. Hence, just throw the execption
    XBT_ERROR("SRC crashes, throw an exception (m-control)");
    // MSG_process_kill(tx_process); // Adrien, I made a merge on Nov 28th 2014, I'm not sure whether this line is
    // required or not
    THROWF(host_error, 0, "Source host '%s' failed during the migration of VM '%s'.", src_pm->cname(), vm->cname());
  } else if ((ret == MSG_TRANSFER_FAILURE) || (ret == MSG_TIMEOUT)) {
    // MSG_TIMEOUT here means that MSG_host_is_avail() returned false.
    XBT_ERROR("DST crashes, throw an exception (m-control)");
    THROWF(host_error, 0, "Destination host '%s' failed during the migration of VM '%s'.", dst_pm->cname(),
           vm->cname());
  }

  char* expected_task_name = get_mig_task_name(vm, src_pm, dst_pm, 4);
  xbt_assert(strcmp(task->name, expected_task_name) == 0);
  xbt_free(expected_task_name);
  MSG_task_destroy(task);
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
    THROWF(vm_error, 0, "Cannot suspend VM '%s', which is migrating", vm->cname());

  simcall_vm_suspend(vm);

  XBT_DEBUG("vm_suspend done");

  if (TRACE_msg_vm_is_enabled()) {
    container_t vm_container = PJ_container_get(vm->cname());
    type_t type              = PJ_type_get("MSG_VM_STATE", vm_container->type);
    val_t value              = PJ_value_get_or_new("suspend", "1 0 0", type); // suspend is red
    new_pajePushState(MSG_get_clock(), vm_container, type, value);
  }
}

/** @brief Resume the execution of the VM. All processes on the VM run again.
 *  @ingroup msg_VMs
 *
 * No resume cost occurs.
 */
void MSG_vm_resume(msg_vm_t vm)
{
  simcall_vm_resume(vm);

  if (TRACE_msg_vm_is_enabled()) {
    container_t vm_container = PJ_container_get(vm->cname());
    type_t type              = PJ_type_get("MSG_VM_STATE", vm_container->type);
    new_pajePopState(MSG_get_clock(), vm_container, type);
  }
}


/** @brief Immediately save the execution of all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * This function stops the execution of the VM. All the processes on this VM
 * will pause. The state of the VM is preserved. We can later resume it again.
 *
 * FIXME: No suspension cost occurs. If you want to simulate this too, you want to use a \ref MSG_file_write() before
 * or after, depending on the exact semantic of VM save to you.
 */
void MSG_vm_save(msg_vm_t vm)
{
  if (MSG_vm_is_migrating(vm))
    THROWF(vm_error, 0, "Cannot save VM '%s', which is migrating.", vm->cname());

  simcall_vm_save(vm);

  if (TRACE_msg_vm_is_enabled()) {
    container_t vm_container = PJ_container_get(vm->cname());
    type_t type              = PJ_type_get("MSG_VM_STATE", vm_container->type);
    val_t value              = PJ_value_get_or_new("save", "0 1 0", type); // save is green
    new_pajePushState(MSG_get_clock(), vm_container, type, value);
  }
}

/** @brief Restore the execution of the VM. All processes on the VM run again.
 *  @ingroup msg_VMs
 *
 * FIXME: No restore cost occurs. If you want to simulate this too, you want to use a \ref MSG_file_read() before or
 * after, depending on the exact semantic of VM restore to you.
 */
void MSG_vm_restore(msg_vm_t vm)
{
  simgrid::simix::kernelImmediate([vm]() {
    if (static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getState() != SURF_VM_STATE_SAVED)
      THROWF(vm_error, 0, "VM(%s) was not saved", vm->name().c_str());

    XBT_DEBUG("restore VM(%s), where %d processes exist", vm->name().c_str(),
              xbt_swag_size(sg_host_simix(vm)->process_list));

    /* jump to vm_ws_restore() */
    static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->restore();

    smx_actor_t smx_process, smx_process_safe;
    xbt_swag_foreach_safe(smx_process, smx_process_safe, sg_host_simix(vm)->process_list)
    {
      XBT_DEBUG("resume %s", smx_process->name.c_str());
      SIMIX_process_resume(smx_process);
    }
  });

  if (TRACE_msg_vm_is_enabled()) {
    container_t vm_container = PJ_container_get(vm->cname());
    type_t type              = PJ_type_get("MSG_VM_STATE", vm_container->type);
    new_pajePopState(MSG_get_clock(), vm_container, type);
  }
}

/** @brief Get the physical host of a given VM.
 *  @ingroup msg_VMs
 */
msg_host_t MSG_vm_get_pm(msg_vm_t vm)
{
  return static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->getPm();
}

/** @brief Set a CPU bound for a given VM.
 *  @ingroup msg_VMs
 *
 * 1. Note that in some cases MSG_task_set_bound() may not intuitively work for VMs.
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
 * This is because a VM has the dummy CPU action in the PM layer. Putting a task on the VM does not affect the bound of
 * the dummy CPU action. The bound of the dummy CPU action is unlimited.
 *
 * There are some solutions for this problem. One option is to update the bound of the dummy CPU action automatically.
 * It should be the sum of all tasks on the VM. But, this solution might be costly, because we have to scan all tasks
 * on the VM in share_resource() or we have to trap both the start and end of task execution.
 *
 * The current solution is to use MSG_vm_set_bound(), which allows us to directly set the bound of the dummy CPU action.
 *
 * 2. Note that bound == 0 means no bound (i.e., unlimited). But, if a host has multiple CPU cores, the CPU share of a
 *    computation task (or a VM) never exceeds the capacity of a CPU core.
 */
void MSG_vm_set_bound(msg_vm_t vm, double bound)
{
  simgrid::simix::kernelImmediate(
      [vm, bound]() { static_cast<simgrid::s4u::VirtualMachine*>(vm)->pimpl_vm_->setBound(bound); });
}
