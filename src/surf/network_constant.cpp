/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_constant.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

/*********
 * Model *
 *********/
void surf_network_model_init_Constant()
{
  auto net_model = std::make_unique<simgrid::kernel::resource::NetworkConstantModel>();
  simgrid::kernel::EngineImpl::get_instance()->add_model(simgrid::kernel::resource::Model::Type::NETWORK,
                                                         std::move(net_model), true);
}

namespace simgrid {
namespace kernel {
namespace resource {

NetworkConstantModel::NetworkConstantModel() : NetworkModel(Model::UpdateAlgo::FULL)
{
}

LinkImpl* NetworkConstantModel::create_link(const std::string& name, const std::vector<double>& /*bandwidth*/,
                                            s4u::Link::SharingPolicy)
{
  xbt_die("Refusing to create the link %s: there is no link in the Constant network model. "
          "Please remove any link from your platform (and switch to routing='None')",
          name.c_str());
  return nullptr;
}

double NetworkConstantModel::next_occurring_event(double /*now*/)
{
  double min = -1.0;
  for (kernel::resource::Action const& action : *get_started_action_set()) {
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
        double_update(&action.latency_, delta, sg_surf_precision);
      } else {
        action.latency_ = 0.0;
      }
    }
    action.update_remains(action.get_cost() * delta / action.initial_latency_);
    action.update_max_duration(delta);

    if ((action.get_remains_no_update() <= 0) ||
        ((action.get_max_duration() != NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(Action::State::FINISHED);
    }
  }
}

Action* NetworkConstantModel::communicate(s4u::Host* src, s4u::Host* dst, double size, double)
{
  auto* action = new NetworkConstantAction(this, *src, *dst, size, sg_latency_factor);

  s4u::Link::on_communicate(*action);
  return action;
}

/**********
 * Action *
 **********/
NetworkConstantAction::NetworkConstantAction(NetworkConstantModel* model_, s4u::Host& src, s4u::Host& dst, double size,
                                             double latency)
    : NetworkAction(model_, src, dst, size, false), initial_latency_(latency)
{
  latency_ = latency;
  if (latency_ <= 0.0)
    NetworkConstantAction::set_state(Action::State::FINISHED);
}

NetworkConstantAction::~NetworkConstantAction() = default;

void NetworkConstantAction::update_remains_lazy(double /*now*/)
{
  THROW_IMPOSSIBLE;
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
