/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

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
  ~CpuCas01Model();

  Cpu *createCpu(simgrid::s4u::Host *host, xbt_dynar_t speedPeak, int pstate,
                   double speedScale,
                          tmgr_trace_t speedTrace, int core,
                          int initiallyOn,
                          tmgr_trace_t state_trace) override;
  double shareResourcesFull(double now) override;
  void addTraces() override;
  ActionList *p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
};

/************
 * Resource *
 ************/

class CpuCas01 : public Cpu {
  friend CpuCas01Model;
public:
  CpuCas01(CpuCas01Model *model, simgrid::s4u::Host *host, xbt_dynar_t speedPeak,
        int pstate, double speedScale, tmgr_trace_t speedTrace, int core,
        int initiallyOn, tmgr_trace_t stateTrace) ;
  ~CpuCas01();
  void updateState(tmgr_trace_iterator_t event_type, double value, double date) override;
  CpuAction *execution_start(double size) override;
  CpuAction *sleep(double duration) override;

  bool isUsed() override;

  xbt_dynar_t getSpeedPeakList(); // FIXME: killme to hide our internals

protected:
  void onSpeedChange() override;

private:

  tmgr_trace_iterator_t p_stateEvent = nullptr;
  tmgr_trace_iterator_t p_speedEvent = nullptr;
};

/**********
 * Action *
 **********/
class CpuCas01Action: public CpuAction {
  friend CpuAction *CpuCas01::execution_start(double size);
  friend CpuAction *CpuCas01::sleep(double duration);
public:
  CpuCas01Action(Model *model, double cost, bool failed, double speed,
                 lmm_constraint_t constraint);

  ~CpuCas01Action() {};
};

}
}
