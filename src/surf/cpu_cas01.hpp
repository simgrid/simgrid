/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

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

class CpuCas01Model : public kernel::resource::CpuModel {
public:
  explicit CpuCas01Model(kernel::resource::Model::UpdateAlgo algo);
  CpuCas01Model(const CpuCas01Model&) = delete;
  CpuCas01Model& operator=(const CpuCas01Model&) = delete;
  ~CpuCas01Model() override;

  kernel::resource::Cpu* create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate, int core) override;
};

/************
 * Resource *
 ************/

class CpuCas01 : public kernel::resource::Cpu {
public:
  CpuCas01(CpuCas01Model* model, simgrid::s4u::Host* host, const std::vector<double>& speed_per_pstate, int core);
  CpuCas01(const CpuCas01&) = delete;
  CpuCas01& operator=(const CpuCas01&) = delete;
  ~CpuCas01() override;
  void apply_event(simgrid::kernel::profile::Event* event, double value) override;
  kernel::resource::CpuAction* execution_start(double size) override;
  kernel::resource::CpuAction* execution_start(double size, int requested_cores) override;
  kernel::resource::CpuAction* sleep(double duration) override;

  bool is_used() override;

protected:
  void on_speed_change() override;
};

/**********
 * Action *
 **********/
class CpuCas01Action : public kernel::resource::CpuAction {
  friend kernel::resource::CpuAction* CpuCas01::execution_start(double size);
  friend kernel::resource::CpuAction* CpuCas01::sleep(double duration);

public:
  CpuCas01Action(kernel::resource::Model* model, double cost, bool failed, double speed,
                 kernel::lmm::Constraint* constraint, int core_count);
  CpuCas01Action(kernel::resource::Model* model, double cost, bool failed, double speed,
                 kernel::lmm::Constraint* constraint);
  CpuCas01Action(const CpuCas01Action&) = delete;
  CpuCas01Action& operator=(const CpuCas01Action&) = delete;
  ~CpuCas01Action() override;
  int requested_core();

private:
  int requested_core_ = 1;
};

}
}
