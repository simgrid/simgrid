/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_RANDOM_HPP
#define SIMGRID_MC_TRANSITION_RANDOM_HPP

#include "src/mc/transition/Transition.hpp"

namespace simgrid {
namespace mc {

class RandomTransition : public Transition {
  int min_;
  int max_;

public:
  std::string to_string(bool verbose) const override;
  RandomTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  bool depends(const Transition* other) const override { return false; } // Independent with any other transition
};

} // namespace mc
} // namespace simgrid

#endif
