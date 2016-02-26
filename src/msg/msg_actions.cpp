/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/dynar.h"
#include "xbt/replay.h"

#include <errno.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_action, msg, "MSG actions for trace driven simulation");

void MSG_action_init()
{
  _xbt_replay_action_init();
  MSG_function_register_default(xbt_replay_action_runner);
}

void MSG_action_exit()
{
  _xbt_replay_action_exit();
}

/** \ingroup msg_trace_driven
 * \brief A trace loader
 *
 *  If path!=NULL, load a trace file containing actions, and execute them.
 *  Else, assume that each process gets the path in its deployment file
 */
msg_error_t MSG_action_trace_run(char *path)
{
  msg_error_t res;
  char *name;
  xbt_dynar_t todo;
  xbt_dict_cursor_t cursor;

  xbt_action_fp=NULL;
  if (path) {
    xbt_action_fp = fopen(path, "r");
    xbt_assert(xbt_action_fp != NULL, "Cannot open %s: %s", path, strerror(errno));
  }
  res = MSG_main();

  if (!xbt_dict_is_empty(xbt_action_queues)) {
    XBT_WARN("Not all actions got consumed. If the simulation ended successfully (without deadlock),"
             " you may want to add new processes to your deployment file.");

    xbt_dict_foreach(xbt_action_queues, cursor, name, todo) {
      XBT_WARN("Still %lu actions for %s", xbt_dynar_length(todo), name);
    }
  }

  if (path)
    fclose(xbt_action_fp);
  xbt_dict_free(&xbt_action_queues);
  xbt_action_queues = xbt_dict_new_homogeneous(NULL);

  return res;
}
