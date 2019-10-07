/* Copyright (c) 2007-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "simgrid/plugins/load.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int execute_load_test(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_host_t host = MSG_host_by_name("MyHost1");

  XBT_INFO("Initial peak speed: %.0E flop/s; number of flops computed so far: %.0E (should be 0)",
           MSG_host_get_speed(host), sg_host_get_computed_flops(host));

  double start = MSG_get_clock();
  XBT_INFO("Sleep for 10 seconds");
  MSG_process_sleep(10);

  XBT_INFO("Done sleeping %.2fs; peak speed: %.0E flop/s; number of flops computed so far: %.0E (nothing should have "
           "changed)",
           MSG_get_clock() - start, MSG_host_get_speed(host), sg_host_get_computed_flops(host));

  // Run a task
  start            = MSG_get_clock();
  msg_task_t task1 = MSG_task_create("t1", 100E6, 0, NULL);
  XBT_INFO("Run a task of %.0E flops", MSG_task_get_flops_amount(task1));
  MSG_task_execute(task1);
  MSG_task_destroy(task1);

  XBT_INFO("Done working on my task; this took %.2fs; current peak speed: %.0E flop/s; number of flops computed so "
           "far: %.0E",
           MSG_get_clock() - start, MSG_host_get_speed(host), sg_host_get_computed_flops(host));

  // ========= Change power peak =========
  int pstate = 2;
  sg_host_set_pstate(host, pstate);
  XBT_INFO("========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s)", pstate,
           MSG_host_get_power_peak_at(host, pstate), MSG_host_get_speed(host));

  // Run a second task
  start = MSG_get_clock();
  task1 = MSG_task_create("t2", 100E6, 0, NULL);
  XBT_INFO("Run a task of %.0E flops", MSG_task_get_flops_amount(task1));
  MSG_task_execute(task1);
  MSG_task_destroy(task1);
  XBT_INFO("Done working on my task; this took %.2fs; current peak speed: %.0E flop/s; number of flops computed so "
           "far: %.0E",
           MSG_get_clock() - start, MSG_host_get_speed(host), sg_host_get_computed_flops(host));

  start = MSG_get_clock();
  XBT_INFO("========= Requesting a reset of the computation counter");
  sg_host_load_reset(host);
  XBT_INFO("Sleep for 4 seconds");
  MSG_process_sleep(4);
  XBT_INFO("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
           MSG_get_clock() - start, MSG_host_get_speed(host), sg_host_get_computed_flops(host));

  // =========== Turn the other host off ==========
  XBT_INFO("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 computed %.0f flops so far.",
           MSG_host_get_computed_flops(MSG_host_by_name("MyHost2")));
  MSG_host_off(MSG_host_by_name("MyHost2"));
  start = MSG_get_clock();
  MSG_process_sleep(10);
  XBT_INFO("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
           MSG_get_clock() - start, MSG_host_get_speed(host), sg_host_get_computed_flops(host));
  return 0;
}

int main(int argc, char* argv[])
{
  sg_host_load_plugin_init();
  MSG_init(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_process_create("load_test", execute_load_test, NULL, MSG_get_host_by_name("MyHost1"));

  msg_error_t res = MSG_main();

  XBT_INFO("Total simulation time: %.2f", MSG_get_clock());

  return res != MSG_OK;
}
