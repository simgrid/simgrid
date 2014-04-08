/* Copyright (c) 2004-2014. The SimGrid Team.
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
typedef CpuModel *CpuModelPtr;

class Cpu;
typedef Cpu *CpuPtr;

class CpuAction;
typedef CpuAction *CpuActionPtr;

class CpuPlugin;
typedef CpuPlugin *CpuPluginPtr;

/*************
 * Callbacks *
 *************/
XBT_PUBLIC(CpuPtr) getActionCpu(CpuActionPtr action);

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Cpu creation *
 * @details Callback functions have the following signature: `void(CpuPtr)`
 */
XBT_PUBLIC_DATA( surf_callback(void, CpuPtr)) cpuCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Cpu destruction *
 * @details Callback functions have the following signature: `void(CpuPtr)`
 */
XBT_PUBLIC_DATA( surf_callback(void, CpuPtr)) cpuDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Cpu State changed *
 * @details Callback functions have the following signature: `void(CpuActionPtr action, e_surf_resource_state_t old, e_surf_resource_state_t current)`
 */
XBT_PUBLIC_DATA( surf_callback(void, CpuPtr, e_surf_resource_state_t, e_surf_resource_state_t)) cpuStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after CpuAction State changed *
 * @details Callback functions have the following signature: `void(CpuActionPtr action, e_surf_action_state_t old, e_surf_action_state_t current)`
 */
XBT_PUBLIC_DATA( surf_callback(void, CpuActionPtr, e_surf_action_state_t, e_surf_action_state_t)) cpuActionStateChangedCallbacks;

void parse_cpu_init(sg_platf_host_cbarg_t host);

void add_traces_cpu();

/*********
 * Model *
 *********/

 /** @ingroup SURF_cpu_interface
 * @brief SURF cpu model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class CpuModel : public Model {
public:
  /**
   * @brief CpuModel constructor
   *
   * @param name The name of the model
   */
  CpuModel(const char *name) : Model(name) {};

  /**
   * @brief Create a Cpu
   *
   * @param host [TODO]
   */
  void parseInit(sg_platf_host_cbarg_t host);

  virtual CpuPtr createResource(const char *name, xbt_dynar_t power_peak,
                      int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties)=0;
  void setState(e_surf_resource_state_t state);

  void updateActionsStateLazy(double now, double delta);
  void updateActionsStateFull(double now, double delta);

  virtual void addTraces() =0;
};

/************
 * Resource *
 ************/

/** @ingroup SURF_cpu_interface
* @brief SURF cpu resource interface class
* @details A Cpu represent a cpu associated to a workstation
*/
class Cpu : public Resource {
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
  Cpu(ModelPtr model, const char *name, xbt_dict_t props,
	  lmm_constraint_t constraint, int core, double powerPeak, double powerScale);

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
  Cpu(ModelPtr model, const char *name, xbt_dict_t props,
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
  virtual CpuActionPtr execute(double size)=0;

  /**
   * @brief Make a process sleep for duration (in seconds)
   *
   * @param duration The number of seconds to sleep
   * @return The CpuAction corresponding to the sleeping
   */
  virtual CpuActionPtr sleep(double duration)=0;

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
  virtual double getCurrentPowerPeak()=0;


  virtual double getPowerPeakAt(int pstate_index)=0;

  virtual int getNbPstates()=0;

  virtual void setPowerPeakAt(int pstate_index)=0;

  void setState(e_surf_resource_state_t state);

  void addTraces(void);
  int m_core;
  double m_powerPeak;            /*< CPU power peak */
  double m_powerScale;           /*< Percentage of CPU disponible */

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
class CpuAction : public Action {
friend CpuPtr getActionCpu(CpuActionPtr action);
public:
  /**
   * @brief CpuAction constructor
   *
   * @param model The CpuModel associated to this CpuAction
   * @param cost [TODO]
   * @param failed [TODO]
   */
  CpuAction(ModelPtr model, double cost, bool failed)
    : Action(model, cost, failed) {} //FIXME:REMOVE

  /**
   * @brief CpuAction constructor
   *
   * @param model The CpuModel associated to this CpuAction
   * @param cost [TODO]
   * @param failed [TODO]
   * @param var The lmm variable associated to this CpuAction if it is part of a LMM component
   */
  CpuAction(ModelPtr model, double cost, bool failed, lmm_variable_t var)
    : Action(model, cost, failed, var) {}

  /**
   * @brief Set the affinity of the current CpuAction
   * @details [TODO]
   *
   * @param cpu [TODO]
   * @param mask [TODO]
   */
  virtual void setAffinity(CpuPtr cpu, unsigned long mask);

  void setState(e_surf_action_state_t state);

  void updateRemainingLazy(double now);

};

#endif /* SURF_CPU_INTERFACE_HPP_ */
