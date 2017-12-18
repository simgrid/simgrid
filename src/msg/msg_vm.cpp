/* Copyright (c) 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* TODO:
 * 1. add the support of trace
 * 2. use parallel tasks to simulate CPU overhead and remove the experimental code generating micro computation tasks
 */

#include <xbt/ex.hpp>

#include "simgrid/plugins/live_migration.h"
#include "src/instr/instr_private.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/plugins/vm/VmHostExt.hpp"

#include "simgrid/host.h"
#include "simgrid/simix.hpp"
#include "xbt/string.hpp"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_vm, msg, "Cloud-oriented parts of the MSG API");

const char* MSG_vm_get_name(msg_vm_t vm)
{
  return vm->getCname();
}

/** \ingroup m_vm_management
 * \brief Set the parameters of a given host
 *
 * \param vm a vm
 * \param params a parameter object
 */
void MSG_vm_set_params(msg_vm_t vm, vm_params_t params)
{
  vm->setParameters(params);
}

/** \ingroup m_vm_management
 * \brief Get the parameters of a given host
 *
 * \param vm the vm you are interested into
 * \param params a prameter object
 */
void MSG_vm_get_params(msg_vm_t vm, vm_params_t params)
{
  vm->getParameters(params);
}

void MSG_vm_set_ramsize(msg_vm_t vm, size_t size)
{
  vm->setRamsize(size);
}
size_t MSG_vm_get_ramsize(msg_vm_t vm)
{
  return vm->getRamsize();
}

/* **** Check state of a VM **** */
static inline int __MSG_vm_is_state(msg_vm_t vm, e_surf_vm_state_t state)
{
  return vm->pimpl_vm_ != nullptr && vm->getState() == state;
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
  return vm->isMigrating();
}

/** @brief Returns whether the given VM is currently suspended, not running.
 *  @ingroup msg_VMs
 */
int MSG_vm_is_suspended(msg_vm_t vm)
{
  return __MSG_vm_is_state(vm, SURF_VM_STATE_SUSPENDED);
}

/* **** ******** MSG vm actions ********* **** */
/** @brief Create a new VM with specified parameters.
 *  @ingroup msg_VMs*
 *  @param pm        Physical machine that will host the VM
 *  @param name      Must be unique
 *  @param coreAmount Must be >= 1
 *  @param ramsize   [TODO]
 *  @param mig_netspeed Amount of Mbyte/s allocated to the migration (cannot be larger than net_cap). Use 0 if unsure.
 *  @param dp_intensity Dirty page percentage according to migNetSpeed, [0-100]. Use 0 if unsure.
 */
msg_vm_t MSG_vm_create(msg_host_t pm, const char* name, int coreAmount, int ramsize, int mig_netspeed, int dp_intensity)
{
  simgrid::vm::VmHostExt::ensureVmExtInstalled();

  /* For the moment, intensity_rate is the percentage against the migration bandwidth */

  msg_vm_t vm = new simgrid::s4u::VirtualMachine(name, pm, coreAmount, static_cast<sg_size_t>(ramsize) * 1024 * 1024);
  s_vm_params_t params;
  params.max_downtime = 0.03;
  params.mig_speed    = static_cast<double>(mig_netspeed) * 1024 * 1024; // mig_speed
  params.dp_intensity = static_cast<double>(dp_intensity) / 100;
  params.dp_cap       = vm->getRamsize() * 0.9; // assume working set memory is 90% of ramsize

  XBT_DEBUG("migspeed : %f intensity mem : %d", params.mig_speed, dp_intensity);
  vm->setParameters(&params);

  return vm;
}

