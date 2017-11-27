/* Copyright (c) 2004-2017. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_NS3_HPP_
#define NETWORK_NS3_HPP_

#include "xbt/base.h"

#include "network_interface.hpp"
#include "src/surf/ns3/ns3_interface.hpp"

namespace simgrid {
namespace surf {

class NetworkNS3Model : public NetworkModel {
public:
  NetworkNS3Model();
  ~NetworkNS3Model();
  LinkImpl* createLink(const std::string& name, double bandwidth, double latency,
                       e_surf_link_sharing_policy_t policy) override;
  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;
  double nextOccuringEvent(double now) override;
  bool nextOccuringEventIsIdempotent() override { return false; }
  void updateActionsState(double now, double delta) override;
};

/************
 * Resource *
 ************/
class LinkNS3 : public LinkImpl {
public:
  explicit LinkNS3(NetworkNS3Model* model, const std::string& name, double bandwidth, double latency);
  ~LinkNS3();

  void apply_event(tmgr_trace_event_t event, double value) override;
  void setBandwidth(double value) override { THROW_UNIMPLEMENTED; }
  void setLatency(double value) override { THROW_UNIMPLEMENTED; }
  void setBandwidthTrace(tmgr_trace_t trace) override;
  void setLatencyTrace(tmgr_trace_t trace) override;
};

/**********
 * Action *
 **********/
class XBT_PRIVATE NetworkNS3Action : public NetworkAction {
public:
  NetworkNS3Action(Model* model, double cost, s4u::Host* src, s4u::Host* dst);

  bool isSuspended() override;
  int unref() override;
  void suspend() override;
  void resume() override;
  std::list<LinkImpl*> links() override;

  // private:
  double lastSent_ = 0;
  s4u::Host* src_;
  s4u::Host* dst_;
};

}
}

#endif /* NETWORK_NS3_HPP_ */
