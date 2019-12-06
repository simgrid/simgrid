/* Copyright (c) 2015-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/sg_config.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"

#if HAVE_SMPI
#include "smpi/smpi.h"
#endif

#include <cstring>
#include <memory>
#include <unistd.h>

static inline
char** argvdup(int argc, char** argv)
{
  char** argv_copy = new char*[argc + 1];
  std::memcpy(argv_copy, argv, sizeof(char*) * argc);
  argv_copy[argc] = nullptr;
  return argv_copy;
}

static std::unique_ptr<simgrid::mc::Checker> create_checker(simgrid::mc::Session& session)
{
  if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
    return std::unique_ptr<simgrid::mc::Checker>(simgrid::mc::createCommunicationDeterminismChecker(session));
  else if (_sg_mc_property_file.get().empty())
    return std::unique_ptr<simgrid::mc::Checker>(simgrid::mc::createSafetyChecker(session));
  else
    return std::unique_ptr<simgrid::mc::Checker>(simgrid::mc::createLivenessChecker(session));
}

int main(int argc, char** argv)
{
  if (argc < 2)
    xbt_die("Missing arguments.\n");

  // Currently, we need this before sg_config_init:
  _sg_do_model_check = 1;

  // The initialization function can touch argv.
  // We make a copy of argv before modifying it in order to pass the original
  // value to the model-checked:
  char** argv_copy = argvdup(argc, argv);
  xbt_log_init(&argc, argv);
#if HAVE_SMPI
  smpi_init_options();//only performed once
#endif
  sg_config_init(&argc, argv);
  simgrid::mc::session = new simgrid::mc::Session([argv_copy] { execvp(argv_copy[1], argv_copy + 1); });
  delete[] argv_copy;

  std::unique_ptr<simgrid::mc::Checker> checker = create_checker(*simgrid::mc::session);
  int res                                       = SIMGRID_MC_EXIT_SUCCESS;
  try {
    checker->run();
  } catch (const simgrid::mc::DeadlockError&) {
    res = SIMGRID_MC_EXIT_DEADLOCK;
  } catch (const simgrid::mc::TerminationError&) {
    res = SIMGRID_MC_EXIT_NON_TERMINATION;
  } catch (const simgrid::mc::LivenessError&) {
    res = SIMGRID_MC_EXIT_LIVENESS;
  }
  checker = nullptr;
  simgrid::mc::session->close();
  return res;
}
