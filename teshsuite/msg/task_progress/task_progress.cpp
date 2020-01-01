/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static std::vector<msg_task_t> tasks = std::vector<msg_task_t>();

static int seq_task(int /*argc*/, char* /*argv*/ [])
{
  double task_comp_size = 5E7;
  double task_comm_size = 1E6;
  double progress;

  msg_task_t task = MSG_task_create("simple", task_comp_size, task_comm_size, NULL);
  tasks.push_back(task);

  XBT_INFO("get the progress of %s before the task starts", MSG_task_get_name(task));
  progress = MSG_task_get_remaining_work_ratio(task);
  xbt_assert(progress == 1.0, "Progress should be 1.0 not %f", progress);

  XBT_INFO("Executing task: \"%s\"", MSG_task_get_name(task));
  MSG_task_execute(task);

  XBT_INFO("get the progress of %s after the task finishes", MSG_task_get_name(task));
  progress = MSG_task_get_remaining_work_ratio(task);
  xbt_assert(progress == 0.0, "Progress should be equal to 0.0 not %f", progress);

  MSG_task_destroy(task);
  XBT_INFO("Goodbye now!");
  return 0;
}

static int par_task(int /*argc*/, char* /*argv*/ [])
{
  double * computation_amount = new double[2] {10E7, 10E7};
  double * communication_amount = new double[4] {1E6, 1E6, 1E6, 1E6};
  double progress;

  std::vector<msg_host_t> hosts_to_use = std::vector<msg_host_t>();
  hosts_to_use.push_back(MSG_get_host_by_name("Tremblay"));
  hosts_to_use.push_back(MSG_get_host_by_name("Jupiter"));

  msg_task_t task = MSG_parallel_task_create("ptask", 2, hosts_to_use.data(), computation_amount, communication_amount, NULL);
  tasks.push_back(task);

  XBT_INFO("get the progress of %s before the task starts", MSG_task_get_name(task));
  progress = MSG_task_get_remaining_work_ratio(task);
  xbt_assert(progress == 1.0, "Progress should be 1.0 not %f", progress);

  XBT_INFO("Executing task: \"%s\"", MSG_task_get_name(task));
  MSG_parallel_task_execute(task);

  XBT_INFO("get the progress of %s after the task finishes", MSG_task_get_name(task));
  progress = MSG_task_get_remaining_work_ratio(task);
  xbt_assert(progress == 0.0, "Progress should be equal to 0.0 not %f", progress);

  MSG_task_destroy(task);
  delete[] computation_amount;
  delete[] communication_amount;

  XBT_INFO("Goodbye now!");
  return 0;
}

static int get_progress(int /*argc*/, char* /*argv*/ [])
{
  while (tasks.empty()) {
    MSG_process_sleep(0.5);
  }
  double progress;
  for(auto const& task: tasks) {
    double progress_prev = 1;
    for (int i = 0; i < 3; i++) {
      MSG_process_sleep(0.2);
      progress = MSG_task_get_remaining_work_ratio(task);
      xbt_assert(progress >= 0 and progress < 1, "Progress should be in [0, 1[, and not %f", progress);
      xbt_assert(progress < progress_prev, "Progress should decrease, not increase");
      XBT_INFO("Progress of \"%s\": %f", MSG_task_get_name(task), progress);
      progress_prev = progress;
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  MSG_config("host/model", "ptask_L07");
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../examples/platforms/two_hosts.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_process_create("sequential", seq_task, NULL, MSG_get_host_by_name("Tremblay"));

  MSG_process_create("parallel", par_task, NULL, MSG_get_host_by_name("Tremblay"));

  // Create a process to test in progress task
  MSG_process_create("get_progress", get_progress, NULL, MSG_get_host_by_name("Tremblay"));

  msg_error_t res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
