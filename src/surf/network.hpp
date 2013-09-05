#include "surf.hpp"
#include "xbt/fifo.h"

#ifndef SURF_MODEL_NETWORK_H_
#define SURF_MODEL_NETWORK_H_

/***********
 * Classes *
 ***********/
class NetworkModel;
typedef NetworkModel *NetworkModelPtr;

class NetworkCm02Link;
typedef NetworkCm02Link *NetworkCm02LinkPtr;

class NetworkCm02Action;
typedef NetworkCm02Action *NetworkCm02ActionPtr;

class NetworkCm02ActionLmm;
typedef NetworkCm02ActionLmm *NetworkCm02ActionLmmPtr;

/*********
 * Model *
 *********/
class NetworkModel : public Model {
public:
  NetworkModel(string name) : Model(name) {};
  NetworkCm02LinkPtr createResource(string name);
  void updateActionsStateLazy(double now, double delta);
  void updateActionsStateFull(double now, double delta);  
  void gapRemove(ActionLmmPtr action);

  virtual void addTraces() =0;
};

/************
 * Resource *
 ************/
class NetworkCm02Link : public Resource {
public:
  NetworkCm02Link(NetworkModelPtr model, const char* name, xbt_dict_t properties) : Resource(model, name, properties) {};

  /* Using this object with the public part of
    model does not make sense */
  double lat_current;
  tmgr_trace_event_t lat_event;
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
};

#endif /* SURF_MODEL_NETWORK_H_ */
