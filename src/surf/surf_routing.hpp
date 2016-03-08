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
#include "src/surf/AsImpl.hpp"

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
  virtual AsImpl *containingAS()=0; // This is the AS in which I am
  virtual bool isAS()=0;
  virtual bool isHost()=0;
  virtual bool isRouter()=0;
};

struct XBT_PRIVATE NetCardImpl : public NetCard {
public:
  NetCardImpl(const char *name, e_surf_network_element_type_t componentType, AsImpl *as)
  : name_(xbt_strdup(name)),
    componentType_(componentType),
    containingAS_(as)
  {}
  ~NetCardImpl() { xbt_free(name_);};

  int id()           override {return id_;}
  void setId(int id) override {id_ = id;}
  char *name()       override {return name_;}
  AsImpl *containingAS() override {return containingAS_;}

  bool isAS()        override {return componentType_ == SURF_NETWORK_ELEMENT_AS;}
  bool isHost()      override {return componentType_ == SURF_NETWORK_ELEMENT_HOST;}
  bool isRouter()    override {return componentType_ == SURF_NETWORK_ELEMENT_ROUTER;}

private:
  int id_ = -1;
  char *name_;
  e_surf_network_element_type_t componentType_;
  AsImpl *containingAS_;
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
  AsImpl *root_ = nullptr;
  Link *loopback_;
  xbt_dynar_t getOneLinkRoutes(void);
  void getRouteAndLatency(NetCard *src, NetCard *dst, std::vector<Link*> * links, double *latency);
};

/*************
 * Callbacks *
 *************/

XBT_PUBLIC_DATA(simgrid::xbt::signal<void(NetCard*)>) netcardCreatedCallbacks;
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(s4u::As*)>) asCreatedCallbacks;

}
}

#endif /* NETWORK_ROUTING_HPP_ */
