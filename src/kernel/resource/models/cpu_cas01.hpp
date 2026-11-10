/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODEL_CPU_CAS01_HPP
#define SIMGRID_MODEL_CPU_CAS01_HPP

#include "src/kernel/resource/CpuImpl.hpp"

namespace simgrid::kernel::resource {

/***********
 * Classes *
 ***********/

class XBT_PRIVATE CpuCas01Model;
class XBT_PRIVATE CpuCas01;
class XBT_PRIVATE CpuCas01Action;

/*********
 * Model *
 *********/

class CpuCas01Model : public CpuModel {
public:
  explicit CpuCas01Model(const std::string& name);
  CpuCas01Model(const CpuCas01Model&)            = delete;
  CpuCas01Model& operator=(const CpuCas01Model&) = delete;

  CpuImpl* create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate) override;
};

/************
 * Resource *
 ************/

class CpuCas01 : public CpuImpl {
  std::function<double(s4u::Host&, double)> factor_cpu_cb_ = {};

public:
  using CpuImpl::CpuImpl;
  CpuCas01(const CpuCas01&)            = delete;
  CpuCas01& operator=(const CpuCas01&) = delete;
  void apply_event(profile::Event* event, double value) override;
  CpuAction* execution_start(double size, double user_bound) override;
  CpuAction* execution_start(double size, int requested_cores, double user_bound) override;
  CpuAction* sleep(double duration) override;
  void set_cpu_factor_cb(const std::function<double(s4u::Host&, double)>& cb) override;

protected:
  void on_speed_change() override;
};

/**********
 * Action *
 **********/
class CpuCas01Action : public CpuAction {
  double requested_cores_ = 1;

protected:
  double get_intrinsic_bound() const override;

public:
  CpuCas01Action(Model* model, double cost, bool failed, double speed, lmm::Constraint* constraint, int requested_core);
  CpuCas01Action(const CpuCas01Action&)            = delete;
  CpuCas01Action& operator=(const CpuCas01Action&) = delete;
  double get_requested_cores() const { return requested_cores_; }
  /** Change the amount of cores that this action may use, and update its bound. It may be fractional, for example
   *  for the action representing a VM on its PM, which only uses the cores needed by the execs it contains. */
  void set_requested_cores(double cores);
};

} // namespace simgrid::kernel::resource

#endif
