#include "network.hpp"

/***********
 * Classes *
 ***********/

class NetworkSmpiModel;
typedef NetworkSmpiModel *NetworkSmpiModelPtr;

class NetworkSmpiLink;
typedef NetworkSmpiLink *NetworkSmpiLinkPtr;

class NetworkSmpiLinkLmm;
typedef NetworkSmpiLinkLmm *NetworkSmpiLinkLmmPtr;

class NetworkSmpiAction;
typedef NetworkSmpiAction *NetworkSmpiActionPtr;

class NetworkSmpiActionLmm;
typedef NetworkSmpiActionLmm *NetworkSmpiActionLmmPtr;

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/

class NetworkSmpiModel : public NetworkCm02Model {
public:
  NetworkSmpiModel(){};
  void gapAppend(double size, const NetworkCm02LinkLmmPtr link, NetworkCm02ActionLmmPtr action);
  void gapRemove(ActionLmmPtr action);
  double latencyFactor(double size);
  double bandwidthFactor(double size);
  double bandwidthConstraint(double rate, double bound, double size);
  void communicateCallBack() {};
};


/************
 * Resource *
 ************/

class NetworkSmpiLinkLmm : public NetworkCm02LinkLmm {
public:
  NetworkSmpiLinkLmm(NetworkSmpiModelPtr model, const char *name, xbt_dict_t props,
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
};


/**********
 * Action *
 **********/

class NetworkSmpiActionLmm : public NetworkCm02ActionLmm {
public:
  NetworkSmpiActionLmm(ModelPtr model, double cost, bool failed);
};


