/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/log.h"

namespace lmm = simgrid::kernel::lmm;

TEST_CASE("kernel::lmm Single constraint shared systems", "[kernel-lmm-shared-single-sys]")
{
  lmm::System* Sys = lmm::make_new_maxmin_system(false);

  SECTION("Variable penalty")
  {
    /*
     * A variable with twice the penalty gets half of the share
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=1
     *   o sharing_penalty:    p1=1 ; p2=2
     *
     * Expectations
     *   o rho1 = 2* rho2 (because rho2 has twice the penalty)
     *   o rho1 + rho2 = C (because all weights are 1)
     */

    lmm::Constraint* sys_cnst = Sys->constraint_new(nullptr, 3);
    lmm::Variable* rho_1      = Sys->variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys->variable_new(nullptr, 2);

    Sys->expand(sys_cnst, rho_1, 1);
    Sys->expand(sys_cnst, rho_2, 1);
    Sys->solve();

    REQUIRE(double_equals(rho_1->get_value(), 2, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 1, sg_maxmin_precision));
  }

  SECTION("Consumption weight")
  {
    /*
     * Variables of higher consumption weight consume more resource but get the same share
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=2
     *   o sharing_penalty:    p1=1 ; p2=1
     *
     * Expectations
     *   o rho1 = rho2 (because all penalties are 1)
     *   o rho1 + 2* rho2 = C (because weight_2 is 2)
     *   o so, rho1 = rho2 = 1 (because C is 3)
     */

    lmm::Constraint* sys_cnst = Sys->constraint_new(nullptr, 3);
    lmm::Variable* rho_1      = Sys->variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys->variable_new(nullptr, 1);

    Sys->expand(sys_cnst, rho_1, 1);
    Sys->expand(sys_cnst, rho_2, 2);
    Sys->solve();

    REQUIRE(double_equals(rho_1->get_value(), 1, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 1, sg_maxmin_precision));
  }

  SECTION("Consumption weight + variable penalty")
  {

    /*
     * Resource proportionality between variable is kept while
     * varying consumption weight
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=2
     *   o sharing_penalty:    p1=1 ; p2=2
     *
     * Expectations
     *   o rho1 = 2* rho2 (because rho2 has twice the penalty)
     *   o rho1 + 2*rho2 = C (because consumption weight of rho2 is 2)
     */

    lmm::Constraint* sys_cnst = Sys->constraint_new(nullptr, 20);
    lmm::Variable* rho_1      = Sys->variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys->variable_new(nullptr, 2);

    Sys->expand(sys_cnst, rho_1, 1);
    Sys->expand(sys_cnst, rho_2, 2);
    Sys->solve();

    double rho_1_share = 10;
    REQUIRE(double_equals(rho_1->get_value(), rho_1_share, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), rho_1_share / 2, sg_maxmin_precision));
  }

  SECTION("Multiple constraints systems")
  {

    /*
     * Multiple constraint systems can be solved with shared variables
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C1
     *              a3 * p1 * \rho1  +  a4 * p3 * \rho3 < C2
     *   o consumption_weight: a1=1 ; a2=2 ; a3=2 ; a4=1
     *   o sharing_penalty:    p1=1 ; p2=2 ; p3=1
     *   o load: load_1=C1/(p1/a1 + p2/a2)=20 ; load_2=C2/(a2/p1 + a3/p3)=30
     *
     * Expectations
     *   o First constraint will be solve first (because load_1 < load_2)
     *   o rho1 = 2* rho2 (because rho2 has twice the penalty)
     *   o rho1 + 2*rho2 = C1 (because consumption weight of rho2 is 2)
     *   o 2*rho1 + rho3 = C2 (because consumption weight of rho1 is 2)
     */

    lmm::Constraint* sys_cnst_1 = Sys->constraint_new(nullptr, 20);
    lmm::Constraint* sys_cnst_2 = Sys->constraint_new(nullptr, 60);

    lmm::Variable* rho_1 = Sys->variable_new(nullptr, 1, -1, 2);
    lmm::Variable* rho_2 = Sys->variable_new(nullptr, 2, -1, 1);
    lmm::Variable* rho_3 = Sys->variable_new(nullptr, 1, -1, 1);

    // Constraint 1
    Sys->expand(sys_cnst_1, rho_1, 1);
    Sys->expand(sys_cnst_1, rho_2, 2);

    // Constraint 2
    Sys->expand(sys_cnst_2, rho_1, 2);
    Sys->expand(sys_cnst_2, rho_3, 1);
    Sys->solve();

    double rho_1_share = 10; // Start by solving the first constraint (results is the same as previous tests)
    REQUIRE(double_equals(rho_1->get_value(), rho_1_share, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), rho_1_share / 2, sg_maxmin_precision));
    REQUIRE(double_equals(rho_3->get_value(), 60 - 2 * rho_1_share, sg_maxmin_precision));
  }

  Sys->variable_free_all();
  delete Sys;
}

