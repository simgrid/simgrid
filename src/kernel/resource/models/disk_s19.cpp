/* Copyright (c) 2013-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simgrid/sg_config.hpp"
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/kernel/resource/models/disk_s19.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/simgrid/module.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_disk);
/***********
 * Options *
 ***********/
static simgrid::config::Flag<std::string> cfg_disk_solver("disk/solver",
                                                          "Set linear equations solver used by disk model", "maxmin",
                                                          &simgrid::kernel::lmm::System::validate_solver);

/*********
 * Model *
 *********/

SIMGRID_REGISTER_DISK_MODEL(S19, "Simplistic disk model.", []() {
  auto disk_model = std::make_shared<simgrid::kernel::resource::DiskS19Model>("Disk");
  auto* engine    = simgrid::kernel::EngineImpl::get_instance();
  engine->add_model(disk_model);
  engine->get_netzone_root()->set_disk_model(disk_model);
});

namespace simgrid::kernel::resource {
/*********
 * Model *
 *********/

DiskS19Model::DiskS19Model(const std::string& name) : DiskModel(name)
{
  set_maxmin_system(lmm::System::build(cfg_disk_solver.get(), true /* selective update */));
}

DiskImpl* DiskS19Model::create_disk(const std::string& name, double read_bandwidth, double write_bandwidth)
{
  return (new DiskS19(name, read_bandwidth, write_bandwidth))->set_model(this);
}

void DiskS19Model::update_actions_state(double /*now*/, double delta)
{
  for (auto it = std::begin(*get_started_action_set()); it != std::end(*get_started_action_set());) {
    auto& action = *it;
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    action.update_remains(rint(action.get_rate() * delta));
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

void DiskS19::apply_event(kernel::profile::Event* triggered, double value)
{
  /* Find out which of my iterators was triggered, and react accordingly */
  if (triggered == get_read_event()) {
    set_read_bandwidth(value);
    unref_read_event();
  } else if (triggered == get_write_event()) {
    set_write_bandwidth(value);
    unref_write_event();
  } else if (triggered == get_state_event()) {
    if (value > 0)
      turn_on();
    else
      turn_off();
    unref_state_event();
  } else {
    xbt_die("Unknown event!\n");
  }

  XBT_DEBUG("There was a resource state event, need to update actions related to the constraint (%p)",
            get_constraint());
}

/**********
 * Action *
 **********/

DiskS19Action::DiskS19Action(Model* model, double cost, bool failed)
    : DiskAction(model, cost, failed, model->get_maxmin_system()->variable_new(this, 1.0, -1.0, 3))
{
}

DiskAction* DiskS19::io_start(sg_size_t size, s4u::Io::OpType type)
{
  auto* action = new DiskS19Action(get_model(), static_cast<double>(size), not is_on());
  get_model()->get_maxmin_system()->expand(get_constraint(), action->get_variable(), 1.0);
  switch (type) {
    case s4u::Io::OpType::READ:
      get_model()->get_maxmin_system()->expand(get_read_constraint(), action->get_variable(), 1.0);
      break;
    case s4u::Io::OpType::WRITE:
      get_model()->get_maxmin_system()->expand(get_write_constraint(), action->get_variable(), 1.0);
      break;
    default:
      THROW_UNIMPLEMENTED;
  }
  if (const auto& factor_cb = get_factor_cb()) { // handling disk variability
    action->set_rate_factor(factor_cb(size, type));
  }
  return action;
}

} // namespace simgrid::kernel::resource
