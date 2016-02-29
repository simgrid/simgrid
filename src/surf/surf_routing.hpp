/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_ROUTING_HPP_
#define NETWORK_ROUTING_HPP_

#include <xbt/base.h>
#include <xbt/signal.hpp>

#include "surf_interface.hpp"
#include "src/surf/xml/platf_private.hpp" // FIXME: including this here is pure madness. KILKILKIL XML.
#include <float.h>

#include <vector>
#include <map>

SG_BEGIN_DECL()
XBT_PUBLIC(void) routing_model_create(Link *loopback);
XBT_PRIVATE xbt_node_t new_xbt_graph_node (xbt_graph_t graph, const char *name, xbt_dict_t nodes);
XBT_PRIVATE xbt_edge_t new_xbt_graph_edge (xbt_graph_t graph, xbt_node_t s, xbt_node_t d, xbt_dict_t edges);
SG_END_DECL()

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

class As;
class XBT_PRIVATE RoutingModelDescription;
class XBT_PRIVATE Onelink;
class RoutingPlatf;

/** @ingroup SURF_routing_interface
 * @brief Network cards are the vertices in the graph representing the network, used to compute paths between nodes.
 *
 * @details This represents a position in the network. One can route information between two netcards
 */
class NetCard {
public:
  virtual ~NetCard(){};
  virtual int id()=0; // Our rank in the vertices_ array of our containing AS.
  virtual void setId(int id)=0;
  virtual char *name()=0;
  virtual As *containingAS()=0; // This is the AS in which I am
  virtual bool isAS()=0;
  virtual bool isHost()=0;
  virtual bool isRouter()=0;
};

/** @ingroup SURF_routing_interface
 * @brief Network Autonomous System (AS)
 */
class As {
public:
  As(const char*name);
  /** @brief Close that AS: no more content can be added to it */
  virtual void Seal();
  virtual ~As();

  e_surf_routing_hierarchy_t hierarchy_ = SURF_ROUTING_NULL;
  xbt_dynar_t upDownLinks = xbt_dynar_new(sizeof(s_surf_parsing_link_up_down_t),NULL);

  char *name_ = nullptr;
  NetCard *netcard_ = nullptr; // Our representative in the father AS
  As *father_ = nullptr;
  xbt_dict_t children_ = xbt_dict_new_homogeneous(NULL); // sub-ASes
  xbt_dynar_t vertices_ = xbt_dynar_new(sizeof(char*),NULL); // our content, as known to our graph routing algorithm (maps vertexId -> vertex)

private:
  bool sealed_ = false; // We cannot add more content when sealed

  friend RoutingPlatf;
  std::map<std::string, std::vector<Link*>*> bypassRoutes_;
  static void getRouteRecursive(NetCard *src, NetCard *dst, /* OUT */ std::vector<Link*> * links, double *latency);
  std::vector<Link*> *getBypassRoute(NetCard *src, NetCard *dst);

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
  virtual void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t into, double *latency)=0;
  /** @brief retrieves the list of all routes of size 1 (of type src x dst x Link) */
  virtual xbt_dynar_t getOneLinkRoutes();

  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)=0;

  /* Add content to the AS, at parsing time. It should be sealed afterward. */
  virtual int addComponent(NetCard *elm); /* A host, a router or an AS, whatever */
  virtual void addRoute(sg_platf_route_cbarg_t route);
  void addBypassRoute(sg_platf_route_cbarg_t e_route);

};

struct XBT_PRIVATE NetCardImpl : public NetCard {
public:
  NetCardImpl(const char *name, e_surf_network_element_type_t componentType, As *as)
  : name_(xbt_strdup(name)),
    componentType_(componentType),
    containingAS_(as)
  {}
  ~NetCardImpl() { xbt_free(name_);};

  int id()           override {return id_;}
  void setId(int id) override {id_ = id;}
  char *name()       override {return name_;}
  As *containingAS() override {return containingAS_;}

  bool isAS()        override {return componentType_ == SURF_NETWORK_ELEMENT_AS;}
  bool isHost()      override {return componentType_ == SURF_NETWORK_ELEMENT_HOST;}
  bool isRouter()    override {return componentType_ == SURF_NETWORK_ELEMENT_ROUTER;}

private:
  int id_ = -1;
  char *name_;
  e_surf_network_element_type_t componentType_;
  As *containingAS_;
};

/** @ingroup SURF_routing_interface
 * @brief Link of length 1, alongside with its source and destination. This is mainly useful in the ns3 bindings
 */
class Onelink {
public:
  Onelink(void *link, NetCard *src, NetCard *dst)
    : src_(src), dst_(dst), link_(link) {};
  NetCard *src_;
  NetCard *dst_;
  void *link_;
};

/** @ingroup SURF_routing_interface
 * @brief The class representing a whole routing platform
 */
XBT_PUBLIC_CLASS RoutingPlatf {
public:
  RoutingPlatf(Link *loopback);
  ~RoutingPlatf();
  As *root_ = nullptr;
  Link *loopback_;
  xbt_dynar_t getOneLinkRoutes(void);
  void getRouteAndLatency(NetCard *src, NetCard *dst, std::vector<Link*> * links, double *latency);
};

/*************
 * Callbacks *
 *************/

XBT_PUBLIC_DATA(simgrid::xbt::signal<void(NetCard*)>) netcardCreatedCallbacks;
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(As*)>) asCreatedCallbacks;

}
}

#endif /* NETWORK_ROUTING_HPP_ */
