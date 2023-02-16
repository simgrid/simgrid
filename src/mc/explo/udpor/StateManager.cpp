/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/StateManager.hpp"
#include "xbt/asserts.h"

namespace simgrid::mc::udpor {

StateManager::Handle StateManager::record_state(std::unique_ptr<State> state)
{
  if (state.get() == nullptr) {
    throw std::invalid_argument("Expected a newly-allocated State but got NULL instead");
  }

  const auto integer_handle = this->current_handle_;
  this->state_map_.insert({integer_handle, std::move(state)});

  this->current_handle_++;
  xbt_assert(integer_handle <= this->current_handle_, "Too many states were vended out during exploration via UDPOR.\n"
                                                      "It's suprising you didn't run out of memory elsewhere first...\n"
                                                      "Please report this as a bug along with the very large program");

  return integer_handle;
}

std::optional<std::reference_wrapper<State>> StateManager::get_state(StateManager::Handle handle)
{
  auto state = this->state_map_.find(handle);
  if (state == this->state_map_.end()) {
    return std::nullopt;
  }
  auto& state_ref = *state->second.get();
  return std::optional<std::reference_wrapper<State>>{state_ref};
}

} // namespace simgrid::mc::udpor
