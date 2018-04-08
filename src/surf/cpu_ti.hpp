/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

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
class XBT_PRIVATE CpuTiModel;
class XBT_PRIVATE CpuTi;

/*********
 * Trace *
 *********/
class CpuTiTrace {
public:
  explicit CpuTiTrace(tmgr_trace_t speedTrace);
  CpuTiTrace(const CpuTiTrace&) = delete;
  CpuTiTrace& operator=(const CpuTiTrace&) = delete;
  ~CpuTiTrace();

  double integrate_simple(double a, double b);
  double integrate_simple_point(double a);
  double solve_simple(double a, double amount);

  double* time_points_;
  double *integral_;
  int nb_points_;
  int binary_search(double* array, double a, int low, int high);
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
  CpuTiTgmr(const CpuTiTgmr&) = delete;
  CpuTiTgmr& operator=(const CpuTiTgmr&) = delete;
  ~CpuTiTgmr();

  double integrate(double a, double b);
  double solve(double a, double amount);
  double get_power_scale(double a);

  trace_type type_;
  double value_;                 /*< Percentage of cpu speed available. Value fixed between 0 and 1 */

  /* Dynamic */
  double last_time_ = 0.0;             /*< Integral interval last point (discrete time) */
  double total_    = 0.0;             /*< Integral total between 0 and last_pointn */

  CpuTiTrace *trace_ = nullptr;
  tmgr_trace_t speed_trace_ = nullptr;
};

/**********
 * Action *
 **********/

class XBT_PRIVATE CpuTiAction : public CpuAction {
  friend class CpuTi;
public:
  CpuTiAction(CpuTiModel *model, double cost, bool failed, CpuTi *cpu);
  ~CpuTiAction();

  void set_state(simgrid::kernel::resource::Action::State state) override;
  void cancel() override;
  void suspend() override;
  void resume() override;
  void set_max_duration(double duration) override;
  void set_priority(double priority) override;
  double get_remains() override;

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
  CpuTi(CpuTiModel* model, simgrid::s4u::Host* host, std::vector<double>* speed_per_pstate, int core);
  ~CpuTi() override;

  void set_speed_trace(tmgr_trace_t trace) override;

  void apply_event(tmgr_trace_event_t event, double value) override;
  void update_actions_finish_time(double now);
  void update_remaining_amount(double now);

  bool is_used() override;
  CpuAction *execution_start(double size) override;
  simgrid::kernel::resource::Action* execution_start(double size, int requestedCores) override
  {
    THROW_UNIMPLEMENTED;
    return nullptr;
  }
  CpuAction *sleep(double duration) override;
  double get_available_speed() override;

  void set_modified(bool modified);

  CpuTiTgmr* speed_integrated_trace_ = nullptr; /*< Structure with data needed to integrate trace file */
  ActionTiList action_set_;                     /*< set with all actions running on cpu */
  double sum_priority_ = 0;                  /*< the sum of actions' priority that are running on cpu */
  double last_update_  = 0;                  /*< last update of actions' remaining amount done */

  double current_frequency_;

  boost::intrusive::list_member_hook<> cpu_ti_hook;
};

typedef boost::intrusive::member_hook<CpuTi, boost::intrusive::list_member_hook<>, &CpuTi::cpu_ti_hook> CpuTiListOptions;
typedef boost::intrusive::list<CpuTi, CpuTiListOptions> CpuTiList;

/*********
 * Model *
 *********/
class CpuTiModel : public CpuModel {
public:
  CpuTiModel() : CpuModel(Model::UpdateAlgo::Full){};
  ~CpuTiModel() override;
  Cpu* createCpu(simgrid::s4u::Host* host, std::vector<double>* speed_per_pstate, int core) override;
  double next_occuring_event(double now) override;
  void update_actions_state(double now, double delta) override;

  kernel::resource::Action::StateSet runningActionSetThatDoesNotNeedBeingChecked_;
  CpuTiList modified_cpus_;
};

}
}
