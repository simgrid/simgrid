/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "StorageImpl.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "surf_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf, "Logging specific to the SURF storage module");

simgrid::kernel::resource::StorageModel* surf_storage_model = nullptr;

namespace simgrid {
namespace kernel {
namespace resource {

/*********
 * Model *
 *********/

StorageModel::StorageModel() : Model(Model::UpdateAlgo::FULL)
{
  set_maxmin_system(new simgrid::kernel::lmm::System(true /* selective update */));
}

StorageModel::~StorageModel()
{
  surf_storage_model = nullptr;
}

/************
 * Resource *
 ************/

StorageImpl::StorageImpl(kernel::resource::Model* model, const std::string& name, kernel::lmm::System* maxminSystem,
                         double bread, double bwrite, const std::string& type_id, const std::string& content_name,
                         sg_size_t size, const std::string& attach)
    : Resource(model, name, maxminSystem->constraint_new(this, std::max(bread, bwrite)))
    , piface_(name, this)
    , typeId_(type_id)
    , attach_(attach)
    , content_name_(content_name)
    , size_(size)
{
  StorageImpl::turn_on();
  XBT_DEBUG("Create resource with Bread '%f' Bwrite '%f' and Size '%llu'", bread, bwrite, size);
  constraint_read_  = maxminSystem->constraint_new(this, bread);
  constraint_write_ = maxminSystem->constraint_new(this, bwrite);
}

StorageImpl::~StorageImpl()
{
  xbt_assert(currently_destroying_, "Don't delete Storages directly. Call destroy() instead.");
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a Storage, call s->destroy() instead.
 */
void StorageImpl::destroy()
{
  if (not currently_destroying_) {
    currently_destroying_ = true;
    s4u::Storage::on_destruction(this->piface_);
    delete this;
  }
}

bool StorageImpl::is_used() const
{
  THROW_UNIMPLEMENTED;
}

void StorageImpl::apply_event(kernel::profile::Event* /*event*/, double /*value*/)
{
  THROW_UNIMPLEMENTED;
}

void StorageImpl::turn_on()
{
  if (not is_on()) {
    Resource::turn_on();
    s4u::Storage::on_state_change(this->piface_);
  }
}
void StorageImpl::turn_off()
{
  if (is_on()) {
    Resource::turn_off();
    s4u::Storage::on_state_change(this->piface_);
  }
}
xbt::signal<void(StorageAction const&, Action::State, Action::State)> StorageAction::on_state_change;

/**********
 * Action *
 **********/
void StorageAction::set_state(Action::State state)
{
  Action::State old = get_state();
  Action::set_state(state);
  on_state_change(*this, old, state);
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