/** @brief Create a new VM object with the default parameters
 *  @ingroup msg_VMs*
 *
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
msg_vm_t MSG_vm_create_core(msg_host_t pm, const char* name)
{
  xbt_assert(sg_host_by_name(name) == nullptr,
             "Cannot create a VM named %s: this name is already used by an host or a VM", name);

  msg_vm_t vm = new simgrid::s4u::VirtualMachine(name, pm, 1);
  s_vm_params_t params;
  memset(&params, 0, sizeof(params));
  vm->setParameters(&params);
  return vm;
}
/** @brief Create a new VM object with the default parameters, but with a specified amount of cores
 *  @ingroup msg_VMs*
 *
 * A VM is treated as a host. The name of the VM must be unique among all hosts.
 */
msg_vm_t MSG_vm_create_multicore(msg_host_t pm, const char* name, int coreAmount)
{
  xbt_assert(sg_host_by_name(name) == nullptr,
             "Cannot create a VM named %s: this name is already used by an host or a VM", name);

  msg_vm_t vm = new simgrid::s4u::VirtualMachine(name, pm, coreAmount);
  s_vm_params_t params;
  memset(&params, 0, sizeof(params));
  vm->setParameters(&params);
  return vm;
}

/** @brief Destroy a VM. Destroy the VM object from the simulation.
 *  @ingroup msg_VMs
 */
void MSG_vm_destroy(msg_vm_t vm)
{
  if (vm->isMigrating())
    THROWF(vm_error, 0, "Cannot destroy VM '%s', which is migrating.", vm->getCname());

  /* First, terminate all processes on the VM if necessary */
  vm->shutdown();

  /* Then, destroy the VM object */
  vm->destroy();

  if (TRACE_msg_vm_is_enabled()) {
    container_t container = simgrid::instr::Container::byName(vm->getName());
    container->removeFromParent();
    delete container;
  }
}

/** @brief Start a vm (i.e., boot the guest operating system)
 *  @ingroup msg_VMs
 *
 *  If the VM cannot be started (because of memory over-provisioning), an exception is generated.
 */
void MSG_vm_start(msg_vm_t vm)
{
  vm->start();
  if (TRACE_msg_vm_is_enabled()) {
    simgrid::instr::StateType* state = simgrid::instr::Container::byName(vm->getName())->getState("MSG_VM_STATE");
    state->addEntityValue("start", "0 0 1"); // start is blue
    state->pushEvent("start");
  }
}

/** @brief Immediately kills all processes within the given VM.
 *  @ingroup msg_VMs
 *
 * Any memory that they allocated will be leaked, unless you used #MSG_process_on_exit().
 *
 * No extra delay occurs. If you want to simulate this too, you want to use a #MSG_process_sleep().
 */
void MSG_vm_shutdown(msg_vm_t vm)
{
  vm->shutdown();
}

static std::string get_mig_process_tx_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  return std::string("__pr_mig_tx:") + vm->getCname() + "(" + src_pm->getCname() + "-" + dst_pm->getCname() + ")";
}

static std::string get_mig_process_rx_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm)
{
  return std::string("__pr_mig_rx:") + vm->getCname() + "(" + src_pm->getCname() + "-" + dst_pm->getCname() + ")";
}

static std::string get_mig_task_name(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm, int stage)
{
  return std::string("__task_mig_stage") + std::to_string(stage) + ":" + vm->getCname() + "(" + src_pm->getCname() +
         "-" + dst_pm->getCname() + ")";
}

struct migration_session {
  msg_vm_t vm;
  msg_host_t src_pm;
  msg_host_t dst_pm;

  /* The miration_rx process uses mbox_ctl to let the caller of do_migration()
   * know the completion of the migration. */
  std::string mbox_ctl;
  /* The migration_rx and migration_tx processes use mbox to transfer migration data. */
  std::string mbox;
};

