/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BFSODPOR_HPP
#define SIMGRID_MC_BFSODPOR_HPP

#include "simgrid/forward.h"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"

namespace simgrid::mc {

class BFSODPOR : public Reduction {

public:
  BFSODPOR()  = default;
  ~BFSODPOR() = default;

  void races_computation(odpor::Execution& E, stack_t* S, std::vector<StatePtr>* opened_states) override;
  aid_t next_to_explore(odpor::Execution& E, stack_t* S) override;
  StatePtr state_create(RemoteApp& remote_app, StatePtr parent_state) override;
  void on_backtrack(State* s) override {}
};

} // namespace simgrid::mc

#endif
