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

#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc::udpor {

struct IndependentAction : public Transition {
  IndependentAction() = default;
  IndependentAction(Type type, aid_t issuer, int times_considered = 0) : Transition(type, issuer, times_considered) {}
  IndependentAction(aid_t issuer, int times_considered = 0)
      : IndependentAction(simgrid::mc::Transition::Type::UNKNOWN, issuer, times_considered)
  {
  }

  // Independent with everyone else (even if run by the same actor). NOTE: This is
  // only for the convenience of testing: in general, transitions are dependent with
  // one another if run by the same actor
  bool depends(const Transition* other) const override
  {
    if (aid_ == other->aid_) {
      return true;
    }
    return false;
  }
};

struct DependentAction : public Transition {
  DependentAction() = default;
  DependentAction(Type type, aid_t issuer, int times_considered = 0) : Transition(type, issuer, times_considered) {}
  DependentAction(aid_t issuer, int times_considered = 0)
      : DependentAction(simgrid::mc::Transition::Type::UNKNOWN, issuer, times_considered)
  {
  }

  // Dependent with everyone else (except IndependentAction)
  bool depends(const Transition* other) const override
  {
    if (aid_ == other->aid_) {
      return true;
    }
    return dynamic_cast<const IndependentAction*>(other) == nullptr;
  }
};

struct ConditionallyDependentAction : public Transition {
  ConditionallyDependentAction() = default;
  ConditionallyDependentAction(Type type, aid_t issuer, int times_considered = 0)
      : Transition(type, issuer, times_considered)
  {
  }
  ConditionallyDependentAction(aid_t issuer, int times_considered = 0)
      : ConditionallyDependentAction(simgrid::mc::Transition::Type::UNKNOWN, issuer, times_considered)
  {
  }

  // Dependent only with DependentAction (i.e. not itself)
  bool depends(const Transition* other) const override
  {
    if (aid_ == other->aid_) {
      return true;
    }
    return dynamic_cast<const DependentAction*>(other) != nullptr;
  }
};

} // namespace simgrid::mc::udpor

#endif
