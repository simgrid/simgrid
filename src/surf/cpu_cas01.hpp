/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "cpu_interface.hpp"

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {

class XBT_PRIVATE CpuCas01Model;
class XBT_PRIVATE CpuCas01;
class XBT_PRIVATE CpuCas01Action;

/*********
 * Model *
 *********/

class CpuCas01Model : public simgrid::surf::CpuModel {
public:
  CpuCas01Model();
  ~CpuCas01Model() override;

  Cpu *createCpu(simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core) override;
  ActionList p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
};

/************
 * Resource *
 ************/

class CpuCas01 : public Cpu {
public:
  CpuCas01(CpuCas01Model *model, simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core);
  ~CpuCas01() override;
  void apply_event(tmgr_trace_event_t event, double value) override;
  CpuAction *execution_start(double size) override;
  CpuAction* execution_start(double size, int requestedCore) override;
  CpuAction *sleep(double duration) override;

  bool isUsed() override;

  std::vector<double> * getSpeedPeakList(); // FIXME: killme to hide our internals

protected:
  void onSpeedChange() override;
};

/**********
 * Action *
 **********/
class CpuCas01Action: public CpuAction {
  friend CpuAction *CpuCas01::execution_start(double size);
  friend CpuAction *CpuCas01::sleep(double duration);
public:
  CpuCas01Action(Model* model, double cost, bool failed, double speed, lmm_constraint_t constraint, int coreAmount);
  CpuCas01Action(Model* model, double cost, bool failed, double speed, lmm_constraint_t constraint);
  ~CpuCas01Action() override;
  int requestedCore();

private:
  int requestedCore_ = 1;
};

}
}
