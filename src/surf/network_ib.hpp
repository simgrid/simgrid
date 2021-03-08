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
  int id;
  // store related links, to ease computation of the penalties
  std::vector<ActiveComm*> ActiveCommsUp;
  // store the number of comms received from each node
  std::map<IBNode*, int> ActiveCommsDown;
  // number of comms the node is receiving
  int nbActiveCommsDown = 0;
  explicit IBNode(int id) : id(id){};
};

class XBT_PRIVATE NetworkIBModel : public NetworkSmpiModel {
  double Bs;
  double Be;
  double ys;
  void updateIBfactors_rec(IBNode* root, std::vector<bool>& updatedlist) const;
  void computeIBfactors(IBNode* root) const;

public:
  NetworkIBModel();
  explicit NetworkIBModel(const char* name);
  NetworkIBModel(const NetworkIBModel&) = delete;
  NetworkIBModel& operator=(const NetworkIBModel&) = delete;
  void updateIBfactors(NetworkAction* action, IBNode* from, IBNode* to, int remove) const;

  std::unordered_map<std::string, IBNode> active_nodes;
  std::unordered_map<NetworkAction*, std::pair<IBNode*, IBNode*>> active_comms;
};
} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif
