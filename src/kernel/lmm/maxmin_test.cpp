/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/log.h"

namespace lmm = simgrid::kernel::lmm;

TEST_CASE("kernel::lmm Single constraint shared systems", "[kernel-lmm-shared-single-sys]")
{
  lmm::System Sys(false);

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

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 3);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 2);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();

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

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 3);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 2);
    Sys.solve();

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

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 20);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 2);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 2);
    Sys.solve();

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

    lmm::Constraint* sys_cnst_1 = Sys.constraint_new(nullptr, 20);
    lmm::Constraint* sys_cnst_2 = Sys.constraint_new(nullptr, 60);

    lmm::Variable* rho_1 = Sys.variable_new(nullptr, 1, -1, 2);
    lmm::Variable* rho_2 = Sys.variable_new(nullptr, 2, -1, 1);
    lmm::Variable* rho_3 = Sys.variable_new(nullptr, 1, -1, 1);

    // Constraint 1
    Sys.expand(sys_cnst_1, rho_1, 1);
    Sys.expand(sys_cnst_1, rho_2, 2);

    // Constraint 2
    Sys.expand(sys_cnst_2, rho_1, 2);
    Sys.expand(sys_cnst_2, rho_3, 1);
    Sys.solve();

    double rho_1_share = 10; // Start by solving the first constraint (results is the same as previous tests)
    REQUIRE(double_equals(rho_1->get_value(), rho_1_share, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), rho_1_share / 2, sg_maxmin_precision));
    REQUIRE(double_equals(rho_3->get_value(), 60 - 2 * rho_1_share, sg_maxmin_precision));
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::lmm Single constraint unshared systems", "[kernel-lmm-unshared-single-sys]")
{
  lmm::System Sys(false);

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

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 10);
    sys_cnst->unshare(); // FATPIPE
    lmm::Variable* rho_1 = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2 = Sys.variable_new(nullptr, 2);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();

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

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 10);
    sys_cnst->unshare(); // FATPIPE
    lmm::Variable* rho_1 = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2 = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 2);
    Sys.solve();

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

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 10);
    sys_cnst->unshare();
    lmm::Variable* sys_var_1 = Sys.variable_new(nullptr, 1);
    lmm::Variable* sys_var_2 = Sys.variable_new(nullptr, 2);

    Sys.expand(sys_cnst, sys_var_1, 1);
    Sys.expand(sys_cnst, sys_var_2, 2);
    Sys.solve();

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

    lmm::Constraint* sys_cnst_1 = Sys.constraint_new(nullptr, 10);
    lmm::Constraint* sys_cnst_2 = Sys.constraint_new(nullptr, 60);
    sys_cnst_1->unshare(); // FATPIPE
    sys_cnst_2->unshare();

    lmm::Variable* rho_1 = Sys.variable_new(nullptr, 1, -1, 2);
    lmm::Variable* rho_2 = Sys.variable_new(nullptr, 2, -1, 1);
    lmm::Variable* rho_3 = Sys.variable_new(nullptr, 1, -1, 1);

    // Constraint 1
    Sys.expand(sys_cnst_1, rho_1, 1);
    Sys.expand(sys_cnst_1, rho_2, 2);

    // Constraint 2
    Sys.expand(sys_cnst_2, rho_1, 2);
    Sys.expand(sys_cnst_2, rho_3, 1);
    Sys.solve();

    double rho_1_share = 10; // Start by solving the first constraint (results is the same as previous tests)
    REQUIRE(double_equals(rho_1->get_value(), rho_1_share, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), rho_1_share / 2, sg_maxmin_precision));
    REQUIRE(double_equals(rho_3->get_value(), 60, sg_maxmin_precision));
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::lmm dynamic constraint shared systems", "[kernel-lmm-shared-single-sys]")
{
  auto cb = [](double bound, int flows) -> double {
    // decrease 10 % for each extra flow sharing this resource
    return bound - (flows - 1) * .10 * bound;
  };
  lmm::System Sys(false);
  lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 10);
  sys_cnst->set_sharing_policy(lmm::Constraint::SharingPolicy::NONLINEAR, cb);

  SECTION("1 activity, 100% C")
  {
    /*
     * A single variable gets all the share
     *
     * In details:
     *   o System:  a1 * p1 * \rho1 < C
     *   o consumption_weight: a1=1
     *   o sharing_penalty:    p1=1
     *
     * Expectations
     *   o rho1 = C (because all weights are 1)
     */

    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 10, sg_maxmin_precision));
  }

  SECTION("2 activities, but ignore crosstraffic 100% C")
  {
    /*
     * Ignore small activities (e.g. crosstraffic)
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=0.05
     *   o sharing_penalty:    p1=1 ; p2=1
     *
     * Expectations
     *   o rho1 = C/1.05
     *   o rho2 = C/1.05
     *   o rho1 = rho2 (because rho1 and rho2 has the same penalty)
     */

    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 0.05);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 10 / 1.05, sg_maxmin_precision));
    REQUIRE(double_equals(rho_1->get_value(), rho_2->get_value(), sg_maxmin_precision));
  }

  SECTION("2 activities, 1 inactive 100% C")
  {
    /*
     * 2 activities but 1 is inactive (sharing_penalty = 0)
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=1
     *   o sharing_penalty:    p1=1 ; p2=0
     *
     * Expectations
     *   o rho1 = C
     *   o rho2 = 0
     */

    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 0);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 10, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 0, sg_maxmin_precision));
  }

  SECTION("2 activity, 90% C")
  {
    /*
     * 2 similar variables degrades performance, but get same share
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < .9C
     *   o consumption_weight: a1=1 ; a2=1
     *   o sharing_penalty:    p1=1 ; p2=1
     *
     * Expectations
     *   o rho1 = rho2
     *   o rho1 + rho2 = C (because all weights are 1)
     */

    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 4.5, sg_maxmin_precision));
    REQUIRE(double_equals(rho_1->get_value(), 4.5, sg_maxmin_precision));
  }

  SECTION("3 activity, 80% C")
  {
    /*
     * 3 similar variables degrades performance, sharing proportional to penalty
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 + a3 * p3 * \rho3 < .8C
     *   o consumption_weight: a1=1 ; a2=1 ; a3=1
     *   o sharing_penalty:    p1=1 ; p2=3 ; p3=4
     *
     * Expectations
     *   o rho1 = 2*rho2 = 2*rho3
     *   0 rho1 = 4, rho2 = 2, rho3 = 2
     *   o rho1 + rho2 + rho3 = .8C (because all weights are 1)
     */

    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 2);
    lmm::Variable* rho_3      = Sys.variable_new(nullptr, 2);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.expand(sys_cnst, rho_3, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 4, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 2, sg_maxmin_precision));
    REQUIRE(double_equals(rho_3->get_value(), 2, sg_maxmin_precision));
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::lmm shared systems with crosstraffic", "[kernel-lmm-shared-crosstraffic]")
{
  lmm::System Sys(false);

  SECTION("3 flows, 3 resource: crosstraffic")
  {
    /*
     * 3 flows sharing 2 constraints, single
     *
     * In details:
     *   o System:  a1 * \rho1 + a2 * \rho2 + epsilon * \rho3 < C1
     *              epsilon * \rho1 + epsilon * \rho2 + a3 * \rho3 < C2
     *   o consumption_weight: a1=1, a2=1, a3=1, epsilon=0.05
     *   o C1 = C2 = 1
     *
     * Expectations
     *   o rho1 = rho2 = rho3 = 1/2
     */
    lmm::Constraint* sys_cnst  = Sys.constraint_new(nullptr, 1);
    lmm::Constraint* sys_cnst2 = Sys.constraint_new(nullptr, 1);
    lmm::Variable* rho_1       = Sys.variable_new(nullptr, 1, -1, 2);
    lmm::Variable* rho_2       = Sys.variable_new(nullptr, 1, -1, 2);
    lmm::Variable* rho_3       = Sys.variable_new(nullptr, 1, -1, 2);

    double epsilon = 0.05;
    Sys.expand(sys_cnst, rho_1, 1.0);
    Sys.expand(sys_cnst2, rho_1, epsilon);
    Sys.expand(sys_cnst, rho_2, 1.0);
    Sys.expand(sys_cnst2, rho_2, epsilon);
    Sys.expand(sys_cnst2, rho_3, 1.0);
    Sys.expand(sys_cnst, rho_3, epsilon);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 1.0 / (2.0 + epsilon), sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 1.0 / (2.0 + epsilon), sg_maxmin_precision));
    REQUIRE(double_equals(rho_3->get_value(), 1.0 / (2.0 + epsilon), sg_maxmin_precision));
  }

  Sys.variable_free_all();
}