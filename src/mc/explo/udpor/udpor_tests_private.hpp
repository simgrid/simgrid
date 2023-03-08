/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @file udpor_tests_private.hpp
 *
 * A private header file for tests involving events in
 * configurations
 */

#ifndef SIMGRID_MC_UDPOR_TEST_PRIVATE_HPP
#define SIMGRID_MC_UDPOR_TEST_PRIVATE_HPP

namespace simgrid::mc::udpor {

struct IndependentAction : public Transition {
  // Independent with everyone else
  bool depends(const Transition* other) const override { return false; }
};

struct DependentAction : public Transition {
  // Dependent with everyone else (except IndependentAction)
  bool depends(const Transition* other) const override
  {
    return dynamic_cast<const IndependentAction*>(other) == nullptr;
  }
};

struct ConditionallyDependentAction : public Transition {
  // Dependent only with DependentAction (i.e. not itself)
  bool depends(const Transition* other) const override
  {
    return dynamic_cast<const DependentAction*>(other) != nullptr;
  }
};

} // namespace simgrid::mc::udpor

#endif
