/* Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_WAITTEST_OBSERVER_HPP
#define SIMGRID_MC_SIMCALL_WAITTEST_OBSERVER_HPP

#include "simgrid/forward.h"
#include "src/kernel/actor/SimcallObserver.hpp"

namespace simgrid::kernel::actor {

class ActivityTestSimcall final : public DelayedSimcallObserver<bool> {
  activity::ActivityImpl* const activity_;
  std::string fun_call_;

public:
  ActivityTestSimcall(ActorImpl* actor, activity::ActivityImpl* activity, std::string_view fun_call)
      : DelayedSimcallObserver(actor, true), activity_(activity), fun_call_(fun_call)
  {
  }
  activity::ActivityImpl* get_activity() const { return activity_; }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
};

class ActivityTestanySimcall final : public DelayedSimcallObserver<ssize_t> {
  const std::vector<activity::ActivityImpl*>& activities_;
  std::vector<int> indexes_; // indexes in activities_ pointing to ready activities (=whose test() is positive)
  int next_value_ = 0;
  std::string fun_call_;

public:
  ActivityTestanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities,
                         std::string_view fun_call);
  bool is_enabled() override { return true; /* can return -1 if no activity is ready */ }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  int get_max_consider() const override;
  void prepare(int times_considered) override;
  const std::vector<activity::ActivityImpl*>& get_activities() const { return activities_; }
  int get_value() const { return next_value_; }
};

class ActivityWaitSimcall final : public DelayedSimcallObserver<bool> {
  activity::ActivityImpl* activity_;
  const double timeout_;
  std::string fun_call_;

public:
  ActivityWaitSimcall(ActorImpl* actor, activity::ActivityImpl* activity, double timeout, std::string_view fun_call)
      : DelayedSimcallObserver(actor, false), activity_(activity), timeout_(timeout), fun_call_(fun_call)
  {
  }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  bool is_enabled() override;
  activity::ActivityImpl* get_activity() const { return activity_; }
  void set_activity(activity::ActivityImpl* activity) { activity_ = activity; }
  double get_timeout() const { return timeout_; }
};

class ActivityWaitanySimcall final : public DelayedSimcallObserver<ssize_t> {
  const std::vector<activity::ActivityImpl*>& activities_;
  std::vector<int> indexes_; // indexes in activities_ pointing to ready activities (=whose test() is positive)
  const double timeout_;
  int next_value_ = 0;
  std::string fun_call_;

public:
  ActivityWaitanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities, double timeout,
                         std::string_view fun_call);
  bool is_enabled() override;
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  void prepare(int times_considered) override;
  int get_max_consider() const override;
  const std::vector<activity::ActivityImpl*>& get_activities() const { return activities_; }
  double get_timeout() const { return timeout_; }
  int get_value() const { return next_value_; }
};

}; // namespace simgrid::kernel::actor

#endif