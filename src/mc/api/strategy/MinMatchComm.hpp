/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MINWAITTAKEN_HPP
#define SIMGRID_MC_MINWAITTAKEN_HPP

#include "src/mc/api/strategy/StratLocalInfo.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/transition/TransitionComm.hpp"

namespace simgrid::mc {

/** Wait MC guiding class that aims at maximizing the number of in-fly communication.
 *  When possible, it will try not to match communications. */
class MinMatchComm : public StratLocalInfo {
  /** Stores for each mailbox what kind of transition is waiting on it.
   *  Negative number means that much recv are waiting on that mailbox, while
   *  a positiv number means that much send are waiting there. */
  std::map<unsigned, int> mailbox_;
  /**  Used to valuate the state. Corresponds to a maximum minus the number of in-fly communications.
   *   Maximum should be set in order not to reach 0.*/
  int value_of_state_ = _sg_mc_max_depth;
    
    // The two next values are used to save the operation we execute so the next strategy can update its field accordingly
  Transition::Type last_transition_;
  unsigned last_mailbox_ = 0;

public:
  void copy_from(const StratLocalInfo* strategy) override;
  MinMatchComm()                     = default;
  ~MinMatchComm() override           = default;

  int get_actor_valuation(aid_t aid) const override { return value_of_state_; }

  std::pair<aid_t, int> best_transition(bool must_be_todo) const override;

  // FIXME: this is not using the strategy
  void consider_best_among_set(std::unordered_set<aid_t> E) override;

  void execute_next(aid_t aid, RemoteApp& app) override;
};

} // namespace simgrid::mc

#endif
