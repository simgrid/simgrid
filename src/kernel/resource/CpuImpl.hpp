/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef CPU_IMPL_HPP_
#define CPU_IMPL_HPP_

#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/kernel/resource/Resource.hpp"

#include <list>

namespace simgrid {
namespace kernel {
namespace resource {

/***********
 * Classes *
 ***********/

class CpuAction;

/*********
 * Model *
 *********/
class XBT_PUBLIC CpuModel : public Model {
public:
  using Model::Model;

  /**
   * @brief Create a Cpu
   *
   * @param host The host that will have this CPU
   * @param speed_per_pstate Processor speed (in Flops) of each pstate.
   *                         This ignores any potential external load coming from a trace.
   * @param core The number of core of this Cpu
   */
  virtual CpuImpl* create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate) = 0;

  void update_actions_state_lazy(double now, double delta) override;
  void update_actions_state_full(double now, double delta) override;
};

/************
 * Resource *
 ************/

class XBT_PUBLIC CpuImpl : public Resource_T<CpuImpl> {
  friend VirtualMachineImpl; // Resets the VCPU

  s4u::Host* piface_;
  int core_count_       = 1;
  unsigned long pstate_ = 0;             /*< Current pstate (index in the speed_per_pstate_)*/
  std::vector<double> speed_per_pstate_; /*< List of supported CPU capacities (pstate related). Not 'const' because VCPU
                                            get modified on migration */
  s4u::Host::SharingPolicy sharing_policy_ = s4u::Host::SharingPolicy::LINEAR;
  s4u::NonLinearResourceCb sharing_policy_cb_;

  void apply_sharing_policy_cfg() const;

public:
  /**
   * @brief Cpu constructor
   *
   * @param host The host in which this Cpu should be plugged
   * @param speed_per_pstate Processor speed (in flop per second) for each pstate
   */
  CpuImpl(s4u::Host* host, const std::vector<double>& speed_per_pstate);

  CpuImpl(const CpuImpl&) = delete;
  CpuImpl& operator=(const CpuImpl&) = delete;

  /** @brief Public interface */
  s4u::Host* get_iface() const { return piface_; }

  CpuImpl* set_core_count(int core_count);
  virtual int get_core_count() const { return core_count_; }

  void seal() override;

  /** @brief Get a forecast of the speed (in flops/s) if the load were as provided.
   *
   * The provided load should encompasses both the application's activities and the external load that come from a
   * trace.
   *
   * Use a load of 1.0 to compute the amount of flops that the Cpu would deliver with one CPU-bound task.
   * If you use a load of 0, this function will return 0: when nobody is using the Cpu, it delivers nothing.
   *
   * If you want to know the amount of flops currently delivered, use  load = get_load()*get_speed_ratio()
   */
  virtual double get_speed(double load) const { return load * speed_.peak; }

  /** @brief Get the available speed ratio, in [0:1]. This accounts for external load (see @ref set_speed_profile()). */
  virtual double get_speed_ratio() { return speed_.scale; }

  /** @brief Get the peak processor speed (in flops/s), at the specified pstate */
  virtual double get_pstate_peak_speed(unsigned long pstate_index) const;

  virtual unsigned long get_pstate_count() const { return speed_per_pstate_.size(); }

  virtual unsigned long get_pstate() const { return pstate_; }
  virtual CpuImpl* set_pstate(unsigned long pstate_index);

  /*< @brief Setup the profile file with availability events (peak speed changes due to external load).
   * Profile must contain relative values (ratio between 0 and 1)
   */
  virtual CpuImpl* set_speed_profile(profile::Profile* profile);

  /**
   * @brief Set the CPU's speed
   *
   * @param speed_per_state list of powers for this processor (default power is at index 0)
   */
  CpuImpl* set_pstate_speed(const std::vector<double>& speed_per_state);

  void set_sharing_policy(s4u::Host::SharingPolicy policy, const s4u::NonLinearResourceCb& cb);
  s4u::Host::SharingPolicy get_sharing_policy() const { return sharing_policy_; }

  /**
   * @brief Sets factor callback
   * Implemented only for cas01
   */
  virtual void set_factor_cb(const std::function<s4u::Host::CpuFactorCb>& cb) { THROW_UNIMPLEMENTED; }

  /**
   * @brief Execute some quantity of computation
   *
   * @param size The value of the processing amount (in flop) needed to process
   * @param user_bound User's bound for execution speed
   * @return The CpuAction corresponding to the processing
   */
  virtual CpuAction* execution_start(double size, double user_bound) = 0;

  /**
   * @brief Execute some quantity of computation on more than one core
   *
   * @param size The value of the processing amount (in flop) needed to process
   * @param requested_cores The desired amount of cores. Must be >= 1
   * @param user_bound User's bound for execution speed
   * @return The CpuAction corresponding to the processing
   */
  virtual CpuAction* execution_start(double size, int requested_cores, double user_bound) = 0;

  /**
   * @brief Make a process sleep for duration (in seconds)
   *
   * @param duration The number of seconds to sleep
   * @return The CpuAction corresponding to the sleeping
   */
  virtual CpuAction* sleep(double duration) = 0;

protected:
  /** @brief Take speed changes (either load or max) into account */
  virtual void on_speed_change();

  /** Reset most characteristics of this CPU to the one of that CPU.
   *
   * Used to reset a VCPU when its VM migrates to another host, so it only resets the fields that should be in this
   *case.
   **/
  virtual void reset_vcpu(CpuImpl* that);

  Metric speed_ = {1.0, 0, nullptr};
};

/**********
 * Action *
 **********/

/** @ingroup SURF_cpu_interface
 * @brief A CpuAction represents the execution of code on one or several Cpus
 */
class XBT_PUBLIC CpuAction : public Action {
public:
  using Action::Action;

  /** @brief Signal emitted when the action state changes (ready/running/done, etc)
   *  Signature: `void(CpuAction const& action, simgrid::kernel::resource::Action::State previous)`
   */
  static xbt::signal<void(CpuAction const&, Action::State)> on_state_change;

  void set_state(Action::State state) override;

  void update_remains_lazy(double now) override;
  std::list<CpuImpl*> cpus() const;

  void suspend() override;
  void resume() override;
};
} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* CPU_IMPL_HPP_ */
