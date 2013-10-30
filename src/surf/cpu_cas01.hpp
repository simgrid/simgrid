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
  CpuCas01LmmPtr createResource(const char *name, xbt_dynar_t power_peak, int pstate,
		                  double power_scale,
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
/*
 * Energy-related properties for the cpu_cas01 model
 */
typedef struct energy_cpu_cas01 {
	xbt_dynar_t power_range_watts_list;		/*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
	double total_energy;					/*< Total energy consumed by the host */
	double last_updated;					/*< Timestamp of the last energy update event*/
} s_energy_cpu_cas01_t, *energy_cpu_cas01_t;

class CpuCas01Lmm : public CpuLmm {
public:
  CpuCas01Lmm(CpuCas01ModelPtr model, const char *name, xbt_dynar_t power_peak,
        int pstate, double powerScale, tmgr_trace_t powerTrace, int core,
        e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
	xbt_dict_t properties) ;
  ~CpuCas01Lmm();
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  ActionPtr execute(double size);
  ActionPtr sleep(double duration);

  xbt_dynar_t getWattsRangeList();
  double getCurrentWattsValue(double cpu_load);
  void updateEnergy(double cpu_load);

  double getCurrentPowerPeak();
  double getPowerPeakAt(int pstate_index);
  int getNbPstates();
  void setPowerPeakAt(int pstate_index);
  double getConsumedEnergy();

  bool isUsed();

  tmgr_trace_event_t p_powerEvent;

  xbt_dynar_t p_powerPeakList;				/*< List of supported CPU capacities */
  int m_pstate;								/*< Current pstate (index in the power_peak_list)*/
  energy_cpu_cas01_t p_energy;				/*< Structure with energy-consumption data */
};

/**********
 * Action *
 **********/
class CpuCas01ActionLmm: public CpuActionLmm {
public:
  CpuCas01ActionLmm() {};
  CpuCas01ActionLmm(ModelPtr model, double cost, bool failed): Action(model, cost, failed), CpuActionLmm(model, cost, failed) {};
  void updateEnergy();

};
