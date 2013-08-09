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

class CpuAction;
typedef CpuAction *CpuActionPtr;

/*********
 * Model *
 *********/
class CpuModel : public Model {
public:
  CpuModel(string name) : Model(name) {};
  CpuPtr createResource(string name);

  virtual void addTraces() =0;
};

/************
 * Resource *
 ************/
class Cpu : public Resource {
public:
  Cpu(CpuModelPtr model, string name, xbt_dict_t properties) : Resource(model, name, properties) {};
  CpuActionPtr execute(double size);
  CpuActionPtr sleep(double duration);
  e_surf_resource_state_t getState();
  int getCore();
  double getSpeed(double load);
  double getAvailableSpeed();
  void addTraces(void);

protected:
  double m_powerPeak;
  //virtual boost::shared_ptr<Action> execute(double size) = 0;
  //virtual boost::shared_ptr<Action> sleep(double duration) = 0;
};

/**********
 * Action *
 **********/
class CpuAction : public Action {
public:
  CpuAction(ModelPtr model, double cost, bool failed): Action(model, cost, failed) {};
};

#endif /* SURF_MODEL_CPU_H_ */
