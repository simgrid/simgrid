/* Copyright (c) 2004-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_NS3_HPP_
#define NETWORK_NS3_HPP_

#include "xbt/base.h"
#include "xbt/ex.h"

#include "src/kernel/resource/NetworkModel.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"

namespace simgrid::kernel::resource {

class NetworkNS3Model : public NetworkModel {
public:
  explicit NetworkNS3Model(const std::string& name);
  ~NetworkNS3Model() override;
  StandardLinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidth,
                                routing::NetZoneImpl* englobing_zone) override;
  StandardLinkImpl* create_wifi_link(const std::string& name, const std::vector<double>& bandwidth,
                                     routing::NetZoneImpl* englobing_zone) override;
  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool streamed) override;
  double next_occurring_event(double now) override;
  bool next_occurring_event_is_idempotent() override;
  void update_actions_state(double now, double delta) override;
};

/************
 * Resource *
 ************/
class LinkNS3 : public StandardLinkImpl {
public:
  explicit LinkNS3(const std::string& name, double bandwidth, s4u::Link::SharingPolicy sharing_policy,
                   routing::NetZoneImpl* englobing_zone);
  ~LinkNS3() override;

  void apply_event(profile::Event* event, double value) override;
  void set_bandwidth(double) override { THROW_UNIMPLEMENTED; }
  void set_latency(double) override;
  void set_bandwidth_profile(profile::Profile* profile) override;
  void set_latency_profile(profile::Profile* profile) override;
};

/**********
 * Action *
 **********/
class XBT_PRIVATE NetworkNS3Action : public NetworkAction {
public:
  NetworkNS3Action(Model* model, double cost, s4u::Host* src, s4u::Host* dst);

  void suspend() override;
  void resume() override;
  std::list<StandardLinkImpl*> get_links() const override;
  void update_remains_lazy(double now) override;

  // private:
  double last_sent_ = 0;
};

} // namespace simgrid::kernel::resource

#endif /* NETWORK_NS3_HPP_ */
