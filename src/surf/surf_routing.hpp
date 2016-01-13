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
 * @brief A network card
 * @details This represents a position in the network. One can route information between two netcards
 */
class NetCard {
public:
  virtual ~NetCard(){};
  virtual int getId()=0;
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
  xbt_dynar_t p_indexNetworkElm = xbt_dynar_new(sizeof(char*),NULL);
  xbt_dict_t p_bypassRoutes;    /* store bypass routes */
  routing_model_description_t p_modelDesc;
  e_surf_routing_hierarchy_t p_hierarchy;
  char *p_name = nullptr;
  As *p_routingFather = nullptr;
  xbt_dict_t p_routingSons = xbt_dict_new_homogeneous(NULL);
  NetCard *p_netElem;
  xbt_dynar_t p_linkUpDownList = NULL;

  /**
   * @brief The As constructor
   */
  As(){};

  /**
   * @brief The As destructor
   */
  virtual ~As(){
    xbt_dict_free(&p_routingSons);
    xbt_dynar_free(&p_indexNetworkElm);
    xbt_dynar_free(&p_linkUpDownList);
    xbt_free(p_name);
    if (p_netElem)
      delete p_netElem;
  };

  /**
   * @brief Get the characteristics of the routing path between two points
   *
   * This function is used by the networking model to find the information it needs when starting a communication.
   *
   * The things are not straightforward because the platform can be routed using several routing models.
   * Some ASes may be routed in full, others may have only some connection information and use a shortest path on top of that, and so on.
   * Some ASes may even not have any predefined links and use only coordinate informations to compute the latency.
   *
   * So, the path is constructed recursively, with each traversed AS adding its information to the set.
   * The algorithm for that is explained in http://hal.inria.fr/hal-00650233/
   * 
   * @param src Initial point of the routing path
   * @param dst Final point of the routing path
   * @param into Container into which the links should be pushed
   * @param latency Accumulator in which the latencies should be added
   */
  virtual void getRouteAndLatency(
      NetCard *src, NetCard *dst,
      sg_platf_route_cbarg_t into, double *latency)=0;
  virtual xbt_dynar_t getOneLinkRoutes()=0;
  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)=0;
  virtual sg_platf_route_cbarg_t getBypassRoute(
      NetCard *src, NetCard *dst,
      double *lat)=0;

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  virtual int parsePU(NetCard *elm)=0; /* A host or a router, whatever */
  virtual int parseAS(NetCard *elm)=0;
  virtual void parseRoute(sg_platf_route_cbarg_t route)=0;
  virtual void parseASroute(sg_platf_route_cbarg_t route)=0;
  virtual void parseBypassroute(sg_platf_route_cbarg_t e_route)=0;
};

struct XBT_PRIVATE NetCardImpl : public NetCard {
public:
  NetCardImpl(char *name, int id, e_surf_network_element_type_t rcType, As *rcComponent)
  : p_rcComponent(rcComponent), p_rcType(rcType), m_id(id), p_name(name) {}
  ~NetCardImpl() { xbt_free(p_name);};

  int getId() {return m_id;}
  int *getIdPtr() {return &m_id;}
  void setId(int id) {m_id = id;}
  char *getName() {return p_name;}
  As *getRcComponent() {return p_rcComponent;}
  e_surf_network_element_type_t getRcType() {return p_rcType;}
private:
  As *p_rcComponent;
  e_surf_network_element_type_t p_rcType;
  int m_id;
  char *p_name;
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
  ~RoutingPlatf();
  As *p_root;
  void *p_loopback;
  xbt_dynar_t p_lastRoute;
  xbt_dynar_t getOneLinkRoutes(void);
  xbt_dynar_t recursiveGetOneLinkRoutes(As *rc);
  void getRouteAndLatency(NetCard *src, NetCard *dst, xbt_dynar_t * links, double *latency);
};

/*************
 * Callbacks *
 *************/

XBT_PUBLIC_DATA(simgrid::xbt::signal<void(NetCard*)>) routingEdgeCreatedCallbacks;
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(As*)>) asCreatedCallbacks;

}
}

#endif /* NETWORK_ROUTING_HPP_ */
