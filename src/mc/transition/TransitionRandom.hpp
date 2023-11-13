/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_RANDOM_HPP
#define SIMGRID_MC_TRANSITION_RANDOM_HPP

#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc {

class RandomTransition : public Transition {
  int min_;
  int max_;

public:
  std::string to_string(bool verbose) const override;
  RandomTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  bool depends(const Transition* other) const override
  {
    if (other->type_ < type_)
      return other->depends(this);

    return aid_ == other->aid_;
  } // Independent with any other transition
  bool reversible_race(const Transition* other) const override;
};

} // namespace simgrid::mc

#endif
