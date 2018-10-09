/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_MODEL_CPUTI_H_
#define SURF_MODEL_CPUTI_H_

#include "src/surf/cpu_interface.hpp"
#include "src/surf/trace_mgr.hpp"

#include <boost/intrusive/list.hpp>

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

class CpuTiTmgr {
  enum class Type {
    FIXED,  /*< Trace fixed, no availability file */
    DYNAMIC /*< Dynamic, have an availability file */
  };

public:
  explicit CpuTiTmgr(double value) : type_(Type::FIXED), value_(value){};
  CpuTiTmgr(tmgr_trace_t speed_trace, double value);
  CpuTiTmgr(const CpuTiTmgr&) = delete;
  CpuTiTmgr& operator=(const CpuTiTmgr&) = delete;
  ~CpuTiTmgr();

  double integrate(double a, double b);
  double solve(double a, double amount);
  double get_power_scale(double a);

private:
  Type type_;
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
  CpuTiAction(CpuTi* cpu, double cost);
  ~CpuTiAction();

  void set_state(kernel::resource::Action::State state) override;
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
  simgrid::kernel::resource::Action* execution_start(double size, int requested_cores) override
  {
    THROW_UNIMPLEMENTED;
    return nullptr;
  }
  CpuAction *sleep(double duration) override;
  double get_speed_ratio() override;

  void set_modified(bool modified);

  CpuTiTmgr* speed_integrated_trace_ = nullptr; /*< Structure with data needed to integrate trace file */
  ActionTiList action_set_;                     /*< set with all actions running on cpu */
  double sum_priority_ = 0;                  /*< the sum of actions' priority that are running on cpu */
  double last_update_  = 0;                  /*< last update of actions' remaining amount done */

  boost::intrusive::list_member_hook<> cpu_ti_hook;
};

typedef boost::intrusive::member_hook<CpuTi, boost::intrusive::list_member_hook<>, &CpuTi::cpu_ti_hook> CpuTiListOptions;
typedef boost::intrusive::list<CpuTi, CpuTiListOptions> CpuTiList;

/*********
 * Model *
 *********/
class CpuTiModel : public CpuModel {
public:
  static void create_pm_vm_models(); // Make both models be TI models

  CpuTiModel();
  ~CpuTiModel() override;
  Cpu* create_cpu(simgrid::s4u::Host* host, std::vector<double>* speed_per_pstate, int core) override;
  double next_occuring_event(double now) override;
  void update_actions_state(double now, double delta) override;

  CpuTiList modified_cpus_;
};

}
}

#endif /* SURF_MODEL_CPUTI_H_ */
