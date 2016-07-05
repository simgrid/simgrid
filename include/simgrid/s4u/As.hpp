/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_AS_HPP
#define SIMGRID_S4U_AS_HPP

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <xbt/base.h>
#include <xbt/graph.h>

#include <simgrid/s4u/forward.hpp>

#include "src/surf/xml/platf_private.hpp" // FIXME: kill sg_platf_route_cbarg_t to remove that UGLY include

namespace simgrid {

namespace surf {
  class Link;
}
namespace routing {
  class AsImpl;
  class NetCard;
}
namespace s4u {

/** @brief Autonomous Systems
 *
 * An AS is a network container, in charge of routing information between elements (hosts) and to the nearby ASes.
 * In SimGrid, there is a hierarchy of ASes, with a unique root AS (that you can retrieve from the s4u::Engine).
 */
XBT_PUBLIC_CLASS As {
protected:
  friend simgrid::routing::AsImpl;

  explicit As(const char *name);
  virtual ~As();
  
public:
  /** @brief Seal your AS once you're done adding content, and before routing stuff through it */
  virtual void seal();
  char *name();
  As *father();;
  xbt_dict_t children(); // Sub AS
  xbt_dynar_t hosts();   // my content

  As *father_ = nullptr; // FIXME: hide me
public:
  /* Add content to the AS, at parsing time. It should be sealed afterward. */
  virtual int addComponent(routing::NetCard *elm); /* A host, a router or an AS, whatever */
  virtual void addRoute(sg_platf_route_cbarg_t route);
  void addBypassRoute(sg_platf_route_cbarg_t e_route);

protected:
  char *name_ = nullptr;
  xbt_dict_t children_ = xbt_dict_new_homogeneous(nullptr); // sub-ASes
  xbt_dynar_t vertices_ = xbt_dynar_new(sizeof(char*),nullptr); // our content, as known to our graph routing algorithm (maps vertexId -> vertex)

  std::map<std::pair<std::string, std::string>, std::vector<surf::Link*>*> bypassRoutes_; // srcName x dstName -> route

private:
  bool sealed_ = false; // We cannot add more content when sealed
};

}}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_AS_HPP */
