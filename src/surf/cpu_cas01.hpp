#include "cpu.hpp"

/***********
 * Classes *
 ***********/
class CpuCas01Model;
typedef CpuCas01Model *CpuCas01ModelPtr;

class CpuCas01Lmm;
typedef CpuCas01Lmm *CpuCas01LmmPtr;

class CpuCas01ActionLmm;
typedef CpuCas01ActionLmm *CpuCas01ActionLmmPtr;

/*********
 * Model *
 *********/
class CpuCas01Model : public CpuModel {
public:
  CpuCas01Model();
  ~CpuCas01Model();

  double (CpuCas01Model::*shareResources)(double now);
  void (CpuCas01Model::*updateActionsState)(double now, double delta);

  void parseInit(sg_platf_host_cbarg_t host);  
  CpuCas01LmmPtr createResource(const char *name, double power_peak, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties);
  double shareResourcesFull(double now);  
  void addTraces();
};

/************
 * Resource *
 ************/
class CpuCas01Lmm : public CpuLmm {
public:
  CpuCas01Lmm(CpuCas01ModelPtr model, const char *name, double powerPeak,
        double powerScale, tmgr_trace_t powerTrace, int core,
        e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
	xbt_dict_t properties) ;
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  ActionPtr execute(double size);
  ActionPtr sleep(double duration);

  bool isUsed();

  tmgr_trace_event_t p_powerEvent;
};

/**********
 * Action *
 **********/
class CpuCas01ActionLmm: public CpuActionLmm {
public:
  CpuCas01ActionLmm() {};
  CpuCas01ActionLmm(ModelPtr model, double cost, bool failed): Action(model, cost, failed), CpuActionLmm(model, cost, failed) {};

};
