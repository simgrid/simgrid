/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_ROUTING_HPP_
#define NETWORK_ROUTING_HPP_

#include <xbt/base.h>
#include <xbt/signal.hpp>

#include "surf_interface.hpp"
#include <float.h>

XBT_PUBLIC(void) routing_model_create( void *loopback);

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
  virtual int getId()=0; // Our rank in the vertices_ array of our container AS.
  virtual int *getIdPtr()=0;
  virtual void setId(int id)=0;
  virtual char *getName()=0;
  virtual As *getRcComponent()=0;
  virtual e_surf_network_element_type_t getRcType()=0;
};

/** @ingroup SURF_routing_interface
 * @brief The Autonomous System (AS) routing interface
 * @details [TODO]
 */
class As {
public:
  As(const char*name);
  /** @brief Close that AS: no more content can be added to it */
  virtual void Seal()=0;
  virtual ~As();

  char *name_ = nullptr;
  NetCard *netcard_ = nullptr;
  As *father_ = nullptr;
  xbt_dict_t sons_ = xbt_dict_new_homogeneous(NULL);

  xbt_dynar_t vertices_ = xbt_dynar_new(sizeof(char*),NULL); // our content, as known to our graph routing algorithm (maps vertexId -> vertex)
  xbt_dict_t bypassRoutes_ = nullptr;
  e_surf_routing_hierarchy_t hierarchy_ = SURF_ROUTING_NULL;
  xbt_dynar_t upDownLinks = xbt_dynar_new(sizeof(s_surf_parsing_link_up_down_t),NULL);



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
  virtual xbt_dynar_t getOneLinkRoutes()=0;

  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)=0;

  virtual sg_platf_route_cbarg_t getBypassRoute(NetCard *src, NetCard *dst,double *lat)=0;

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  virtual int addComponent(NetCard *elm); /* A host, a router or an AS, whatever */
  virtual void parseRoute(sg_platf_route_cbarg_t route)=0;
  virtual void parseASroute(sg_platf_route_cbarg_t route)=0;
  virtual void parseBypassroute(sg_platf_route_cbarg_t e_route)=0;
};

struct XBT_PRIVATE NetCardImpl : public NetCard {
public:
  NetCardImpl(const char *name, e_surf_network_element_type_t componentType, As *component)
  : component_(component),
    componentType_(componentType),
    name_(xbt_strdup(name))
  {}
  ~NetCardImpl() { xbt_free(name_);};

  int getId() {return id_;}
  int *getIdPtr() {return &id_;}
  void setId(int id) {id_ = id;}
  char *getName() {return name_;}
  As *getRcComponent() {return component_;}
  e_surf_network_element_type_t getRcType() {return componentType_;}
private:
  As *component_;
  e_surf_network_element_type_t componentType_;
  int id_ = -1;
  char *name_;
};

/** @ingroup SURF_routing_interface
 * @brief Link of lenght 1, alongside with its source and destination. This is mainly usefull in the ns3 bindings
 */
class Onelink {
public:
  Onelink(void *link, NetCard *src, NetCard *dst)
    : p_src(src), p_dst(dst), p_link(link) {};
  NetCard *p_src;
  NetCard *p_dst;
  void *p_link;
};

/** @ingroup SURF_routing_interface
 * @brief The class representing a whole routing platform
 */
XBT_PUBLIC_CLASS RoutingPlatf {
public:
  RoutingPlatf(void *loopback);
  ~RoutingPlatf();
  As *p_root = nullptr;
  void *p_loopback;
  xbt_dynar_t p_lastRoute = xbt_dynar_new(sizeof(sg_routing_link_t),NULL);
  xbt_dynar_t getOneLinkRoutes(void);
  xbt_dynar_t recursiveGetOneLinkRoutes(As *rc);
  void getRouteAndLatency(NetCard *src, NetCard *dst, xbt_dynar_t * links, double *latency);
};

/*************
 * Callbacks *
 *************/

XBT_PUBLIC_DATA(simgrid::xbt::signal<void(NetCard*)>) netcardCreatedCallbacks;
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(As*)>) asCreatedCallbacks;

}
}

#endif /* NETWORK_ROUTING_HPP_ */
