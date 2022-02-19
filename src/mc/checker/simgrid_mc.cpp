/* Copyright (c) 2015-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/sg_config.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/internal_config.h"

#if HAVE_SMPI
#include "smpi/smpi.h"
#endif

#include <cstring>
#include <memory>
#include <unistd.h>

using api = simgrid::mc::Api;

static inline
char** argvdup(int argc, char** argv)
{
  auto* argv_copy = new char*[argc + 1];
  std::memcpy(argv_copy, argv, sizeof(char*) * argc);
  argv_copy[argc] = nullptr;
  return argv_copy;
}

int main(int argc, char** argv)
{
  xbt_assert(argc >= 2, "Missing arguments");

  // Currently, we need this before sg_config_init:
  _sg_do_model_check = 1;

  // The initialization function can touch argv.
  // We make a copy of argv before modifying it in order to pass the original value to the model-checked application:
  char** argv_copy = argvdup(argc, argv);

  xbt_log_init(&argc, argv);
#if HAVE_SMPI
  smpi_init_options(); // only performed once
#endif
  sg_config_init(&argc, argv);

  simgrid::mc::CheckerAlgorithm algo;
  if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
    algo = simgrid::mc::CheckerAlgorithm::CommDeterminism;
  else if (_sg_mc_unfolding_checker)
    algo = simgrid::mc::CheckerAlgorithm::UDPOR;
  else if (_sg_mc_property_file.get().empty())
    algo = simgrid::mc::CheckerAlgorithm::Safety;
  else
    algo = simgrid::mc::CheckerAlgorithm::Liveness;

  int res      = SIMGRID_MC_EXIT_SUCCESS;
  auto checker = api::get().initialize(argv_copy, algo);
  try {
    checker->run();
  } catch (const simgrid::mc::DeadlockError&) {
    res = SIMGRID_MC_EXIT_DEADLOCK;
  } catch (const simgrid::mc::TerminationError&) {
    res = SIMGRID_MC_EXIT_NON_TERMINATION;
  } catch (const simgrid::mc::LivenessError&) {
    res = SIMGRID_MC_EXIT_LIVENESS;
  }
  api::get().s_close();
  delete[] argv_copy;
  delete checker;
  return res;
}
