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

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Cpu creation *
 * @details Callback functions have the following signature: `void(CpuPtr)`
 */
XBT_PUBLIC_DATA( surf_callback(void, Cpu*)) cpuCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Cpu destruction *
 * @details Callback functions have the following signature: `void(CpuPtr)`
 */
XBT_PUBLIC_DATA( surf_callback(void, Cpu*)) cpuDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Cpu State changed *
 * @details Callback functions have the following signature: `void(CpuAction *action, e_surf_resource_state_t old, e_surf_resource_state_t current)`
 */
XBT_PUBLIC_DATA( surf_callback(void, Cpu*, e_surf_resource_state_t, e_surf_resource_state_t)) cpuStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after CpuAction State changed *
 * @details Callback functions have the following signature: `void(CpuAction *action, e_surf_action_state_t old, e_surf_action_state_t current)`
 */
XBT_PUBLIC_DATA( surf_callback(void, CpuAction*, e_surf_action_state_t, e_surf_action_state_t)) cpuActionStateChangedCallbacks;

XBT_PUBLIC(void) cpu_add_traces();

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
   * @param name The name of the Cpu
   * @param speedPeak The peak spead (max speed in Flops)
   * @param pstate [TODO]
   * @param speedScale The speed scale (in [O;1] available speed from peak)
   * @param speedTrace Trace variations
   * @param core The number of core of this Cpu
   * @param state_initial [TODO]
   * @param state_trace [TODO]
   */
  virtual Cpu *createCpu(const char *name, xbt_dynar_t speedPeak,
                      int pstate, double speedScale,
                          tmgr_trace_t speedTrace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace)=0;

  void updateActionsStateLazy(double now, double delta);
  void updateActionsStateFull(double now, double delta);
  bool shareResourcesIsIdempotent() {return true;}
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
  static simgrid::xbt::FacetLevel<simgrid::Host, Cpu> LEVEL;
  static void init();
  Cpu();

  /**
   * @brief Cpu constructor
   *
   * @param model The CpuModel associated to this Cpu
   * @param name The name of the Cpu
   * @param constraint The lmm constraint associated to this Cpu if it is part of a LMM component
   * @param core The number of core of this Cpu
   * @param speedPeak The speed peak of this Cpu in flops (max speed)
   * @param speedScale The speed scale of this Cpu in [0;1] (available amount)
   * @param stateInitial whether it is created running or crashed
   */
  Cpu(simgrid::surf::Model *model, const char *name,
	  lmm_constraint_t constraint, int core, double speedPeak, double speedScale,
	  e_surf_resource_state_t stateInitial);

  /**
   * @brief Cpu constructor
   *
   * @param model The CpuModel associated to this Cpu
   * @param name The name of the Cpu
   * @param core The number of core of this Cpu
   * @param speedPeak The speed peak of this Cpu in flops (max speed)
   * @param speedScale The speed scale of this Cpu in [0;1] (available amount)
   * @param stateInitial whether it is created running or crashed
   */
  Cpu(simgrid::surf::Model *model, const char *name,
	  int core, double speedPeak, double speedScale,
	  e_surf_resource_state_t stateInitial);

  Cpu(simgrid::surf::Model *model, const char *name,
	  lmm_constraint_t constraint, int core, double speedPeak, double speedScale);
  Cpu(simgrid::surf::Model *model, const char *name,
	  int core, double speedPeak, double speedScale);

  ~Cpu();

  /**
   * @brief Execute some quantity of computation
   *
   * @param size The value of the processing amount (in flop) needed to process
   * @return The CpuAction corresponding to the processing
   */
  virtual simgrid::surf::Action *execute(double size)=0;

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

  /** @brief Get the available speed of the current Cpu */
  virtual double getAvailableSpeed();

  /** @brief Get the current Cpu power peak */
  virtual double getCurrentPowerPeak();

  virtual double getPowerPeakAt(int pstate_index)=0;

  virtual int getNbPstates()=0;
  virtual void setPstate(int pstate_index)=0;
  virtual int  getPstate()=0;

  void setState(e_surf_resource_state_t state);
  void plug(simgrid::Host* host);

  void addTraces(void);
  simgrid::Host* getHost() { return m_host; }

protected:
  virtual void onDie() override;

public:
  int m_core = 1;                /* Amount of cores */
  double m_speedPeak;            /*< CPU speed peak, ie max value */
  double m_speedScale;           /*< Percentage of CPU available according to the trace, in [O,1] */
  simgrid::Host* m_host = nullptr;

  /* Note (hypervisor): */
  lmm_constraint_t *p_constraintCore=NULL;
  void **p_constraintCoreId=NULL;

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
  /** @brief CpuAction constructor */
  CpuAction(simgrid::surf::Model *model, double cost, bool failed)
    : Action(model, cost, failed) {} //FIXME:REMOVE

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
