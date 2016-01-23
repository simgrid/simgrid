/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "src/surf/cpu_interface.hpp"
#include "src/surf/trace_mgr.hpp"
#include "surf/surf_routing.h"

/* Epsilon */
#define EPSILON 0.000000001

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/
class XBT_PRIVATE CpuTiTrace;
class XBT_PRIVATE CpuTiTgmr;
class XBT_PRIVATE CpuTiModel;
class XBT_PRIVATE CpuTi;
class XBT_PRIVATE CpuTiAction;

struct tiTag;

/*********
 * Trace *
 *********/
class CpuTiTrace {
public:
  CpuTiTrace(tmgr_trace_t speedTrace);
  ~CpuTiTrace();

  double integrateSimple(double a, double b);
  double integrateSimplePoint(double a);
  double solveSimple(double a, double amount);

  double *p_timePoints;
  double *p_integral;
  int m_nbPoints;
  int binarySearch(double *array, double a, int low, int high);
};

enum trace_type {

  TRACE_FIXED,                /*< Trace fixed, no availability file */
  TRACE_DYNAMIC               /*< Dynamic, have an availability file */
};

class CpuTiTgmr {
public:
  CpuTiTgmr(trace_type type, double value)
    : m_type(type), m_value(value)
	{};
  CpuTiTgmr(tmgr_trace_t speedTrace, double value);
  ~CpuTiTgmr();

  double integrate(double a, double b);
  double solve(double a, double amount);
  double solveSomewhatSimple(double a, double amount);
  double getPowerScale(double a);

  trace_type m_type;
  double m_value;                 /*< Percentage of cpu speed available. Value fixed between 0 and 1 */

  /* Dynamic */
  double m_lastTime = 0.0;             /*< Integral interval last point (discrete time) */
  double m_total = 0.0;                 /*< Integral total between 0 and last_pointn */

  CpuTiTrace *p_trace = nullptr;
  tmgr_trace_t p_speedTrace = nullptr;
};

/**********
 * Action *
 **********/

class CpuTiAction: public CpuAction {
  friend class CpuTi;
public:
  CpuTiAction(CpuTiModel *model, double cost, bool failed,
                   CpuTi *cpu);

  void setState(e_surf_action_state_t state) override;
  int unref() override;
  void cancel() override;
  void updateIndexHeap(int i);
  void suspend() override;
  void resume() override;
  void setMaxDuration(double duration) override;
  void setPriority(double priority) override;
  double getRemains() override;
  void setAffinity(Cpu * /*cpu*/, unsigned long /*mask*/) override {};

  CpuTi *p_cpu;
  int m_indexHeap;
  int m_suspended = 0;
public:
  boost::intrusive::list_member_hook<> action_ti_hook;
};

typedef boost::intrusive::member_hook<
  CpuTiAction, boost::intrusive::list_member_hook<>, &CpuTiAction::action_ti_hook> ActionTiListOptions;
typedef boost::intrusive::list<
  CpuTiAction, ActionTiListOptions > ActionTiList;

/************
 * Resource *
 ************/
class CpuTi : public Cpu {
public:
  CpuTi(CpuTiModel *model, simgrid::s4u::Host *host, xbt_dynar_t speedPeak,
        int pstate, double speedScale, tmgr_trace_t speedTrace, int core,
        int initiallyOn, tmgr_trace_t stateTrace) ;
  ~CpuTi();

  void updateState(tmgr_trace_iterator_t event_type, double value, double date) override;
  void updateActionsFinishTime(double now);
  bool isUsed() override;
  CpuAction *execute(double size) override;
  CpuAction *sleep(double duration) override;
  double getAvailableSpeed() override;

  void modified(bool modified);

  CpuTiTgmr *p_availTrace;       /*< Structure with data needed to integrate trace file */
  tmgr_trace_iterator_t p_stateEvent = NULL; /*< trace file with states events (ON or OFF) */
  tmgr_trace_iterator_t p_speedEvent = NULL; /*< trace file with availability events */
  ActionTiList *p_actionSet;        /*< set with all actions running on cpu */
  double m_sumPriority;          /*< the sum of actions' priority that are running on cpu */
  double m_lastUpdate = 0;       /*< last update of actions' remaining amount done */

  double current_frequency;

  void updateRemainingAmount(double now);
public:
  boost::intrusive::list_member_hook<> cpu_ti_hook;
};

typedef boost::intrusive::member_hook<
  CpuTi, boost::intrusive::list_member_hook<>, &CpuTi::cpu_ti_hook> CpuTiListOptions;
typedef boost::intrusive::list<CpuTi, CpuTiListOptions> CpuTiList;

/*********
 * Model *
 *********/
class CpuTiModel : public CpuModel {
public:
  CpuTiModel();
  ~CpuTiModel();
  Cpu *createCpu(simgrid::s4u::Host *host,  xbt_dynar_t speedPeak,
                          int pstate, double speedScale,
                          tmgr_trace_t speedTrace, int core,
                          int initiallyOn,
                          tmgr_trace_t state_trace) override;
  double shareResources(double now) override;
  void updateActionsState(double now, double delta) override;
  void addTraces() override;

  ActionList *p_runningActionSetThatDoesNotNeedBeingChecked;
  CpuTiList *p_modifiedCpu;
  xbt_heap_t p_tiActionHeap;

protected:
  void NotifyResourceTurnedOn(simgrid::surf::Resource*){};
  void NotifyResourceTurnedOff(simgrid::surf::Resource*){};

  void NotifyActionCancel(Action*){};
  void NotifyActionResume(Action*){};
  void NotifyActionSuspend(Action*){};
};

}
}
