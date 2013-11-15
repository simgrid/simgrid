#include "network.hpp"

#ifndef NETWORK_CONSTANT_HPP_
#define NETWORK_CONSTANT_HPP_

/***********
 * Classes *
 ***********/
class NetworkConstantModel;
typedef NetworkConstantModel *NetworkConstantModelPtr;

class NetworkConstantLinkLmm;
typedef NetworkConstantLinkLmm *NetworkConstantLinkLmmPtr;

class NetworkConstantActionLmm;
typedef NetworkConstantActionLmm *NetworkConstantActionLmmPtr;

/*********
 * Model *
 *********/
class NetworkConstantModel : public NetworkCm02Model {
public:
  NetworkConstantModel() : NetworkCm02Model("constant time network") {};
  NetworkCm02LinkLmmPtr createResource(string name);
  double shareResources(double now);
  void updateActionsState(double now, double delta);
  ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                           double size, double rate);
  void gapRemove(ActionLmmPtr action);
  //FIXME:virtual void addTraces() =0;
};

/************
 * Resource *
 ************/
class NetworkConstantLinkLmm : public NetworkCm02LinkLmm {
public:
  NetworkConstantLinkLmm(NetworkCm02ModelPtr model, const char* name, xbt_dict_t properties);
  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  double getBandwidth();
  double getLatency();
  bool isShared();
};

/**********
 * Action *
 **********/
class NetworkConstantActionLmm : public NetworkCm02ActionLmm {
public:
  NetworkConstantActionLmm(NetworkConstantModelPtr model, double latency):
	  Action(model, 0, false), NetworkCm02ActionLmm(model, 0, false), m_latInit(latency) {
	m_latency = latency;
	if (m_latency <= 0.0) {
	  p_stateSet = p_model->p_doneActionSet;
	  xbt_swag_insert(static_cast<ActionPtr>(this), p_stateSet);
	}
  };
  int unref();
  void recycle();
  void cancel();
  void setCategory(const char *category);
  void suspend();
  void resume();
  bool isSuspended();
  double m_latInit;
  int m_suspended;
};

#endif /* NETWORK_CONSTANT_HPP_ */
