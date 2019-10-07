/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_CM02_HPP_
#define SURF_NETWORK_CM02_HPP_

#include <xbt/base.h>

#include "network_interface.hpp"
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
public:
  explicit NetworkCm02Model(lmm::System* (*make_new_sys)(bool) = &lmm::make_new_maxmin_system);
  virtual ~NetworkCm02Model() = default;
  LinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths, double latency,
                        s4u::Link::SharingPolicy policy) override;
  void update_actions_state_lazy(double now, double delta) override;
  void update_actions_state_full(double now, double delta) override;
  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;
};

/************
 * Resource *
 ************/

class NetworkCm02Link : public LinkImpl {
public:
  NetworkCm02Link(NetworkCm02Model* model, const std::string& name, double bandwidth, double latency,
                  s4u::Link::SharingPolicy policy, lmm::System* system);
  ~NetworkCm02Link() override = default;
  void apply_event(kernel::profile::Event* event, double value) override;
  void set_bandwidth(double value) override;
  void set_latency(double value) override;
};

class NetworkWifiLink : public NetworkCm02Link {
  /** @brief Hold every rates association between host and links (host name, rates id) */
  std::map<xbt::string, int> host_rates_;

  /** @brief A link can have several bandwith attach to it (mostly use by wifi model) */
  std::vector<Metric> bandwidths_;

public:
  NetworkWifiLink(NetworkCm02Model* model, const std::string& name, std::vector<double> bandwidths,
                  s4u::Link::SharingPolicy policy, lmm::System* system);

  void set_host_rate(s4u::Host* host, int rate_level);
  /** @brief Get the AP rate associated to the host (or -1 if not associated to the AP) */
  double get_host_rate(s4u::Host* host);
  s4u::Link::SharingPolicy get_sharing_policy() override;
};

/**********
 * Action *
 **********/
class NetworkCm02Action : public NetworkAction {
  friend Action* NetworkCm02Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate);

public:
  NetworkCm02Action(Model* model, double cost, bool failed) : NetworkAction(model, cost, failed){};
  virtual ~NetworkCm02Action() = default;
  void update_remains_lazy(double now) override;
};
}
}
} // namespace simgrid
#endif /* SURF_NETWORK_CM02_HPP_ */
