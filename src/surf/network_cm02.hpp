/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_CM02_HPP_
#define SURF_NETWORK_CM02_HPP_

#include <xbt/base.h>

#include "network_interface.hpp"
#include "xbt/fifo.h"
#include "xbt/graph.h"



/***********
 * Classes *
 ***********/

namespace simgrid {
  namespace surf {

    class XBT_PRIVATE NetworkCm02Model;
    class XBT_PRIVATE NetworkCm02Action;
    class XBT_PRIVATE NetworkSmpiModel;

  }
}

/*********
 * Model *
 *********/

namespace simgrid {
  namespace surf {

    class NetworkCm02Model : public NetworkModel {
    public:
      NetworkCm02Model();
      explicit NetworkCm02Model(void (*solve_fun)(lmm_system_t self));
      virtual ~NetworkCm02Model();
      Link* createLink(const char *name, double bandwidth,  double latency, e_surf_link_sharing_policy_t policy,
          xbt_dict_t properties) override;
      void updateActionsStateLazy(double now, double delta) override;
      void updateActionsStateFull(double now, double delta) override;
      Action *communicate(kernel::routing::NetCard *src, kernel::routing::NetCard *dst, double size, double rate) override;
      virtual void gapAppend(double size, const Link* link, NetworkAction* action);
    protected:
      bool haveGap_ = false;
    };

    /************
     * Resource *
     ************/

    class NetworkCm02Link : public Link {
    public:
      NetworkCm02Link(NetworkCm02Model *model, const char *name, xbt_dict_t props,
          double bandwidth, double latency, e_surf_link_sharing_policy_t policy,
          lmm_system_t system);
      ~NetworkCm02Link() override;
      void apply_event(tmgr_trace_iterator_t event, double value) override;
      void updateBandwidth(double value) override;
      void updateLatency(double value) override;
      virtual void gapAppend(double size, const Link* link, NetworkAction* action);
    };


    /**********
     * Action *
     **********/
    class NetworkCm02Action : public NetworkAction {
      friend Action *NetworkCm02Model::communicate(kernel::routing::NetCard *src, kernel::routing::NetCard *dst, double size, double rate);
      friend NetworkSmpiModel;
    public:
      NetworkCm02Action(Model *model, double cost, bool failed)
      : NetworkAction(model, cost, failed) {};
      ~NetworkCm02Action() override;
      void updateRemainingLazy(double now) override;
    protected:
      double senderGap_;
    };

  }
}

#endif /* SURF_NETWORK_CM02_HPP_ */
