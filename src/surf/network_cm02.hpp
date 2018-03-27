/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_CM02_HPP_
#define SURF_NETWORK_CM02_HPP_

#include <xbt/base.h>

#include "network_interface.hpp"
#include "xbt/graph.h"


/***********
 * Classes *
 ***********/

namespace simgrid {
  namespace surf {

    class XBT_PRIVATE NetworkCm02Model;
    class XBT_PRIVATE NetworkCm02Action;
    class XBT_PRIVATE NetworkSmpiModel;

  }
}

/*********
 * Model *
 *********/

namespace simgrid {
namespace surf {

class NetworkCm02Model : public NetworkModel {
public:
  explicit NetworkCm02Model(kernel::lmm::System* (*make_new_sys)(bool) = &simgrid::kernel::lmm::make_new_maxmin_system);
  virtual ~NetworkCm02Model() = default;
  LinkImpl* createLink(const std::string& name, double bandwidth, double latency,
                       e_surf_link_sharing_policy_t policy) override;
  void update_actions_state_lazy(double now, double delta) override;
  void update_actions_state_full(double now, double delta) override;
  kernel::resource::Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;
};

/************
 * Resource *
 ************/

class NetworkCm02Link : public LinkImpl {
public:
  NetworkCm02Link(NetworkCm02Model* model, const std::string& name, double bandwidth, double latency,
                  e_surf_link_sharing_policy_t policy, kernel::lmm::System* system);
  virtual ~NetworkCm02Link() = default;
  void apply_event(tmgr_trace_event_t event, double value) override;
  void setBandwidth(double value) override;
  void setLatency(double value) override;
};

/**********
 * Action *
 **********/
class NetworkCm02Action : public NetworkAction {
  friend Action* NetworkCm02Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate);
  friend NetworkSmpiModel;

public:
  NetworkCm02Action(kernel::resource::Model* model, double cost, bool failed) : NetworkAction(model, cost, failed){};
  virtual ~NetworkCm02Action() = default;
  void update_remains_lazy(double now) override;
};
}
}

#endif /* SURF_NETWORK_CM02_HPP_ */
