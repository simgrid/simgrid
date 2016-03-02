/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_AS_HPP
#define SIMGRID_S4U_AS_HPP

#include "xbt/base.h"
#include "xbt/graph.h"

#include "simgrid/s4u/forward.hpp"
#include <vector>
#include <map>

#include "src/surf/xml/platf_private.hpp" // FIXME: kill sg_platf_route_cbarg_t to remove that UGLY include

namespace simgrid {

namespace surf {
  class Link;
  class NetCard;
  class RoutingPlatf; // FIXME: KILLME
}
namespace s4u {

/** @brief Autonomous Systems
 *
 * An AS is a network container, in charge of routing information between elements (hosts) and to the nearby ASes.
 * In SimGrid, there is a hierarchy of ASes, with a unique root AS (that you can retrieve from the s4u::Engine).
 */
XBT_PUBLIC_CLASS As {
protected:
  As(const char *name);
  virtual ~As();
  
public:
  /** @brief Seal your AS once you're done adding content, and before routing stuff through it */
  virtual void Seal();
  char *name();
  As *father();;
  xbt_dict_t children(); // Sub AS
  xbt_dynar_t hosts();   // my content


public:
  /**
   * @brief Probe the routing path between two points
   *
   * The networking model uses this function when creating a communication
   * to retrieve both the list of links that the create communication will use,
   * and the summed latency that these links represent.
   *
   * The network could recompute the latency by itself from the list, but it would
   * require an additional link set traversal. This operation being on the critical
   * path of SimGrid, the routing computes the latency in behalf of the network.
   *
   * Things are rather complex here because we have to find the path from ASes to ASes, and within each.
   * In addition, the different ASes may use differing routing models.
   * Some ASes may be routed in full, others may have only some connection information and use a shortest path on top of that, and so on.
   * Some ASes may even not have any predefined links and use only coordinate informations to compute the latency.
   *
   * So, the path is constructed recursively, with each traversed AS adding its information to the set.
   * The algorithm for that is explained in http://hal.inria.fr/hal-00650233/
   *
   * @param src Initial point of the routing path
   * @param dst Final point of the routing path
   * @param into Container into which the traversed links should be pushed
   * @param latency Accumulator in which the latencies should be added (caller must set it to 0)
   */
  virtual void getRouteAndLatency(surf::NetCard *src, surf::NetCard *dst, sg_platf_route_cbarg_t into, double *latency)=0;
  /** @brief retrieves the list of all routes of size 1 (of type src x dst x Link) */
  virtual xbt_dynar_t getOneLinkRoutes();

  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)=0;

  /* Add content to the AS, at parsing time. It should be sealed afterward. */
  virtual int addComponent(surf::NetCard *elm); /* A host, a router or an AS, whatever */
  virtual void addRoute(sg_platf_route_cbarg_t route);
  void addBypassRoute(sg_platf_route_cbarg_t e_route);


  enum RoutingKind {
    ROUTING_NULL = 0,   /**< Undefined type                                   */
    ROUTING_BASE,       /**< Base case: use simple link lists for routing     */
    ROUTING_RECURSIVE   /**< Recursive case: also return gateway informations */
  };
  /* FIXME: protect the following fields once the construction madness is sorted out */
  RoutingKind hierarchy_ = ROUTING_NULL;
  xbt_dynar_t upDownLinks = xbt_dynar_new(sizeof(s_surf_parsing_link_up_down_t),NULL);
  As *father_ = nullptr;
  surf::NetCard *netcard_ = nullptr; // Our representative in the father AS

protected:
  char *name_ = nullptr;
  xbt_dict_t children_ = xbt_dict_new_homogeneous(NULL); // sub-ASes
  xbt_dynar_t vertices_ = xbt_dynar_new(sizeof(char*),NULL); // our content, as known to our graph routing algorithm (maps vertexId -> vertex)

private:
  bool sealed_ = false; // We cannot add more content when sealed

  friend surf::RoutingPlatf;
  std::map<std::pair<std::string, std::string>, std::vector<surf::Link*>*> bypassRoutes_; // srcName x dstName -> route
  static void getRouteRecursive(surf::NetCard *src, surf::NetCard *dst, /* OUT */ std::vector<surf::Link*> * links, double *latency);
  std::vector<surf::Link*> *getBypassRoute(surf::NetCard *src, surf::NetCard *dst);

};

}}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_AS_HPP */
