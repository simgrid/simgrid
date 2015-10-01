/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "cpu_interface.hpp"
#include "trace_mgr_private.h"
#include "surf/surf_routing.h"

/* Epsilon */
#define EPSILON 0.000000001

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

  CpuTiTrace *p_trace;
  tmgr_trace_t p_powerTrace;
};

/**********
 * Action *
 **********/

class CpuTiAction: public CpuAction {
  friend class CpuTi;
  // friend CpuAction *CpuTi::execute(double size);
  // friend CpuAction *CpuTi::sleep(double duration);
  // friend void CpuTi::updateActionsFinishTime(double now);//FIXME
  // friend void CpuTi::updateRemainingAmount(double now);//FIXME
public:
  CpuTiAction(CpuTiModel *model, double cost, bool failed,
                   CpuTi *cpu);

  void setState(e_surf_action_state_t state);
  int unref();
  void cancel();
  void updateIndexHeap(int i);
  void suspend();
  void resume();
  bool isSuspended();
  void setMaxDuration(double duration);
  void setPriority(double priority);
  double getRemains();
  void setAffinity(Cpu * /*cpu*/, unsigned long /*mask*/) {};

  CpuTi *p_cpu;
  int m_indexHeap;
  int m_suspended;
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
  CpuTi() {};
  CpuTi(CpuTiModel *model, const char *name, xbt_dynar_t powerPeak,
        int pstate, double powerScale, tmgr_trace_t powerTrace, int core,
        e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
	xbt_dict_t properties) ;
  ~CpuTi();

  void updateState(tmgr_trace_event_t event_type, double value, double date);
  void updateActionsFinishTime(double now);
  bool isUsed();
  void printCpuTiModel();
  CpuAction *execute(double size);
  CpuAction *sleep(double duration);
  double getAvailableSpeed();

  double getCurrentPowerPeak() {THROW_UNIMPLEMENTED;};
  double getPowerPeakAt(int /*pstate_index*/) {THROW_UNIMPLEMENTED;};
  int getNbPstates() {THROW_UNIMPLEMENTED;};
  void setPstate(int /*pstate_index*/) {THROW_UNIMPLEMENTED;};
  int  getPstate() { THROW_UNIMPLEMENTED;}
  void modified(bool modified);

  CpuTiTgmr *p_availTrace;       /*< Structure with data needed to integrate trace file */
  tmgr_trace_event_t p_stateEvent;       /*< trace file with states events (ON or OFF) */
  tmgr_trace_event_t p_powerEvent;       /*< trace file with availability events */
  ActionTiList *p_actionSet;        /*< set with all actions running on cpu */
  double m_sumPriority;          /*< the sum of actions' priority that are running on cpu */
  double m_lastUpdate;           /*< last update of actions' remaining amount done */

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
  Cpu *createCpu(const char *name,  xbt_dynar_t powerPeak,
                          int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties);
  double shareResources(double now);
  void updateActionsState(double now, double delta);
  void addTraces();

  ActionList *p_runningActionSetThatDoesNotNeedBeingChecked;
  CpuTiList *p_modifiedCpu;
  xbt_heap_t p_tiActionHeap;

protected:
  void NotifyResourceTurnedOn(Resource*){};
  void NotifyResourceTurnedOff(Resource*){};

  void NotifyActionCancel(Action*){};
  void NotifyActionResume(Action*){};
  void NotifyActionSuspend(Action*){};
};
