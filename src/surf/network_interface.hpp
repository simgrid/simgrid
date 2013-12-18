/*
 * network_interface.hpp
 *
 *  Created on: Nov 29, 2013
 *      Author: bedaride
 */
#include "surf_interface.hpp"
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

class NetworkAction;
typedef NetworkAction *NetworkActionPtr;

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
  NetworkModel(const char *name) : Model(name) {
	f_networkSolve = lmm_solve;
	m_haveGap = false;
  };
  ~NetworkModel() {
	if (p_maxminSystem)
	  lmm_system_free(p_maxminSystem);
	if (p_actionHeap)
	  xbt_heap_free(p_actionHeap);
	if (p_modifiedSet)
	  delete p_modifiedSet;
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

  virtual void gapAppend(double /*size*/, const NetworkLinkPtr /*link*/, NetworkActionPtr /*action*/) {};
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

class NetworkLink : public Resource {
public:
  NetworkLink(NetworkModelPtr model, const char *name, xbt_dict_t props);
  NetworkLink(NetworkModelPtr model, const char *name, xbt_dict_t props,
  		      lmm_constraint_t constraint,
  	          tmgr_history_t history,
  	          tmgr_trace_t state_trace);
  virtual double getBandwidth();
  virtual double getLatency();
  virtual bool isShared();
  bool isUsed();

  /* Using this object with the public part of
    model does not make sense */
  double m_latCurrent;
  tmgr_trace_event_t p_latEvent;

  /* LMM */
  tmgr_trace_event_t p_stateEvent;
  s_surf_metric_t p_power;
};

/**********
 * Action *
 **********/
class NetworkAction : public Action {
public:
  NetworkAction(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed) {}
  NetworkAction(ModelPtr model, double cost, bool failed, lmm_variable_t var)
  : Action(model, cost, failed, var) {};
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

#endif /* SURF_NETWORK_INTERFACE_HPP_ */


