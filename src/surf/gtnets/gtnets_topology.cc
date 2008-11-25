/* $ID$ */

/* Copyright (c) 2007 Kayo Fujiwara. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//GTNETS_Link, GTNETS_Node, GTNETS_Topology: 
//Temporary classes for generating GTNetS topology

#include "gtnets_topology.h"

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
  if (is_router_){
    fprintf(stderr, "Cannot add a host to a router node.\n");
    return -1;
  }
  hosts_.insert(hostid);
  return 0;
}

// Add a router. If this node already has a router/host,
// return -1.
int GTNETS_Node::add_router(int routerid){
  if (hosts_.size() > 1){
    fprintf(stderr, "Router node should have only one router.\n");
    return -1;
  }else if (hosts_.size() == 1){
    if (hosts_.find(routerid) != hosts_.end()){
      //printf("the router already exists\n");
      return 0;
    }else{
      fprintf(stderr, "Node %d is a different router.\n");
      return -1;
    }
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
  printf("[");
  for (it = hosts_.begin(); it != hosts_.end(); it++){
    printf(" %d", *it);
  }
  printf("]");
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
  printf("== LINKID: %d\n", ID_);
  if (src_node_){
    printf("  [SRC] ID: %d, router?: %d, hosts[]: ",
	   src_node_->id(), src_node_->is_router());
    src_node_->print_hosts();
    printf("\n");
  }

  if (dst_node_){
    printf("  [DST] ID: %d, router?: %d, hosts[]: ", 
	   dst_node_->id(), dst_node_->is_router());
    dst_node_->print_hosts();
    printf("\n");
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
  if (cur_id ==  src_node_->id()) return dst_node_->id();
  else if (cur_id == dst_node_->id()) return src_node_->id();
  else {
    fprintf(stderr, "node not found\n");
    return -1;
  }
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

  if(iter == links_.end()) {
    GTNETS_Link* link= new GTNETS_Link(id);
    //printf("link %d is added: %d\n", id, link->id());
    //links_.insert(make_pair(id, link));
    links_[id] = link;
    return 0;
  }else{
    fprintf(stderr, "Link %d already exists.\n", id);
    return -1;
  }
}

int GTNETS_Topology::add_router(int id){
  set<int>::iterator iter = routers_.find(id);

  if(iter == routers_.end()) {
    //printf("router %d is inserted\n", id);
    routers_.insert(id);
    return 0;
  }else{
    fprintf(stderr, "Router %d already exists.\n", id);
    return -1;
  }
}

bool GTNETS_Topology::is_router(int id){
  set<int>::iterator iter = routers_.find(id);
  if(iter == routers_.end()) return false;
  else return true;
}

//return the node id of the peer of cur_id by linkid.
int GTNETS_Topology::peer_node_id(int linkid, int cur_id){
  //printf("linkid: %d, cur_id: %d\n", linkid, cur_id);
  GTNETS_Link* link = links_[linkid];
  if (!link) {
    fprintf(stderr, "link %d not found\n", linkid);
    return -1;
  }
  if ((cur_id < 0) || (cur_id > nodes_.size()-1)){
    fprintf(stderr, "node %d not found\n", cur_id);
    return -1;
  }
  int peer  = link->peer_node(nodes_[cur_id]->id());
  if (peer < 0){
    fprintf(stderr, "peer not found\n");
    return -1;
  }
  return peer;
}

int GTNETS_Topology::add_onehop_route(int src, int dst, int linkid){
  GTNETS_Link* link;

  map<int, GTNETS_Link*>::iterator iter = links_.find(linkid);

  if(iter == links_.end()) {
    fprintf(stderr, "Link %d not found.\n", linkid);
    return -1;
  }else{
    link = iter->second;
  }

  //  printf("add_onehop_route: src: %d, dst: %d, linkid: %d, %d\n",
  //	 src, dst, linkid, link->id());

  GTNETS_Node *src_node, *dst_node;
  src_node = link->src_node();
  dst_node = link->dst_node();


  // If not exists a route, add one.
  if (!link->route_exists()){
    //check whether there exists a node for the src host/router.
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

  // other: has either src or dst (error)
  }else if (!src_node || !dst_node){
    fprintf(stderr, "Either src or dst is null\n");
    return -1;
  }

  // case 1: link has two routers
  else if (src_node->is_router() && dst_node->is_router()){
    int tmpsrc1 = src_node->id();
    int tmpsrc2 = nodeid_from_hostid(src);
    int tmpdst1 = dst_node->id();
    int tmpdst2 = nodeid_from_hostid(dst);
    if (((tmpsrc1 == tmpsrc2) && (tmpdst1 == tmpdst2)) ||
	((tmpsrc1 == tmpdst2) && (tmpdst1 == tmpsrc2))){
      //fprintf(stderr, "Route already exists\n");
    }else{
      fprintf(stderr, "Different one hop route defined\n");
      return -1;
    }
  }
  // case 2: link has one router and one host
  else if (src_node->is_router() && !dst_node->is_router()){
    int newsrc, newdst;
    if (is_router(src)){
      newsrc = src;
      newdst = dst;
    }else if (is_router(dst)){
      newsrc = dst;
      newdst = src;
    }else{
      fprintf(stderr, "one of nodes should be a router\n");
      return -1;
    }

    if (src_node->id() != nodeid_from_hostid(newsrc)){
      fprintf(stderr, "The router should be identical\n");
      return -1;
    }

    //now, to add dst to dst_node, dst should be a host.

    if (is_router(newdst)){
      fprintf(stderr, "dst %d is not an endpoint. cannot add it to dst_node\n");
      return -1;
    }

    if (!dst_node->include(newdst)){
      dst_node->add_host(newdst);
      hosts_[newdst] = dst_node->id();
    }
  }
  else if (!src_node->is_router() && dst_node->is_router()){
    int newsrc, newdst;
    if (is_router(src)){
      newsrc = dst;
      newdst = src;
    }else if (is_router(dst)){
      newsrc = src;
      newdst = dst;
    }else{
      fprintf(stderr, "one of nodes should be a router\n");
      return -1;
    }


    if (dst_node->id() != hosts_[newdst]){
      fprintf(stderr, "The router should be identical\n");
      return -1;
    }

    //now, to add dst to src_node, dst should be a host.

    if (is_router(newsrc)){
      fprintf(stderr, "dst %d is not an endpoint. cannot add it to src_node\n");
      return -1;
    }

    if (!src_node->include(newsrc)){
      src_node->add_host(newsrc);
      hosts_[newsrc] = src_node->id();
    }
  }

  // case 3: link has two hosts
  else if (!src_node->is_router() && !dst_node->is_router()){

    if (is_router(src) || is_router(dst)){
      fprintf(stderr, "Cannot add a router to host-host link\n");
      return -1;
    }

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
    fprintf(stderr, "Shouldn't be here\n");
    return -1;
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
  printf("<<<<<================================>>>>>\n");
  printf("Dumping GTNETS topollogy information\n");
  map<int, GTNETS_Link*>::iterator it;
  for (it = links_.begin(); it != links_.end(); it++){
    it->second->print_link_status();
  }
  printf(">>>>>================================<<<<<\n");
  fflush(NULL);
}

const vector<GTNETS_Node*>& GTNETS_Topology::nodes(){
  return nodes_;
}

const map<int, GTNETS_Link*>& GTNETS_Topology::links(){
  return links_;
}

