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
    const MaxMatchComm* cast_strategy = static_cast<MaxMatchComm const*>(strategy);
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

  std::pair<aid_t, int> next_transition() const override
  {
    std::pair<aid_t, int> if_no_match = std::make_pair(-1, 0);
    for (auto const& [aid, actor] : actors_to_run_) {
      if (not actor.is_todo() || not actor.is_enabled() || actor.is_done())
        continue;

      const Transition* transition = actor.get_transition(actor.get_times_considered()).get();

      const CommRecvTransition* cast_recv = static_cast<CommRecvTransition const*>(transition);
      if (cast_recv != nullptr and mailbox_.count(cast_recv->get_mailbox()) > 0 and
          mailbox_.at(cast_recv->get_mailbox()) > 0)
        return std::make_pair(aid, value_of_state_ - 1); // This means we have waiting send corresponding to this recv

      const CommSendTransition* cast_send = static_cast<CommSendTransition const*>(transition);
      if (cast_send != nullptr and mailbox_.count(cast_send->get_mailbox()) > 0 and
          mailbox_.at(cast_send->get_mailbox()) < 0)
        return std::make_pair(aid, value_of_state_ - 1); // This means we have waiting recv corresponding to this send

      if (if_no_match.first == -1)
        if_no_match = std::make_pair(aid, value_of_state_);
    }
    return if_no_match;
  }

  void execute_next(aid_t aid, RemoteApp& app) override
  {
    const Transition* transition = actors_to_run_.at(aid).get_transition(actors_to_run_.at(aid).get_times_considered()).get();
    last_transition_             = transition->type_;

    const CommRecvTransition* cast_recv = static_cast<CommRecvTransition const*>(transition);
    if (cast_recv != nullptr)
      last_mailbox_ = cast_recv->get_mailbox();

    const CommSendTransition* cast_send = static_cast<CommSendTransition const*>(transition);
    if (cast_send != nullptr)
      last_mailbox_ = cast_send->get_mailbox();
  }

  void consider_best() override
  {
    for (auto& [aid, actor] : actors_to_run_)
      if (actor.is_todo())
        return;

    for (auto& [aid, actor] : actors_to_run_) {
      if (not actor.is_enabled() || actor.is_done())
        continue;

      const Transition* transition = actor.get_transition(actor.get_times_considered()).get();

      const CommRecvTransition* cast_recv = static_cast<CommRecvTransition const*>(transition);
      if (cast_recv != nullptr and mailbox_.count(cast_recv->get_mailbox()) > 0 and
          mailbox_.at(cast_recv->get_mailbox()) > 0) {
        actor.mark_todo();
        return;
      }

      const CommSendTransition* cast_send = static_cast<CommSendTransition const*>(transition);
      if (cast_send != nullptr and mailbox_.count(cast_send->get_mailbox()) > 0 and
          mailbox_.at(cast_send->get_mailbox()) < 0) {
        actor.mark_todo();
        return;
      }
    }
    for (auto& [_, actor] : actors_to_run_) {
      if (actor.is_enabled() and not actor.is_done()) {
        actor.mark_todo();
        return;
      }
    }
  }
};

} // namespace simgrid::mc

#endif
