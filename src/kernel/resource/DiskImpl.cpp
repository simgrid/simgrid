/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "DiskImpl.hpp"

#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/lmm/maxmin.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(disk_kernel, surf, "Logging specific to the disk kernel resource");

simgrid::kernel::resource::DiskModel* surf_disk_model = nullptr;

namespace simgrid {
namespace kernel {
namespace resource {

/*********
 * Model *
 *********/

DiskModel::DiskModel() : Model(Model::UpdateAlgo::FULL)
{
  set_maxmin_system(new simgrid::kernel::lmm::System(true /* selective update */));
}

DiskModel::~DiskModel()
{
  surf_disk_model = nullptr;
}

/************
 * Resource *
 ************/

DiskImpl::DiskImpl(kernel::resource::Model* model, const std::string& name, kernel::lmm::System* maxminSystem,
                   double read_bw, double write_bw)
    : Resource(model, name, maxminSystem->constraint_new(this, std::max(read_bw, write_bw))), piface_(name, this)
{
  DiskImpl::turn_on();
  XBT_DEBUG("Create resource with read_bw '%f' write_bw '%f'", read_bw, write_bw);
  constraint_read_  = maxminSystem->constraint_new(this, read_bw);
  constraint_write_ = maxminSystem->constraint_new(this, write_bw);
}

DiskImpl::~DiskImpl()
{
  xbt_assert(currently_destroying_, "Don't delete Disks directly. Call destroy() instead.");
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a Disk, call d->destroy() instead.
 */
void DiskImpl::destroy()
{
  if (not currently_destroying_) {
    currently_destroying_ = true;
    s4u::Disk::on_destruction(this->piface_);
    delete this;
  }
}

bool DiskImpl::is_used()
{
  THROW_UNIMPLEMENTED;
}

void DiskImpl::apply_event(kernel::profile::Event* /*event*/, double /*value*/)
{
  THROW_UNIMPLEMENTED;
}

void DiskImpl::turn_on()
{
  if (not is_on()) {
    Resource::turn_on();
    s4u::Disk::on_state_change(this->piface_);
  }
}
void DiskImpl::turn_off()
{
  if (is_on()) {
    Resource::turn_off();
    s4u::Disk::on_state_change(this->piface_);
  }
}

xbt::signal<void(DiskAction const&, Action::State, Action::State)> DiskAction::on_state_change;

/**********
 * Action *
 **********/
void DiskAction::set_state(Action::State state)
{
  Action::State old = get_state();
  Action::set_state(state);
  on_state_change(*this, old, state);
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
