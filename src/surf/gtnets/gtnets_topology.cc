/* $ID$ */

/* Copyright (c) 2007 Kayo Fujiwara. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//GTNETS_Link, GTNETS_Node, GTNETS_Topology: 
//Temporary classes for generating GTNetS topology

#include "gtnets_topology.h"
#ifdef DEBUG0
	#undef DEBUG0
#endif
#include "xbt/log.h"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_gtnets_topology, surf_network_gtnets,
                                "Logging specific to the SURF network GTNetS simulator");

// 
//  GTNETS_Node
// 

// Constructor
GTNETS_Node::GTNETS_Node(int id):ID_(id),is_router_(false){}
// Copy constructor
GTNETS_Node::GTNETS_Node(const GTNETS_Node& node){
  ID_ = node.ID_;
  is_router_ = node.is_router_;
  hosts_ = node.hosts_;
}

GTNETS_Node::~GTNETS_Node(){

}

// hostid = network_card_id
int GTNETS_Node::add_host(int hostid){
  xbt_assert0(!(is_router_), "Cannot add a host to a router node");
  hosts_.insert(hostid);
  return 0;
}

// Add a router. If this node already has a router/host,
// return -1.
int GTNETS_Node::add_router(int routerid){
  xbt_assert0(!(hosts_.size() > 1), "Router node should have only one router");
  if (hosts_.size() == 1){
	  xbt_assert1((hosts_.find(routerid) != hosts_.end()), "Node %d is a different router", routerid);
	  return 0;
  }
  is_router_ = true;
  hosts_.insert(routerid);
  return 0;
}

bool GTNETS_Node::is_router(){
  return is_router_;
}

bool GTNETS_Node::include(int hostid){
  if (hosts_.find(hostid) == hosts_.end()) return false;
  else return true;
}

void GTNETS_Node::print_hosts(){
  set<int>::iterator it;
  for (it = hosts_.begin(); it != hosts_.end(); it++){
    DEBUG1("host id %d", *it);
  }
}

//
//  GTNETS_Link
//

// Constructor
GTNETS_Link::GTNETS_Link(){
  ID_=-1;
  src_node_ = 0;
  dst_node_ = 0;
}
GTNETS_Link::GTNETS_Link(int id):ID_(id), src_node_(0), dst_node_(0){}

// Copy constructor
GTNETS_Link::GTNETS_Link(const GTNETS_Link& link){
  ID_ = link.ID_;
  src_node_ = link.src_node_;
  dst_node_ = link.dst_node_;
}

GTNETS_Link::~GTNETS_Link(){
  
}

void GTNETS_Link::print_link_status(){
  DEBUG1("link id: %d", ID_);
  if (src_node_){
    DEBUG2("[src] id: %d, is it router?: %d, host list: ",src_node_->id(), src_node_->is_router());
    src_node_->print_hosts();
  }

  if (dst_node_){
    DEBUG2("[dst] id: %d, is it router?: %d, host list: ",dst_node_->id(), dst_node_->is_router());
    dst_node_->print_hosts();
  }
}

GTNETS_Node* GTNETS_Link::src_node(){
  return src_node_;
}

GTNETS_Node* GTNETS_Link::dst_node(){
  return dst_node_;
}

bool GTNETS_Link::route_exists(){
  if (src_node_ && dst_node_) return true;
  else return false;
}

// return the peer node id
int GTNETS_Link::peer_node(int cur_id){
  xbt_assert0(((cur_id ==  src_node_->id())||(cur_id == dst_node_->id())), "Node not found");

  if (cur_id ==  src_node_->id()) return dst_node_->id();
  else if (cur_id == dst_node_->id()) return src_node_->id();
}

int GTNETS_Link::add_src(GTNETS_Node* src){
  src_node_ = src;
}

int GTNETS_Link::add_dst(GTNETS_Node* dst){
  dst_node_ = dst;
}


//
//  GTNETS_Topology
//

// Constructor
GTNETS_Topology::GTNETS_Topology(){
  nodeID_ = 0;
}

// Destructor
GTNETS_Topology::~GTNETS_Topology(){
  map<int, GTNETS_Link*>::iterator it1;
  for (it1 = links_.begin(); it1 != links_.end(); it1++){
    delete it1->second;
  }
  vector<GTNETS_Node*>::iterator it2;
  for (it2 = nodes_.begin(); it2 != nodes_.end(); it2++){
    delete *it2;
  }
}


int GTNETS_Topology::link_size(){
  return links_.size();
}

int GTNETS_Topology::node_size(){
  return nodes_.size();
}

int GTNETS_Topology::add_link(int id){
  map<int,GTNETS_Link*>::iterator iter = links_.find(id);
  xbt_assert1((iter == links_.end()), "Link %d already exists", id);

  if(iter == links_.end()) {
    GTNETS_Link* link= new GTNETS_Link(id);
    links_[id] = link;
  }
  return 0;
}

int GTNETS_Topology::add_router(int id){
  set<int>::iterator iter = routers_.find(id);
  if(iter == routers_.end()){
	  routers_.insert(id);
  }else{
	  DEBUG1("Router (#%d) already exists", id);
  }
  return 0;
}

bool GTNETS_Topology::is_router(int id){
  set<int>::iterator iter = routers_.find(id);
  if(iter == routers_.end()) return false;
  else return true;
}

//return the node id of the peer of cur_id by linkid.
int GTNETS_Topology::peer_node_id(int linkid, int cur_id){
  GTNETS_Link* link = links_[linkid];
  xbt_assert1((link), "Link %d not found", linkid);
  xbt_assert1(!((cur_id < 0) || (cur_id > nodes_.size()-1)), "Node %d not found", cur_id);

  int peer  = link->peer_node(nodes_[cur_id]->id());
  xbt_assert0(!(peer < 0), "Peer not found");

  return peer;
}

int GTNETS_Topology::add_onehop_route(int src, int dst, int linkid){
  GTNETS_Link* link;

  map<int, GTNETS_Link*>::iterator iter = links_.find(linkid);

  xbt_assert1(!(iter == links_.end()), "Link %d not found", linkid);
  link = iter->second;

  DEBUG4("Add onehop route, src: %d, dst: %d, linkid: %d, %d",src, dst, linkid, link->id());

  GTNETS_Node *src_node, *dst_node;
  src_node = link->src_node();
  dst_node = link->dst_node();

  // If not exists a route, add one.
  if (!link->route_exists()){
    //check whether there exists a node for the src.
    int s_node_id = nodeid_from_hostid(src);
    int node_id;

    if (s_node_id < 0){//not exist, create one.
      s_node_id = nodeID_;
      GTNETS_Node* node1 = new GTNETS_Node(s_node_id);
      nodes_.push_back(node1);
      hosts_[src] = nodes_[s_node_id]->id();

      nodeID_++;
    }

    if (is_router(src))
      nodes_[s_node_id]->add_router(src);
    else
      nodes_[s_node_id]->add_host(src);

    link->add_src(nodes_[s_node_id]);

    //check whether there exists a node for the dst host/router.
    int d_node_id = nodeid_from_hostid(dst);
    if (d_node_id < 0){//not exist, create one.
      d_node_id = nodeID_;
      GTNETS_Node* node2 = new GTNETS_Node(d_node_id);
      nodes_.push_back(node2);
      hosts_[dst] = nodes_[d_node_id]->id();
      nodeID_++;
    }

    if (is_router(dst))
      nodes_[d_node_id]->add_router(dst);
    else
      nodes_[d_node_id]->add_host(dst);

    link->add_dst(nodes_[d_node_id]);
  }else if (!(src_node && dst_node)){
      xbt_assert0((src_node && dst_node), "Either src or dst is null");
  }

  // case 1: link has two routers
  else if (src_node->is_router() && dst_node->is_router()){
    int tmpsrc1 = src_node->id();
    int tmpsrc2 = nodeid_from_hostid(src);
    int tmpdst1 = dst_node->id();
    int tmpdst2 = nodeid_from_hostid(dst);
    xbt_assert0( (((tmpsrc1 == tmpsrc2) && (tmpdst1 == tmpdst2)) ||
	((tmpsrc1 == tmpdst2) && (tmpdst1 == tmpsrc2))), "Different one hop route defined");
  }

  // case 2: link has one router and one host
  else if (src_node->is_router() && !dst_node->is_router()){
    int newsrc, newdst;
    xbt_assert0( ((is_router(src))||(is_router(dst))), "One of nodes should be a router");

    if (is_router(src)){
      newsrc = src;
      newdst = dst;
    }else if (is_router(dst)){
      newsrc = dst;
      newdst = src;
    }

    xbt_assert0(!(src_node->id() != nodeid_from_hostid(newsrc)), "The router should be identical");

    //now, to add dst to dst_node, dst should be a host.
    xbt_assert1(!(is_router(newdst)), "Dst %d is not an endpoint. cannot add it to dst_node", newdst);

    if (!dst_node->include(newdst)){
      dst_node->add_host(newdst);
      hosts_[newdst] = dst_node->id();
    }
  }
  else if (!src_node->is_router() && dst_node->is_router()){
    int newsrc, newdst;
    xbt_assert0(((is_router(src))||(is_router(dst))), "One of nodes should be a router");

    if (is_router(src)){
      newsrc = dst;
      newdst = src;
    }else if (is_router(dst)){
      newsrc = src;
      newdst = dst;
    }

    xbt_assert0(!(dst_node->id() != hosts_[newdst]), "The router should be identical");
    //now, to add dst to src_node, dst should be a host.
    xbt_assert1(!(is_router(newsrc)), "Src %d is not an endpoint. cannot add it to src_node", newsrc);

    if (!src_node->include(newsrc)){
      src_node->add_host(newsrc);
      hosts_[newsrc] = src_node->id();
    }
  }

  // case 3: link has two hosts
  else if (!src_node->is_router() && !dst_node->is_router()){
	xbt_assert0(!(is_router(src) || is_router(dst)), "Cannot add a router to host-host link");

    //if both are hosts, the order doesn't matter.
    if (src_node->include(src)){
      if (dst_node->include(dst)){
	    //nothing
      }else{
	    dst_node->add_host(dst);
	    hosts_[dst] = dst_node->id();
      }
    }else if (src_node->include(dst)){
      if (dst_node->include(src)){
	    //nothing
      }else{
	    dst_node->add_host(src);
	    hosts_[src] = dst_node->id();
      }
    }else if (dst_node->include(src)){
      if (src_node->include(dst)){
	    //nothing
      }else{
	    src_node->add_host(dst);
	    hosts_[dst] = src_node->id();
      }
    }else if (dst_node->include(dst)){
      if (src_node->include(src)){
	    //nothing
      }else{
	    src_node->add_host(src);
	    hosts_[src] = src_node->id();
      }
    }else{
      src_node->add_host(src);
      dst_node->add_host(dst);
      hosts_[src] = src_node->id();
      hosts_[dst] = dst_node->id();
    }   
      
  }
  else{
    xbt_assert0(0, "Shouldn't be here");
  }

  if (XBT_LOG_ISENABLED(surf_network_gtnets_topology, xbt_log_priority_debug)) {
    link->print_link_status();
    src_node->print_hosts();
    dst_node->print_hosts();
  }

  return 0;
}

int GTNETS_Topology::nodeid_from_hostid(int hostid){
  map<int,int>::iterator it = hosts_.find(hostid);
  if (it == hosts_.end())
    return -1;
  else return it->second;
}

void GTNETS_Topology::print_topology(){
  DEBUG0("<<<<<================================>>>>>");
  DEBUG0("Dumping GTNETS topollogy information");
  map<int, GTNETS_Link*>::iterator it;
  for (it = links_.begin(); it != links_.end(); it++){
    it->second->print_link_status();
  }
  DEBUG0(">>>>>================================<<<<<");
  fflush(NULL);
}

const vector<GTNETS_Node*>& GTNETS_Topology::nodes(){
  return nodes_;
}

const map<int, GTNETS_Link*>& GTNETS_Topology::links(){
  return links_;
}

