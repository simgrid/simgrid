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
}