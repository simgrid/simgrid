/* Copyright (c) 2009-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.h"
#include "xbt/replay.hpp"

#include <errno.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_action, msg, "MSG actions for trace driven simulation");

SG_BEGIN_DECL()

void MSG_action_init()
{
  simgrid::xbt::replay_init();
  MSG_function_register_default(simgrid::xbt::replay_runner);
}

void MSG_action_exit()
{
  simgrid::xbt::replay_exit();
}

/** \ingroup msg_trace_driven
 * \brief A trace loader
 *
 *  If path!=nullptr, load a trace file containing actions, and execute them.
 *  Else, assume that each process gets the path in its deployment file
 */
msg_error_t MSG_action_trace_run(char *path)
{
  msg_error_t res;
  char *name;
  xbt_dynar_t todo;
  xbt_dict_cursor_t cursor;

  if (path) {
    simgrid::xbt::action_fs = new std::ifstream(path, std::ifstream::in);
  }
  res = MSG_main();

  if (!xbt_dict_is_empty(xbt_action_queues)) {
    XBT_WARN("Not all actions got consumed. If the simulation ended successfully (without deadlock),"
             " you may want to add new processes to your deployment file.");

    xbt_dict_foreach(xbt_action_queues, cursor, name, todo) {
      XBT_WARN("Still %lu actions for %s", xbt_dynar_length(todo), name);
    }
  }

  if (path) {
    delete simgrid::xbt::action_fs;
    simgrid::xbt::action_fs = nullptr;
  }

  xbt_dict_free(&xbt_action_queues);
  xbt_action_queues = xbt_dict_new_homogeneous(nullptr);

  return res;
}

SG_END_DECL()
