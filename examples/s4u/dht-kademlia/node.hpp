/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _KADEMLIA_NODE_HPP
#define _KADEMLIA_NODE_HPP
#include "answer.hpp"
#include "message.hpp"
#include "routing_table.hpp"
#include "s4u-dht-kademlia.hpp"

namespace kademlia {

class Node {
  unsigned int id_;              // node id - 160 bits
  RoutingTable table;            // node routing table
public:
  simgrid::s4u::CommPtr receive_comm = nullptr;
  void* received_msg             = nullptr;
  unsigned int find_node_success = 0; // Number of find_node which have succeeded.
  unsigned int find_node_failed  = 0; // Number of find_node which have failed.
  explicit Node(unsigned int node_id) : id_(node_id), table(node_id) {}
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  unsigned int getId() { return id_; }

  bool join(unsigned int known_id);
  void sendFindNode(unsigned int id, unsigned int destination);
  unsigned int sendFindNodeToBest(Answer* node_list);
  void routingTableUpdate(unsigned int id);
  Answer* findClosest(unsigned int destination_id);
  bool findNode(unsigned int id_to_find, bool count_in_stats);
  void randomLookup();
  void handleFindNode(Message* msg);
};
}
// identifier functions
unsigned int get_id_in_prefix(unsigned int id, unsigned int prefix);
unsigned int get_node_prefix(unsigned int id, unsigned int nb_bits);

#endif /* _MSG_EXAMPLES_ROUTING_H */
