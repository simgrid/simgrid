/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"

TEST_CASE("simgrid::mc::udpor::Unfolding: Creating an unfolding")
{
  using namespace simgrid::mc::udpor;

  Unfolding unfolding;
  CHECK(unfolding.empty());
  REQUIRE(unfolding.size() == 0);
}