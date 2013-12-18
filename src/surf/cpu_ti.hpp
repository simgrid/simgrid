#include "cpu_interface.hpp"
#include "trace_mgr_private.h"
#include "surf/surf_routing.h"

/* Epsilon */
#define EPSILON 0.000000001

/***********
 * Classes *
 ***********/
class CpuTiTrace;
typedef CpuTiTrace *CpuTiTracePtr;

class CpuTiTgmr;
typedef CpuTiTgmr *CpuTiTgmrPtr;

class CpuTiModel;
typedef CpuTiModel *CpuTiModelPtr;

class CpuTi;
typedef CpuTi *CpuTiPtr;

class CpuTiAction;
typedef CpuTiAction *CpuTiActionPtr;

/*********
 * Trace *
 *********/
class CpuTiTrace {
public:
  CpuTiTrace(tmgr_trace_t powerTrace);
  ~CpuTiTrace();

  double integrateSimple(double a, double b);
  double integrateSimplePoint(double a);
  double solveSimple(double a, double amount);

  double *p_timePoints;
  double *p_integral;
  int m_nbPoints;
  int binarySearch(double *array, double a, int low, int high);

private:
};

enum trace_type {
  
  TRACE_FIXED,                /*< Trace fixed, no availability file */
  TRACE_DYNAMIC               /*< Dynamic, availability file disponible */
};

class CpuTiTgmr {
public:
  CpuTiTgmr(trace_type type, double value): m_type(type), m_value(value){};
  CpuTiTgmr(tmgr_trace_t power_trace, double value);
  ~CpuTiTgmr();

  double integrate(double a, double b);
  double solve(double a, double amount);
  double solveSomewhatSimple(double a, double amount);
  double getPowerScale(double a);

  trace_type m_type;
  double m_value;                 /*< Percentage of cpu power disponible. Value fixed between 0 and 1 */

  /* Dynamic */
  double m_lastTime;             /*< Integral interval last point (discret time) */
  double m_total;                 /*< Integral total between 0 and last_pointn */

  CpuTiTracePtr p_trace;
  tmgr_trace_t p_powerTrace;
};


/*********
 * Model *
 *********/
class CpuTiModel : public CpuModel {
public:
  CpuTiModel();
  ~CpuTiModel();

  void parseInit(sg_platf_host_cbarg_t host);
  CpuTiPtr createResource(const char *name,  xbt_dynar_t powerPeak,
                          int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties);
  CpuTiActionPtr createAction(double cost, bool failed);
  double shareResources(double now);
  void updateActionsState(double now, double delta);
  void addTraces();

  ActionListPtr p_runningActionSetThatDoesNotNeedBeingChecked;
  xbt_swag_t p_modifiedCpu;
  xbt_heap_t p_tiActionHeap;

protected:
  void NotifyResourceTurnedOn(ResourcePtr){};
  void NotifyResourceTurnedOff(ResourcePtr){};

  void NotifyActionCancel(ActionPtr){};
  void NotifyActionResume(ActionPtr){};
  void NotifyActionSuspend(ActionPtr){};
};

/************
 * Resource *
 ************/
class CpuTi : public Cpu {
public:
  CpuTi() {};
  CpuTi(CpuTiModelPtr model, const char *name, xbt_dynar_t powerPeak,
        int pstate, double powerScale, tmgr_trace_t powerTrace, int core,
        e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
	xbt_dict_t properties) ;
  ~CpuTi();

  void updateState(tmgr_trace_event_t event_type, double value, double date);  
  void updateActionsFinishTime(double now);
  bool isUsed();
  void printCpuTiModel();
  CpuActionPtr execute(double size);
  CpuActionPtr sleep(double duration);
  double getAvailableSpeed();

  xbt_dynar_t getWattsRangeList() {THROW_UNIMPLEMENTED;};
  double getCurrentWattsValue(double /*cpu_load*/) {THROW_UNIMPLEMENTED;};
  void updateEnergy(double /*cpu_load*/) {THROW_UNIMPLEMENTED;};

  double getCurrentPowerPeak() {THROW_UNIMPLEMENTED;};
  double getPowerPeakAt(int /*pstate_index*/) {THROW_UNIMPLEMENTED;};
  int getNbPstates() {THROW_UNIMPLEMENTED;};
  void setPowerPeakAt(int /*pstate_index*/) {THROW_UNIMPLEMENTED;};
  double getConsumedEnergy() {THROW_UNIMPLEMENTED;};

  CpuTiTgmrPtr p_availTrace;       /*< Structure with data needed to integrate trace file */
  tmgr_trace_event_t p_stateEvent;       /*< trace file with states events (ON or OFF) */
  tmgr_trace_event_t p_powerEvent;       /*< trace file with availability events */
  xbt_swag_t p_actionSet;        /*< set with all actions running on cpu */
  s_xbt_swag_hookup_t p_modifiedCpuHookup;      /*< hookup to swag that indicates whether share resources must be recalculated or not */
  double m_sumPriority;          /*< the sum of actions' priority that are running on cpu */
  double m_lastUpdate;           /*< last update of actions' remaining amount done */

  int m_pstate;								/*< Current pstate (index in the power_peak_list)*/
  double current_frequency;

  void updateRemainingAmount(double now);
};

/**********
 * Action *
 **********/

class CpuTiAction: public CpuAction {
  friend CpuActionPtr CpuTi::execute(double size);
  friend CpuActionPtr CpuTi::sleep(double duration);
  friend void CpuTi::updateActionsFinishTime(double now);//FIXME
  friend void CpuTi::updateRemainingAmount(double now);//FIXME

public:
  CpuTiAction() {};
  CpuTiAction(CpuTiModelPtr model, double cost, bool failed,
  		                 CpuTiPtr cpu);

  void setState(e_surf_action_state_t state);
  int unref();
  void cancel();
  void recycle();
  void updateIndexHeap(int i);
  void suspend();
  void resume();
  bool isSuspended();
  void setMaxDuration(double duration);
  void setPriority(double priority);
  double getRemains();
  void setAffinity(CpuPtr /*cpu*/, unsigned long /*mask*/) {};
  void setBound(double /*bound*/) {};
  void updateEnergy() {};

  CpuTiPtr p_cpu;
  int m_indexHeap;
  s_xbt_swag_hookup_t p_cpuListHookup;
  int m_suspended;
private:
};
