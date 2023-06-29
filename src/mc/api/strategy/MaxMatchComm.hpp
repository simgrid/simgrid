/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_WAITSTRATEGY_HPP
#define SIMGRID_MC_WAITSTRATEGY_HPP

#include "src/mc/transition/TransitionComm.hpp"

namespace simgrid::mc {

/** Wait MC guiding class that aims at minimizing the number of in-fly communication.
 *  When possible, it will try to match corresponding in-fly communications. */
class MaxMatchComm : public Strategy {
  /** Stores for each mailbox what kind of transition is waiting on it.
   *  Negative number means that much recv are waiting on that mailbox, while
   *  a positiv number means that much send are waiting there. */
  std::map<unsigned, int> mailbox_;
    int value_of_state_ = 0; // used to valuate the state. Corresponds to the number of in-fly communications
    // The two next values are used to save the operation we execute so the next strategy can update its field accordingly
  Transition::Type last_transition_; 
  unsigned last_mailbox_ = 0;

public:
  void copy_from(const Strategy* strategy) override
  {
    const auto* cast_strategy = dynamic_cast<MaxMatchComm const*>(strategy);
    xbt_assert(cast_strategy != nullptr);
    for (auto& [id, val] : cast_strategy->mailbox_)
      mailbox_[id] = val;
    if (cast_strategy->last_transition_ == Transition::Type::COMM_ASYNC_RECV)
      mailbox_[cast_strategy->last_mailbox_]--;
    if (cast_strategy->last_transition_ == Transition::Type::COMM_ASYNC_SEND)
      mailbox_[cast_strategy->last_mailbox_]++;

    for (auto const& [_, val] : mailbox_)
	value_of_state_ += std::abs(val);
  }
  MaxMatchComm()                     = default;
  ~MaxMatchComm() override           = default;

  std::pair<aid_t, int> best_transition(bool must_be_todo) const override
  {
    std::pair<aid_t, int> min_found = std::make_pair(-1, value_of_state_ + 2);
    for (auto const& [aid, actor] : actors_to_run_) {
	if ((not actor.is_todo() && must_be_todo) || not actor.is_enabled() || actor.is_done())
	    continue;

      int aid_value = value_of_state_;
      const Transition* transition = actor.get_transition(actor.get_times_considered()).get();

      if (auto const* cast_recv = dynamic_cast<CommRecvTransition const*>(transition)) {
            if (mailbox_.count(cast_recv->get_mailbox()) > 0 && mailbox_.at(cast_recv->get_mailbox()) > 0) {
              aid_value--; // This means we have waiting recv corresponding to this recv
            } else {
              aid_value++;
            }
      }

      if (auto const* cast_send = dynamic_cast<CommSendTransition const*>(transition)) {
            if (mailbox_.count(cast_send->get_mailbox()) > 0 && mailbox_.at(cast_send->get_mailbox()) < 0) {
              aid_value--; // This means we have waiting recv corresponding to this send
            } else {
              aid_value++;
            }
      }

      if (aid_value < min_found.second)
	  min_found = std::make_pair(aid, aid_value);
    }
    return min_found;
  }

  void execute_next(aid_t aid, RemoteApp& app) override
  {
    const Transition* transition = actors_to_run_.at(aid).get_transition(actors_to_run_.at(aid).get_times_considered()).get();
    last_transition_             = transition->type_;

    if (auto const* cast_recv = dynamic_cast<CommRecvTransition const*>(transition))
      last_mailbox_ = cast_recv->get_mailbox();

    if (auto const* cast_send = dynamic_cast<CommSendTransition const*>(transition))
      last_mailbox_ = cast_send->get_mailbox();
  }
};

} // namespace simgrid::mc

#endif
