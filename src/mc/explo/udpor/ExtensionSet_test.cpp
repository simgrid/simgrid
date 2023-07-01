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
    aid_t issuer = 1;

    const auto async_send      = std::make_shared<CommSendTransition>(issuer, times_considered, comm, mbox, tag);
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
    aid_t issuer = 2;

    const auto async_recv      = std::make_shared<CommRecvTransition>(issuer, times_considered, comm, mbox, tag);
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

    const auto async_send           = std::make_shared<CommSendTransition>(1, times_considered, comm, mbox, tag);
    const auto incremental_exC_send = ExtensionSetCalculator::partially_extend(C, &U, async_send);

    // Check that the events have been added to `U`
    REQUIRE(U.size() == 1);

    // Make assertions about the contents of ex(C)
    UnfoldingEvent e_send(EventSet(), async_send);
    REQUIRE(incremental_exC_send.contains_equivalent_to(&e_send));

    // Consider the extension with `2: AsyncRecv(m)`
    const auto async_recv           = std::make_shared<CommRecvTransition>(2, times_considered, comm, mbox, tag);
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

    // Consider the extension with `1: AsyncSend(m)`
    const auto async_send           = std::make_shared<CommSendTransition>(1, times_considered, comm, mbox, tag);
    const auto incremental_exC_send = ExtensionSetCalculator::partially_extend(C, &U, async_send);

    // Check that event `a` has been added to `U`
    REQUIRE(U.size() == 1);
    UnfoldingEvent e_send(EventSet(), async_send);
    REQUIRE(incremental_exC_send.contains_equivalent_to(&e_send));

    // Consider the extension with `2: AsyncRecv(m)`
    const auto async_recv           = std::make_shared<CommRecvTransition>(2, times_considered, comm, mbox, tag);
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

