/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/plugins/vm/VmLiveMigration.hpp"
#include "simgrid/Exception.hpp"
#include "src/instr/instr_private.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/plugins/vm/VmHostExt.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(vm_live_migration, s4u, "S4U virtual machines live migration");

namespace simgrid {
namespace vm {
simgrid::xbt::Extension<s4u::Host, VmMigrationExt> VmMigrationExt::EXTENSION_ID;

void VmMigrationExt::ensureVmMigrationExtInstalled()
{
  if (not EXTENSION_ID.valid())
    EXTENSION_ID = simgrid::s4u::Host::extension_create<VmMigrationExt>();
}

void MigrationRx::operator()()
{
  XBT_DEBUG("mig: rx_start");
  bool received_finalize = false;

  std::string finalize_task_name =
      std::string("__mig_stage3:") + vm_->get_cname() + "(" + src_pm_->get_cname() + "-" + dst_pm_->get_cname() + ")";

  while (not received_finalize) {
    std::string* payload = static_cast<std::string*>(mbox->get());

    if (finalize_task_name == *payload)
      received_finalize = true;

    delete payload;
  }

  // Here Stage 1, 2  and 3 have been performed.
  // Hence complete the migration

  /* Update the vm location */
  /* precopy migration makes the VM temporally paused */
  xbt_assert(vm_->get_state() == s4u::VirtualMachine::state::SUSPENDED);

  /* Update the vm location and resume it */
  vm_->set_pm(dst_pm_);
  vm_->resume();

  // Now the VM is running on the new host (the migration is completed) (even if the SRC crash)
  vm_->get_impl()->is_migrating_ = false;
  XBT_DEBUG("VM(%s) moved from PM(%s) to PM(%s)", vm_->get_cname(), src_pm_->get_cname(), dst_pm_->get_cname());

  if (TRACE_vm_is_enabled()) {
    static long long int counter = 0;
    std::string key              = std::to_string(counter);
    counter++;

    // start link
    container_t msg = simgrid::instr::Container::by_name(vm_->get_name());
    simgrid::instr::Container::get_root()->get_link("VM_LINK")->start_event(msg, "M", key);

    // destroy existing container of this vm
    simgrid::instr::Container::by_name(vm_->get_name())->remove_from_parent();

    // create new container on the new_host location
    new simgrid::instr::Container(vm_->get_name(), "VM", simgrid::instr::Container::by_name(dst_pm_->get_name()));

    // end link
    msg = simgrid::instr::Container::by_name(vm_->get_name());
    simgrid::instr::Container::get_root()->get_link("VM_LINK")->end_event(msg, "M", key);
  }
  // Inform the SRC that the migration has been correctly performed
  std::string* payload = new std::string("__mig_stage4:");
  *payload             = *payload + vm_->get_cname() + "(" + src_pm_->get_cname() + "-" + dst_pm_->get_cname() + ")";

  mbox_ctl->put(payload, 0);

  XBT_DEBUG("mig: rx_done");
}

static sg_size_t get_updated_size(double computed, double dp_rate, sg_size_t dp_cap)
{
  sg_size_t updated_size = static_cast<sg_size_t>(computed * dp_rate);
  XBT_DEBUG("updated_size %llu dp_rate %f", updated_size, dp_rate);
  if (updated_size > dp_cap) {
    updated_size = dp_cap;
  }

  return updated_size;
}

sg_size_t MigrationTx::sendMigrationData(sg_size_t size, int stage, int stage2_round, double mig_speed, double timeout)
{
  sg_size_t sent   = size;
  std::string* msg = new std::string("__mig_stage");
  *msg             = *msg + std::to_string(stage) + ":" + vm_->get_cname() + "(" + src_pm_->get_cname() + "-" +
         dst_pm_->get_cname() + ")";

  double clock_sta = s4u::Engine::get_clock();

  s4u::Activity* comm = nullptr;
  try {
    if (mig_speed > 0)
      comm = mbox->put_init(msg, size)->set_rate(mig_speed)->wait_for(timeout);
    else
      comm = mbox->put_async(msg, size)->wait_for(timeout);
  } catch (const Exception&) {
    if (comm) {
      sg_size_t remaining = static_cast<sg_size_t>(comm->get_remaining());
      XBT_VERB("timeout (%lf s) in sending_migration_data, remaining %llu bytes of %llu", timeout, remaining, size);
      sent -= remaining;
    }
    delete msg;
  }

  double clock_end    = s4u::Engine::get_clock();
  double duration     = clock_end - clock_sta;
  double actual_speed = size / duration;

  if (stage == 2)
    XBT_DEBUG("mig-stage%d.%d: sent %llu duration %f actual_speed %f (target %f)", stage, stage2_round, size, duration,
              actual_speed, mig_speed);
  else
    XBT_DEBUG("mig-stage%d: sent %llu duration %f actual_speed %f (target %f)", stage, size, duration, actual_speed,
              mig_speed);

  return sent;
}

void MigrationTx::operator()()
{
  XBT_DEBUG("mig: tx_start");

  double host_speed       = vm_->get_pm()->get_speed();
  const sg_size_t ramsize = vm_->get_ramsize();
  const double dp_rate =
      host_speed ? (sg_vm_get_migration_speed(vm_) * sg_vm_get_dirty_page_intensity(vm_)) / host_speed : 1;
  const sg_size_t dp_cap = sg_vm_get_working_set_memory(vm_);
  const double mig_speed = sg_vm_get_migration_speed(vm_);
  double max_downtime    = sg_vm_get_max_downtime(vm_);

  double mig_timeout = 10000000.0;
  bool skip_stage2   = false;

  size_t remaining_size = ramsize;

  double clock_prev_send;
  double clock_post_send;
  double bandwidth;
  size_t threshold;

  /* check parameters */
  if (ramsize == 0)
    XBT_WARN("migrate a VM, but ramsize is zero");

  if (max_downtime <= 0) {
    XBT_WARN("use the default max_downtime value 30ms");
    max_downtime = 0.03;
  }

  /* Stage1: send all memory pages to the destination. */
  XBT_DEBUG("mig-stage1: remaining_size %zu", remaining_size);
  sg_vm_start_dirty_page_tracking(vm_);

  double computed_during_stage1 = 0;
  clock_prev_send               = s4u::Engine::get_clock();

  try {
    /* At stage 1, we do not need timeout. We have to send all the memory pages even though the duration of this
     * transfer exceeds the timeout value. */
    XBT_VERB("Stage 1: Gonna send %llu bytes", ramsize);
    sg_size_t sent = sendMigrationData(ramsize, 1, 0, mig_speed, -1);
    remaining_size -= sent;
    computed_during_stage1 = sg_vm_lookup_computed_flops(vm_);

    if (sent < ramsize) {
      XBT_VERB("mig-stage1: timeout, force moving to stage 3");
      skip_stage2 = true;
    } else if (sent > ramsize)
      XBT_CRITICAL("bug");

  } catch (const Exception&) {
    // hostfailure (if you want to know whether this is the SRC or the DST check directly in send_migration_data code)
    // Stop the dirty page tracking an return (there is no memory space to release)
    sg_vm_stop_dirty_page_tracking(vm_);
    return;
  }

  clock_post_send = s4u::Engine::get_clock();
  mig_timeout -= (clock_post_send - clock_prev_send);
  if (mig_timeout < 0) {
    XBT_VERB("The duration of stage 1 exceeds the timeout value, skip stage 2");
    skip_stage2 = true;
  }

  /* estimate bandwidth */
  bandwidth = ramsize / (clock_post_send - clock_prev_send);
  threshold = bandwidth * max_downtime;
  XBT_DEBUG("actual bandwidth %f (MB/s), threshold %zu", bandwidth / 1024 / 1024, threshold);

  /* Stage2: send update pages iteratively until the size of remaining states becomes smaller than threshold value. */
  if (not skip_stage2) {

    int stage2_round = 0;
    /* just after stage1, nothing has been updated. But, we have to send the data updated during stage1 */
    sg_size_t updated_size = get_updated_size(computed_during_stage1, dp_rate, dp_cap);
    remaining_size += updated_size;
    XBT_DEBUG("mig-stage2.%d: remaining_size %zu (%s threshold %zu)", stage2_round, remaining_size,
              (remaining_size < threshold) ? "<" : ">", threshold);

    /* When the remaining size is below the threshold value, move to stage 3. */
    while (threshold < remaining_size) {

      XBT_DEBUG("mig-stage 2:%d updated_size %llu computed_during_stage1 %f dp_rate %f dp_cap %llu", stage2_round,
                updated_size, computed_during_stage1, dp_rate, dp_cap);

      sg_size_t sent  = 0;
      clock_prev_send = s4u::Engine::get_clock();
      try {
        XBT_DEBUG("Stage 2, gonna send %llu", updated_size);
        sent = sendMigrationData(updated_size, 2, stage2_round, mig_speed, mig_timeout);
      } catch (const Exception&) {
        // hostfailure (if you want to know whether this is the SRC or the DST check directly in send_migration_data
        // code)
        // Stop the dirty page tracking an return (there is no memory space to release)
        sg_vm_stop_dirty_page_tracking(vm_);
        return;
      }

      remaining_size -= sent;
      double computed = sg_vm_lookup_computed_flops(vm_);

      clock_post_send = s4u::Engine::get_clock();

      if (sent == updated_size) {
        bandwidth = updated_size / (clock_post_send - clock_prev_send);
        threshold = bandwidth * max_downtime;
        XBT_DEBUG("actual bandwidth %f, threshold %zu", bandwidth / 1024 / 1024, threshold);
        stage2_round += 1;
        mig_timeout -= (clock_post_send - clock_prev_send);
        xbt_assert(mig_timeout > 0);
        XBT_DEBUG("mig-stage2.%d: remaining_size %zu (%s threshold %zu)", stage2_round, remaining_size,
                  (remaining_size < threshold) ? "<" : ">", threshold);
        updated_size = get_updated_size(computed, dp_rate, dp_cap);
        remaining_size += updated_size;
      } else {
        /* When timeout happens, we move to stage 3. The size of memory pages
         * updated before timeout must be added to the remaining size. */
        XBT_VERB("mig-stage2.%d: timeout, force moving to stage 3. sent %llu / %llu, eta %lf", stage2_round, sent,
                 updated_size, (clock_post_send - clock_prev_send));
        updated_size    = get_updated_size(computed, dp_rate, dp_cap);
        remaining_size += updated_size;
        break;
      }
    }
  }

  /* Stage3: stop the VM and copy the rest of states. */
  XBT_DEBUG("mig-stage3: remaining_size %zu", remaining_size);
  vm_->suspend();
  sg_vm_stop_dirty_page_tracking(vm_);

  try {
    XBT_DEBUG("Stage 3: Gonna send %zu bytes", remaining_size);
    sendMigrationData(remaining_size, 3, 0, mig_speed, -1);
  } catch (const Exception&) {
    // hostfailure (if you want to know whether this is the SRC or the DST check directly in send_migration_data code)
    // Stop the dirty page tracking an return (there is no memory space to release)
    vm_->resume();
    return;
  }

  // At that point the Migration is considered valid for the SRC node but remind that the DST side should relocate
  // effectively the VM on the DST node.
  XBT_DEBUG("mig: tx_done");
}
}
}

