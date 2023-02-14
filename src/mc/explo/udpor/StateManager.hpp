/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_GLOBAL_HPP
#define SIMGRID_MC_UDPOR_GLOBAL_HPP

#include "src/mc/api/State.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <optional>

namespace simgrid::mc::udpor {

class StateManager {
private:
  using Handle = StateHandle;

  Handle current_handle_ = 1ul;
  std::unordered_map<Handle, std::unique_ptr<State>> state_map_;

public:
  Handle record_state(std::unique_ptr<State>);
  std::optional<std::reference_wrapper<State>> get_state(Handle);
};

} // namespace simgrid::mc::udpor
#endif
