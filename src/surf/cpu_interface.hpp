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
CpuPtr getActionCpu(CpuActionPtr action);

extern surf_callback(void, CpuPtr) createCpuCallbacks;
extern surf_callback(void, CpuPtr) deleteCpuCallbacks;
extern surf_callback(void, CpuActionPtr) updateCpuActionCallbacks;

/*********
 * Model *
 *********/
class CpuModel : public Model {
public:
  CpuModel(const char *name) : Model(name) {};
  CpuPtr createResource(string name);
  void updateActionsStateLazy(double now, double delta);
  void updateActionsStateFull(double now, double delta);

  virtual void addTraces() =0;
};

/************
 * Resource *
 ************/
class Cpu : public Resource {
public:
  Cpu(){surf_callback_emit(createCpuCallbacks, this);};
  Cpu(ModelPtr model, const char *name, xbt_dict_t props,
	  lmm_constraint_t constraint, int core, double powerPeak, double powerScale);
  Cpu(ModelPtr model, const char *name, xbt_dict_t props,
	  int core, double powerPeak, double powerScale);
  ~Cpu();
  virtual CpuActionPtr execute(double size)=0;
  virtual CpuActionPtr sleep(double duration)=0;
  virtual int getCore();
  virtual double getSpeed(double load);
  virtual double getAvailableSpeed();

  virtual double getCurrentPowerPeak()=0;
  virtual double getPowerPeakAt(int pstate_index)=0;
  virtual int getNbPstates()=0;
  virtual void setPowerPeakAt(int pstate_index)=0;

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
class CpuAction : public Action {
friend CpuPtr getActionCpu(CpuActionPtr action);
public:
  CpuAction(){};
  CpuAction(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed) {} //FIXME:REMOVE
  CpuAction(ModelPtr model, double cost, bool failed, lmm_variable_t var)
  : Action(model, cost, failed, var) {}
  virtual void setAffinity(CpuPtr cpu, unsigned long mask);
  virtual void setBound(double bound);

  void updateRemainingLazy(double now);
  double m_bound;
};

#endif /* SURF_CPU_INTERFACE_HPP_ */