static int migration_rx_fun(int argc, char *argv[])
{
  XBT_DEBUG("mig: rx_start");

  // The structure has been created in the do_migration function and should only be freed in the same place ;)
  migration_session* ms = static_cast<migration_session*>(MSG_process_get_data(MSG_process_self()));

  bool received_finalize = false;

  std::string finalize_task_name = get_mig_task_name(ms->vm, ms->src_pm, ms->dst_pm, 3);
  while (not received_finalize) {
    msg_task_t task = nullptr;
    int ret         = MSG_task_recv(&task, ms->mbox.c_str());

    if (ret != MSG_OK) {
      // An error occurred, clean the code and return
      // The owner did not change, hence the task should be only destroyed on the other side
      return 0;
    }

    if (finalize_task_name == task->name)
      received_finalize = 1;

    MSG_task_destroy(task);
  }

  // Here Stage 1, 2  and 3 have been performed.
  // Hence complete the migration

  // Copy the reference to the vm (if SRC crashes now, do_migration will free ms)
  // This is clearly ugly but I (Adrien) need more time to do something cleaner (actually we should copy the whole ms
  // structure at the beginning and free it at the end of each function)
  simgrid::s4u::VirtualMachine* vm = ms->vm;
  msg_host_t dst_pm                = ms->dst_pm;

  // Make sure that we cannot get interrupted between the migrate and the resume to not end in an inconsistent state
  simgrid::simix::kernelImmediate([vm, dst_pm]() {
    /* Update the vm location */
    /* precopy migration makes the VM temporally paused */
    xbt_assert(vm->getState() == SURF_VM_STATE_SUSPENDED);

    /* Update the vm location and resume it */
    vm->pimpl_vm_->setPm(dst_pm);
    vm->resume();
  });


  // Now the VM is running on the new host (the migration is completed) (even if the SRC crash)
  vm->pimpl_vm_->isMigrating = false;
  XBT_DEBUG("VM(%s) moved from PM(%s) to PM(%s)", ms->vm->getCname(), ms->src_pm->getCname(), ms->dst_pm->getCname());

  if (TRACE_msg_vm_is_enabled()) {
    static long long int counter = 0;
    std::string key              = std::to_string(counter);
    counter++;

    // start link
    container_t msg = simgrid::instr::Container::byName(vm->getName());
    simgrid::instr::Container::getRoot()->getLink("MSG_VM_LINK")->startEvent(msg, "M", key);

    // destroy existing container of this vm
    container_t existing_container = simgrid::instr::Container::byName(vm->getName());
    existing_container->removeFromParent();
    delete existing_container;

    // create new container on the new_host location
    new simgrid::instr::Container(vm->getCname(), "MSG_VM", simgrid::instr::Container::byName(ms->dst_pm->getName()));

    // end link
    msg  = simgrid::instr::Container::byName(vm->getName());
    simgrid::instr::Container::getRoot()->getLink("MSG_VM_LINK")->endEvent(msg, "M", key);
  }

  // Inform the SRC that the migration has been correctly performed
  std::string task_name = get_mig_task_name(ms->vm, ms->src_pm, ms->dst_pm, 4);
  msg_task_t task       = MSG_task_create(task_name.c_str(), 0, 0, nullptr);
  msg_error_t ret       = MSG_task_send(task, ms->mbox_ctl.c_str());
  if(ret == MSG_HOST_FAILURE){
    // The DST has crashed, this is a problem has the VM since we are not sure whether SRC is considering that the VM
    // has been correctly migrated on the DST node
    // TODO What does it mean ? What should we do ?
    MSG_task_destroy(task);
  } else if(ret == MSG_TRANSFER_FAILURE){
    // The SRC has crashed, this is not a problem has the VM has been correctly migrated on the DST node
    MSG_task_destroy(task);
  }

  XBT_DEBUG("mig: rx_done");
  return 0;
}

