#include "surf.hpp"
#include "xbt/fifo.h"
#include "xbt/graph.h"
#include "surf_routing.hpp"

#ifndef SURF_MODEL_NETWORK_H_
#define SURF_MODEL_NETWORK_H_

/***********
 * Classes *
 ***********/
class NetworkCm02Model;
typedef NetworkCm02Model *NetworkCm02ModelPtr;

class NetworkCm02Link;
typedef NetworkCm02Link *NetworkCm02LinkPtr;

class NetworkCm02LinkLmm;
typedef NetworkCm02LinkLmm *NetworkCm02LinkLmmPtr;

class NetworkCm02Action;
typedef NetworkCm02Action *NetworkCm02ActionPtr;

class NetworkCm02ActionLmm;
typedef NetworkCm02ActionLmm *NetworkCm02ActionLmmPtr;

/*********
 * Tools *
 *********/
extern NetworkCm02ModelPtr surf_network_model;



/*********
 * Model *
 *********/
class NetworkCm02Model : public Model {
public:
  NetworkCm02Model(string name);
  NetworkCm02Model();
  //FIXME:NetworkCm02LinkPtr createResource(string name);
  NetworkCm02LinkLmmPtr createResource(const char *name,
                                   double bw_initial,
                                   tmgr_trace_t bw_trace,
                                   double lat_initial,
                                   tmgr_trace_t lat_trace,
                                   e_surf_resource_state_t state_initial,
                                   tmgr_trace_t state_trace,
                                   e_surf_link_sharing_policy_t policy,
                                   xbt_dict_t properties);
  void updateActionsStateLazy(double now, double delta);
  void updateActionsStateFull(double now, double delta);
  virtual void gapAppend(double size, const NetworkCm02LinkLmmPtr link, NetworkCm02ActionLmmPtr action) {};
  NetworkCm02ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                           double size, double rate);
  xbt_dynar_t getRoute(RoutingEdgePtr src, RoutingEdgePtr dst); //FIXME: kill field? That is done by the routing nowadays
  //FIXME: virtual void addTraces() =0;
  void (*f_networkSolve)(lmm_system_t) = lmm_solve;
  double latencyFactor(double size);
  double bandwidthFactor(double size);
  double bandwidthConstraint(double rate, double bound, double size);
  bool m_haveGap = false;
};

/************
 * Resource *
 ************/

class NetworkCm02Link : virtual public Resource {
public:
  NetworkCm02Link(){};
  NetworkCm02Link(NetworkCm02ModelPtr model, const char* name, xbt_dict_t properties) : Resource(model, name, properties) {};
  virtual double getBandwidth()=0;
  double getLatency();
  virtual bool isShared()=0;
  /* Using this object with the public part of
    model does not make sense */
  double m_latCurrent;
  tmgr_trace_event_t p_latEvent;
};

class NetworkCm02LinkLmm : public ResourceLmm, public NetworkCm02Link {
public:
  NetworkCm02LinkLmm(NetworkCm02ModelPtr model, const char *name, xbt_dict_t props,
	                           lmm_system_t system,
	                           double constraint_value,
	                           tmgr_history_t history,
	                           e_surf_resource_state_t state_init,
	                           tmgr_trace_t state_trace,
	                           double metric_peak,
	                           tmgr_trace_t metric_trace,
	                           double lat_initial,
	                           tmgr_trace_t lat_trace,
                               e_surf_link_sharing_policy_t policy);
  bool isShared();
  bool isUsed();
  double getBandwidth();
  void updateState(tmgr_trace_event_t event_type, double value, double date);
};


/**********
 * Action *
 **********/
class NetworkCm02Action : virtual public Action {
public:
  NetworkCm02Action(ModelPtr model, double cost, bool failed): Action(model, cost, failed) {};
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

class NetworkCm02ActionLmm : public ActionLmm, public NetworkCm02Action {
public:
  NetworkCm02ActionLmm(ModelPtr model, double cost, bool failed): ActionLmm(model, cost, failed), NetworkCm02Action(model, cost, failed) {};
  void updateRemainingLazy(double now);
  void recycle();
};

#endif /* SURF_MODEL_NETWORK_H_ */
