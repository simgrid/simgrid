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
  std::pair<aid_t, double> next_transition() const override
  {

    for (auto const& [aid, actor] : actors_to_run_) {
      /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
      if (not actor.is_todo() || not actor.is_enabled() || actor.is_done()) {
        continue;
      }

      return std::make_pair(aid, 1.0);
    }
    return std::make_pair(-1, 0.0);
  }
  void execute_next(aid_t aid, RemoteApp& app) override { return; }
};

} // namespace simgrid::mc

#endif
