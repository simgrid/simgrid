/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODEL_NETWORK_CM02_HPP_
#define SIMGRID_MODEL_NETWORK_CM02_HPP_

#include "src/kernel/resource/NetworkModel.hpp"
#include "src/kernel/resource/Resource.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include "xbt/base.h"

/***********
 * Classes *
 ***********/

namespace simgrid::kernel::resource {

class XBT_PRIVATE NetworkCm02Model;
class XBT_PRIVATE NetworkCm02Action;
class XBT_PRIVATE NetworkSmpiModel;

/*********
 * Model *
 *********/

class NetworkCm02Model : public NetworkModel {
  /** @brief Get route information (2-way) */
  bool comm_get_route_info(const s4u::Host* src, const s4u::Host* dst, /* OUT */ double& latency,
                           std::vector<StandardLinkImpl*>& route, std::vector<StandardLinkImpl*>& back_route,
                           std::unordered_set<kernel::routing::NetZoneImpl*>& netzones) const;
  /** @brief Create network action for this communication */
  NetworkCm02Action* comm_action_create(s4u::Host* src, s4u::Host* dst, double size,
                                        const std::vector<StandardLinkImpl*>& route, bool failed);
  /** @brief Expand link contraint considering this new communication action */
  void comm_action_expand_constraints(const s4u::Host* src, const s4u::Host* dst, const NetworkCm02Action* action,
                                      const std::vector<StandardLinkImpl*>& route,
                                      const std::vector<StandardLinkImpl*>& back_route) const;
  /** @brief Set communication bounds for latency and bandwidth */
  void comm_action_set_bounds(const s4u::Host* src, const s4u::Host* dst, double size, NetworkCm02Action* action,
                              const std::vector<StandardLinkImpl*>& route,
                              const std::unordered_set<kernel::routing::NetZoneImpl*>& netzones, double rate) const;
  /** @brief Create maxmin variable in communication action */
  void comm_action_set_variable(NetworkCm02Action* action, const std::vector<StandardLinkImpl*>& route,
                                const std::vector<StandardLinkImpl*>& back_route, bool streamed);

public:
  explicit NetworkCm02Model(const std::string& name);
  StandardLinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths,
                                routing::NetZoneImpl* englobing_zone) final;
  StandardLinkImpl* create_wifi_link(const std::string& name, const std::vector<double>& bandwidths,
                                     routing::NetZoneImpl* englobing_zone) override;
  void update_actions_state_lazy(double now, double delta) override;
  void update_actions_state_full(double now, double delta) override;
  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool streamed) override;
};

/************
 * Resource *
 ************/

class NetworkCm02Link : public StandardLinkImpl {
public:
  NetworkCm02Link(const std::string& name, double bandwidth, lmm::System* system,
                  s4u::Link::SharingPolicy sharing_policy, routing::NetZoneImpl* englobing_zone);
  void apply_event(kernel::profile::Event* event, double value) override;
  void set_bandwidth(double value) override;
  void set_latency(double value) override;
};

/**********
 * Action *
 **********/
class NetworkCm02Action : public NetworkAction {
  friend Action* NetworkCm02Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool streamed);

public:
  using NetworkAction::NetworkAction;
  void update_remains_lazy(double now) override;
};
} // namespace simgrid::kernel::resource
#endif /* SIMGRID_MODEL_NETWORK_CM02_HPP_ */
