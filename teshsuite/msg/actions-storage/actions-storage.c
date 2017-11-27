/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "simgrid/plugins/file_system.h"
#include <xbt/replay.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(storage_actions, "Messages specific for this example");

static xbt_dict_t opened_files = NULL;

#define ACT_DEBUG(...)                                                                                                 \
  if (XBT_LOG_ISENABLED(storage_actions, xbt_log_priority_verbose)) {                                                  \
    char* NAME = xbt_str_join_array(action, " ");                                                                      \
    XBT_DEBUG(__VA_ARGS__);                                                                                            \
    xbt_free(NAME);                                                                                                    \
  } else                                                                                                               \
  ((void)0)

static void log_action(const char* const* action, double date)
{
  if (XBT_LOG_ISENABLED(storage_actions, xbt_log_priority_verbose)) {
    char* name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %f", name, date);
    xbt_free(name);
  }
}

static msg_file_t get_file_descriptor(const char* file_name)
{
  char full_name[1024];

  snprintf(full_name, 1023, "%s:%s", MSG_process_get_name(MSG_process_self()), file_name);

  msg_file_t file = (msg_file_t)xbt_dict_get_or_null(opened_files, full_name);
  return file;
}

static sg_size_t parse_size(const char* string)
{
  sg_size_t size;
  char* endptr;

  size = strtoul(string, &endptr, 10);
  if (*endptr != '\0')
    THROWF(unknown_error, 0, "%s is not a long unsigned int (a.k.a. sg_size_t)", string);
  return size;
}

static void action_open(const char* const* action)
{
  const char* file_name = action[2];
  char full_name[1024];
  msg_file_t file = NULL;
  double clock    = MSG_get_clock();

  snprintf(full_name, 1023, "%s:%s", MSG_process_get_name(MSG_process_self()), file_name);

  ACT_DEBUG("Entering Open: %s (filename: %s)", NAME, file_name);
  file = MSG_file_open(file_name, NULL);

  xbt_dict_set(opened_files, full_name, file, NULL);

  log_action(action, MSG_get_clock() - clock);
}

static void action_read(const char* const* action)
{
  const char* file_name = action[2];
  const char* size_str  = action[3];
  sg_size_t size        = parse_size(size_str);

  double clock = MSG_get_clock();

  msg_file_t file = get_file_descriptor(file_name);

  ACT_DEBUG("Entering Read: %s (size: %llu)", NAME, size);
  MSG_file_read(file, size);

  log_action(action, MSG_get_clock() - clock);
}

static void action_close(const char* const* action)
{
  const char* file_name = action[2];
  msg_file_t file;
  double clock = MSG_get_clock();

  file = get_file_descriptor(file_name);

  ACT_DEBUG("Entering Close: %s (filename: %s)", NAME, file_name);
  MSG_file_close(file);

  log_action(action, MSG_get_clock() - clock);
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  MSG_storage_file_system_init();
  /* Explicit initialization of the action module is required */
  MSG_action_init();

  xbt_assert(argc > 3,
             "Usage: %s platform_file deployment_file [action_files]\n"
             "\texample: %s platform.xml deployment.xml actions # if all actions are in the same file\n"
             "\texample: %s platform.xml deployment.xml # if actions are in separate files, specified in deployment\n",
             argv[0], argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[2]);

  /*   Action registration */
  xbt_replay_action_register("open", action_open);
  xbt_replay_action_register("read", action_read);
  xbt_replay_action_register("close", action_close);

  if (opened_files == NULL)
    opened_files = xbt_dict_new_homogeneous(NULL);
  /* Actually do the simulation using MSG_action_trace_run */
  msg_error_t res = MSG_action_trace_run(argv[3]); // it's ok to pass a NULL argument here

  XBT_INFO("Simulation time %g", MSG_get_clock());

  if (opened_files)
    xbt_dict_free(&opened_files);

  /* Explicit finalization of the action module is required now*/
  MSG_action_exit();

  return res != MSG_OK;
}
