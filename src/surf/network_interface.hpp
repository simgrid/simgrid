/*
 * network_interface.hpp
 *
 *  Created on: Nov 29, 2013
 *      Author: bedaride
 */
#include "surf.hpp"
#include "surf_routing.hpp"

#ifndef SURF_NETWORK_INTERFACE_HPP_
#define SURF_NETWORK_INTERFACE_HPP_

/***********
 * Classes *
 ***********/
class NetworkModel;
typedef NetworkModel *NetworkModelPtr;

class NetworkLink;
typedef NetworkLink *NetworkLinkPtr;

class NetworkLinkLmm;
typedef NetworkLinkLmm *NetworkLinkLmmPtr;

class NetworkAction;
typedef NetworkAction *NetworkActionPtr;

class NetworkActionLmm;
typedef NetworkActionLmm *NetworkActionLmmPtr;

/*********
 * Tools *
 *********/

void net_define_callbacks(void);

/*********
 * Model *
 *********/
class NetworkModel : public Model {
public:
  NetworkModel() : Model("network") {
  };
  NetworkModel(string name) : Model(name) {
	f_networkSolve = lmm_solve;
	m_haveGap = false;
  };
  ~NetworkModel() {
	if (p_maxminSystem)
	  lmm_system_free(p_maxminSystem);
	if (p_actionHeap)
	  xbt_heap_free(p_actionHeap);
	if (p_modifiedSet)
	  xbt_swag_free(p_modifiedSet);
  }

  virtual NetworkLinkPtr createResource(const char *name,
                                   double bw_initial,
                                   tmgr_trace_t bw_trace,
                                   double lat_initial,
                                   tmgr_trace_t lat_trace,
                                   e_surf_resource_state_t state_initial,
                                   tmgr_trace_t state_trace,
                                   e_surf_link_sharing_policy_t policy,
                                   xbt_dict_t properties)=0;

  //FIXME:void updateActionsStateLazy(double now, double delta);
  virtual void gapAppend(double /*size*/, const NetworkLinkLmmPtr /*link*/, NetworkActionLmmPtr /*action*/) {};
  virtual ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                           double size, double rate)=0;
  virtual xbt_dynar_t getRoute(RoutingEdgePtr src, RoutingEdgePtr dst); //FIXME: kill field? That is done by the routing nowadays
  void (*f_networkSolve)(lmm_system_t);
  virtual double latencyFactor(double size);
  virtual double bandwidthFactor(double size);
  virtual double bandwidthConstraint(double rate, double bound, double size);
  bool m_haveGap;
};

/************
 * Resource *
 ************/

class NetworkLink : virtual public Resource {
public:
  NetworkLink() : p_latEvent(NULL) {};
  virtual double getBandwidth()=0;
  virtual double getLatency();
  virtual bool isShared()=0;

  /* Using this object with the public part of
    model does not make sense */
  double m_latCurrent;
  tmgr_trace_event_t p_latEvent;
};

class NetworkLinkLmm : public ResourceLmm, public NetworkLink {
public:
  NetworkLinkLmm() {};
  NetworkLinkLmm(lmm_system_t system,
	             double constraint_value,
	             tmgr_history_t history,
	             e_surf_resource_state_t state_init,
	             tmgr_trace_t state_trace,
	             double metric_peak,
	             tmgr_trace_t metric_trace)
 : ResourceLmm(system, constraint_value, history, state_init, state_trace, metric_peak, metric_trace)
{
}
  bool isShared();
  bool isUsed();
  double getBandwidth();
};

/**********
 * Action *
 **********/
class NetworkAction : virtual public Action {
public:
  NetworkAction() {};
  double m_latency;
  double m_latCurrent;
  double m_weight;
  double m_rate;
  const char* p_senderLinkName;
  double m_senderGap;
  double m_senderSize;
  xbt_fifo_item_t p_senderFifoItem;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  int m_latencyLimited;
#endif

};

class NetworkActionLmm : public ActionLmm, public NetworkAction {
public:
  NetworkActionLmm() {};
};

#endif /* SURF_NETWORK_INTERFACE_HPP_ */


