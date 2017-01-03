/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef KERNEL_ROUTING_NETCARD_HPP_
#define KERNEL_ROUTING_NETCARD_HPP_

#include <xbt/Extendable.hpp>
#include <xbt/base.h>
#include <xbt/signal.hpp>

#include "src/kernel/routing/NetZoneImpl.hpp"
/*
#include <float.h>
#include <vector>
*/
namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief Network cards are the vertices in the graph representing the network, used to compute paths between nodes.
 *
 * @details This represents a position in the network. One can send information between two netcards
 */
class NetCard : public simgrid::xbt::Extendable<NetCard> {

public:
  enum class Type { Host, Router, NetZone };

  NetCard(std::string name, NetCard::Type componentType, NetZoneImpl* netzone_p);
  ~NetCard() = default;

  // Our rank in the vertices_ array of the netzone that contains us.
  unsigned int id() { return id_; }
  std::string name() { return name_; }
  const char* cname() { return name_.c_str(); }
  /** @brief the NetZone in which this netcard is included */
  NetZoneImpl* netzone() { return netzone_; }

  bool isNetZone() { return componentType_ == Type::NetZone; }
  bool isHost() { return componentType_ == Type::Host; }
  bool isRouter() { return componentType_ == Type::Router; }

  static simgrid::xbt::signal<void(NetCard*)> onCreation;

  bool operator<(const NetCard &rhs) const { return name_ < rhs.name_; }

private:
  unsigned int id_;
  std::string name_;
  NetCard::Type componentType_;
  NetZoneImpl* netzone_;
};
}
}
}

XBT_PUBLIC(sg_netcard_t) sg_netcard_by_name_or_null(const char* name);

#endif /* KERNEL_ROUTING_NETCARD_HPP_ */
