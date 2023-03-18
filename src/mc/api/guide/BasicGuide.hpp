/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASICGUIDE_HPP
#define SIMGRID_MC_BASICGUIDE_HPP

namespace simgrid::mc {

/** Basic MC guiding class which corresponds to no guide at all (random choice) */
// Not Yet fully implemented
class BasicGuide : public GuidedState {
public:
  std::pair<aid_t, double> next_transition() const override { return std::make_pair(0, 0); }
  virtual void execute_next(aid_t aid) override { return; }
};

} // namespace simgrid::mc

#endif
