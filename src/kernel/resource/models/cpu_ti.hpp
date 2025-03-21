/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODEL_CPUTI_HPP_
#define SIMGRID_MODEL_CPUTI_HPP_

#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "xbt/ex.h"

#include <boost/intrusive/list.hpp>
#include <memory>

namespace simgrid::kernel::resource {

/***********
 * Classes *
 ***********/
class XBT_PRIVATE CpuTiModel;
class XBT_PRIVATE CpuTi;
class XBT_PRIVATE CpuTiAction;

/***********
 * Profile *
 ***********/
class CpuTiProfile {
  std::vector<double> time_points_;
  std::vector<double> integral_;

public:
  explicit CpuTiProfile(const profile::Profile* profile);

  const std::vector<double>& get_time_points() const { return time_points_; }

  double integrate_simple(double a, double b) const;
  double integrate_simple_point(double a) const;
  double solve_simple(double a, double amount) const;

  static long binary_search(const std::vector<double>& array, double a);
};

class CpuTiTmgr {
  enum class Type {
    FIXED,  /*< Trace fixed, no availability file */
    DYNAMIC /*< Dynamic, have an availability file */
  };
  Type type_    = Type::FIXED;
  double value_ = 0.0; /*< Percentage of cpu speed available. Value fixed between 0 and 1 */

  /* Dynamic */
  double last_time_ = 0.0; /*< Integral interval last point (discrete time) */
  double total_     = 0.0; /*< Integral total between 0 and last point */

  std::unique_ptr<CpuTiProfile> profile_ = nullptr;
  profile::Profile* speed_profile_       = nullptr;

public:
  explicit CpuTiTmgr(double value) : value_(value){};
  CpuTiTmgr(profile::Profile* speed_profile, double value);
  CpuTiTmgr(const CpuTiTmgr&)            = delete;
  CpuTiTmgr& operator=(const CpuTiTmgr&) = delete;

  double integrate(double a, double b) const;
  double solve(double a, double amount) const;
  double get_power_scale(double a) const;
};

/**********
 * Action *
 **********/

class XBT_PRIVATE CpuTiAction : public CpuAction {
  friend class CpuTi;

public:
  CpuTiAction(CpuTi* cpu, double cost);
  CpuTiAction(const CpuTiAction&)            = delete;
  CpuTiAction& operator=(const CpuTiAction&) = delete;
  ~CpuTiAction() override;

  void set_state(Action::State state) override;
  void cancel() override;
  void suspend() override;
  void resume() override;
  void set_sharing_penalty(double sharing_penalty) override;
  double get_remains() override;

  CpuTi* cpu_;

  boost::intrusive::list_member_hook<> action_ti_hook;
};

using ActionTiListOptions =
    boost::intrusive::member_hook<CpuTiAction, boost::intrusive::list_member_hook<>, &CpuTiAction::action_ti_hook>;
using ActionTiList = boost::intrusive::list<CpuTiAction, ActionTiListOptions>;

/************
 * Resource *
 ************/
class CpuTi : public CpuImpl {
public:
  CpuTi(s4u::Host* host, const std::vector<double>& speed_per_pstate);
  CpuTi(const CpuTi&)            = delete;
  CpuTi& operator&(const CpuTi&) = delete;
  ~CpuTi() override;

  CpuImpl* set_speed_profile(profile::Profile* profile) override;
  void turn_off() override;

  void apply_event(profile::Event* event, double value) override;
  void update_actions_finish_time(double now);
  void update_remaining_amount(double now);

  bool is_used() const override;
  CpuAction* execution_start(double size, double user_bound) override;
  CpuAction* execution_start(double, int, double) override
  {
    THROW_UNIMPLEMENTED;
    return nullptr;
  }
  CpuAction* sleep(double duration) override;
  double get_speed_ratio() override;

  void set_modified(bool modified);

  CpuTiTmgr* speed_integrated_trace_ = nullptr; /*< Structure with data needed to integrate trace file */
  ActionTiList action_set_;                     /*< set with all actions running on cpu */
  double sum_priority_ = 0;                     /*< the sum of actions' priority that are running on cpu */
  double last_update_  = 0;                     /*< last update of actions' remaining amount done */

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
  static void create_pm_models(); // Make CPU PM model

  using CpuModel::CpuModel;
  CpuTiModel(const CpuTiModel&)            = delete;
  CpuTiModel& operator=(const CpuTiModel&) = delete;
  CpuImpl* create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate) override;
  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;

  CpuTiList modified_cpus_;
};

} // namespace simgrid::kernel::resource

#endif /* SIMGRID_MODEL_CPUTI_HPP_ */
