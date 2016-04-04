/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

msg_task_t atask = NULL;

static int computation_fun(int argc, char *argv[])
{
  const char *pr_name = MSG_process_get_name(MSG_process_self());
  const char *host_name = MSG_host_get_name(MSG_host_self());
  double clock_sta, clock_end;
  atask = MSG_task_create("Task1", 1e9, 1e9, NULL);
  clock_sta = MSG_get_clock();
  XBT_INFO("%s:%s task 1 created %g", host_name, pr_name, clock_sta);
  MSG_task_execute(atask);
  clock_end = MSG_get_clock();

  XBT_INFO("%s:%s task 1 executed %g", host_name, pr_name, clock_end - clock_sta);

  MSG_task_destroy(atask);
  atask = NULL;

  MSG_process_sleep(1);

  atask = MSG_task_create("Task2", 1e10, 1e10, NULL);

  clock_sta = MSG_get_clock();
  XBT_INFO("%s:%s task 2 created %g", host_name, pr_name, clock_sta);
  MSG_task_execute(atask);
  clock_end = MSG_get_clock();

  XBT_INFO("%s:%s task 2 executed %g", host_name, pr_name, clock_end - clock_sta);

  MSG_task_destroy(atask);
  atask = NULL;

  return 0;
}

static void launch_computation_worker(msg_host_t host)
{
  const char *pr_name = "compute";
  char **argv = xbt_new(char *, 2);
  argv[0] = xbt_strdup(pr_name);
  argv[1] = NULL;

  MSG_process_create_with_arguments(pr_name, computation_fun, NULL, host, 1, argv);
}

static int master_main(int argc, char *argv[])
{
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);
  msg_vm_t vm0;
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  //MSG_process_sleep(1);

  launch_computation_worker(vm0);

  while(MSG_get_clock()<100) {
  if (atask != NULL)
    XBT_INFO("aTask remaining duration: %g", MSG_task_get_flops_amount(atask));
  MSG_process_sleep(1);
  }

  MSG_process_sleep(10000);
  MSG_vm_destroy(vm0);
  xbt_dynar_free(&hosts_dynar);
  return 1;
}

static void launch_master(msg_host_t host)
{
  const char *pr_name = "master_";
  char **argv = xbt_new(char *, 2);
  argv[0] = xbt_strdup(pr_name);
  argv[1] = NULL;

  MSG_process_create_with_arguments(pr_name, master_main, NULL, host, 1, argv);
}

int main(int argc, char *argv[]){
  MSG_init(&argc, argv);

  xbt_assert(argc == 2);
  MSG_create_environment(argv[1]);

  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);
  launch_master(pm0);

  int res = MSG_main();
  XBT_INFO("Bye (simulation time %g)", MSG_get_clock());
  xbt_dynar_free(&hosts_dynar);

  return !(res == MSG_OK);
}

