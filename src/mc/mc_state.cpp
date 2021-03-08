/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_state.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/api.hpp"

#include <boost/range/algorithm.hpp>

using simgrid::mc::remote;
using api = simgrid::mc::Api;

namespace simgrid {
namespace mc {

State::State(unsigned long state_number) : num_(state_number)
{
  const unsigned long maxpid = api::get().get_maxpid();
  actor_states_.resize(maxpid);
  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (state_number % _sg_mc_checkpoint == 0)) || _sg_mc_termination) {
    auto snapshot_ptr = api::get().take_snapshot(num_);
    system_state_ = std::shared_ptr<simgrid::mc::Snapshot>(snapshot_ptr);
    if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
      copy_incomplete_comm_pattern();
      copy_index_comm_pattern();
    }
  }
}

std::size_t State::count_todo() const
{
  return boost::range::count_if(this->actor_states_, [](simgrid::mc::ActorState const& a) { return a.is_todo(); });
}

Transition State::get_transition() const
{
  return this->transition_;
}

void State::copy_incomplete_comm_pattern()
{
  incomplete_comm_pattern_.clear();
  const unsigned long maxpid = api::get().get_maxpid();
  for (unsigned long i = 0; i < maxpid; i++) {
    std::vector<simgrid::mc::PatternCommunication> res;
    for (auto const& comm : incomplete_communications_pattern[i])
      res.push_back(comm->dup());
    incomplete_comm_pattern_.push_back(std::move(res));
  }
}

void State::copy_index_comm_pattern()
{
  communication_indices_.clear();
  for (auto const& list_process_comm : initial_communications_pattern)
    this->communication_indices_.push_back(list_process_comm.index_comm);
}

}
}
