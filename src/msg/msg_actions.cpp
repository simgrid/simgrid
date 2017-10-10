/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "xbt/replay.hpp"

#include <cerrno>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_action, msg, "MSG actions for trace driven simulation");

extern "C" {

void MSG_action_init()
{
  MSG_function_register_default(simgrid::xbt::replay_runner);
}

void MSG_action_exit()
{
  // Nothing to do anymore here
}

/** \ingroup msg_trace_driven
 * \brief A trace loader
 *
 *  If path!=nullptr, load a trace file containing actions, and execute them.
 *  Else, assume that each process gets the path in its deployment file
 */
msg_error_t MSG_action_trace_run(char *path)
{
  if (path) {
    simgrid::xbt::action_fs = new std::ifstream(path, std::ifstream::in);
  }

  msg_error_t res = MSG_main();

  if (not simgrid::xbt::action_queues.empty()) {
    XBT_WARN("Not all actions got consumed. If the simulation ended successfully (without deadlock),"
             " you may want to add new processes to your deployment file.");

    for (auto const& actions_of : simgrid::xbt::action_queues) {
      XBT_WARN("Still %zu actions for %s", actions_of.second->size(), actions_of.first.c_str());
    }
  }

  if (path) {
    delete simgrid::xbt::action_fs;
    simgrid::xbt::action_fs = nullptr;
  }

  return res;
}
}
