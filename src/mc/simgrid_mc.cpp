/* Copyright (c) 2015. The SimGrid Team.
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

#include "src/mc/mc_base.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_protocol.h"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_comm_pattern.h"
#include "src/mc/LivenessChecker.hpp"
#include "src/mc/mc_exit.h"
#include "src/mc/Session.hpp"
#include "src/mc/Checker.hpp"
#include "src/mc/CommunicationDeterminismChecker.hpp"
#include "src/mc/SafetyChecker.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_main, mc, "Entry point for simgrid-mc");

static inline
char** argvdup(int argc, char** argv)
{
  char** argv_copy = xbt_new(char*, argc+1);
  std::memcpy(argv_copy, argv, sizeof(char*) * argc);
  argv_copy[argc] = nullptr;
  return argv_copy;
}

static
std::unique_ptr<simgrid::mc::Checker> createChecker(simgrid::mc::Session& session)
{
  using simgrid::mc::Session;
  using simgrid::mc::FunctionalChecker;

  std::function<int(Session& session)> code;
  if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
    return std::unique_ptr<simgrid::mc::Checker>(
      new simgrid::mc::CommunicationDeterminismChecker(session));
  else if (!_sg_mc_property_file || _sg_mc_property_file[0] == '\0')
    return std::unique_ptr<simgrid::mc::Checker>(
      new simgrid::mc::SafetyChecker(session));
  else
    return std::unique_ptr<simgrid::mc::Checker>(
      new simgrid::mc::LivenessChecker(session));

  return std::unique_ptr<simgrid::mc::Checker>(
    new FunctionalChecker(session, std::move(code)));
}

int main(int argc, char** argv)
{
  using simgrid::mc::Session;

  try {
    if (argc < 2)
      xbt_die("Missing arguments.\n");

    // Currently, we need this before sg_config_init:
    _sg_do_model_check = 1;
    mc_mode = MC_MODE_SERVER;

    // The initialisation function can touch argv.
    // We make a copy of argv before modifying it in order to pass the original
    // value to the model-checked:
    char** argv_copy = argvdup(argc, argv);
    xbt_log_init(&argc, argv);
    sg_config_init(&argc, argv);

    std::unique_ptr<Session> session =
      std::unique_ptr<Session>(Session::spawnvp(argv_copy[1], argv_copy+1));
    free(argv_copy);

    simgrid::mc::session = session.get();
    std::unique_ptr<simgrid::mc::Checker> checker = createChecker(*session);
    int res = checker->run();
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
