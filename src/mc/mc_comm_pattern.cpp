/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>

#include "src/mc/checker/CommunicationDeterminismChecker.hpp"
#include "src/mc/mc_smx.hpp"

using simgrid::mc::remote;

static void MC_patterns_copy(std::vector<simgrid::mc::PatternCommunication*>& dest,
                             std::vector<simgrid::mc::PatternCommunication> const& source)
{
  dest.clear();
  for (simgrid::mc::PatternCommunication const& comm : source) {
    auto* copy_comm = new simgrid::mc::PatternCommunication(comm.dup());
    dest.push_back(copy_comm);
  }
}

void MC_restore_communications_pattern(simgrid::mc::State* state)
{
  for (unsigned i = 0; i < initial_communications_pattern.size(); i++)
    initial_communications_pattern[i].index_comm = state->communication_indices_[i];

  for (unsigned i = 0; i < MC_smx_get_maxpid(); i++)
    MC_patterns_copy(incomplete_communications_pattern[i], state->incomplete_comm_pattern_[i]);
}