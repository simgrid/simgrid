/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"
#include "src/kernel/lmm/bmf.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/log.h"

namespace lmm = simgrid::kernel::lmm;

TEST_CASE("kernel::bmf Basic tests", "[kernel-bmf-basic]")
{
  lmm::BmfSystem Sys(false);

  SECTION("Single flow")
  {
    /*
     * A single variable using a single resource
     *
     * In details:
     *   o System:  a1 * p1 * \rho1 < C
     *   o consumption_weight: a1=1
     *   o sharing_penalty:    p1=1
     *
     * Expectations
     *   o rho1 = C
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 3);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 3, sg_maxmin_precision));
  }

  SECTION("Two flows")
  {
    /*
     * Two flows sharing a single resource
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=10
     *   o sharing_penalty:    p1=1 ; p2=1
     *
     * Expectations
     *   o a1*rho1 = C/2
     *   o a2*rho2 = C/2
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 3);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 10);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 3.0 / 2.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), (3.0 / 2.0) / 10.0, sg_maxmin_precision));
  }

  SECTION("Variable penalty/priority")
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

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 1);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 2);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 2.0 / 3.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 1.0 / 3.0, sg_maxmin_precision));
  }

  SECTION("Disable variable doesn't count")
  {
    /*
     * Two flows sharing a single resource, but one is disabled
     *
     * In details:
     *   o System:  a1 * p1 * \rho1  +  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=10
     *   o sharing_penalty:    p1=1 ; p2=0
     *
     * Expectations
     *   o a1*rho1 = C
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 1);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 0);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 10);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 1.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 0.0, sg_maxmin_precision));
  }

  SECTION("No consumption variable")
  {
    /*
     * An empty variable, no consumption, just assure it receives something
     *
     *   o System:  a1 * p1 * \rho1 < C
     *   o consumption_weight: a1=0
     *   o sharing_penalty:    p1=1
     *
     * Expectations
     *   o rho1 > 0
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 3);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 0);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 10);
    Sys.solve();

    REQUIRE(double_positive(rho_1->get_value(), sg_maxmin_precision));
  }

  SECTION("Bounded variable")
  {
    /*
     * Assures a player receives the min(bound, share) if it's bounded
     *
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C
     *   o bounds: b1=0.1, b2=-1
     *   o consumption_weight: a1=1, a2=1
     *   o sharing_penalty:    p1=1, p2=1
     *
     * Expectations
     *   o rho1 = .1
     *   o rho2 = .8
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 1);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1, .1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 2);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();
    REQUIRE(double_equals(rho_1->get_value(), .1, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), .8, sg_maxmin_precision));
  }

  SECTION("Fatpipe")
  {
    /*
     * Two flows using a fatpipe resource
     *
     * In details:
     *   o System:  a1 * p1 * \rho1 < C and  a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1 ; a2=1
     *   o sharing_penalty:    p1=1 ; p2=1
     *
     * Expectations
     *   o a1*rho1 = C
     *   o a2*rho2 = C
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 3);
    sys_cnst->set_sharing_policy(lmm::Constraint::SharingPolicy::FATPIPE, {});
    lmm::Variable* rho_1 = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2 = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 3.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 3.0, sg_maxmin_precision));
  }

  SECTION("(un)Bounded variable")
  {
    /*
     * Assures a player receives the share if bound is greater than share
     *
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C
     *   o bounds: b1=1, b2=-1
     *   o consumption_weight: a1=1, a2=1
     *   o sharing_penalty:    p1=1, p2=1
     *
     * Expectations
     *   o rho1 = .5
     *   o rho2 = .5
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 1);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();
    REQUIRE(double_equals(rho_1->get_value(), .5, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), .5, sg_maxmin_precision));
  }

  SECTION("Dynamic bounds")
  {
    /*
     * Resource bound is modified by user callback and shares are adapted accordingly
     *
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C
     *   o consumption_weight: a1=1, a2=1
     *   o sharing_penalty:    p1=1, p2=1
     *
     * Expectations
     *   o rho1 = .5 and .25
     *   o rho2 =  - and .25
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 1);
    sys_cnst->set_sharing_policy(lmm::Constraint::SharingPolicy::NONLINEAR,
                                 [](double bound, int n) { return bound / n; });
    // alone, full capacity
    lmm::Variable* rho_1 = Sys.variable_new(nullptr, 1);
    Sys.expand(sys_cnst, rho_1, 1);
    Sys.solve();
    REQUIRE(double_equals(rho_1->get_value(), 1, sg_maxmin_precision));

    // add another variable, half initial capacity
    lmm::Variable* rho_2 = Sys.variable_new(nullptr, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), .25, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), .25, sg_maxmin_precision));
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::bmf Advanced tests", "[kernel-bmf-advanced]")
{
  lmm::BmfSystem Sys(false);

  SECTION("2 flows, 2 resources")
  {
    /*
     * Two flows sharing 2 resources with opposite requirements
     *
     * In details:
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C1
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C2
     *   o C1 == C2
     *   o consumption_weight: a11=1, a12=10, a21=10, a22=1
     *   o sharing_penalty:    p1=1, p2=1
     *
     * Expectations
     *   o rho1 = rho2 = C/11

     * Matrices:
     * [1 10] * [rho1 rho2] = [1]
     * [10 1]                 [1]
     */

    lmm::Constraint* sys_cnst  = Sys.constraint_new(nullptr, 1);
    lmm::Constraint* sys_cnst2 = Sys.constraint_new(nullptr, 1);
    lmm::Variable* rho_1       = Sys.variable_new(nullptr, 1, -1, 2);
    lmm::Variable* rho_2       = Sys.variable_new(nullptr, 1, -1, 2);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst2, rho_1, 10);
    Sys.expand(sys_cnst, rho_2, 10);
    Sys.expand(sys_cnst2, rho_2, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 1.0 / 11.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 1.0 / 11.0, sg_maxmin_precision));
  }

  SECTION("BMF paper example")
  {
    /*
     * 3 flows sharing 3 resources
     *
     * In details:
     * [1  1  1/2] * [rho1 rho2 rho3] = [1]
     * [1 1/2  1 ]                      [1]
     * [1 3/4 3/4]                      [1]
     *
     * Expectations (several possible BMF allocations, our algorithm return this)
     *   o rho1 = rho2 = rho3 = 2/5
     */

    lmm::Constraint* sys_cnst  = Sys.constraint_new(nullptr, 1);
    lmm::Constraint* sys_cnst2 = Sys.constraint_new(nullptr, 1);
    lmm::Constraint* sys_cnst3 = Sys.constraint_new(nullptr, 1);
    lmm::Variable* rho_1       = Sys.variable_new(nullptr, 1, -1, 3);
    lmm::Variable* rho_2       = Sys.variable_new(nullptr, 1, -1, 3);
    lmm::Variable* rho_3       = Sys.variable_new(nullptr, 1, -1, 3);

    Sys.expand(sys_cnst3, rho_1, 1.0); // put this expand first to force a singular A' matrix
    Sys.expand(sys_cnst, rho_1, 1.0);
    Sys.expand(sys_cnst2, rho_1, 1.0);
    Sys.expand(sys_cnst, rho_2, 1.0);
    Sys.expand(sys_cnst2, rho_2, 1.0 / 2.0);
    Sys.expand(sys_cnst3, rho_2, 3.0 / 4.0);
    Sys.expand(sys_cnst, rho_3, 1.0 / 2.0);
    Sys.expand(sys_cnst2, rho_3, 1.0);
    Sys.expand(sys_cnst3, rho_3, 3.0 / 4.0);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 1.0 / 3.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 4.0 / 9.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_3->get_value(), 4.0 / 9.0, sg_maxmin_precision));
  }

  SECTION("IO - example")
  {
    /*
     * Two flows sharing 1 disk
     * read, write and readwrite constraint
     *
     * In details:
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C1
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C2
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C3
     *   o C1 == C2 == C3
     *   o consumption_weight: a1=1, a2=1
     *   o sharing_penalty:    p1=1, p2=1
     *
     * Expectations
     *   o rho1 = rho2 = C/2

     * Matrices:
     * [1 10] * [rho1 rho2] = [1]
     * [10 1]                 [1]
     */

    lmm::Constraint* sys_cnst  = Sys.constraint_new(nullptr, 1e6);
    lmm::Constraint* sys_cnst2 = Sys.constraint_new(nullptr, 1e6);
    lmm::Constraint* sys_cnst3 = Sys.constraint_new(nullptr, 1e6);
    lmm::Variable* rho_1       = Sys.variable_new(nullptr, 1, -1, 3);
    lmm::Variable* rho_2       = Sys.variable_new(nullptr, 1, -1, 3);

    /* A' and C' matrices are dependent on the order of initialization
     * this order is needed to identify a bug in the solver */
    Sys.expand(sys_cnst2, rho_2, 1);
    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst3, rho_1, 1);
    Sys.expand(sys_cnst3, rho_2, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 1e6 / 2.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 1e6 / 2.0, sg_maxmin_precision));
  }

  SECTION("Proportional fairness")
  {
    /*
     * 3 flows sharing 2 resources with crosstraffic
     *
     * Regular max-min would give B/2 for every flow.
     * BMF is equivalent to proportional fairness in this case, and give a quite
     * different sharing.
     *
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

    REQUIRE(double_equals(rho_1->get_value(), 1.0 / (2.0 + 2 * epsilon), sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 1.0 / (2.0 + 2 * epsilon), sg_maxmin_precision));
    REQUIRE(double_equals(rho_3->get_value(), 1.0 / (1.0 + epsilon), sg_maxmin_precision));
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::bmf Subflows", "[kernel-bmf-subflow]")
{
  lmm::BmfSystem Sys(false);

  SECTION("2 subflows and 1 resource")
  {
    /*
     * 2 identical flows composed of 2 subflows
     *
     * They must receive the same share and use same amount of resources
     *
     * In details:
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C
     *   o consumption_weight: a11=5, a12=7, a2=7, a2=5
     *   o sharing_penalty:    p1=1, p2=1
     *
     * Expectations
     *   o rho1 = rho2 = (C/2)/12

     * Matrices:
     * [12 12] * [rho1 rho2] = [1]
     * [12 12]                 [0]
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 5);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 5);
    Sys.expand(sys_cnst, rho_1, 7);
    Sys.expand(sys_cnst, rho_2, 7);
    Sys.expand(sys_cnst, rho_2, 5);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 5.0 / 24.0, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 5.0 / 24.0, sg_maxmin_precision));
  }

  SECTION("1 subflows, 1 flow and 1 resource")
  {
    /*
     * 2 flows, 1 resource
     * 1 flow composed of 2 subflows
     *
     * Same share/rho, but subflow uses 50% more resources since it has a second connection/subflow
     *
     * In details:
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C
     *   o consumption_weight: a11=10, a12=5 a2=10
     *   o sharing_penalty:    p1=1, p2=1
     *
     * Expectations
     *   o rho1 = (C/25)
     *   o rho2 = (C/25)

     * Matrices:
     * [15 10] * [rho1 rho2] = [1]
     * [10 10]                 [0]
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 5);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 10);
    Sys.expand(sys_cnst, rho_1, 5);
    Sys.expand(sys_cnst, rho_2, 10);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), (5.0 / 25.0), sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), (5.0 / 25.0), sg_maxmin_precision));
    REQUIRE(double_equals(15 * rho_1->get_value(), 10 * rho_2->get_value() * 3 / 2, sg_maxmin_precision));
  }

  SECTION("1 subflows using 2 resources: different max for each resource")
  {
    /*
     * Test condition that we may have different max for different resources
     *
     * In details:
     *   o System:  a1 * p1 * \rho1 + a2 * p2 * \rho2 < C
     *   o consumption_weight: a11=1, a12=1, a2=1
     *   o consumption_weight: a21=1/2, a12=1/2 a2=3/2
     *   o sharing_penalty:    p1=1, p2=1
     *
     * Expectations
     *   o rho1 = (C1/3)
     *   o rho2 = (C1/3)

     * Matrices:
     * [2 1 ] * [rho1 rho2] = [1]
     * [1 -1]                 [0]
     */

    lmm::Constraint* sys_cnst  = Sys.constraint_new(nullptr, 1);
    lmm::Constraint* sys_cnst2 = Sys.constraint_new(nullptr, 1);
    lmm::Variable* rho_1       = Sys.variable_new(nullptr, 1, -1, 2);
    lmm::Variable* rho_2       = Sys.variable_new(nullptr, 1, -1, 2);

    Sys.expand(sys_cnst, rho_1, 1.0);
    Sys.expand(sys_cnst, rho_1, 1.0);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.expand(sys_cnst2, rho_1, 1.0 / 2.0);
    Sys.expand(sys_cnst2, rho_1, 1.0 / 2.0);
    Sys.expand(sys_cnst2, rho_2, 3.0 / 2.0);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), (1.0 / 3.0), sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), (1.0 / 3.0), sg_maxmin_precision));
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::bmf Loop", "[kernel-bmf-loop]")
{
  lmm::BmfSystem Sys(false);

  SECTION("Initial allocation loops")
  {
    /*
     * Complex matrix whose initial allocation loops and is unable
     * to stabilize after 10 iterations.
     *
     * The algorithm needs to restart from another point
     */

    std::vector<double> C              = {1.0, 1.0, 1.0, 1.0, 1.0};
    std::vector<std::vector<double>> A = {
        {0.0918589, 0.980201, 0.553352, 0.0471331, 0.397493, 0.0494386, 0.158874, 0.737557, 0.822504, 0.364411},
        {0.852866, 0.383171, 0.924183, 0.318345, 0.937625, 0.980201, 0.0471331, 0.0494386, 0.737557, 0.364411},
        {0.12043, 0.985661, 0.153195, 0.852866, 0.247113, 0.318345, 0.0918589, 0.0471331, 0.158874, 0.364411},
        {0.387291, 0.159939, 0.641492, 0.985661, 0.0540999, 0.383171, 0.318345, 0.980201, 0.0494386, 0.364411},
        {0.722983, 0.924512, 0.474874, 0.819576, 0.572598, 0.0540999, 0.247113, 0.937625, 0.397493, 0.364411}};

    std::vector<lmm::Constraint*> sys_cnst;
    for (auto c : C) {
      sys_cnst.push_back(Sys.constraint_new(nullptr, c));
    }
    std::vector<lmm::Variable*> vars;
    std::for_each(A[0].begin(), A[0].end(),
                  [&vars, &Sys, &A](const auto&) { vars.push_back(Sys.variable_new(nullptr, 1, -1, A.size())); });
    for (size_t j = 0; j < A.size(); j++) {
      for (size_t i = 0; i < A[j].size(); i++) {
        Sys.expand(sys_cnst[j], vars[i], A[j][i]);
      }
    }
    Sys.solve();

    for (const auto* rho : vars) {
      REQUIRE(double_positive(rho->get_value(), sg_maxmin_precision));
    }
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::bmf Bugs", "[kernel-bmf-bug]")
{
  lmm::BmfSystem Sys(false);

  SECTION("DadOu's bug: sum of bounds/phi greater than C")
  {
    /*
     * Ptasks in a g5k platform.
     * Extracted from original test.
     * The sum of bounds for 1 resource exceed its capacity, giving a negative value in C'
     */

    lmm::Constraint* sys_cnst  = Sys.constraint_new(nullptr, 2.5e9);
    lmm::Constraint* sys_cnst2 = Sys.constraint_new(nullptr, 2.5e9);
    lmm::Variable* rho_1       = Sys.variable_new(nullptr, 1, 2.27328e-10, 2);
    lmm::Variable* rho_2       = Sys.variable_new(nullptr, 1, 2.27328e-10, 2);
    lmm::Variable* rho_3       = Sys.variable_new(nullptr, 1);

    Sys.expand(sys_cnst, rho_1, 1.84467e+19);
    Sys.expand(sys_cnst2, rho_1, 1.84467e+19);
    Sys.expand(sys_cnst, rho_2, 1.84467e+19);
    Sys.expand(sys_cnst, rho_3, 1.91268e+11);
    Sys.solve();
  }

  SECTION("DadOu's bug: complete matrix")
  {
    constexpr int cnsts = 71;
    constexpr int flows = 3;

    Eigen::MatrixXd A(cnsts, flows);
    A << 0, 0, 0, 0, 0, 0, 0, 0, 1.84467e+19, 1.91268e+11, 1.84467e+19, 1.84467e+19, 0, 0, 1.84467e+19, 0, 1.84467e+19,
        0, 0, 1.84467e+19, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0,
        4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11,
        0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 1.256e+09, 0, 0, 2.2372e+10, 0, 0,
        2.2684e+10, 0, 0, 2.2588e+10, 0, 0, 2.3188e+10, 0, 0, 2.228e+10, 0, 0, 2.058e+10, 0, 0, 2.2684e+10, 0, 0,
        2.2824e+10, 0, 0, 2.2976e+10, 0, 0, 2.2632e+10, 0, 0, 2.2584e+10, 0, 0, 2.2736e+10, 0, 0, 2.2616e+10, 0, 0,
        2.0828e+10, 0, 0, 2.3184e+10, 0, 0, 2.2524e+10, 0, 0, 2.278e+10, 0, 0, 2.2164e+10, 0, 0, 1.26e+09, 0, 0,
        2.1872e+10, 0, 0, 1.4e+09, 0, 0, 2.3184e+10, 0, 0, 8.52e+08, 0, 0, 2.2268e+10, 0, 0, 1.756e+09, 0, 0,
        2.0636e+10, 0, 0, 3.4e+09, 0, 0, 2.2576e+10, 0, 0, 1.352e+09, 0, 0, 2.2832e+10, 0, 0, 1.2e+09, 0, 0, 2.3092e+10,
        0, 0, 9.48e+08, 0, 0, 2.2436e+10, 0, 0, 1.4e+09, 0, 0, 2.2572e+10, 0, 0, 1.452e+09, 0, 0, 2.2692e+10, 0, 0,
        1.3e+09, 0, 0, 2.2832e+10, 0, 0, 1.2e+09, 0, 0, 2.1232e+10, 0, 0, 2.804e+09, 0, 0, 2.3184e+10, 0, 0,
        8.56001e+08, 0, 0, 2.2512e+10, 0, 0, 1.5e+09, 0, 0;

    Eigen::MatrixXd maxA(cnsts, flows);
    maxA << 0, 0, 0, 0, 0, 0, 0, 0, 1.84467e+19, 3.1e+09, 1.84467e+19, 1.84467e+19, 0, 0, 1.84467e+19, 0, 1.84467e+19,
        0, 0, 1.84467e+19, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 1, 4.975e+11, 0, 0,
        4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11,
        0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 4.975e+11, 0, 0, 1.256e+09, 0, 0, 3.504e+09, 0, 0,
        3.056e+09, 0, 0, 3.1e+09, 0, 0, 2.952e+09, 0, 0, 2.404e+09, 0, 0, 2.304e+09, 0, 0, 2.556e+09, 0, 0, 2.4e+09, 0,
        0, 2.804e+09, 0, 0, 2.552e+09, 0, 0, 2.408e+09, 0, 0, 2.9e+09, 0, 0, 2.4e+09, 0, 0, 2.256e+09, 0, 0, 3.504e+09,
        0, 0, 3.244e+09, 0, 0, 2.556e+09, 0, 0, 2.952e+09, 0, 0, 1.26e+09, 0, 0, 2.552e+09, 0, 0, 1.4e+09, 0, 0,
        3.244e+09, 0, 0, 8.52e+08, 0, 0, 2.556e+09, 0, 0, 1.756e+09, 0, 0, 2.256e+09, 0, 0, 3.4e+09, 0, 0, 2.6e+09, 0,
        0, 1.352e+09, 0, 0, 2.952e+09, 0, 0, 1.2e+09, 0, 0, 2.452e+09, 0, 0, 9.48e+08, 0, 0, 2.804e+09, 0, 0, 1.4e+09,
        0, 0, 3.1e+09, 0, 0, 1.452e+09, 0, 0, 2.404e+09, 0, 0, 1.3e+09, 0, 0, 2.952e+09, 0, 0, 1.2e+09, 0, 0, 3.056e+09,
        0, 0, 2.804e+09, 0, 0, 2.4e+09, 0, 0, 8.56001e+08, 0, 0, 2.9e+09, 0, 0, 1.5e+09, 0, 0;

    Eigen::VectorXd C(cnsts);
    C << 3.2e+20, 3.2e+20, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20,
        3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 3.2e+20, 1.83484e+10,
        2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09,
        2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10,
        2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10,
        2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10,
        2.5e+09, 1.83484e+10, 2.5e+09, 1.83484e+10;

    Eigen::VectorXd phi(flows);
    phi << 1.35273, 2.27328e-10, 2.27328e-10;

    std::vector<bool> shared(cnsts, true);
    lmm::BmfSolver solver(A, maxA, C, shared, phi);
    auto rho = solver.solve();
    REQUIRE((rho.array() > 0).all());
  }

  SECTION("is_bmf bug: all limited by bound")
  {
    /*
     * Particular case, 1 flow is saturated and the other increases
     * its speed until the contraint is reached
     */

    lmm::Constraint* sys_cnst  = Sys.constraint_new(nullptr, 10);
    lmm::Constraint* sys_cnst2 = Sys.constraint_new(nullptr, 8);
    lmm::Variable* rho_1       = Sys.variable_new(nullptr, 1, 1.5, 2);
    lmm::Variable* rho_2       = Sys.variable_new(nullptr, 1, 3, 2);

    Sys.expand(sys_cnst, rho_1, 5.0);
    Sys.expand(sys_cnst2, rho_1, 1.0);
    Sys.expand(sys_cnst, rho_2, 1.0);
    Sys.expand(sys_cnst2, rho_2, 1.0);
    Sys.solve();
    REQUIRE(double_equals(rho_1->get_value(), 1.4, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 3, sg_maxmin_precision));
  }

  SECTION("s4u-cloud-capping bug: all limited by bound extra case")
  {
    /*
     * Inspired by s4u-cloud-capping test.
     * Step 9: "Put an activity on a PM and an activity on the VM capped by 10%."
     * Extracted from original test.
     * The sum of bounds for 1 resource is smaller than C.
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 7.6296e+07);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1, 7.6296e+06, 1);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, 1, 3.8148e+07, 1);

    Sys.expand(sys_cnst, rho_1, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.solve();
    REQUIRE(double_equals(rho_1->get_value(), 7.6296e+06, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 3.8148e+07, sg_maxmin_precision));
  }

  SECTION("Variable penalty with bounds: thread bug")
  {
    /*
     * Detected by exec-thread.
     * Fair-sharing vector depends on the penalty too.
     */

    /* don't change the order of creation, important to trigger the bug */
    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 4e8);
    lmm::Variable* rho_2      = Sys.variable_new(nullptr, .25, 4e8, 1);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1, 1e8, 1);
    Sys.expand(sys_cnst, rho_2, 1);
    Sys.expand(sys_cnst, rho_1, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 8e7, sg_maxmin_precision));
    REQUIRE(double_equals(rho_2->get_value(), 3.2e8, sg_maxmin_precision));
  }

  SECTION("Variable penalty with bounds greater than C")
  {
    /*
     * Detected by exec-thread. 6 thread running in a 4 core CPU.
     */

    lmm::Constraint* sys_cnst = Sys.constraint_new(nullptr, 4e8);
    lmm::Variable* rho_1      = Sys.variable_new(nullptr, 1.0 / 6.0, 6e8, 1);
    Sys.expand(sys_cnst, rho_1, 1);
    Sys.solve();

    REQUIRE(double_equals(rho_1->get_value(), 4e8, sg_maxmin_precision));
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::bmf Stress-tests", "[.kernel-bmf-stress]")
{
  lmm::BmfSystem Sys(false);

  auto create_cnsts = [&Sys](int C, int capacity) -> auto
  {
    std::vector<lmm::Constraint*> sys_cnst;
    for (int i = 0; i < C; i++) {
      sys_cnst.push_back(Sys.constraint_new(nullptr, capacity));
    }
    return sys_cnst;
  };

  auto test_shared = [&Sys, &create_cnsts](int C, int N, int capacity, const auto& data) {
    auto sys_cnst = create_cnsts(C, capacity);
    for (int j = 0; j < N; j++) {
      lmm::Variable* rho = Sys.variable_new(nullptr, 1, -1, C);
      for (int i = 0; i < C; i++) {
        Sys.expand(sys_cnst[i], rho, data[i * j + j]);
      }
    }
    Sys.solve();
  };

  SECTION("Random consumptions - independent flows")
  {
    int C         = 5;
    int N         = 2;
    auto data     = GENERATE_COPY(chunk(C * N, take(100000, random(0., 1.0))));
    auto sys_cnst = create_cnsts(C, 1);
    for (int j = 0; j < N; j++) {
      for (int i = 0; i < C; i++) {
        lmm::Variable* rho = Sys.variable_new(nullptr, 1);
        Sys.expand(sys_cnst[i], rho, data[i * j + j]);
      }
    }
    Sys.solve();
  }

  SECTION("Random consumptions - flows sharing resources")
  {
    int C     = 5;
    int N     = 10;
    auto data = GENERATE_COPY(chunk(C * N, take(100000, random(0., 1.0))));
    test_shared(C, N, 1, data);
  }

  SECTION("Random integer consumptions - flows sharing resources")
  {
    int C     = 5;
    int N     = 10;
    auto data = GENERATE_COPY(chunk(C * N, take(100000, random(1, 10))));
    test_shared(C, N, 10, data);
  }

  SECTION("Random consumptions - high number of constraints")
  {
    int C     = 500;
    int N     = 10;
    auto data = GENERATE_COPY(chunk(C * N, take(100000, random(0., 1.0))));
    test_shared(C, N, 1, data);
  }

  SECTION("Random integer consumptions - high number of constraints")
  {
    int C     = 500;
    int N     = 10;
    auto data = GENERATE_COPY(chunk(C * N, take(100000, random(1, 10))));
    test_shared(C, N, 10, data);
  }

  Sys.variable_free_all();
}

TEST_CASE("kernel::AllocationGenerator Basic tests", "[kernel-bmf-allocation-gen]")
{
  SECTION("Full combinations")
  {
    Eigen::MatrixXd A(3, 3);
    A << 1, .5, 1, 1, 1, .5, 1, .75, .75;
    lmm::AllocationGenerator gen(std::move(A));
    int i = 0;
    std::vector<int> alloc;
    while (gen.next(alloc))
      i++;
    REQUIRE(i == 3 * 3 * 3);
  }

  SECTION("Few options per player")
  {
    Eigen::MatrixXd A(3, 3);
    A << 1, 0, 0, 0, 1, 0, 0, 1, 1;
    lmm::AllocationGenerator gen(std::move(A));
    int i = 0;
    std::vector<int> alloc;
    while (gen.next(alloc))
      i++;
    REQUIRE(i == 1 * 2 * 1);
  }
}
