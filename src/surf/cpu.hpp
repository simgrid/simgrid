#include "surf.hpp"

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
  Cpu(CpuModelPtr model, const char* name, xbt_dict_t properties) : Resource(model, name, properties) {};
  virtual ActionPtr execute(double size)=0;
  virtual ActionPtr sleep(double duration)=0;
  int getCore();
  double getSpeed(double load);
  double getAvailableSpeed();
  void addTraces(void);
  double m_powerPeak;            /*< CPU power peak */
  double m_powerScale;           /*< Percentage of CPU disponible */
protected:
  int m_core;

  //virtual boost::shared_ptr<Action> execute(double size) = 0;
  //virtual boost::shared_ptr<Action> sleep(double duration) = 0;
};

class CpuLmm : public ResourceLmm, public Cpu {
public:
  CpuLmm(){};
  CpuLmm(CpuModelPtr model, const char* name, xbt_dict_t properties) : ResourceLmm(), Cpu(model, name, properties) {};

};

/**********
 * Action *
 **********/
class CpuAction : virtual public Action {
public:
  CpuAction(){};
  CpuAction(ModelPtr model, double cost, bool failed): Action(model, cost, failed) {};
};

class CpuActionLmm : public ActionLmm, public CpuAction {
public:
  CpuActionLmm(){};
  CpuActionLmm(ModelPtr model, double cost, bool failed): ActionLmm(model, cost, failed), CpuAction(model, cost, failed) {};
  void updateRemainingLazy(double now);
};


#endif /* SURF_MODEL_CPU_H_ */
