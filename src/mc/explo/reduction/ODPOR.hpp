/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_HPP
#define SIMGRID_MC_ODPOR_HPP

#include "simgrid/forward.h"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"

namespace simgrid::mc {

class ODPOR : public Reduction {

public:
  ODPOR()           = default;
  ~ODPOR() override = default;

  void races_computation(odpor::Execution& E, stack_t* S, std::vector<std::shared_ptr<State>>* opened_states) override;
  aid_t next_to_explore(odpor::Execution& E, stack_t* S) override;
  std::shared_ptr<State> state_create(RemoteApp& remote_app, std::shared_ptr<State> parent_state) override;
  void on_backtrack(State* s) override;
};

} // namespace simgrid::mc

#endif
