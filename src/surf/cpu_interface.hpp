/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"
#include "maxmin_private.hpp"

#ifndef SURF_CPU_INTERFACE_HPP_
#define SURF_CPU_INTERFACE_HPP_

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {

class CpuModel;
class Cpu;
class CpuAction;
class CpuPlugin;

/*************
 * Callbacks *
 *************/
XBT_PUBLIC(Cpu*) getActionCpu(CpuAction *action);

/*********
 * Model *
 *********/

 /** @ingroup SURF_cpu_interface
 * @brief SURF cpu model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
XBT_PUBLIC_CLASS CpuModel : public Model {
public:
  CpuModel() : Model() {};

  /**
   * @brief Create a Cpu
   *
   * @param host The host that will have this CPU
   * @param speedPeak The peak spead (max speed in Flops when no external load comes from a trace)
   * @param speedTrace Trace variations
   * @param core The number of core of this Cpu
   * @param state_trace [TODO]
   */
  virtual Cpu *createCpu(simgrid::s4u::Host *host, xbt_dynar_t speedPeak,
                          tmgr_trace_t speedTrace, int core,
                          tmgr_trace_t state_trace)=0;

  void updateActionsStateLazy(double now, double delta);
  void updateActionsStateFull(double now, double delta);
  bool next_occuring_event_isIdempotent() {return true;}
};

/************
 * Resource *
 ************/

/** @ingroup SURF_cpu_interface
* @brief SURF cpu resource interface class
* @details A Cpu represent a cpu associated to a host
*/
XBT_PUBLIC_CLASS Cpu : public simgrid::surf::Resource {
public:
  /**
   * @brief Cpu constructor
   *
   * @param model The CpuModel associated to this Cpu
   * @param host The host in which this Cpu should be plugged
   * @param constraint The lmm constraint associated to this Cpu if it is part of a LMM component
   * @param speedPeakList [TODO]
   * @param core The number of core of this Cpu
   * @param speedPeak The speed peak of this Cpu in flops (max speed)
   */
  Cpu(simgrid::surf::Model *model, simgrid::s4u::Host *host,
    lmm_constraint_t constraint, xbt_dynar_t speedPeakList, int core, double speedPeak);

  /**
   * @brief Cpu constructor
   *
   * @param model The CpuModel associated to this Cpu
   * @param host The host in which this Cpu should be plugged
   * @param speedPeakList [TODO]
   * @param core The number of core of this Cpu
   * @param speedPeak The speed peak of this Cpu in flops (max speed)
   */
  Cpu(simgrid::surf::Model *model, simgrid::s4u::Host *host,
      xbt_dynar_t speedPeakList, int core, double speedPeak);

  ~Cpu();

  /**
   * @brief Execute some quantity of computation
   *
   * @param size The value of the processing amount (in flop) needed to process
   * @return The CpuAction corresponding to the processing
   */
  virtual simgrid::surf::Action *execution_start(double size)=0;

  /**
   * @brief Make a process sleep for duration (in seconds)
   *
   * @param duration The number of seconds to sleep
   * @return The CpuAction corresponding to the sleeping
   */
  virtual simgrid::surf::Action *sleep(double duration)=0;

  /** @brief Get the amount of cores */
  virtual int getCore();

  /** @brief Get the speed, accounting for the trace load and provided process load instead of the real current one */
  virtual double getSpeed(double load);

protected:
  /** @brief Take speed changes (either load or max) into account */
  virtual void onSpeedChange();

public:
  /** @brief Get the available speed of the current Cpu */
  virtual double getAvailableSpeed();

  /** @brief Get the current Cpu power peak */
  virtual double getCurrentPowerPeak();

  virtual double getPowerPeakAt(int pstate_index);

  virtual int getNbPStates();
  virtual void setPState(int pstate_index);
  virtual int  getPState();

  simgrid::s4u::Host* getHost() { return host_; }

public:
  int coresAmount_ = 1;
  simgrid::s4u::Host* host_;

  xbt_dynar_t speedPeakList_ = NULL; /*< List of supported CPU capacities (pstate related) */
  int pstate_ = 0;                   /*< Current pstate (index in the speedPeakList)*/

  /* Note (hypervisor): */
  lmm_constraint_t *p_constraintCore=NULL;
  void **p_constraintCoreId=NULL;

public:
  virtual void setStateTrace(tmgr_trace_t trace); /*< setup the trace file with states events (ON or OFF). Trace must contain boolean values (0 or 1). */
  virtual void setSpeedTrace(tmgr_trace_t trace); /*< setup the trace file with availability events (peak speed changes due to external load). Trace must contain relative values (ratio between 0 and 1) */

  tmgr_trace_iterator_t stateEvent_ = nullptr;
  s_surf_metric_t speed_ = {1.0, 0, nullptr};
};

/**********
 * Action *
 **********/

 /** @ingroup SURF_cpu_interface
 * @brief SURF Cpu action interface class
 * @details A CpuAction represent the execution of code on a Cpu
 */
XBT_PUBLIC_CLASS CpuAction : public simgrid::surf::Action {
friend XBT_PUBLIC(Cpu*) getActionCpu(CpuAction *action);
public:
/** @brief Callbacks handler which emit the callbacks after CpuAction State changed *
 * @details Callback functions have the following signature: `void(CpuAction *action, e_surf_action_state_t previous)`
 */
  static simgrid::xbt::signal<void(simgrid::surf::CpuAction*, e_surf_action_state_t)> onStateChange;

  /** @brief CpuAction constructor */
  CpuAction(simgrid::surf::Model *model, double cost, bool failed)
    : Action(model, cost, failed) {} //FIXME:DEADCODE?

  /** @brief CpuAction constructor */
  CpuAction(simgrid::surf::Model *model, double cost, bool failed, lmm_variable_t var)
    : Action(model, cost, failed, var) {}

  /**
   * @brief Set the affinity of the current CpuAction
   * @details [TODO]
   */
  virtual void setAffinity(Cpu *cpu, unsigned long mask);

  void setState(e_surf_action_state_t state);

  void updateRemainingLazy(double now);

};

}
}

#endif /* SURF_CPU_INTERFACE_HPP_ */
