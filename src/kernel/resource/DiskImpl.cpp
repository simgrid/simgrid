/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "DiskImpl.hpp"

#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/kernel/resource/profile/Profile.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(res_disk, ker_resource, "Disk resources, that fuel I/O activities");

namespace simgrid {
namespace kernel {
namespace resource {

xbt::signal<void(DiskAction const&, Action::State, Action::State)> DiskAction::on_state_change;

/*********
 * Model *
 *********/

DiskModel::DiskModel(const std::string& name) : Model(name)
{
  set_maxmin_system(new lmm::System(true /* selective update */));
}

/************
 * Resource *
 ************/
DiskImpl::DiskImpl(const std::string& name, double read_bandwidth, double write_bandwidth)
    : Resource_T(name), piface_(this)
{
  read_bw_.peak   = read_bandwidth;
  read_bw_.scale  = 1.0;
  write_bw_.peak  = write_bandwidth;
  write_bw_.scale = 1.0;
}

DiskImpl* DiskImpl::set_host(s4u::Host* host)
{
  xbt_assert(host, "Cannot set host, none given");
  host_ = host;
  return this;
}

DiskImpl* DiskImpl::set_read_constraint(lmm::Constraint* constraint_read)
{
  constraint_read_ = constraint_read;
  return this;
}

DiskImpl* DiskImpl::set_write_constraint(lmm::Constraint* constraint_write)
{
  constraint_write_ = constraint_write;
  return this;
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a Disk, call d->destroy() instead.
 */
void DiskImpl::destroy()
{
  s4u::Disk::on_destruction(piface_);
  delete this;
}

void DiskImpl::turn_on()
{
  if (not is_on()) {
    Resource::turn_on();
    s4u::Disk::on_state_change(piface_);
  }
}
void DiskImpl::turn_off()
{
  if (is_on()) {
    Resource::turn_off();
    s4u::Disk::on_state_change(piface_);
  }
}

DiskImpl* DiskImpl::set_read_bandwidth_profile(profile::Profile* profile)
{
  if (profile) {
    xbt_assert(read_bw_.event == nullptr, "Cannot set a second read bandwidth profile to Disk %s", get_cname());
    read_bw_.event = profile->schedule(&profile::future_evt_set, this);
  }
  return this;
}

DiskImpl* DiskImpl::set_write_bandwidth_profile(profile::Profile* profile)
{
  if (profile) {
    xbt_assert(write_bw_.event == nullptr, "Cannot set a second read bandwidth profile to Disk %s", get_cname());
    write_bw_.event = profile->schedule(&profile::future_evt_set, this);
  }
  return this;
}

void DiskImpl::seal()
{
  if (is_sealed())
    return;

  xbt_assert(this->get_model(), "Cannot seal Disk (%s) without setting the model first", get_cname());
  lmm::System* maxmin_system = get_model()->get_maxmin_system();
  /* set readwrite constraint if not configured by user */
  if (readwrite_bw_ == -1) {
    readwrite_bw_ = std::max(read_bw_.peak, write_bw_.peak);
  }
  this->set_read_constraint(maxmin_system->constraint_new(this, read_bw_.peak * read_bw_.scale))
      ->set_write_constraint(maxmin_system->constraint_new(this, write_bw_.peak * write_bw_.scale))
      ->set_constraint(maxmin_system->constraint_new(this, readwrite_bw_));
  apply_sharing_policy_cfg();
  XBT_DEBUG("Create resource with read_bw '%f' write_bw '%f'", read_bw_.peak, write_bw_.peak);
  Resource::seal();
  turn_on();
}

constexpr kernel::lmm::Constraint::SharingPolicy to_maxmin_policy(s4u::Disk::SharingPolicy policy)
{
  kernel::lmm::Constraint::SharingPolicy lmm_policy = kernel::lmm::Constraint::SharingPolicy::SHARED;
  if (policy == s4u::Disk::SharingPolicy::NONLINEAR)
    lmm_policy = kernel::lmm::Constraint::SharingPolicy::NONLINEAR;
  return lmm_policy;
}

void DiskImpl::set_sharing_policy(s4u::Disk::Operation op, s4u::Disk::SharingPolicy policy,
                                  const s4u::NonLinearResourceCb& cb)
{
  sharing_policy_[op]    = policy;
  sharing_policy_cb_[op] = cb;
  apply_sharing_policy_cfg();
}

s4u::Disk::SharingPolicy DiskImpl::get_sharing_policy(s4u::Disk::Operation op) const
{
  return sharing_policy_.at(op);
}

void DiskImpl::apply_sharing_policy_cfg()
{
  if (get_constraint())
    get_constraint()->set_sharing_policy(to_maxmin_policy(sharing_policy_[s4u::Disk::Operation::READWRITE]),
                                         sharing_policy_cb_[s4u::Disk::Operation::READWRITE]);
  if (constraint_read_)
    constraint_read_->set_sharing_policy(to_maxmin_policy(sharing_policy_[s4u::Disk::Operation::READ]),
                                         sharing_policy_cb_[s4u::Disk::Operation::READ]);
  if (constraint_write_)
    constraint_write_->set_sharing_policy(to_maxmin_policy(sharing_policy_[s4u::Disk::Operation::WRITE]),
                                          sharing_policy_cb_[s4u::Disk::Operation::WRITE]);
}

void DiskImpl::set_factor_cb(const std::function<s4u::Disk::IoFactorCb>& cb)
{
  xbt_assert(not is_sealed(), "Cannot set I/O factor callback in an already sealed disk(%s)", get_cname());
  factor_cb_ = cb;
}

/**********
 * Action *
 **********/
void DiskAction::set_state(Action::State new_state)
{
  Action::State previous_state = get_state();
  if (new_state != previous_state) { // Trigger only if the state changed
    Action::set_state(new_state);
    on_state_change(*this, previous_state, new_state);
  }
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
