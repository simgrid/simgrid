/* Copyright (c) 2013-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Engine.hpp>

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/resource/models/network_constant.hpp"
#include "src/simgrid/math_utils.h"
#include "src/simgrid/module.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

/*********
 * Model *
 *********/
SIMGRID_REGISTER_NETWORK_MODEL(
    Constant,
    "Simplistic network model where all communication take a constant time (one second). This model "
    "provides the lowest realism, but is (marginally) faster. It is mostly useful when studying theoretical "
    "distributed algorithms where the network is usually abstracted away.",
    []() {
      auto net_model = std::make_shared<simgrid::kernel::resource::NetworkConstantModel>("Network_Constant");
      auto* engine   = simgrid::kernel::EngineImpl::get_instance();
      engine->add_model(net_model);
      engine->get_netzone_root()->set_network_model(net_model);
    });

namespace simgrid::kernel::resource {

StandardLinkImpl* NetworkConstantModel::create_link(const std::string& name, const std::vector<double>& /*bandwidth*/)
{
  xbt_die("Refusing to create the link %s: there is no link in the Constant network model. "
          "Please remove any link from your platform (and switch to routing='None')",
          name.c_str());
  return nullptr;
}

StandardLinkImpl* NetworkConstantModel::create_wifi_link(const std::string& name, const std::vector<double>& bandwidths)
{
  return create_link(name, bandwidths);
}

double NetworkConstantModel::next_occurring_event(double /*now*/)
{
  double min = -1.0;
  for (Action const& action : *get_started_action_set()) {
    const auto& net_action = static_cast<const NetworkConstantAction&>(action);
    if (net_action.latency_ > 0 && (min < 0 || net_action.latency_ < min))
      min = net_action.latency_;
  }
  return min;
}

void NetworkConstantModel::update_actions_state(double /*now*/, double delta)
{
  for (auto it = std::begin(*get_started_action_set()); it != std::end(*get_started_action_set());) {
    auto& action = static_cast<NetworkConstantAction&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    if (action.latency_ > 0) {
      if (action.latency_ > delta) {
        double_update(&action.latency_, delta, sg_precision_timing);
      } else {
        action.latency_ = 0.0;
      }
    }
    action.update_remains(action.get_cost() * delta / get_latency_factor());
    action.update_max_duration(delta);

    if ((action.get_remains_no_update() <= 0) ||
        ((action.get_max_duration() != NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(Action::State::FINISHED);
    }
  }
}

Action* NetworkConstantModel::communicate(s4u::Host* src, s4u::Host* dst, double size, double /*rate*/,
                                          bool /*streamed*/)
{
  return (new NetworkConstantAction(this, *src, *dst, size));
}

/**********
 * Action *
 **********/
NetworkConstantAction::NetworkConstantAction(NetworkConstantModel* model_, s4u::Host& src, s4u::Host& dst, double size)
    : NetworkAction(model_, src, dst, size, false)
{
  latency_ = model_->get_latency_factor();
  if (latency_ <= 0.0)
    set_state(Action::State::FINISHED);
}

void NetworkConstantAction::update_remains_lazy(double /*now*/)
{
  THROW_IMPOSSIBLE;
}

} // namespace simgrid::kernel::resource
