/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_CONSTANT_HPP_
#define NETWORK_CONSTANT_HPP_

#include <xbt/base.h>

#include "network_interface.hpp"

namespace simgrid {
  namespace surf {

    /***********
     * Classes *
     ***********/

    class XBT_PRIVATE NetworkConstantModel;
    class XBT_PRIVATE NetworkConstantAction;

    /*********
     * Model *
     *********/
    class NetworkConstantModel : public NetworkModel {
    public:
      Action* communicate(simgrid::s4u::Host* src, simgrid::s4u::Host* dst, double size, double rate) override;
      double nextOccuringEvent(double now) override;
      void updateActionsState(double now, double delta) override;

      LinkImpl* createLink(const std::string& name, double bw, double lat,
                           e_surf_link_sharing_policy_t policy) override;
    };

    /**********
     * Action *
     **********/
    class NetworkConstantAction : public NetworkAction {
    public:
      NetworkConstantAction(NetworkConstantModel *model_, double size, double latency);
      ~NetworkConstantAction();
      double initialLatency_;
    };

  }
}

#endif /* NETWORK_CONSTANT_HPP_ */
