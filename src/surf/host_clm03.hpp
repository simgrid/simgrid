/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/HostImpl.hpp"

#ifndef SURF_HOST_CLM03_HPP_
#define SURF_HOST_CLM03_HPP_

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {

class XBT_PRIVATE HostCLM03Model : public HostModel {
public:
  HostCLM03Model();
  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;
  kernel::resource::Action* execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                                             const double* bytes_amount, double rate) override;
};
}
}

#endif /* SURF_HOST_CLM03_HPP_ */
