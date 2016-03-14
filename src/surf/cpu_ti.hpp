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

  double *timePoints_;
  double *integral_;
  int nbPoints_;
  int binarySearch(double *array, double a, int low, int high);
};

enum trace_type {

  TRACE_FIXED,                /*< Trace fixed, no availability file */
  TRACE_DYNAMIC               /*< Dynamic, have an availability file */
};

class CpuTiTgmr {
public:
  CpuTiTgmr(trace_type type, double value)
    : type_(type), value_(value)
  {};
  CpuTiTgmr(tmgr_trace_t speedTrace, double value);
  ~CpuTiTgmr();

  double integrate(double a, double b);
  double solve(double a, double amount);
  double solveSomewhatSimple(double a, double amount);
  double getPowerScale(double a);

  trace_type type_;
  double value_;                 /*< Percentage of cpu speed available. Value fixed between 0 and 1 */

  /* Dynamic */
  double lastTime_ = 0.0;             /*< Integral interval last point (discrete time) */
  double total_    = 0.0;             /*< Integral total between 0 and last_pointn */

  CpuTiTrace *trace_ = nullptr;
  tmgr_trace_t speedTrace_ = nullptr;
};

/**********
 * Action *
 **********/

class CpuTiAction: public CpuAction {
  friend class CpuTi;
public:
  CpuTiAction(CpuTiModel *model, double cost, bool failed, CpuTi *cpu);

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

  CpuTi *cpu_;
  int indexHeap_;
  int suspended_ = 0;
public:
  boost::intrusive::list_member_hook<> action_ti_hook;
};

typedef boost::intrusive::member_hook<CpuTiAction, boost::intrusive::list_member_hook<>, &CpuTiAction::action_ti_hook> ActionTiListOptions;
typedef boost::intrusive::list<CpuTiAction, ActionTiListOptions > ActionTiList;

/************
 * Resource *
 ************/
class CpuTi : public Cpu {
public:
  CpuTi(CpuTiModel *model, simgrid::s4u::Host *host, xbt_dynar_t speedPeak,
        tmgr_trace_t speedTrace, int core,
        tmgr_trace_t stateTrace) ;
  ~CpuTi();

  void setSpeedTrace(tmgr_trace_t trace) override;

  void apply_event(tmgr_trace_iterator_t event, double value) override;
  void updateActionsFinishTime(double now);
  void updateRemainingAmount(double now);

  bool isUsed() override;
  CpuAction *execution_start(double size) override;
  CpuAction *sleep(double duration) override;
  double getAvailableSpeed() override;

  void modified(bool modified);

  CpuTiTgmr *availTrace_;       /*< Structure with data needed to integrate trace file */
  ActionTiList *actionSet_;        /*< set with all actions running on cpu */
  double sumPriority_;          /*< the sum of actions' priority that are running on cpu */
  double lastUpdate_ = 0;       /*< last update of actions' remaining amount done */

  double currentFrequency_;

public:
  boost::intrusive::list_member_hook<> cpu_ti_hook;
};

typedef boost::intrusive::member_hook<CpuTi, boost::intrusive::list_member_hook<>, &CpuTi::cpu_ti_hook> CpuTiListOptions;
typedef boost::intrusive::list<CpuTi, CpuTiListOptions> CpuTiList;

/*********
 * Model *
 *********/
class CpuTiModel : public CpuModel {
public:
  CpuTiModel();
  ~CpuTiModel();
  Cpu *createCpu(simgrid::s4u::Host *host,  xbt_dynar_t speedPeak,
      tmgr_trace_t speedTrace, int core, tmgr_trace_t state_trace) override;
  double next_occuring_event(double now) override;
  void updateActionsState(double now, double delta) override;

  ActionList *runningActionSetThatDoesNotNeedBeingChecked_;
  CpuTiList *modifiedCpu_;
  xbt_heap_t tiActionHeap_;

protected:
  void NotifyResourceTurnedOn(simgrid::surf::Resource*){};
  void NotifyResourceTurnedOff(simgrid::surf::Resource*){};

  void NotifyActionCancel(Action*){};
  void NotifyActionResume(Action*){};
  void NotifyActionSuspend(Action*){};
};

}
}
