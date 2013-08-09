#include "cpu.hpp"
#include "trace_mgr_private.h"
#include "surf/surf_routing.h"

/* Epsilon */
#define EPSILON 0.000000001

/***********
 * Classes *
 ***********/
class CpuTiModel;
typedef CpuTiModel *CpuTiModelPtr;

class CpuTi;
typedef CpuTi *CpuTiPtr;

class CpuTiAction;
typedef CpuTiAction *CpuTiActionPtr;

/*********
 * Trace *
 *********/
typedef struct surf_cpu_ti_trace {
  double *time_points;
  double *integral;
  int nb_points;
} s_surf_cpu_ti_trace_t, *surf_cpu_ti_trace_t;

enum trace_type { 
  TRACE_FIXED,                /*< Trace fixed, no availability file */
  TRACE_DYNAMIC               /*< Dynamic, availability file disponible */
};
typedef struct surf_cpu_ti_tgmr {
  trace_type type;

  double value;                 /*< Percentage of cpu power disponible. Value fixed between 0 and 1 */

  /* Dynamic */
  double last_time;             /*< Integral interval last point (discret time) */
  double total;                 /*< Integral total between 0 and last_pointn */

  surf_cpu_ti_trace_t trace;
  tmgr_trace_t power_trace;

} s_surf_cpu_ti_tgmr_t, *surf_cpu_ti_tgmr_t;


/*********
 * Model *
 *********/
class CpuTiModel : public CpuModel {
public:
  CpuTiModel();
  ~CpuTiModel() {}

  CpuTiPtr createResource(string name, double power_peak, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties);
  CpuTiActionPtr createAction(double cost, bool failed);

protected:
  void NotifyResourceTurnedOn(ResourcePtr r){};
  void NotifyResourceTurnedOff(ResourcePtr r){};

  void NotifyActionCancel(ActionPtr a){};
  void NotifyActionResume(ActionPtr a){};
  void NotifyActionSuspend(ActionPtr a){};
};

/************
 * Resource *
 ************/
class CpuTi : public Cpu {
public:
  CpuTi(CpuTiModelPtr model, string name, double powerPeak,
        double powerScale, tmgr_trace_t powerTrace, int core,
        e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
	xbt_dict_t properties) ;
  ~CpuTi() {};
  virtual double getSpeed (double load);
  virtual double getAvailableSpeed ();
  void printCpuTiModel();
  CpuTiModelPtr getModel();
  
  double m_powerPeak;            /*< CPU power peak */
  double m_powerScale;           /*< Percentage of CPU disponible */
  surf_cpu_ti_tgmr_t m_availTrace;       /*< Structure with data needed to integrate trace file */
  e_surf_resource_state_t m_stateCurrent;        /*< CPU current state (ON or OFF) */
  tmgr_trace_event_t m_stateEvent;       /*< trace file with states events (ON or OFF) */
  tmgr_trace_event_t m_powerEvent;       /*< trace file with availabitly events */
  std::vector<CpuTiActionPtr> m_actionSet;        /*< set with all actions running on cpu */
  s_xbt_swag_hookup_t m_modifiedCpuHookup;      /*< hookup to swag that indicacates whether share resources must be recalculated or not */
  double m_sumPriority;          /*< the sum of actions' priority that are running on cpu */
  double m_lastUpdate;           /*< last update of actions' remaining amount done */

};

/**********
 * Action *
 **********/
class CpuTiAction: public CpuAction {
public:
  CpuTiAction(CpuTiModelPtr model, double cost, bool failed): CpuAction(model, cost, failed) {};

  virtual double getRemains();
  virtual double getStartTime();
  virtual double getFinishTime();
};
