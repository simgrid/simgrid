/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SURF_CPUCAS01_HPP
#define SIMGRID_SURF_CPUCAS01_HPP

#include "src/kernel/resource/CpuImpl.hpp"
#include "xbt/base.h"

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace kernel {
namespace resource {

class XBT_PRIVATE CpuCas01Model;
class XBT_PRIVATE CpuCas01;
class XBT_PRIVATE CpuCas01Action;

/*********
 * Model *
 *********/

class CpuCas01Model : public CpuModel {
public:
  explicit CpuCas01Model(const std::string& name);
  CpuCas01Model(const CpuCas01Model&) = delete;
  CpuCas01Model& operator=(const CpuCas01Model&) = delete;

  CpuImpl* create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate) override;
};

/************
 * Resource *
 ************/

class CpuCas01 : public CpuImpl {
  std::function<s4u::Host::CpuFactorCb> factor_cb_ = {};

public:
  using CpuImpl::CpuImpl;
  CpuCas01(const CpuCas01&) = delete;
  CpuCas01& operator=(const CpuCas01&) = delete;
  void apply_event(profile::Event* event, double value) override;
  CpuAction* execution_start(double size, double user_bound) override;
  CpuAction* execution_start(double size, int requested_cores, double user_bound) override;
  CpuAction* sleep(double duration) override;
  void set_factor_cb(const std::function<s4u::Host::CpuFactorCb>& cb) override;

protected:
  void on_speed_change() override;
};

/**********
 * Action *
 **********/
class CpuCas01Action : public CpuAction {
  int requested_core_ = 1;

public:
  CpuCas01Action(Model* model, double cost, bool failed, double speed, lmm::Constraint* constraint, int requested_core);
  CpuCas01Action(const CpuCas01Action&) = delete;
  CpuCas01Action& operator=(const CpuCas01Action&) = delete;
  int requested_core() const { return requested_core_; }
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif
