/* Copyright (c) 2013-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/HostImpl.hpp"

#ifndef HOST_CLM03_HPP_
#define HOST_CLM03_HPP_

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace kernel {
namespace resource {

class XBT_PRIVATE HostCLM03Model : public HostModel {
public:
  using HostModel::HostModel;
  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;
  Action* execute_thread(const s4u::Host* host, double flops_amount, int thread_count) override;
  Action* execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                           const double* bytes_amount, double rate) override;
};
} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* HOST_CLM03_HPP_ */
