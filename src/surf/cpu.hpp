#include "surf.hpp"
#include "maxmin_private.h"

#ifndef SURF_MODEL_CPU_H_
#define SURF_MODEL_CPU_H_

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
  CpuModel(string name) : Model(name) {};
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
  Cpu(CpuModelPtr model, const char* name, xbt_dict_t properties, int core, double powerPeak, double powerScale)
   : Resource(model, name, properties), m_core(core), m_powerPeak(powerPeak), m_powerScale(powerScale)
   {};
  virtual ActionPtr execute(double size)=0;
  virtual ActionPtr sleep(double duration)=0;
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
  CpuLmm() : p_constraintCore(NULL) {};
  CpuLmm(CpuModelPtr model, const char* name, xbt_dict_t properties, int core, double powerPeak, double powerScale);
  ~CpuLmm();
  /* Note (hypervisor): */
  lmm_constraint_t *p_constraintCore;
};

/**********
 * Action *
 **********/
class CpuAction : virtual public Action {
public:
  CpuAction(){};
  CpuAction(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed) {};
  virtual void setAffinity(CpuPtr cpu, unsigned long mask)=0;
  virtual void setBound(double bound)=0;
};

class CpuActionLmm : public ActionLmm, public CpuAction {
public:
  CpuActionLmm(){};
  CpuActionLmm(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed), ActionLmm(model, cost, failed), CpuAction(model, cost, failed) {};
  void updateRemainingLazy(double now);
  virtual void updateEnergy()=0;
  void setAffinity(CpuPtr cpu, unsigned long mask);
  void setBound(double bound);
  double m_bound;
};


#endif /* SURF_MODEL_CPU_H_ */
