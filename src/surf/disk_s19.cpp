/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "disk_s19.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/xml/platf.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_disk);

/*********
 * Model *
 *********/

void surf_disk_model_init_default()
{
  surf_disk_model = new simgrid::kernel::resource::DiskS19Model();
}

namespace simgrid {
namespace kernel {
namespace resource {

DiskS19Model::DiskS19Model()
{
  all_existing_models.push_back(this);
  models_by_type[simgrid::kernel::resource::Model::Type::DISK].push_back(this);
}

DiskImpl* DiskS19Model::create_disk(const std::string& name, double read_bandwidth, double write_bandwidth)
{
  return (new DiskS19(name, read_bandwidth, write_bandwidth))->set_model(this);
}

double DiskS19Model::next_occurring_event(double now)
{
  return DiskModel::next_occurring_event_full(now);
}

void DiskS19Model::update_actions_state(double /*now*/, double delta)
{
  for (auto it = std::begin(*get_started_action_set()); it != std::end(*get_started_action_set());) {
    auto& action = *it;
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    action.update_remains(rint(action.get_variable()->get_value() * delta));
    action.update_max_duration(delta);

    if (((action.get_remains_no_update() <= 0) && (action.get_variable()->get_penalty() > 0)) ||
        ((action.get_max_duration() != NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(Action::State::FINISHED);
    }
  }
}

/************
 * Resource *
 ************/
DiskAction* DiskS19::io_start(sg_size_t size, s4u::Io::OpType type)
{
  return new DiskS19Action(get_model(), static_cast<double>(size), not is_on(), this, type);
}

DiskAction* DiskS19::read(sg_size_t size)
{
  return new DiskS19Action(get_model(), static_cast<double>(size), not is_on(), this, s4u::Io::OpType::READ);
}

DiskAction* DiskS19::write(sg_size_t size)
{
  return new DiskS19Action(get_model(), static_cast<double>(size), not is_on(), this, s4u::Io::OpType::WRITE);
}

/**********
 * Action *
 **********/

DiskS19Action::DiskS19Action(Model* model, double cost, bool failed, DiskImpl* disk, s4u::Io::OpType type)
    : DiskAction(model, cost, failed, model->get_maxmin_system()->variable_new(this, 1.0, -1.0, 3), disk, type)
{
  XBT_IN("(%s,%g", disk->get_cname(), cost);

  // Must be less than the max bandwidth for all actions
  model->get_maxmin_system()->expand(disk->get_constraint(), get_variable(), 1.0);
  switch (type) {
    case s4u::Io::OpType::READ:
      model->get_maxmin_system()->expand(disk->get_read_constraint(), get_variable(), 1.0);
      break;
    case s4u::Io::OpType::WRITE:
      model->get_maxmin_system()->expand(disk->get_write_constraint(), get_variable(), 1.0);
      break;
    default:
      THROW_UNIMPLEMENTED;
  }
  XBT_OUT();
}

void DiskS19Action::cancel()
{
  set_state(Action::State::FAILED);
}

void DiskS19Action::suspend()
{
  XBT_IN("(%p)", this);
  if (is_running()) {
    get_model()->get_maxmin_system()->update_variable_penalty(get_variable(), 0.0);
    set_suspend_state(Action::SuspendStates::SUSPENDED);
  }
  XBT_OUT();
}

void DiskS19Action::resume()
{
  THROW_UNIMPLEMENTED;
}

void DiskS19Action::set_max_duration(double /*duration*/)
{
  THROW_UNIMPLEMENTED;
}

void DiskS19Action::set_sharing_penalty(double)
{
  THROW_UNIMPLEMENTED;
}
void DiskS19Action::update_remains_lazy(double /*now*/)
{
  THROW_IMPOSSIBLE;
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
