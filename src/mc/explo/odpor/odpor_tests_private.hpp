/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @file odpor_tests_private.hpp
 *
 * A private header file for all ODPOR tests
 */

#ifndef SIMGRID_MC_ODPOR_TEST_PRIVATE_HPP
#define SIMGRID_MC_ODPOR_TEST_PRIVATE_HPP

#include "src/mc/explo/udpor/udpor_tests_private.hpp"
#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc::odpor {

struct DependentIfSameValueAction : public Transition {
private:
  const int value;

public:
  DependentIfSameValueAction(Type type, aid_t issuer, int value, int times_considered = 0)
      : Transition(type, issuer, times_considered), value(value)
  {
  }
  DependentIfSameValueAction(aid_t issuer, int value, int times_considered = 0)
      : Transition(simgrid::mc::Transition::Type::UNKNOWN, issuer, times_considered), value(value)
  {
  }

  // Dependent only with DependentAction (i.e. not itself)
  bool depends(const Transition* other) const override
  {
    if (aid_ == other->aid_) {
      return true;
    }

    if (const auto* same_value = dynamic_cast<const DependentIfSameValueAction*>(other); same_value != nullptr) {
      return value == same_value->value;
    }

    // `DependentAction` is dependent with everyone who's not the `IndependentAction`
    return dynamic_cast<const simgrid::mc::udpor::DependentAction*>(other) != nullptr;
  }
};

} // namespace simgrid::mc::odpor

#endif
