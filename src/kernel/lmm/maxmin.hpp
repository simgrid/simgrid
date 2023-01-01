/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_LMM_MAXMIN_HPP
#define SIMGRID_KERNEL_LMM_MAXMIN_HPP

#include "src/kernel/lmm/System.hpp"

namespace simgrid::kernel::lmm {

class XBT_PUBLIC MaxMin : public System {
public:
  using System::System;

private:
  void do_solve() final;
  template <class CnstList> void maxmin_solve(CnstList& cnst_list);

  using dyn_light_t = std::vector<int>;

  std::vector<ConstraintLight> cnst_light_vec;
  dyn_light_t saturated_constraints;
};

} // namespace simgrid::kernel::lmm

#endif
