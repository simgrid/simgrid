/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/ExtensionSetCalculator.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"

using namespace simgrid::mc;
using namespace simgrid::mc::udpor;

TEST_CASE("simgrid::mc::udpor: Testing Computation with AsyncSend/AsyncReceive Only")
{
  // This test checks that the unfolding constructed for the very
  // simple program described below is extended correctly. Each
  // step of UDPOR is observed to ensure that computations are carried
  // out correctly. The program described is simply:
  //
  //      1                 2
  // AsyncSend(m)      AsyncRecv(m)
  //
  // The unfolding of the simple program is as follows:
  //
  //                         ⊥
  //                  /            /
  //       (a) 1: AsyncSend(m)  (b) 2: AsyncRecv(m)

  const int times_considered = 0;
  const int tag              = 0;
  const unsigned mbox        = 0;
  const uintptr_t comm       = 0;

  Unfolding U;

  SECTION("Computing ex({⊥}) with 1: AsyncSend")
  {
    // Consider the extension with `1: AsyncSend(m)`
    Configuration C;
    aid_t issuer          = 1;
    const uintptr_t sbuff = 0;
    const size_t size     = 100;

    const auto async_send =
        std::make_shared<CommSendTransition>(issuer, times_considered, comm, mbox, sbuff, size, tag);
    const auto incremental_exC = ExtensionSetCalculator::partially_extend(C, &U, async_send);

    // Check that the events have been added to `U`
    REQUIRE(U.size() == 1);

    // Make assertions about the contents of ex(C)
    UnfoldingEvent e(EventSet(), async_send);
    REQUIRE(incremental_exC.contains_equivalent_to(&e));
  }

  SECTION("Computing ex({⊥}) with 2: AsyncRecv")
  {
    // Consider the extension with `2: AsyncRecv(m)`
    Configuration C;
    aid_t issuer          = 2;
    const uintptr_t rbuff = 0;

    const auto async_recv      = std::make_shared<CommRecvTransition>(issuer, times_considered, comm, mbox, rbuff, tag);
    const auto incremental_exC = ExtensionSetCalculator::partially_extend(C, &U, async_recv);

    // Check that the events have been added to `U`
    REQUIRE(U.size() == 1);

    // Make assertions about the contents of ex(C)
    UnfoldingEvent e(EventSet(), async_recv);
    REQUIRE(incremental_exC.contains_equivalent_to(&e));
  }

  SECTION("Computing ex({⊥}) fully")
  {
    // Consider the extension with `1: AsyncSend(m)`
    Configuration C;
    const uintptr_t rbuff = 0;
    const uintptr_t sbuff = 0;
    const size_t size     = 100;

    const auto async_send = std::make_shared<CommSendTransition>(1, times_considered, comm, mbox, sbuff, size, tag);
    const auto incremental_exC_send = ExtensionSetCalculator::partially_extend(C, &U, async_send);

    // Check that the events have been added to `U`
    REQUIRE(U.size() == 1);

    // Make assertions about the contents of ex(C)
    UnfoldingEvent e_send(EventSet(), async_send);
    REQUIRE(incremental_exC_send.contains_equivalent_to(&e_send));

    // Consider the extension with `2: AsyncRecv(m)`
    const auto async_recv           = std::make_shared<CommRecvTransition>(2, times_considered, comm, mbox, rbuff, tag);
    const auto incremental_exC_recv = ExtensionSetCalculator::partially_extend(C, &U, async_recv);

    // Check that the events have been added to `U`
    REQUIRE(U.size() == 2);

    // Make assertions about the contents of ex(C)
    UnfoldingEvent e_recv(EventSet(), async_recv);
    REQUIRE(incremental_exC_recv.contains_equivalent_to(&e_recv));
  }

  SECTION("Computing the full sequence of extensions")
  {
    Configuration C;
    const uintptr_t rbuff = 0;
    const uintptr_t sbuff = 0;
    const size_t size     = 100;

    // Consider the extension with `1: AsyncSend(m)`
    const auto async_send = std::make_shared<CommSendTransition>(1, times_considered, comm, mbox, sbuff, size, tag);
    const auto incremental_exC_send = ExtensionSetCalculator::partially_extend(C, &U, async_send);

    // Check that event `a` has been added to `U`
    REQUIRE(U.size() == 1);
    UnfoldingEvent e_send(EventSet(), async_send);
    REQUIRE(incremental_exC_send.contains_equivalent_to(&e_send));

    // Consider the extension with `2: AsyncRecv(m)`
    const auto async_recv           = std::make_shared<CommRecvTransition>(2, times_considered, comm, mbox, rbuff, tag);
    const auto incremental_exC_recv = ExtensionSetCalculator::partially_extend(C, &U, async_recv);

    // Check that event `b` has been added to `U`
    REQUIRE(U.size() == 2);
    UnfoldingEvent e_recv(EventSet(), async_recv);
    REQUIRE(incremental_exC_recv.contains_equivalent_to(&e_recv));

    // At this point, UDPOR will pick one of the two events equivalent to
    // `e_recv` and e`_send`

    // Suppose it picks the event labeled `a` in the graph above first
    // (ultimately, UDPOR will choose to search both directions since
    // {⊥, b} will be an alternative to {⊥, a})

    const auto* e_a = *incremental_exC_send.begin();
    const auto* e_b = *incremental_exC_recv.begin();

    SECTION("Pick `a` first (ex({⊥, a}))")
    {
      // After picking `a`, we need only consider the extensions
      // possible with `2: AsyncRecv(m)` since actor 1 no longer
      // has any actions to run
      Configuration C_with_send{e_a};
      const auto incremental_exC_with_send = ExtensionSetCalculator::partially_extend(C_with_send, &U, async_recv);

      // Check that event `b` has not been duplicated
      REQUIRE(U.size() == 2);

      // Indeed, in this case we assert that the SAME identity has been
      // supplied by the unfolding (it should note that `ex({⊥, a})`
      // and `ex({⊥})` have an overlapping event `b`)
      REQUIRE(incremental_exC_with_send.contains(e_b));
    }

    SECTION("Pick `b` first (ex({⊥, b}))")
    {
      // After picking `b`, we need only consider the extensions
      // possible with `1: AsyncSend(m)` since actor 2 no longer
      // has any actions to run
      Configuration C_with_recv{e_b};
      const auto incremental_exC_with_recv = ExtensionSetCalculator::partially_extend(C_with_recv, &U, async_send);

      // Check that event `a` has not been duplicated
      REQUIRE(U.size() == 2);

      // Indeed, in this case we assert that the SAME identity has been
      // supplied by the unfolding (it should note that `ex({⊥, b})`
      // and `ex({⊥})` have an overlapping event `a`)
      REQUIRE(incremental_exC_with_recv.contains(e_a));
    }
  }
}