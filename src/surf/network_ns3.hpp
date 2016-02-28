/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "network_interface.hpp"
#include "src/surf/ns3/ns3_interface.h"

#ifndef NETWORK_NS3_HPP_
#define NETWORK_NS3_HPP_

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {

class XBT_PRIVATE NetworkNS3Model;
class XBT_PRIVATE NetworkNS3Action;

}
}

/*********
 * Tools *
 *********/

XBT_PRIVATE void net_define_callbacks(void);

/*********
 * Model *
 *********/

namespace simgrid {
namespace surf {

class NetworkNS3Model : public NetworkModel {
public:
  NetworkNS3Model();

  ~NetworkNS3Model();
  Link* createLink(const char *name,
      double bw_initial, tmgr_trace_t bw_trace,
      double lat_initial, tmgr_trace_t lat_trace,
      tmgr_trace_t state_trace,
      e_surf_link_sharing_policy_t policy,
      xbt_dict_t properties) override;
  Action *communicate(NetCard *src, NetCard *dst, double size, double rate);
  double next_occuring_event(double now) override;
  bool next_occuring_event_isIdempotent() {return false;}
  void updateActionsState(double now, double delta) override;
};

/************
 * Resource *
 ************/
class NetworkNS3Link : public Link {
public:
  NetworkNS3Link(NetworkNS3Model *model, const char *name, xbt_dict_t props,
               double bw_initial, double lat_initial);
  ~NetworkNS3Link();

  void apply_event(tmgr_trace_iterator_t event, double value) override;
  void updateBandwidth(double value) override {THROW_UNIMPLEMENTED;}
  void updateLatency(double value) override {THROW_UNIMPLEMENTED;}

//private:
 int m_created;
};

/**********
 * Action *
 **********/
class NetworkNS3Action : public NetworkAction {
public:
  NetworkNS3Action(Model *model, double cost, bool failed);

#ifdef HAVE_LATENCY_BOUND_TRACKING
  int getLatencyLimited();
#endif

bool isSuspended();
int unref();
void suspend();
void resume();

//private:
  double m_lastSent;
  NetCard *p_srcElm;
  NetCard *p_dstElm;
};

}
}

#endif /* NETWORK_NS3_HPP_ */
