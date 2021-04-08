/* Copyright (c) 2014-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_IB_HPP_
#define SURF_NETWORK_IB_HPP_

#include "src/surf/network_smpi.hpp"
#include "xbt/base.h"

#include <map>
#include <vector>

namespace simgrid {
namespace kernel {
namespace resource {

class XBT_PRIVATE IBNode;

struct XBT_PRIVATE ActiveComm {
  IBNode* destination   = nullptr;
  NetworkAction* action = nullptr;
  double init_rate      = -1;
};

class IBNode {
public:
  int id_;
  // store related links, to ease computation of the penalties
  std::vector<ActiveComm*> active_comms_up_;
  // store the number of comms received from each node
  std::map<IBNode*, int> active_comms_down_;
  // number of comms the node is receiving
  int nb_active_comms_down_ = 0;
  explicit IBNode(int id) : id_(id){};
};

class XBT_PRIVATE NetworkIBModel : public NetworkSmpiModel {
  std::unordered_map<std::string, IBNode> active_nodes;
  std::unordered_map<NetworkAction*, std::pair<IBNode*, IBNode*>> active_comms;

  double Bs_;
  double Be_;
  double ys_;
  void update_IB_factors_rec(IBNode* root, std::vector<bool>& updatedlist) const;
  void compute_IB_factors(IBNode* root) const;

public:
  explicit NetworkIBModel(const std::string& name);
  NetworkIBModel(const NetworkIBModel&) = delete;
  NetworkIBModel& operator=(const NetworkIBModel&) = delete;
  void update_IB_factors(NetworkAction* action, IBNode* from, IBNode* to, int remove) const;

  static void IB_create_host_callback(s4u::Host const& host);
  static void IB_action_state_changed_callback(NetworkAction& action, Action::State /*previous*/);
  static void IB_action_init_callback(NetworkAction& action);
};
} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif
