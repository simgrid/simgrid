/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <sys/time.h>
#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/*
 * Usage:
 * ./examples/msg/cloud/scale ../examples/platforms/g5k.xml
 *
 * 1. valgrind --tool=callgrind ./examples/msg/cloud/scale ../examples/platforms/g5k.xml
 * 2. kcachegrind
 **/

static double time_precise(void) {
  struct timeval tv;
  int ret = gettimeofday(&tv, NULL);
  xbt_assert(ret >= 0, "gettimeofday");
  return (double) tv.tv_sec + tv.tv_usec * 0.001 * 0.001;
}

static int computation_fun(int argc, char *argv[]) {
  for (;;) {
    // double clock_sta = time_precise();

    msg_task_t task = MSG_task_create("Task", 10000000, 0, NULL);
    MSG_task_execute(task);
    MSG_task_destroy(task);

    // double clock_end = time_precise();

    // XBT_INFO("%f", clock_end - clock_sta);
  }
  return 0;
}

static void launch_computation_worker(msg_host_t host) {
  MSG_process_create("compute", computation_fun, NULL, host);
}

static int master_main(int argc, char *argv[])
{
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();

  int npm = 10;
  int nvm = 1000;

  msg_host_t *pm = xbt_new(msg_host_t, npm);
  msg_vm_t   *vm = xbt_new(msg_vm_t, nvm);

  int i = 0;
  for (i = 0; i < npm; i++) {
    pm[i] = xbt_dynar_get_as(hosts_dynar, i, msg_host_t);
  }

  for (i = 0; i < nvm; i++) {
    int pm_index = i % npm;
    char *vm_name = bprintf("vm%d", i);
    vm[i] = MSG_vm_create_core(pm[pm_index], vm_name);
    MSG_vm_start(vm[i]);

    launch_computation_worker(vm[i]);

    xbt_free(vm_name);
  }

  XBT_INFO("## Test (start)");

  for (i = 0; i < 10; i++) {
    double clock_sta = time_precise();
    MSG_process_sleep(1);
    double clock_end = time_precise();
    XBT_INFO("duration %f", clock_end - clock_sta);
  }

  for (i = 0; i < nvm; i++) {
    MSG_vm_destroy(vm[i]);
  }

  XBT_INFO("## Test (ended)");
  return 0;
}

static void launch_master(msg_host_t host)
{
  const char *pr_name = "master_";
  char **argv = xbt_new(char *, 2);
  argv[0] = xbt_strdup(pr_name);
  argv[1] = NULL;

  MSG_process_create_with_arguments(pr_name, master_main, NULL, host, 1, argv);
}


int main(int argc, char *argv[])
{
  /* Get the arguments */
  MSG_init(&argc, argv);

  /* load the platform file */
  xbt_assert(argc == 2);
  MSG_create_environment(argv[1]);

  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);
  launch_master(pm0);

  int res = MSG_main();
  XBT_INFO("Bye (simulation time %g)", MSG_get_clock());

  return !(res == MSG_OK);
}
