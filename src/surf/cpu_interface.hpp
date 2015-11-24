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

XBT_PUBLIC(void) cpu_parse_init(sg_platf_host_cbarg_t host);

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
   * @param power_peak The power peak of this Cpu
   * @param pstate [TODO]
   * @param power_scale The power scale of this Cpu
   * @param power_trace [TODO]
   * @param core The number of core of this Cpu
   * @param state_initial [TODO]
   * @param state_trace [TODO]
   * @param cpu_properties Dictionary of properties associated to this Cpu
   */
  virtual Cpu *createCpu(const char *name, xbt_dynar_t power_peak,
                      int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties)=0;

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
XBT_PUBLIC_CLASS Cpu : public Resource {
public:
  /**
   * @brief Cpu constructor
   */
  Cpu();

  /**
   * @brief Cpu constructor
   *
   * @param model The CpuModel associated to this Cpu
   * @param name The name of the Cpu
   * @param props Dictionary of properties associated to this Cpu
   * @param constraint The lmm constraint associated to this Cpu if it is part of a LMM component
   * @param core The number of core of this Cpu
   * @param powerPeak The power peak of this Cpu
   * @param powerScale The power scale of this Cpu
   */
  Cpu(Model *model, const char *name, xbt_dict_t props,
	  lmm_constraint_t constraint, int core, double powerPeak, double powerScale,
    e_surf_resource_state_t stateInitial);

  /**
   * @brief Cpu constructor
   *
   * @param model The CpuModel associated to this Cpu
   * @param name The name of the Cpu
   * @param props Dictionary of properties associated to this Cpu
   * @param core The number of core of this Cpu
   * @param powerPeak The power peak of this Cpu in [TODO]
   * @param powerScale The power scale of this Cpu in [TODO]
   */
  Cpu(Model *model, const char *name, xbt_dict_t props,
	  int core, double powerPeak, double powerScale,
    e_surf_resource_state_t stateInitial);

  Cpu(Model *model, const char *name, xbt_dict_t props,
	  lmm_constraint_t constraint, int core, double powerPeak, double powerScale);
  Cpu(Model *model, const char *name, xbt_dict_t props,
	  int core, double powerPeak, double powerScale);

  /**
   * @brief Cpu destructor
   */
  ~Cpu();

  /**
   * @brief Execute some quantity of computation
   *
   * @param size The value of the processing amount (in flop) needed to process
   * @return The CpuAction corresponding to the processing
   */
  virtual CpuAction *execute(double size)=0;

  /**
   * @brief Make a process sleep for duration (in seconds)
   *
   * @param duration The number of seconds to sleep
   * @return The CpuAction corresponding to the sleeping
   */
  virtual CpuAction *sleep(double duration)=0;

  /**
   * @brief Get the number of cores of the current Cpu
   *
   * @return The number of cores of the current Cpu
   */
  virtual int getCore();

  /**
   * @brief Get the speed of the current Cpu
   * @details [TODO] load * m_powerPeak
   *
   * @param load [TODO]
   *
   * @return The speed of the current Cpu
   */
  virtual double getSpeed(double load);

  /**
   * @brief Get the available speed of the current Cpu
   * @details [TODO]
   *
   * @return The available speed of the current Cpu
   */
  virtual double getAvailableSpeed();

  /**
   * @brief Get the current Cpu power peak
   *
   * @return The current Cpu power peak
   */
  virtual double getCurrentPowerPeak();


  virtual double getPowerPeakAt(int pstate_index)=0;

  virtual int getNbPstates()=0;

  virtual void setPstate(int pstate_index)=0;
  virtual int  getPstate()=0;

  void setState(e_surf_resource_state_t state);

  void addTraces(void);
  int m_core;
  double m_powerPeak;            /*< CPU power peak */
  double m_powerScale;           /*< Percentage of CPU available */

  /* Note (hypervisor): */
  lmm_constraint_t *p_constraintCore;
  void **p_constraintCoreId;
};

/**********
 * Action *
 **********/

 /** @ingroup SURF_cpu_interface
 * @brief SURF Cpu action interface class
 * @details A CpuAction represent the execution of code on a Cpu
 */
XBT_PUBLIC_CLASS CpuAction : public Action {
friend XBT_PUBLIC(Cpu*) getActionCpu(CpuAction *action);
public:
  /**
   * @brief CpuAction constructor
   *
   * @param model The CpuModel associated to this CpuAction
   * @param cost [TODO]
   * @param failed [TODO]
   */
  CpuAction(Model *model, double cost, bool failed)
    : Action(model, cost, failed) {} //FIXME:REMOVE

  /**
   * @brief CpuAction constructor
   *
   * @param model The CpuModel associated to this CpuAction
   * @param cost [TODO]
   * @param failed [TODO]
   * @param var The lmm variable associated to this CpuAction if it is part of a LMM component
   */
  CpuAction(Model *model, double cost, bool failed, lmm_variable_t var)
    : Action(model, cost, failed, var) {}

  /**
   * @brief Set the affinity of the current CpuAction
   * @details [TODO]
   *
   * @param cpu [TODO]
   * @param mask [TODO]
   */
  virtual void setAffinity(Cpu *cpu, unsigned long mask);

  void setState(e_surf_action_state_t state);

  void updateRemainingLazy(double now);

};

#endif /* SURF_CPU_INTERFACE_HPP_ */
