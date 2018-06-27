/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_interface.hpp"
#include "xbt/base.h"

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
  explicit CpuCas01Model(kernel::resource::Model::UpdateAlgo algo);
  ~CpuCas01Model() override;

  Cpu* create_cpu(simgrid::s4u::Host* host, std::vector<double>* speed_per_pstate, int core) override;
};

/************
 * Resource *
 ************/

class CpuCas01 : public Cpu {
public:
  CpuCas01(CpuCas01Model* model, simgrid::s4u::Host* host, std::vector<double>* speed_per_pstate, int core);
  ~CpuCas01() override;
  void apply_event(tmgr_trace_event_t event, double value) override;
  CpuAction* execution_start(double size) override;
  CpuAction* execution_start(double size, int requested_cores) override;
  CpuAction* sleep(double duration) override;

  bool is_used() override;

protected:
  void on_speed_change() override;
};

/**********
 * Action *
 **********/
class CpuCas01Action: public CpuAction {
  friend CpuAction *CpuCas01::execution_start(double size);
  friend CpuAction *CpuCas01::sleep(double duration);
public:
  CpuCas01Action(kernel::resource::Model* model, double cost, bool failed, double speed,
                 kernel::lmm::Constraint* constraint, int core_count);
  CpuCas01Action(kernel::resource::Model* model, double cost, bool failed, double speed,
                 kernel::lmm::Constraint* constraint);
  ~CpuCas01Action() override;
  int requested_core();

private:
  int requested_core_ = 1;
};

}
}
