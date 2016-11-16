/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SURF_AS_HPP
#define SIMGRID_SURF_AS_HPP

#include "xbt/graph.h"

#include "simgrid/s4u/forward.hpp"
#include "simgrid/s4u/As.hpp"

#include "src/surf/xml/platf_private.hpp" // FIXME: kill sg_platf_route_cbarg_t to remove that UGLY include

namespace simgrid {
namespace kernel {
namespace routing {
  class RoutingPlatf; // FIXME: KILLME
  class Onelink;

  /** @brief Autonomous Systems
   *
   * An AS is a network container, in charge of routing information between elements (hosts) and to the nearby ASes.
   * In SimGrid, there is a hierarchy of ASes, ie a tree with a unique root AS, that you can retrieve from the
   * s4u::Engine.
   *
   * The purpose of the kernel::routing module is to retrieve the routing path between two points in a time- and
   * space-efficient manner. This is done by AsImpl::getGlobalRoute(), called when creating a communication to
   * retrieve both the list of links that the create communication will use, and the summed latency that these
   * links represent.
   *
   * The network could recompute the latency by itself from the list, but it would require an additional link
   * set traversal. This operation being on the critical path of SimGrid, the routing computes the latency on the
   * behalf of the network.
   *
   * Finding the path between two nodes is rather complex because we navigate a hierarchy of ASes, each of them
   * being a full network. In addition, the routing can declare shortcuts (called bypasses), either within an AS
   * at the route level or directly between ASes. Also, each AS can use a differing routing algorithm, depending
   * on its class. @ref{AsFull} have a full matrix giving explicitly the path between any pair of their
   * contained nodes, while @ref{AsDijkstra} or @ref{AsFloyd} rely on a shortest path algorithm. @ref{AsVivaldi}
   * does not even have any link but only use only coordinate information to compute the latency.
   *
   * So AsImpl::getGlobalRoute builds the path recursively asking its specific information to each traversed AS with
   * AsImpl::getLocalRoute, that is redefined in each sub-class.
   * The algorithm for that is explained in http://hal.inria.fr/hal-00650233/
   *
   */
  XBT_PUBLIC_CLASS AsImpl : public s4u::As
  {
    friend simgrid::kernel::routing::RoutingPlatf;

  protected:
    explicit AsImpl(As * father, const char* name);

  public:
    /** @brief Make an host within that AS */
    simgrid::s4u::Host* createHost(const char* name, std::vector<double>* speedPerPstate, int coreAmount);

  protected:
    /**
     * @brief Probe the routing path between two points that are local to the called AS.
     *
     * @param src where from
     * @param dst where to
     * @param into Container into which the traversed links and gateway informations should be pushed
     * @param latency Accumulator in which the latencies should be added (caller must set it to 0)
     */
    virtual void getLocalRoute(NetCard * src, NetCard * dst, sg_platf_route_cbarg_t into, double* latency) = 0;
    /** @brief retrieves the list of all routes of size 1 (of type src x dst x Link) */
    /* returns whether we found a bypass path */
    bool getBypassRoute(routing::NetCard * src, routing::NetCard * dst,
                        /* OUT */ std::vector<surf::Link*> * links, double* latency);

  public:
    /* @brief get the route between two nodes in the full platform
     *
     * @param src where from
     * @param dst where to
     * @param links Accumulator in which all traversed links should be pushed (caller must empty it)
     * @param latency Accumulator in which the latencies should be added (caller must set it to 0)
     */
    static void getGlobalRoute(routing::NetCard * src, routing::NetCard * dst,
                               /* OUT */ std::vector<surf::Link*> * links, double* latency);

    virtual void getOneLinkRoutes(std::vector<Onelink*> * accumulator);
    virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) = 0;
    enum class RoutingMode {
      unset = 0, /**< Undefined type                                   */
      base,      /**< Base case: use simple link lists for routing     */
      recursive  /**< Recursive case: also return gateway information  */
    };
    /* FIXME: protect the following fields once the construction madness is sorted out */
    RoutingMode hierarchy_     = RoutingMode::unset;

  private:
    routing::NetCard* netcard_ = nullptr; // Our representative in the father AS
};

}}}; // Namespace simgrid::kernel::routing

#endif /* SIMGRID_SURF_AS_HPP */