static sg_size_t send_migration_data(msg_vm_t vm, msg_host_t src_pm, msg_host_t dst_pm, sg_size_t size,
                                     const std::string& mbox, int stage, int stage2_round, double mig_speed,
                                     double timeout)
{
  sg_size_t sent = 0;
  std::string task_name = get_mig_task_name(vm, src_pm, dst_pm, stage);
  msg_task_t task       = MSG_task_create(task_name.c_str(), 0, static_cast<double>(size), nullptr);

  double clock_sta = MSG_get_clock();

  msg_error_t ret;
  if (mig_speed > 0)
    ret = MSG_task_send_with_timeout_bounded(task, mbox.c_str(), timeout, mig_speed);
  else
    ret = MSG_task_send(task, mbox.c_str());

  if (ret == MSG_OK) {
    sent = size;
  } else if (ret == MSG_TIMEOUT) {
    sg_size_t remaining = static_cast<sg_size_t>(MSG_task_get_remaining_communication(task));
    sent = size - remaining;
    XBT_VERB("timeout (%lf s) in sending_migration_data, remaining %llu bytes of %llu", timeout, remaining, size);
  }

  /* FIXME: why try-and-catch is used here? */
  if(ret == MSG_HOST_FAILURE){
    XBT_DEBUG("SRC host failed during migration of %s (stage %d)", vm->getCname(), stage);
    MSG_task_destroy(task);
    THROWF(host_error, 0, "SRC host failed during migration of %s (stage %d)", vm->getCname(), stage);
  }else if(ret == MSG_TRANSFER_FAILURE){
    XBT_DEBUG("DST host failed during migration of %s (stage %d)", vm->getCname(), stage);
    MSG_task_destroy(task);
    THROWF(host_error, 0, "DST host failed during migration of %s (stage %d)", vm->getCname(), stage);
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
    updated_size = dp_cap;
  }

  return static_cast<sg_size_t>(updated_size);
}

