/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/strategy/MaxMatchComm.hpp"

namespace simgrid::mc {

void MaxMatchComm::copy_from(const StratLocalInfo* strategy)
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

std::pair<aid_t, int> MaxMatchComm::best_transition(bool must_be_todo) const
{
  std::pair<aid_t, int> min_found = std::make_pair(-1, value_of_state_ + 2);
  for (auto const& [aid, actor] : actors_to_run_) {
    if ((not actor.is_todo() && must_be_todo) || not actor.is_enabled() || actor.is_done())
      continue;

    int aid_value                = value_of_state_;
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

// FIXME: this is not using the strategy
void MaxMatchComm::consider_best_among_set(std::unordered_set<aid_t> E)
{
  if (std::any_of(begin(E), end(E), [this](const auto& aid) { return actors_to_run_.at(aid).is_todo(); }))
    return;
  for (auto& [aid, actor] : actors_to_run_) {
    /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
    if (E.count(aid) > 0) {
      xbt_assert(actor.is_enabled(), "Asked to consider one among a set containing the disabled actor %ld", aid);
      actor.mark_todo();
      return;
    }
  }
}

void MaxMatchComm::execute_next(aid_t aid, RemoteApp& app)
{
  const Transition* transition =
      actors_to_run_.at(aid).get_transition(actors_to_run_.at(aid).get_times_considered()).get();
  last_transition_ = transition->type_;

  if (auto const* cast_recv = dynamic_cast<CommRecvTransition const*>(transition))
    last_mailbox_ = cast_recv->get_mailbox();

  if (auto const* cast_send = dynamic_cast<CommSendTransition const*>(transition))
    last_mailbox_ = cast_send->get_mailbox();
}

} // namespace simgrid::mc
