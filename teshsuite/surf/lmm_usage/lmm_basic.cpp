/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/log.h"

namespace lmm = simgrid::kernel::lmm;

TEST_CASE("kernel::lmm Test on small systems", "[kernel-lmm-small-sys]")
{
  lmm::System* Sys = lmm::make_new_maxmin_system(false);

  SECTION("Basic variable weight")
  {
    /*
     * System under consideration:
     * 1\times\rho_1^{1} + 1\times\rho_2^{2} + 1\times\rho_3^{3} \le 10
     */

    lmm::Constraint* sys_cnst = Sys->constraint_new(nullptr, 10);
    lmm::Variable* sys_var_1  = Sys->variable_new(nullptr, 1, 0.0, 1);
    lmm::Variable* sys_var_2  = Sys->variable_new(nullptr, 2, 0.0, 1);
    lmm::Variable* sys_var_3  = Sys->variable_new(nullptr, 3, 0.0, 1);

    Sys->expand(sys_cnst, sys_var_1, 1);
    Sys->expand(sys_cnst, sys_var_2, 1);
    Sys->expand(sys_cnst, sys_var_3, 1);
    Sys->solve();

    REQUIRE(double_equals(sys_var_1->get_value(), 5.45455, sg_maxmin_precision));
    REQUIRE(double_equals(sys_var_2->get_value(), 2.72727, sg_maxmin_precision));
    REQUIRE(double_equals(sys_var_3->get_value(), 1.81818, sg_maxmin_precision));
  }
}
