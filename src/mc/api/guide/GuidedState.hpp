/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_GUIDEDSTATE_HPP
#define SIMGRID_MC_GUIDEDSTATE_HPP

namespace simgrid::mc {

class GuidedState {
  /** State's exploration status by actor. Not all the actors are there, only the ones that are ready-to-run in this
   * state */
  std::map<aid_t, ActorState> actors_to_run_;

public:
  virtual std::pair<aid_t, double> next_transition() const = 0;
  virtual void execute_next(aid_t aid)                     = 0;
  friend class State;
};

} // namespace simgrid::mc

#endif
