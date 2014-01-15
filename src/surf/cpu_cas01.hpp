#include "cpu_interface.hpp"

/***********
 * Classes *
 ***********/
class CpuCas01Model;
typedef CpuCas01Model *CpuCas01ModelPtr;

class CpuCas01;
typedef CpuCas01 *CpuCas01Ptr;

class CpuCas01Action;
typedef CpuCas01Action *CpuCas01ActionPtr;

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
  CpuPtr createResource(const char *name, xbt_dynar_t power_peak, int pstate,
		                  double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties);
  double shareResourcesFull(double now);  
  void addTraces();
  ActionListPtr p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
};

/************
 * Resource *
 ************/
/*
 * Energy-related properties for the cpu_cas01 model
 */
typedef struct energy_cpu_cas01 {
	xbt_dynar_t power_range_watts_list;		/*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
	double total_energy;					/*< Total energy consumed by the host */
	double last_updated;					/*< Timestamp of the last energy update event*/
} s_energy_cpu_cas01_t, *energy_cpu_cas01_t;

class CpuCas01 : public Cpu {
public://FIXME:
  tmgr_trace_event_t p_stateEvent;
public:
  CpuCas01(CpuCas01ModelPtr model, const char *name, xbt_dynar_t power_peak,
        int pstate, double powerScale, tmgr_trace_t powerTrace, int core,
        e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
	xbt_dict_t properties) ;
  ~CpuCas01();
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  CpuActionPtr execute(double size);
  CpuActionPtr sleep(double duration);

  double getCurrentPowerPeak();
  double getPowerPeakAt(int pstate_index);
  int getNbPstates();
  void setPowerPeakAt(int pstate_index);

  bool isUsed();

  tmgr_trace_event_t p_powerEvent;
  xbt_dynar_t p_powerPeakList;				/*< List of supported CPU capacities */
  int m_pstate;								/*< Current pstate (index in the power_peak_list)*/
};

/**********
 * Action *
 **********/
class CpuCas01Action: public CpuAction {
  friend CpuActionPtr CpuCas01::execute(double size);
  friend CpuActionPtr CpuCas01::sleep(double duration);
public:
  CpuCas01Action() {};
  CpuCas01Action(ModelPtr model, double cost, bool failed, double power, lmm_constraint_t constraint);

  ~CpuCas01Action() {};
};
