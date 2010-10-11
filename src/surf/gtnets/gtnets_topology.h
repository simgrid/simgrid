/* $ID$ */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _GTNETS_TOPOLOGY_H
#define _GTNETS_TOPOLOGY_H

#include <map>
#include <vector>
#include <set>
#include <iostream>

using namespace std;

class GTNETS_Link;

class GTNETS_Node {

public:
  GTNETS_Node(int);
   GTNETS_Node(const GTNETS_Node & node);
  ~GTNETS_Node();

  int add_host(int);
  int add_router(int);
  int id() {
    return ID_;
  };
  bool is_router();
  bool include(int);
  void print_hosts();

private:
  int ID_;
  int is_router_;
  set < int >hosts_;            //simgrid hosts
};

class GTNETS_Link {

public:
  GTNETS_Link();
  GTNETS_Link(int id);
   GTNETS_Link(const GTNETS_Link &);
  ~GTNETS_Link();

  GTNETS_Node *src_node();
  GTNETS_Node *dst_node();
  int peer_node(int);
  int id() {
    return ID_;
  };
  void print_link_status();
  int add_src(GTNETS_Node *);
  int add_dst(GTNETS_Node *);
  bool route_exists();

private:
  int ID_;
  GTNETS_Node *src_node_;
  GTNETS_Node *dst_node_;

};

// To create a topology:
// 1. add links
// 2. add routers
// 3. add onehop links
class GTNETS_Topology {
public:
  GTNETS_Topology();
  ~GTNETS_Topology();

  bool is_router(int id);
  int peer_node_id(int linkid, int cur_id);
  int add_link(int id);
  int add_router(int id);
  int add_onehop_route(int src, int dst, int link);

  int nodeid_from_hostid(int);
  int link_size();
  int node_size();
  void print_topology();
  const vector < GTNETS_Node * >&nodes();
  const map < int, GTNETS_Link * >&links();

private:

  int nodeID_;
   map < int, GTNETS_Link * >links_;
   vector < GTNETS_Node * >nodes_;

   map < int, int >hosts_;      //hostid->nodeid

   set < int >routers_;
};

#endif