TEST_CASE("kernel::lmm Single constraint unshared systems", "[kernel-lmm-unshared-single-sys]")
{
  lmm::System* Sys = lmm::make_new_maxmin_system(false);

  SECTION("Variable penalty")
  {

    /*
     * A variable with a penalty of two get half of the max_share
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C1
     *   o consumption_weight: a1=1 ; a2=1
     *   o sharing_penalty:    p1=1 ; p2=2
     *   o max_share: max(C1/(a1/p1),C1/(a2/p2))
     *
     * Expectations
     *   o rho1 = max_share
     *   o rho2 = max_share/2 (because penalty of rho2 is 2)
     */

    lmm::Constraint* sys_cnst = Sys->constraint_new(nullptr, 10);
    sys_cnst->unshare(); // FATPIPE
    lmm::Variable* rho_1 = Sys->variable_new(nullptr, 1);
    lmm::Variable* rho_2 = Sys->variable_new(nullptr, 2);

    Sys->expand(sys_cnst, rho_1, 1);
    Sys->expand(sys_cnst, rho_2, 1);
    Sys->solve();

    REQUIRE(double_equals(rho_1->get_value(), 10, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 10 / 2, sg_maxmin_precision));
  }

  SECTION("Consumption weight")
  {

    /*
     * In a given constraint with all variable penalty to 1,
     * the max_share is affected only by the maximum consumption weight
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C1
     *   o consumption_weight: a1=1 ; a2=1
     *   o sharing_penalty:    p1=1 ; p2=2
     *   o max_share: max(C1/(a1/p1),C1/(a2/p2))
     *
     * Expectations
     *   o rho1 = max_share/2 (because penalty of rho1 is 1)
     *   o rho2 = max_share/2 (because penalty of rho2 is 1)
     */

    lmm::Constraint* sys_cnst = Sys->constraint_new(nullptr, 10);
    sys_cnst->unshare(); // FATPIPE
    lmm::Variable* rho_1 = Sys->variable_new(nullptr, 1);
    lmm::Variable* rho_2 = Sys->variable_new(nullptr, 1);

    Sys->expand(sys_cnst, rho_1, 1);
    Sys->expand(sys_cnst, rho_2, 2);
    Sys->solve();

    REQUIRE(double_equals(rho_1->get_value(), 5, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 5, sg_maxmin_precision));
  }

  SECTION("Consumption weight + variable penalty")
  {

    /*
     * Resource proportionality between variable is kept but
     * constraint bound can be violated
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=2
     *   o sharing_penalty:    p1=1 ; p2=2
     *
     * Expectations
     *   o rho1 = 2 * rho2 (because rho2 has twice the penalty)
     *   o rho1 + 2*rho2 can be greater than C
     *   o rho1 <= C and 2*rho2 <= C
     */

    lmm::Constraint* sys_cnst = Sys->constraint_new(nullptr, 10);
    sys_cnst->unshare();
    lmm::Variable* sys_var_1 = Sys->variable_new(nullptr, 1);
    lmm::Variable* sys_var_2 = Sys->variable_new(nullptr, 2);

    Sys->expand(sys_cnst, sys_var_1, 1);
    Sys->expand(sys_cnst, sys_var_2, 2);
    Sys->solve();

    REQUIRE(double_equals(sys_var_1->get_value(), 10, sg_maxmin_precision));
    REQUIRE(double_equals(sys_var_2->get_value(), 5, sg_maxmin_precision));
  }

  SECTION("Multiple constraints systems")
  {

    /*
     * Multiple constraint systems can be solved with shared variables
     * on unshared constraints.
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C1
     *              a3 * p1 * \rho1  +  a4 * p3 * \rho3 < C2
     *   o consumption_weight: a1=1 ; a2=2 ; a3=2 ; a4=1
     *   o sharing_penalty:    p1=1 ; p2=2 ; p3=1
     *   o load: load_1=C1/max(p1/a1,p2/a2)=20 ; load_2=C2/max(a3/p1,a4/p3)=30
     *
     * Expectations
     *   o First constraint will be solve first (because load_1 < load_2)
     *   o Second constraint load will not be updated !
     *   o Each constraint should satisfy max(a_i * rho_i) <= C_r
     */

    lmm::Constraint* sys_cnst_1 = Sys->constraint_new(nullptr, 10);
    lmm::Constraint* sys_cnst_2 = Sys->constraint_new(nullptr, 60);
    sys_cnst_1->unshare(); // FATPIPE
    sys_cnst_2->unshare();

    lmm::Variable* rho_1 = Sys->variable_new(nullptr, 1, -1, 2);
    lmm::Variable* rho_2 = Sys->variable_new(nullptr, 2, -1, 1);
    lmm::Variable* rho_3 = Sys->variable_new(nullptr, 1, -1, 1);

    // Constraint 1
    Sys->expand(sys_cnst_1, rho_1, 1);
    Sys->expand(sys_cnst_1, rho_2, 2);

    // Constraint 2
    Sys->expand(sys_cnst_2, rho_1, 2);
    Sys->expand(sys_cnst_2, rho_3, 1);
    Sys->solve();

    double rho_1_share = 10; // Start by solving the first constraint (results is the same as previous tests)
    REQUIRE(double_equals(rho_1->get_value(), rho_1_share, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), rho_1_share / 2, sg_maxmin_precision));
    REQUIRE(double_equals(rho_3->get_value(), 60, sg_maxmin_precision));
  }

  Sys->variable_free_all();
  delete Sys;
}
