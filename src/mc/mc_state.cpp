/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_state.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_api.hpp"

#include <boost/range/algorithm.hpp>

using simgrid::mc::remote;
using mcapi = simgrid::mc::mc_api;

namespace simgrid {
namespace mc {

State::State(unsigned long state_number) : num_(state_number)
{
  this->internal_comm_.clear();
  auto maxpid = mcapi::get().get_maxpid();
  actor_states_.resize(maxpid);
  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (state_number % _sg_mc_checkpoint == 0)) || _sg_mc_termination) {
    auto snapshot_ptr = mcapi::get().take_snapshot(num_);
    system_state_ = std::shared_ptr<simgrid::mc::Snapshot>(snapshot_ptr);
    if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
      mcapi::get().copy_incomplete_comm_pattern(this);
      mcapi::get().copy_index_comm_pattern(this);
    }
  }
}

std::size_t State::interleave_size() const
{
  return boost::range::count_if(this->actor_states_, [](simgrid::mc::ActorState const& a) { return a.is_todo(); });
}

Transition State::get_transition() const
{
  return this->transition_;
}

}
}
