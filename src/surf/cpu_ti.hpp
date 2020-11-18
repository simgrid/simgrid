/* Copyright (c) 2013-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_MODEL_CPUTI_H_
#define SURF_MODEL_CPUTI_H_

#include "src/kernel/resource/profile/Profile.hpp"
#include "src/surf/cpu_interface.hpp"

#include <boost/intrusive/list.hpp>
#include <memory>

namespace simgrid {
namespace kernel {
namespace resource {

/***********
 * Classes *
 ***********/
class XBT_PRIVATE CpuTiModel;
class XBT_PRIVATE CpuTi;

/*********
 * Trace *
 *********/
class CpuTiProfile {
public:
  explicit CpuTiProfile(const profile::Profile* profile);

  double integrate_simple(double a, double b) const;
  double integrate_simple_point(double a) const;
  double solve_simple(double a, double amount) const;

  std::vector<double> time_points_;
  std::vector<double> integral_;
  static int binary_search(const std::vector<double>& array, double a);
};

class CpuTiTmgr {
  enum class Type {
    FIXED,  /*< Trace fixed, no availability file */
    DYNAMIC /*< Dynamic, have an availability file */
  };

public:
  explicit CpuTiTmgr(double value) : value_(value){};
  CpuTiTmgr(profile::Profile* speed_profile, double value);
  CpuTiTmgr(const CpuTiTmgr&) = delete;
  CpuTiTmgr& operator=(const CpuTiTmgr&) = delete;

  double integrate(double a, double b) const;
  double solve(double a, double amount) const;
  double get_power_scale(double a) const;

private:
  Type type_ = Type::FIXED;
  double value_;                 /*< Percentage of cpu speed available. Value fixed between 0 and 1 */

  /* Dynamic */
  double last_time_ = 0.0;             /*< Integral interval last point (discrete time) */
  double total_     = 0.0;             /*< Integral total between 0 and last point */

  std::unique_ptr<CpuTiProfile> profile_ = nullptr;
  profile::Profile* speed_profile_       = nullptr;
};

/**********
 * Action *
 **********/

class XBT_PRIVATE CpuTiAction : public CpuAction {
  friend class CpuTi;
public:
  CpuTiAction(CpuTi* cpu, double cost);
  CpuTiAction(const CpuTiAction&) = delete;
  CpuTiAction& operator=(const CpuTiAction&) = delete;
  ~CpuTiAction() override;

  void set_state(Action::State state) override;
  void cancel() override;
  void suspend() override;
  void resume() override;
  void set_max_duration(double duration) override;
  void set_sharing_penalty(double sharing_penalty) override;
  double get_remains() override;

  CpuTi *cpu_;

  boost::intrusive::list_member_hook<> action_ti_hook;
};

using ActionTiListOptions =
    boost::intrusive::member_hook<CpuTiAction, boost::intrusive::list_member_hook<>, &CpuTiAction::action_ti_hook>;
using ActionTiList = boost::intrusive::list<CpuTiAction, ActionTiListOptions>;

/************
 * Resource *
 ************/
class CpuTi : public Cpu {
public:
  CpuTi(CpuTiModel* model, s4u::Host* host, const std::vector<double>& speed_per_pstate, int core);
  CpuTi(const CpuTi&)            = delete;
  CpuTi& operator&(const CpuTi&) = delete;
  ~CpuTi() override;

  void set_speed_profile(profile::Profile* profile) override;

  void apply_event(profile::Event* event, double value) override;
  void update_actions_finish_time(double now);
  void update_remaining_amount(double now);

  bool is_used() const override;
  CpuAction* execution_start(double size) override;
  CpuAction* execution_start(double, int) override
  {
    THROW_UNIMPLEMENTED;
    return nullptr;
  }
  CpuAction* sleep(double duration) override;
  double get_speed_ratio() override;

  void set_modified(bool modified);

  CpuTiTmgr* speed_integrated_trace_ = nullptr; /*< Structure with data needed to integrate trace file */
  ActionTiList action_set_;                     /*< set with all actions running on cpu */
  double sum_priority_ = 0;                  /*< the sum of actions' priority that are running on cpu */
  double last_update_  = 0;                  /*< last update of actions' remaining amount done */

  boost::intrusive::list_member_hook<> cpu_ti_hook;
};

using CpuTiListOptions =
    boost::intrusive::member_hook<CpuTi, boost::intrusive::list_member_hook<>, &CpuTi::cpu_ti_hook>;
using CpuTiList = boost::intrusive::list<CpuTi, CpuTiListOptions>;

/*********
 * Model *
 *********/
class CpuTiModel : public CpuModel {
public:
  static void create_pm_vm_models(); // Make both models be TI models

  CpuTiModel();
  CpuTiModel(const CpuTiModel&) = delete;
  CpuTiModel& operator=(const CpuTiModel&) = delete;
  ~CpuTiModel() override;
  Cpu* create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate, int core) override;
  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;

  CpuTiList modified_cpus_;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* SURF_MODEL_CPUTI_H_ */
