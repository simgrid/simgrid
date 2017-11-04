/* Copyright (c) 2015-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <exception>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <utility>

#include <unistd.h>

#include <xbt/log.h>

#include "simgrid/sg_config.h"
#include "src/xbt_modinter.h"

#include "src/mc/Session.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_base.h"
#include "src/mc/mc_comm_pattern.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_safety.hpp"
#include "src/mc/remote/mc_protocol.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_main, mc, "Entry point for simgrid-mc");
extern std::string _sg_mc_property_file;

static inline
char** argvdup(int argc, char** argv)
{
  char** argv_copy = new char*[argc + 1];
  std::memcpy(argv_copy, argv, sizeof(char*) * argc);
  argv_copy[argc] = nullptr;
  return argv_copy;
}

static std::unique_ptr<simgrid::mc::Checker> createChecker(simgrid::mc::Session& session)
{
  if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
    return std::unique_ptr<simgrid::mc::Checker>(simgrid::mc::createCommunicationDeterminismChecker(session));
  else if (_sg_mc_property_file.empty())
    return std::unique_ptr<simgrid::mc::Checker>(simgrid::mc::createSafetyChecker(session));
  else
    return std::unique_ptr<simgrid::mc::Checker>(simgrid::mc::createLivenessChecker(session));
}

int main(int argc, char** argv)
{
  using simgrid::mc::Session;
  XBT_LOG_CONNECT(mc_main);

  try {
    if (argc < 2)
      xbt_die("Missing arguments.\n");

    // Currently, we need this before sg_config_init:
    _sg_do_model_check = 1;

    // The initialization function can touch argv.
    // We make a copy of argv before modifying it in order to pass the original
    // value to the model-checked:
    char** argv_copy = argvdup(argc, argv);
    xbt_log_init(&argc, argv);
    sg_config_init(&argc, argv);

    std::unique_ptr<Session> session =
      std::unique_ptr<Session>(Session::spawnvp(argv_copy[1], argv_copy+1));
    delete[] argv_copy;

    simgrid::mc::session = session.get();
    std::unique_ptr<simgrid::mc::Checker> checker = createChecker(*session);
    int res = SIMGRID_MC_EXIT_SUCCESS;
    try {
      checker->run();
    } catch (simgrid::mc::DeadlockError& de) {
      res = SIMGRID_MC_EXIT_DEADLOCK;
    } catch (simgrid::mc::TerminationError& te) {
      res = SIMGRID_MC_EXIT_NON_TERMINATION;
    } catch (simgrid::mc::LivenessError& le) {
      res = SIMGRID_MC_EXIT_LIVENESS;
    }
    checker = nullptr;
    session->close();
    return res;
  }
  catch(std::exception& e) {
    XBT_ERROR("Exception: %s", e.what());
    return SIMGRID_MC_EXIT_ERROR;
  }
  catch(...) {
    XBT_ERROR("Unknown exception");
    return SIMGRID_MC_EXIT_ERROR;
  }
}
