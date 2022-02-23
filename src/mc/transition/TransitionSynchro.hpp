/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_SYNCHRO_HPP
#define SIMGRID_MC_TRANSITION_SYNCHRO_HPP

#include "src/mc/transition/Transition.hpp"

namespace simgrid {
namespace mc {

class MutexTransition : public Transition {
  uintptr_t mutex_;
  aid_t owner_;

public:
  std::string to_string(bool verbose) const override;
  MutexTransition(aid_t issuer, int times_considered, Type type, std::stringstream& stream);
  bool depends(const Transition* other) const override;
};

} // namespace mc
} // namespace simgrid

#endif
