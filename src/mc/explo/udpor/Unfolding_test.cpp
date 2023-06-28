/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"
#include "src/mc/explo/udpor/udpor_tests_private.hpp"

using namespace simgrid::mc;
using namespace simgrid::mc::udpor;

TEST_CASE("simgrid::mc::udpor::Unfolding: Creating an unfolding")
{
  Unfolding unfolding;
  REQUIRE(unfolding.size() == 0);
  REQUIRE(unfolding.empty());
}

TEST_CASE("simgrid::mc::udpor::Unfolding: Inserting and marking events with an unfolding")
{
  Unfolding unfolding;
  auto e1 = std::make_unique<UnfoldingEvent>(
      EventSet(), std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 0));
  auto e2 =
      std::make_unique<UnfoldingEvent>(EventSet(), std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1));
  const auto* e1_handle = e1.get();
  const auto* e2_handle = e2.get();

  unfolding.insert(std::move(e1));
  REQUIRE(unfolding.size() == 1);
  REQUIRE_FALSE(unfolding.empty());

  unfolding.insert(std::move(e2));
  REQUIRE(unfolding.size() == 2);
  REQUIRE_FALSE(unfolding.empty());

  unfolding.mark_finished(e1_handle);
  REQUIRE(unfolding.size() == 2);
  REQUIRE_FALSE(unfolding.empty());

  unfolding.mark_finished(e2_handle);
  REQUIRE(unfolding.size() == 2);
  REQUIRE_FALSE(unfolding.empty());
}

TEST_CASE("simgrid::mc::udpor::Unfolding: Checking all immediate conflicts restricted to an unfolding") {}
