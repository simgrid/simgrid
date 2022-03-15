/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_LMM_FAIR_BOTTLENECK_HPP
#define SIMGRID_KERNEL_LMM_FAIR_BOTTLENECK_HPP

#include "src/kernel/lmm/System.hpp"

namespace simgrid {
namespace kernel {
namespace lmm {

class XBT_PUBLIC FairBottleneck : public System {
public:
  using System::System;

private:
  void do_solve() final;
};

} // namespace lmm
} // namespace kernel
} // namespace simgrid

#endif
