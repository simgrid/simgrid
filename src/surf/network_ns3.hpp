/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_NS3_HPP_
#define NETWORK_NS3_HPP_

#include "xbt/base.h"

#include "network_interface.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

class NetworkNS3Model : public NetworkModel {
public:
  explicit NetworkNS3Model(const std::string& name);
  LinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidth,
                        s4u::Link::SharingPolicy policy) override;
  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;
  double next_occurring_event(double now) override;
  bool next_occurring_event_is_idempotent() override { return false; }
  void update_actions_state(double now, double delta) override;
};

/************
 * Resource *
 ************/
class LinkNS3 : public LinkImpl {
public:
  explicit LinkNS3(const std::string& name, double bandwidth, s4u::Link::SharingPolicy policy);
  ~LinkNS3() override;
  s4u::Link::SharingPolicy sharing_policy_;

  void apply_event(profile::Event* event, double value) override;
  void set_bandwidth(double) override { THROW_UNIMPLEMENTED; }
  LinkImpl* set_latency(double) override;
  LinkImpl* set_bandwidth_profile(profile::Profile* profile) override;
  LinkImpl* set_latency_profile(profile::Profile* profile) override;
  s4u::Link::SharingPolicy get_sharing_policy() const override { return sharing_policy_; }
};

/**********
 * Action *
 **********/
class XBT_PRIVATE NetworkNS3Action : public NetworkAction {
public:
  NetworkNS3Action(Model* model, double cost, s4u::Host* src, s4u::Host* dst);

  void suspend() override;
  void resume() override;
  std::list<LinkImpl*> get_links() const override;
  void update_remains_lazy(double now) override;

  // private:
  double last_sent_ = 0;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* NETWORK_NS3_HPP_ */