static int migration_tx_fun(int argc, char *argv[])
{
  XBT_DEBUG("mig: tx_start");

  // Note that the ms structure has been allocated in do_migration and hence should be freed in the same function ;)
  migration_session* ms = static_cast<migration_session*>(MSG_process_get_data(MSG_process_self()));

  double host_speed = ms->vm->getPm()->getSpeed();
  s_vm_params_t params;
  ms->vm->getParameters(&params);
  const sg_size_t ramsize   = ms->vm->getRamsize();
  const double dp_rate      = host_speed ? (params.mig_speed * params.dp_intensity) / host_speed : 1;
  const double dp_cap       = params.dp_cap;
  const double mig_speed    = params.mig_speed;
  double max_downtime       = params.max_downtime;

  double mig_timeout = 10000000.0;
  bool skip_stage2   = false;

  size_t remaining_size = ramsize;
  size_t threshold      = 0.0;

  /* check parameters */
  if (ramsize == 0)
    XBT_WARN("migrate a VM, but ramsize is zero");

  if (max_downtime <= 0) {
    XBT_WARN("use the default max_downtime value 30ms");
    max_downtime = 0.03;
  }

  /* Stage1: send all memory pages to the destination. */
  XBT_DEBUG("mig-stage1: remaining_size %zu", remaining_size);
  sg_vm_start_dirty_page_tracking(ms->vm);

  double computed_during_stage1 = 0;
  double clock_prev_send        = MSG_get_clock();

  try {
    /* At stage 1, we do not need timeout. We have to send all the memory pages even though the duration of this
     * transfer exceeds the timeout value. */
    XBT_VERB("Stage 1: Gonna send %llu bytes", ramsize);
    sg_size_t sent = send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, ramsize, ms->mbox, 1, 0, mig_speed, -1);
    remaining_size -= sent;
    computed_during_stage1 = sg_vm_lookup_computed_flops(ms->vm);

    if (sent < ramsize) {
      XBT_VERB("mig-stage1: timeout, force moving to stage 3");
      skip_stage2 = true;
    } else if (sent > ramsize)
      XBT_CRITICAL("bug");

  } catch (xbt_ex& e) {
    // hostfailure (if you want to know whether this is the SRC or the DST check directly in send_migration_data code)
    // Stop the dirty page tracking an return (there is no memory space to release)
    sg_vm_stop_dirty_page_tracking(ms->vm);
    return 0;
  }

  double clock_post_send = MSG_get_clock();
  mig_timeout -= (clock_post_send - clock_prev_send);
  if (mig_timeout < 0) {
    XBT_VERB("The duration of stage 1 exceeds the timeout value, skip stage 2");
    skip_stage2 = true;
  }

  /* estimate bandwidth */
  double bandwidth = ramsize / (clock_post_send - clock_prev_send);
  threshold        = bandwidth * max_downtime;
  XBT_DEBUG("actual bandwidth %f (MB/s), threshold %zu", bandwidth / 1024 / 1024, threshold);

  /* Stage2: send update pages iteratively until the size of remaining states becomes smaller than threshold value. */
  if (not skip_stage2) {

    int stage2_round = 0;
    for (;;) {

      sg_size_t updated_size = 0;
      if (stage2_round == 0) {
        /* just after stage1, nothing has been updated. But, we have to send the data updated during stage1 */
        updated_size = get_updated_size(computed_during_stage1, dp_rate, dp_cap);
      } else {
        double computed = sg_vm_lookup_computed_flops(ms->vm);
        updated_size    = get_updated_size(computed, dp_rate, dp_cap);
      }

      XBT_DEBUG("mig-stage 2:%d updated_size %llu computed_during_stage1 %f dp_rate %f dp_cap %f", stage2_round,
                updated_size, computed_during_stage1, dp_rate, dp_cap);

      /* Check whether the remaining size is below the threshold value. If so, move to stage 3. */
      remaining_size += updated_size;
      XBT_DEBUG("mig-stage2.%d: remaining_size %zu (%s threshold %zu)", stage2_round, remaining_size,
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
        sg_vm_stop_dirty_page_tracking(ms->vm);
        return 0;
      }
      double clock_post_send = MSG_get_clock();

      if (sent == updated_size) {
        /* timeout did not happen */
        double bandwidth = updated_size / (clock_post_send - clock_prev_send);
        threshold        = bandwidth * max_downtime;
        XBT_DEBUG("actual bandwidth %f, threshold %zu", bandwidth / 1024 / 1024, threshold);
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

        double computed = sg_vm_lookup_computed_flops(ms->vm);
        updated_size    = get_updated_size(computed, dp_rate, dp_cap);
        remaining_size += updated_size;
        break;
      } else
        XBT_CRITICAL("bug");
    }
  }

  /* Stage3: stop the VM and copy the rest of states. */
  XBT_DEBUG("mig-stage3: remaining_size %zu", remaining_size);
  simgrid::vm::VirtualMachineImpl* pimpl = ms->vm->pimpl_vm_;
  pimpl->setState(SURF_VM_STATE_RUNNING); // FIXME: this bypass of the checks in suspend() is not nice
  pimpl->isMigrating = false;             // FIXME: this bypass of the checks in suspend() is not nice
  pimpl->suspend(SIMIX_process_self());
  sg_vm_stop_dirty_page_tracking(ms->vm);

  try {
    XBT_DEBUG("Stage 3: Gonna send %zu bytes", remaining_size);
    send_migration_data(ms->vm, ms->src_pm, ms->dst_pm, remaining_size, ms->mbox, 3, 0, mig_speed, -1);
  }
  catch(xbt_ex& e) {
    //hostfailure (if you want to know whether this is the SRC or the DST check directly in send_migration_data code)
    // Stop the dirty page tracking an return (there is no memory space to release)
    ms->vm->resume();
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

  msg_host_t src_pm = vm->getPm();

  if (src_pm->isOff())
    THROWF(vm_error, 0, "Cannot migrate VM '%s' from host '%s', which is offline.", vm->getCname(), src_pm->getCname());
  if (dst_pm->isOff())
    THROWF(vm_error, 0, "Cannot migrate VM '%s' to host '%s', which is offline.", vm->getCname(), dst_pm->getCname());
  if (not MSG_vm_is_running(vm))
    THROWF(vm_error, 0, "Cannot migrate VM '%s' that is not running yet.", vm->getCname());
  if (vm->isMigrating())
    THROWF(vm_error, 0, "Cannot migrate VM '%s' that is already migrating.", vm->getCname());

  vm->pimpl_vm_->isMigrating = true;

  migration_session ms;
  ms.vm     = vm;
  ms.src_pm = src_pm;
  ms.dst_pm = dst_pm;

  /* We have two mailboxes. mbox is used to transfer migration data between source and destination PMs. mbox_ctl is used
   * to detect the completion of a migration. The names of these mailboxes must not conflict with others. */
  ms.mbox_ctl =
      simgrid::xbt::string_printf("__mbox_mig_ctl:%s(%s-%s)", vm->getCname(), src_pm->getCname(), dst_pm->getCname());
  ms.mbox = simgrid::xbt::string_printf("__mbox_mig_src_dst:%s(%s-%s)", vm->getCname(), src_pm->getCname(),
                                        dst_pm->getCname());

  std::string pr_rx_name = get_mig_process_rx_name(vm, src_pm, dst_pm);
  std::string pr_tx_name = get_mig_process_tx_name(vm, src_pm, dst_pm);

  MSG_process_create(pr_rx_name.c_str(), migration_rx_fun, &ms, dst_pm);

  MSG_process_create(pr_tx_name.c_str(), migration_tx_fun, &ms, src_pm);

  /* wait until the migration have finished or on error has occurred */
  XBT_DEBUG("wait for reception of the final ACK (i.e. migration has been correctly performed");
  msg_task_t task = nullptr;
  msg_error_t ret = MSG_task_receive(&task, ms.mbox_ctl.c_str());

  vm->pimpl_vm_->isMigrating = false;

  if (ret == MSG_HOST_FAILURE) {
    // Note that since the communication failed, the owner did not change and the task should be destroyed on the
    // other side. Hence, just throw the execption
    XBT_ERROR("SRC crashes, throw an exception (m-control)");
    // MSG_process_kill(tx_process); // Adrien, I made a merge on Nov 28th 2014, I'm not sure whether this line is
    // required or not
    THROWF(host_error, 0, "Source host '%s' failed during the migration of VM '%s'.", src_pm->getCname(),
           vm->getCname());
  } else if ((ret == MSG_TRANSFER_FAILURE) || (ret == MSG_TIMEOUT)) {
    // MSG_TIMEOUT here means that MSG_host_is_avail() returned false.
    XBT_ERROR("DST crashes, throw an exception (m-control)");
    THROWF(host_error, 0, "Destination host '%s' failed during the migration of VM '%s'.", dst_pm->getCname(),
           vm->getCname());
  }

  xbt_assert(get_mig_task_name(vm, src_pm, dst_pm, 4) == task->name);
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
  vm->suspend();
  if (TRACE_msg_vm_is_enabled()) {
    simgrid::instr::StateType* state = simgrid::instr::Container::byName(vm->getName())->getState("MSG_VM_STATE");
    state->addEntityValue("suspend", "1 0 0"); // suspend is red
    state->pushEvent("suspend");
  }
}

/** @brief Resume the execution of the VM. All processes on the VM run again.
 *  @ingroup msg_VMs
 *
 * No resume cost occurs.
 */
void MSG_vm_resume(msg_vm_t vm)
{
  vm->resume();
  if (TRACE_msg_vm_is_enabled())
    simgrid::instr::Container::byName(vm->getName())->getState("MSG_VM_STATE")->popEvent();
}

/** @brief Get the physical host of a given VM.
 *  @ingroup msg_VMs
 */
msg_host_t MSG_vm_get_pm(msg_vm_t vm)
{
  return vm->getPm();
}

void MSG_vm_set_bound(msg_vm_t vm, double bound)
{
  vm->setBound(bound);
}
}
