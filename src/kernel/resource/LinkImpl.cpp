/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Engine.hpp>

#include "simgrid/sg_config.hpp"
#include "src/kernel/resource/LinkImpl.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/surf/surf_interface.hpp"

#include <numeric>

#ifndef NETWORK_INTERFACE_CPP_
#define NETWORK_INTERFACE_CPP_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(res_network, ker_resource, "Network resources, that fuel communications");

/*********
 * Model *
 *********/

namespace simgrid {
namespace kernel {
namespace resource {

/** @brief Command-line option 'network/TCP-gamma' -- see @ref options_model_network_gamma */
config::Flag<double> NetworkModel::cfg_tcp_gamma(
    "network/TCP-gamma",
    "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; "
    "Use the last given value, which is the max window size)",
    4194304.0);

/** @brief Command-line option 'network/crosstraffic' -- see @ref options_model_network_crosstraffic */
config::Flag<bool> NetworkModel::cfg_crosstraffic(
    "network/crosstraffic",
    "Activate the interferences between uploads and downloads for fluid max-min models (LV08, CM02)", "yes");

NetworkModel::~NetworkModel() = default;

double NetworkModel::next_occurring_event_full(double now)
{
  double minRes = Model::next_occurring_event_full(now);

  for (Action const& action : *get_started_action_set()) {
    const auto& net_action = static_cast<const NetworkAction&>(action);
    if (net_action.latency_ > 0)
      minRes = (minRes < 0) ? net_action.latency_ : std::min(minRes, net_action.latency_);
  }

  XBT_DEBUG("Min of share resources %f", minRes);

  return minRes;
}

/**********
 * Action *
 **********/

void NetworkAction::set_state(Action::State state)
{
  Action::State previous = get_state();
  if (previous != state) { // Trigger only if the state changed
    Action::set_state(state);
    s4u::Link::on_communication_state_change(*this, previous);
  }
}

/** @brief returns a list of all Links that this action is using */
std::list<StandardLinkImpl*> NetworkAction::get_links() const
{
  std::list<StandardLinkImpl*> retlist;
  int llen = get_variable()->get_number_of_constraint();

  for (int i = 0; i < llen; i++) {
    /* Beware of composite actions: ptasks put links and cpus together */
    if (auto* link = dynamic_cast<StandardLinkImpl*>(get_variable()->get_constraint(i)->get_id()))
      retlist.push_back(link);
  }

  return retlist;
}

static void add_latency(const std::vector<StandardLinkImpl*>& links, double* latency)
{
  if (latency)
    *latency = std::accumulate(begin(links), end(links), *latency,
                               [](double lat, const auto* link) { return lat + link->get_latency(); });
}

void add_link_latency(std::vector<StandardLinkImpl*>& result, StandardLinkImpl* link, double* latency)
{
  result.push_back(link);
  if (latency)
    *latency += link->get_latency();
}

void add_link_latency(std::vector<StandardLinkImpl*>& result, const std::vector<StandardLinkImpl*>& links,
                      double* latency)
{
  result.insert(result.end(), begin(links), end(links));
  add_latency(links, latency);
}

void insert_link_latency(std::vector<StandardLinkImpl*>& result, const std::vector<StandardLinkImpl*>& links,
                         double* latency)
{
  result.insert(result.begin(), rbegin(links), rend(links));
  add_latency(links, latency);
}

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* NETWORK_INTERFACE_CPP_ */
