/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <boost/intrusive/list.hpp>

#include <xbt/base.h>

#include "src/surf/cpu_interface.hpp"
#include "src/surf/trace_mgr.hpp"

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
  explicit CpuTiTrace(tmgr_trace_t speedTrace);
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

  void setState(simgrid::surf::Action::State state) override;
  int unref() override;
  void cancel() override;
  void suspend() override;
  void resume() override;
  void setMaxDuration(double duration) override;
  void setSharingWeight(double priority) override;
  double getRemains() override;

  CpuTi *cpu_;

  boost::intrusive::list_member_hook<> action_ti_hook;
};

typedef boost::intrusive::member_hook<CpuTiAction, boost::intrusive::list_member_hook<>, &CpuTiAction::action_ti_hook> ActionTiListOptions;
typedef boost::intrusive::list<CpuTiAction, ActionTiListOptions > ActionTiList;

/************
 * Resource *
 ************/
class CpuTi : public Cpu {
public:
  CpuTi(CpuTiModel *model, simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core);
  ~CpuTi() override;

  void setSpeedTrace(tmgr_trace_t trace) override;

  void apply_event(tmgr_trace_event_t event, double value) override;
  void updateActionsFinishTime(double now);
  void updateRemainingAmount(double now);

  bool isUsed() override;
  CpuAction *execution_start(double size) override;
  CpuAction *sleep(double duration) override;
  double getAvailableSpeed() override;

  void modified(bool modified);

  CpuTiTgmr *speedIntegratedTrace_ = nullptr;/*< Structure with data needed to integrate trace file */
  ActionTiList actionSet_;                   /*< set with all actions running on cpu */
  double sumPriority_ = 0; /*< the sum of actions' priority that are running on cpu */
  double lastUpdate_ = 0;  /*< last update of actions' remaining amount done */

  double currentFrequency_;

  boost::intrusive::list_member_hook<> cpu_ti_hook;
};

typedef boost::intrusive::member_hook<CpuTi, boost::intrusive::list_member_hook<>, &CpuTi::cpu_ti_hook> CpuTiListOptions;
typedef boost::intrusive::list<CpuTi, CpuTiListOptions> CpuTiList;

/*********
 * Model *
 *********/
class CpuTiModel : public CpuModel {
public:
  CpuTiModel() = default;
  ~CpuTiModel() override;
  Cpu *createCpu(simgrid::s4u::Host *host,  std::vector<double>* speedPerPstate, int core) override;
  double nextOccuringEvent(double now) override;
  void updateActionsState(double now, double delta) override;

  ActionList runningActionSetThatDoesNotNeedBeingChecked_;
  CpuTiList modifiedCpu_;
};

}
}
