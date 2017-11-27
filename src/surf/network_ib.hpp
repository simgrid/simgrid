/* Copyright (c) 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_IB_HPP_
#define SURF_NETWORK_IB_HPP_

#include "src/surf/network_smpi.hpp"
#include "xbt/base.h"

#include <unordered_map>
#include <vector>

namespace simgrid {
  namespace surf {

    class XBT_PRIVATE IBNode;

    class XBT_PRIVATE ActiveComm{
    public :
      //IBNode* origin;
      IBNode* destination;
      NetworkAction *action;
      double init_rate;
      ActiveComm() : destination(nullptr),action(nullptr),init_rate(-1){};
      virtual ~ActiveComm() = default;
    };

    class IBNode{
    public :
      int id;
      //store related links, to ease computation of the penalties
      std::vector<ActiveComm*> ActiveCommsUp;
      //store the number of comms received from each node
      std::map<IBNode*, int> ActiveCommsDown;
      //number of comms the node is receiving
      int nbActiveCommsDown;
      explicit IBNode(int id) : id(id),nbActiveCommsDown(0){};
      virtual ~IBNode() = default;
    };

    class XBT_PRIVATE NetworkIBModel : public NetworkSmpiModel {
    private:
      void updateIBfactors_rec(IBNode* root, std::vector<bool>& updatedlist);
      void computeIBfactors(IBNode *root);
    public:
      NetworkIBModel();
      explicit NetworkIBModel(const char *name);
      ~NetworkIBModel() override;
      void updateIBfactors(NetworkAction *action, IBNode *from, IBNode * to, int remove);

      std::unordered_map<std::string, IBNode*> active_nodes;
      std::unordered_map<NetworkAction*, std::pair<IBNode*, IBNode*>> active_comms;

      double Bs;
      double Be;
      double ys;

    };

  }
}

#endif