static void onVirtualMachineShutdown(simgrid::s4u::VirtualMachine const& vm)
{
  if (vm.get_impl()->is_migrating_) {
    vm.extension<simgrid::vm::VmMigrationExt>()->rx_->kill();
    vm.extension<simgrid::vm::VmMigrationExt>()->tx_->kill();
    vm.extension<simgrid::vm::VmMigrationExt>()->issuer_->kill();
    vm.get_impl()->is_migrating_ = false;
  }
}

void sg_vm_live_migration_plugin_init()
{
  sg_vm_dirty_page_tracking_init();
  simgrid::vm::VmMigrationExt::ensureVmMigrationExtInstalled();
  simgrid::s4u::VirtualMachine::on_shutdown.connect(&onVirtualMachineShutdown);
}

simgrid::s4u::VirtualMachine* sg_vm_create_migratable(simgrid::s4u::Host* pm, const char* name, int coreAmount,
                                                      int ramsize, int mig_netspeed, int dp_intensity)
{
  simgrid::vm::VmHostExt::ensureVmExtInstalled();

  /* For the moment, intensity_rate is the percentage against the migration bandwidth */

  sg_vm_t vm = new simgrid::s4u::VirtualMachine(name, pm, coreAmount, static_cast<sg_size_t>(ramsize) * 1024 * 1024);
  sg_vm_set_dirty_page_intensity(vm, dp_intensity / 100.0);
  sg_vm_set_working_set_memory(vm, vm->get_ramsize() * 0.9); // assume working set memory is 90% of ramsize
  sg_vm_set_migration_speed(vm, mig_netspeed * 1024 * 1024.0);

  XBT_DEBUG("migspeed : %f intensity mem : %d", mig_netspeed * 1024 * 1024.0, dp_intensity);

  return vm;
}

int sg_vm_is_migrating(simgrid::s4u::VirtualMachine* vm)
{
  return vm->get_impl()->is_migrating_;
}

void sg_vm_migrate(simgrid::s4u::VirtualMachine* vm, simgrid::s4u::Host* dst_pm)
{
  simgrid::s4u::Host* src_pm = vm->get_pm();

  if (not src_pm->is_on())
    throw simgrid::VmFailureException(
        XBT_THROW_POINT, simgrid::xbt::string_printf("Cannot migrate VM '%s' from host '%s', which is offline.",
                                                     vm->get_cname(), src_pm->get_cname()));
  if (not dst_pm->is_on())
    throw simgrid::VmFailureException(
        XBT_THROW_POINT, simgrid::xbt::string_printf("Cannot migrate VM '%s' to host '%s', which is offline.",
                                                     vm->get_cname(), dst_pm->get_cname()));
  if (vm->get_state() != simgrid::s4u::VirtualMachine::state::RUNNING)
    throw simgrid::VmFailureException(
        XBT_THROW_POINT,
        simgrid::xbt::string_printf("Cannot migrate VM '%s' that is not running yet.", vm->get_cname()));
  if (vm->get_impl()->is_migrating_)
    throw simgrid::VmFailureException(
        XBT_THROW_POINT,
        simgrid::xbt::string_printf("Cannot migrate VM '%s' that is already migrating.", vm->get_cname()));

  vm->get_impl()->is_migrating_ = true;
  simgrid::s4u::VirtualMachine::on_migration_start(*vm);

  std::string rx_name =
      std::string("__pr_mig_rx:") + vm->get_cname() + "(" + src_pm->get_cname() + "-" + dst_pm->get_cname() + ")";
  std::string tx_name =
      std::string("__pr_mig_tx:") + vm->get_cname() + "(" + src_pm->get_cname() + "-" + dst_pm->get_cname() + ")";

  simgrid::s4u::ActorPtr rx =
      simgrid::s4u::Actor::create(rx_name.c_str(), dst_pm, simgrid::vm::MigrationRx(vm, dst_pm));
  simgrid::s4u::ActorPtr tx =
      simgrid::s4u::Actor::create(tx_name.c_str(), src_pm, simgrid::vm::MigrationTx(vm, dst_pm));

  vm->extension_set<simgrid::vm::VmMigrationExt>(new simgrid::vm::VmMigrationExt(simgrid::s4u::Actor::self(), rx, tx));

  /* wait until the migration have finished or on error has occurred */
  XBT_DEBUG("wait for reception of the final ACK (i.e. migration has been correctly performed");
  simgrid::s4u::Mailbox* mbox_ctl = simgrid::s4u::Mailbox::by_name(
      std::string("__mbox_mig_ctl:") + vm->get_cname() + "(" + src_pm->get_cname() + "-" + dst_pm->get_cname() + ")");
  delete static_cast<std::string*>(mbox_ctl->get());
  tx->join();
  rx->join();

  vm->get_impl()->is_migrating_ = false;
  simgrid::s4u::VirtualMachine::on_migration_end(*vm);
}