TEST_CASE("simgrid::mc::udpor: Testing Waits, Receives, and Sends")
{
  // We're going to follow UDPOR down one path of computation
  // in a relatively simple program (although the unfolding quickly
  // becomes quite complex)
  //
  //      1                 2
  // AsyncSend(m)      AsyncRecv(m)
  // Wait(m)           Wait(m)
  //
  // The unfolding of the simple program is as follows:
  //                         ⊥
  //                  /            /
  //       (a) 1: AsyncSend(m)  (b) 2: AsyncRecv(m)
  //                |   \        /        |
  //                |      \  /          |
  //               |      /   \          |
  //               |    /      \         |
  //       (c) 1: Wait(m)       (d) 2: Wait(m)
  const int times_considered = 0;
  const int tag              = 0;
  const unsigned mbox        = 0;
  const uintptr_t comm       = 0x800;
  const bool timeout         = false;

  Unfolding U;
  const auto comm_send   = std::make_shared<CommSendTransition>(1, times_considered, comm, mbox, tag);
  const auto comm_recv   = std::make_shared<CommRecvTransition>(2, times_considered, comm, mbox, tag);
  const auto comm_wait_1 = std::make_shared<CommWaitTransition>(1, times_considered, timeout, comm, 1, 2, mbox);
  const auto comm_wait_2 = std::make_shared<CommWaitTransition>(2, times_considered, timeout, comm, 1, 2, mbox);

  // 1. UDPOR will attempt to expand first ex({⊥})

  // --- ex({⊥}) ---
  const auto incremental_exC_send = ExtensionSetCalculator::partially_extend(Configuration(), &U, comm_send);
  // Assert that event `a` has been added
  UnfoldingEvent e_send(EventSet(), comm_send);
  REQUIRE(incremental_exC_send.size() == 1);
  REQUIRE(incremental_exC_send.contains_equivalent_to(&e_send));
  REQUIRE(U.size() == 1);

  const auto incremental_exC_recv = ExtensionSetCalculator::partially_extend(Configuration(), &U, comm_recv);
  // Assert that event `b` has been added
  UnfoldingEvent e_recv(EventSet(), comm_recv);
  REQUIRE(incremental_exC_recv.size() == 1);
  REQUIRE(incremental_exC_recv.contains_equivalent_to(&e_recv));
  REQUIRE(U.size() == 2);
  // --- ex({⊥}) ---

  // 2. UDPOR will then attempt to expand ex({⊥, a}) or ex({⊥, b}). Both have
  // parallel effects and should simply return events already added to ex(C)
  //
  // NOTE: Note that only once actor is enabled in both cases, meaning that
  // we need only consider one incremental expansion for each

  const auto* e_a = *incremental_exC_send.begin();
  const auto* e_b = *incremental_exC_recv.begin();

  // --- ex({⊥, a}) ---
  const auto incremental_exC_recv2 = ExtensionSetCalculator::partially_extend(Configuration({e_a}), &U, comm_recv);
  // Assert that no event has been added and that
  // e_b is contained in the extension set
  REQUIRE(incremental_exC_recv2.size() == 1);
  REQUIRE(incremental_exC_recv2.contains(e_b));

  // Here, `e_a` shouldn't be added again
  REQUIRE(U.size() == 2);
  // --- ex({⊥, a}) ---

  // --- ex({⊥, b}) ---
  const auto incremental_exC_send2 = ExtensionSetCalculator::partially_extend(Configuration({e_b}), &U, comm_send);
  // Assert that no event has been added and that
  // e_a is contained in the extension set
  REQUIRE(incremental_exC_send2.size() == 1);
  REQUIRE(incremental_exC_send2.contains(e_a));

  // Here, `e_b` shouldn't be added again
  REQUIRE(U.size() == 2);
  // --- ex({⊥, b}) ---

  // 3. Expanding from ex({⊥, a, b}) brings in both `CommWait` events since they
  // become enabled as soon as the communication has been paired

  // --- ex({⊥, a, b}) ---
  const auto incremental_exC_wait_actor_1 =
      ExtensionSetCalculator::partially_extend(Configuration({e_a, e_b}), &U, comm_wait_1);
  // Assert that events `c` has been added
  UnfoldingEvent e_wait_1(EventSet({e_a, e_b}), comm_wait_1);
  REQUIRE(incremental_exC_wait_actor_1.size() == 1);
  REQUIRE(incremental_exC_wait_actor_1.contains_equivalent_to(&e_wait_1));
  REQUIRE(U.size() == 3);

  const auto incremental_exC_wait_actor_2 =
      ExtensionSetCalculator::partially_extend(Configuration({e_a, e_b}), &U, comm_wait_2);
  // Assert that events `d` has been added
  UnfoldingEvent e_wait_2(EventSet({e_a, e_b}), comm_wait_2);
  REQUIRE(incremental_exC_wait_actor_2.size() == 1);
  REQUIRE(incremental_exC_wait_actor_2.contains_equivalent_to(&e_wait_2));
  REQUIRE(U.size() == 4);
  // --- ex({⊥, a, b}) ---

  // 4. Expanding from either wait action should simply yield the other event
  // with a wait action associated with it.
  // This is analogous to the scenario before with send and receive
  // ex({⊥, a, b, c}) or ex({⊥, a, b, d})

  const auto* e_c = *incremental_exC_wait_actor_1.begin();
  const auto* e_d = *incremental_exC_wait_actor_2.begin();

  // --- ex({⊥, a, b, d}) ---
  const auto incremental_exC_wait_actor_1_2 =
      ExtensionSetCalculator::partially_extend(Configuration({e_a, e_b, e_d}), &U, comm_wait_1);
  // Assert that no event has been added and that
  // `e_c` is contained in the extension set
  REQUIRE(incremental_exC_wait_actor_1_2.size() == 1);
  REQUIRE(incremental_exC_wait_actor_1_2.contains(e_c));
  REQUIRE(U.size() == 4);
  // --- ex({⊥, a, b, d}) ---

  // --- ex({⊥, a, b, c}) ---
  const auto incremental_exC_wait_actor_2_2 =
      ExtensionSetCalculator::partially_extend(Configuration({e_a, e_b, e_c}), &U, comm_wait_2);
  // Assert that no event has been added and that
  // `e_d` is contained in the extension set
  REQUIRE(incremental_exC_wait_actor_2_2.size() == 1);
  REQUIRE(incremental_exC_wait_actor_2_2.contains(e_d));
  REQUIRE(U.size() == 4);
  // --- ex({⊥, a, b, c}) ---
}
