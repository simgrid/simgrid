/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

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
  explicit NetworkCm02Model(const std::string& name);
  LinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths,
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
  NetworkCm02Link(const std::string& name, double bandwidth, s4u::Link::SharingPolicy policy, lmm::System* system);
  void apply_event(kernel::profile::Event* event, double value) override;
  void set_bandwidth(double value) override;
  LinkImpl* set_latency(double value) override;
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
}
}
} // namespace simgrid
#endif /* SURF_NETWORK_CM02_HPP_ */
