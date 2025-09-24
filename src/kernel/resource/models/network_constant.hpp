/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_CONSTANT_HPP_
#define NETWORK_CONSTANT_HPP_

#include "src/kernel/resource/NetworkModel.hpp"

namespace simgrid::kernel::resource {

class NetworkConstantModel : public NetworkModel {
public:
  using NetworkModel::NetworkModel;
  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool streamed) override;
  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;

  StandardLinkImpl* create_link(const std::string& name, const std::vector<double>& bws,
                                routing::NetZoneImpl* englobing_zone) override;
  StandardLinkImpl* create_wifi_link(const std::string& name, const std::vector<double>& bws,
                                     routing::NetZoneImpl* englobing_zone) override;
};

class NetworkConstantAction final : public NetworkAction {
public:
  NetworkConstantAction(NetworkConstantModel* model_, s4u::Host& src, s4u::Host& dst, double size);
  XBT_ATTRIB_NORETURN void update_remains_lazy(double now) override;
};

} // namespace simgrid::kernel::resource

#endif /* NETWORK_CONSTANT_HPP_ */
