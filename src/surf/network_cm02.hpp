#include "network_interface.hpp"
#include "xbt/fifo.h"
#include "xbt/graph.h"

#ifndef SURF_NETWORK_CM02_HPP_
#define SURF_NETWORK_CM02_HPP_

/***********
 * Classes *
 ***********/
class NetworkCm02Model;
typedef NetworkCm02Model *NetworkCm02ModelPtr;

class NetworkCm02LinkLmm;
typedef NetworkCm02LinkLmm *NetworkCm02LinkLmmPtr;

class NetworkCm02ActionLmm;
typedef NetworkCm02ActionLmm *NetworkCm02ActionLmmPtr;

/*********
 * Tools *
 *********/

void net_define_callbacks(void);

/*********
 * Model *
 *********/
class NetworkCm02Model : public NetworkModel {
private:
  void initialize();
public:
  NetworkCm02Model(int /*i*/) : NetworkModel("network") {
	f_networkSolve = lmm_solve;
	m_haveGap = false;
  };//FIXME: add network clean interface
  NetworkCm02Model(string name) : NetworkModel(name) {
    this->initialize();
  }
  NetworkCm02Model() : NetworkModel("network") {
    this->initialize();
  }
  ~NetworkCm02Model() {
  }
  NetworkLinkPtr createResource(const char *name,
                                   double bw_initial,
                                   tmgr_trace_t bw_trace,
                                   double lat_initial,
                                   tmgr_trace_t lat_trace,
                                   e_surf_resource_state_t state_initial,
                                   tmgr_trace_t state_trace,
                                   e_surf_link_sharing_policy_t policy,
                                   xbt_dict_t properties);
  void updateActionsStateLazy(double now, double delta);
  void gapAppend(double /*size*/, const NetworkCm02LinkLmmPtr /*link*/, NetworkCm02ActionLmmPtr /*action*/) {};
  ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                           double size, double rate);
};

/************
 * Resource *
 ************/

class NetworkCm02LinkLmm : public NetworkLinkLmm {
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
  void updateState(tmgr_trace_event_t event_type, double value, double date);
};


/**********
 * Action *
 **********/

class NetworkCm02ActionLmm : public NetworkActionLmm {
public:
  NetworkCm02ActionLmm(ModelPtr model, double cost, bool failed)
 : Action(model, cost, failed) {};
  void updateRemainingLazy(double now);
  void recycle();
};

#endif /* SURF_NETWORK_CM02_HPP_ */
