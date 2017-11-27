/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_constant.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

/*********
 * Model *
 *********/
void surf_network_model_init_Constant()
{
  xbt_assert(surf_network_model == nullptr);
  surf_network_model = new simgrid::surf::NetworkConstantModel();
  all_existing_models->push_back(surf_network_model);
}

namespace simgrid {
namespace surf {
LinkImpl* NetworkConstantModel::createLink(const std::string& name, double bw, double lat,
                                           e_surf_link_sharing_policy_t policy)
{

  xbt_die("Refusing to create the link %s: there is no link in the Constant network model. "
          "Please remove any link from your platform (and switch to routing='None')",
          name.c_str());
  return nullptr;
}

double NetworkConstantModel::nextOccuringEvent(double /*now*/)
{
  double min = -1.0;
  for (Action const& action : *getRunningActionSet()) {
    const NetworkConstantAction& net_action = static_cast<const NetworkConstantAction&>(action);
    if (net_action.latency_ > 0 && (min < 0 || net_action.latency_ < min))
      min = net_action.latency_;
  }
  return min;
}

void NetworkConstantModel::updateActionsState(double /*now*/, double delta)
{
  for (auto it = std::begin(*getRunningActionSet()); it != std::end(*getRunningActionSet());) {
    NetworkConstantAction& action = static_cast<NetworkConstantAction&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    if (action.latency_ > 0) {
      if (action.latency_ > delta) {
        double_update(&action.latency_, delta, sg_surf_precision);
      } else {
        action.latency_ = 0.0;
      }
    }
    action.updateRemains(action.getCost() * delta / action.initialLatency_);
    if (action.getMaxDuration() != NO_MAX_DURATION)
      action.updateMaxDuration(delta);

    if ((action.getRemainsNoUpdate() <= 0) ||
        ((action.getMaxDuration() != NO_MAX_DURATION) && (action.getMaxDuration() <= 0))) {
      action.finish(Action::State::done);
    }
  }
}

Action* NetworkConstantModel::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate)
{
  NetworkConstantAction* action = new NetworkConstantAction(this, size, sg_latency_factor);

  simgrid::s4u::Link::onCommunicate(action, src, dst);
  return action;
}

/**********
 * Action *
 **********/
NetworkConstantAction::NetworkConstantAction(NetworkConstantModel* model_, double size, double latency)
    : NetworkAction(model_, size, false), initialLatency_(latency)
{
  latency_ = latency;
  if (latency_ <= 0.0) {
    stateSet_ = model_->getDoneActionSet();
    stateSet_->push_back(*this);
  }
};

NetworkConstantAction::~NetworkConstantAction() = default;
}
}
