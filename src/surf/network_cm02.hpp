/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_CM02_HPP_
#define SURF_NETWORK_CM02_HPP_

#include <xbt/base.h>

#include "network_interface.hpp"
#include "simgrid/kernel/resource/NetworkModelIntf.hpp"
#include "xbt/graph.h"
#include "xbt/string.hpp"

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace kernel {
namespace resource {

class XBT_PRIVATE NetworkCm02Model;
class XBT_PRIVATE NetworkCm02Action;
class XBT_PRIVATE NetworkSmpiModel;

/*********
 * Model *
 *********/

class NetworkCm02Model : public NetworkModel {
  /** @brief Get route information (2-way) */
  bool comm_get_route_info(const s4u::Host* src, const s4u::Host* dst, /* OUT */ double& latency,
                           std::vector<LinkImpl*>& route, std::vector<LinkImpl*>& back_route,
                           std::unordered_set<kernel::routing::NetZoneImpl*>& netzones) const;
  /** @brief Create network action for this communication */
  NetworkCm02Action* comm_action_create(s4u::Host* src, s4u::Host* dst, double size,
                                        const std::vector<LinkImpl*>& route, bool failed);
  /** @brief Expand link contraint considering this new communication action */
  void comm_action_expand_constraints(const s4u::Host* src, const s4u::Host* dst, const NetworkCm02Action* action,
                                      const std::vector<LinkImpl*>& route,
                                      const std::vector<LinkImpl*>& back_route) const;
  /** @brief Set communication bounds for latency and bandwidth */
  void comm_action_set_bounds(const s4u::Host* src, const s4u::Host* dst, double size, NetworkCm02Action* action,
                              const std::vector<LinkImpl*>& route,
                              const std::unordered_set<kernel::routing::NetZoneImpl*>& netzones, double rate);
  /** @brief Create maxmin variable in communication action */
  void comm_action_set_variable(NetworkCm02Action* action, const std::vector<LinkImpl*>& route,
                                const std::vector<LinkImpl*>& back_route);

public:
  explicit NetworkCm02Model(const std::string& name);
  LinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths) final;
  LinkImpl* create_wifi_link(const std::string& name, const std::vector<double>& bandwidths) override;
  void update_actions_state_lazy(double now, double delta) override;
  void update_actions_state_full(double now, double delta) override;
  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;
  void set_lat_factor_cb(const std::function<NetworkFactorCb>& cb) override;
  void set_bw_factor_cb(const std::function<NetworkFactorCb>& cb) override;

protected:
  virtual void check_lat_factor_cb();
  virtual void check_bw_factor_cb();

private:
  std::function<NetworkFactorCb> lat_factor_cb_;
  std::function<NetworkFactorCb> bw_factor_cb_;
};

/************
 * Resource *
 ************/

class NetworkCm02Link : public LinkImpl {
public:
  NetworkCm02Link(const std::string& name, double bandwidth, lmm::System* system);
  void apply_event(kernel::profile::Event* event, double value) override;
  void set_bandwidth(double value) override;
  void set_latency(double value) override;
};

/**********
 * Action *
 **********/
class NetworkCm02Action : public NetworkAction {
  friend Action* NetworkCm02Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate);

public:
  using NetworkAction::NetworkAction;
  void update_remains_lazy(double now) override;
};
} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* SURF_NETWORK_CM02_HPP_ */
