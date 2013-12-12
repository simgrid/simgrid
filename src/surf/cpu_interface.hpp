#include "surf_interface.hpp"
#include "maxmin_private.h"

#ifndef SURF_CPU_INTERFACE_HPP_
#define SURF_CPU_INTERFACE_HPP_

/***********
 * Classes *
 ***********/
class CpuModel;
typedef CpuModel *CpuModelPtr;

class Cpu;
typedef Cpu *CpuPtr;

class CpuLmm;
typedef CpuLmm *CpuLmmPtr;

class CpuAction;
typedef CpuAction *CpuActionPtr;

class CpuActionLmm;
typedef CpuActionLmm *CpuActionLmmPtr;

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
class Cpu : virtual public Resource {
public:
  Cpu(){};
  Cpu(int core, double powerPeak, double powerScale);
  virtual CpuActionPtr execute(double size)=0;
  virtual CpuActionPtr sleep(double duration)=0;
  virtual int getCore();
  virtual double getSpeed(double load);
  virtual double getAvailableSpeed();

  virtual double getCurrentPowerPeak()=0;
  virtual double getPowerPeakAt(int pstate_index)=0;
  virtual int getNbPstates()=0;
  virtual void setPowerPeakAt(int pstate_index)=0;
  virtual double getConsumedEnergy()=0;

  void addTraces(void);
  int m_core;
  double m_powerPeak;            /*< CPU power peak */
  double m_powerScale;           /*< Percentage of CPU disponible */
protected:

  //virtual boost::shared_ptr<Action> execute(double size) = 0;
  //virtual boost::shared_ptr<Action> sleep(double duration) = 0;
};

class CpuLmm : public ResourceLmm, public Cpu {
public:
  CpuLmm(lmm_constraint_t constraint);
  CpuLmm(lmm_constraint_t constraint, int core, double powerPeak, double powerScale);
  ~CpuLmm();
  /* Note (hypervisor): */
  lmm_constraint_t *p_constraintCore;
  void **p_constraintCoreId;

};

/**********
 * Action *
 **********/
class CpuAction : virtual public Action {
public:
  CpuAction(){};
  CpuAction(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed) {};//FIXME:REMOVE
  virtual void setAffinity(CpuPtr cpu, unsigned long mask)=0;
  virtual void setBound(double bound)=0;
};

class CpuActionLmm : public ActionLmm, public CpuAction {
public:
  CpuActionLmm(){};
  CpuActionLmm(lmm_variable_t var)
  : ActionLmm(var), CpuAction() {};
  void updateRemainingLazy(double now);
  virtual void updateEnergy()=0;
  void setAffinity(CpuPtr cpu, unsigned long mask);
  void setBound(double bound);
  double m_bound;
};


#endif /* SURF_CPU_INTERFACE_HPP_ */
