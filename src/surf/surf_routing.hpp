/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_ROUTING_HPP_
#define NETWORK_ROUTING_HPP_

#include <xbt/base.h>

#include "surf_interface.hpp"
#include <float.h>

XBT_PUBLIC(void) routing_model_create( void *loopback);

/* ************************************************************************** */
/* ************************* GRAPH EXPORTING FUNCTIONS ********************** */
XBT_PRIVATE xbt_node_t new_xbt_graph_node (xbt_graph_t graph, const char *name, xbt_dict_t nodes);
XBT_PRIVATE xbt_edge_t new_xbt_graph_edge (xbt_graph_t graph, xbt_node_t s, xbt_node_t d, xbt_dict_t edges);

/***********
 * Classes *
 ***********/

class As;
class XBT_PRIVATE RoutingModelDescription;
class XBT_PRIVATE Onelink;
class RoutingPlatf;

/** @ingroup SURF_routing_interface
 * @brief A routing edge
 * @details [long description]
 */
struct RoutingEdge {
public:
  virtual ~RoutingEdge(){};
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
  xbt_dynar_t p_indexNetworkElm;
  xbt_dict_t p_bypassRoutes;    /* store bypass routes */
  routing_model_description_t p_modelDesc;
  e_surf_routing_hierarchy_t p_hierarchy;
  char *p_name;
  As *p_routingFather;
  xbt_dict_t p_routingSons;
  RoutingEdge *p_netElem;
  xbt_dynar_t p_linkUpDownList;

  /**
   * @brief The As constructor
   */
  As(){};

  /**
   * @brief The As destructor
   */
  virtual ~As(){
	xbt_free(p_name);
	if (p_netElem)
		delete p_netElem;
  };

  /**
   * @brief Get the route and latency between two RoutingEdges
   * @details [long description]
   * 
   * @param src [description]
   * @param dst [description]
   * @param into [description]
   * @param latency [description]
   */
  virtual void getRouteAndLatency(RoutingEdge *src, RoutingEdge *dst, sg_platf_route_cbarg_t into, double *latency)=0;
  virtual xbt_dynar_t getOneLinkRoutes()=0;
  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)=0;
  virtual sg_platf_route_cbarg_t getBypassRoute(RoutingEdge *src, RoutingEdge *dst, double *lat)=0;

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  virtual int parsePU(RoutingEdge *elm)=0; /* A host or a router, whatever */
  virtual int parseAS( RoutingEdge *elm)=0;
  virtual void parseRoute(sg_platf_route_cbarg_t route)=0;
  virtual void parseASroute(sg_platf_route_cbarg_t route)=0;
  virtual void parseBypassroute(sg_platf_route_cbarg_t e_route)=0;
};

struct XBT_PRIVATE RoutingEdgeImpl : public RoutingEdge {
public:
  RoutingEdgeImpl(char *name, int id, e_surf_network_element_type_t rcType, As *rcComponent)
  : p_rcComponent(rcComponent), p_rcType(rcType), m_id(id), p_name(name) {}
  ~RoutingEdgeImpl() { xbt_free(p_name);};

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

struct RoutingEdgeWrapper : public RoutingEdge {
public:
  RoutingEdgeWrapper(RoutingEdge *re) : p_re(re){}
  ~RoutingEdgeWrapper(){}
  int getId() {return p_re->getId();}
  int *getIdPtr() {return p_re->getIdPtr();}
  void setId(int id) {p_re->setId(id);}
  char *getName() {return p_re->getName();}
  As *getRcComponent() {return p_re->getRcComponent();}
  e_surf_network_element_type_t getRcType() {return p_re->getRcType();}
private:
  RoutingEdge *p_re;
};

/** @ingroup SURF_routing_interface
 * @brief Link of lenght 1, alongside with its source and destination. This is mainly usefull in the ns3 bindings
 */
class Onelink {
public:
  Onelink(void *link, RoutingEdge *src, RoutingEdge *dst)
    : p_src(src), p_dst(dst), p_link(link) {};
  RoutingEdge *p_src;
  RoutingEdge *p_dst;
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
  void getRouteAndLatency(RoutingEdge *src, RoutingEdge *dst, xbt_dynar_t * links, double *latency);
};

/*************
 * Callbacks *
 *************/

XBT_PUBLIC_DATA(surf_callback(void, RoutingEdge*)) routingEdgeCreatedCallbacks;

#endif /* NETWORK_ROUTING_HPP_ */
