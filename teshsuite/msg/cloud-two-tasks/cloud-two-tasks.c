/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

msg_task_t atask = NULL;

static int computation_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  const char* pr_name   = MSG_process_get_name(MSG_process_self());
  const char* host_name = MSG_host_get_name(MSG_host_self());
  atask                 = MSG_task_create("Task1", 1e9, 1e9, NULL);
  double clock_sta      = MSG_get_clock();
  XBT_INFO("%s:%s task 1 created %g", host_name, pr_name, clock_sta);
  MSG_task_execute(atask);
  double clock_end = MSG_get_clock();

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

static int master_main(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_host_t pm0 = MSG_host_by_name("Fafard");
  msg_vm_t vm0   = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);

  MSG_process_create("compute", computation_fun, NULL, (msg_host_t)vm0);

  while (MSG_get_clock() < 100) {
    if (atask != NULL)
      XBT_INFO("aTask remaining duration: %g", MSG_task_get_flops_amount(atask));
    MSG_process_sleep(1);
  }

  MSG_process_sleep(10000);
  MSG_vm_destroy(vm0);
  return 1;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);

  xbt_assert(argc == 2);
  MSG_create_environment(argv[1]);

  MSG_process_create("master_", master_main, NULL, MSG_host_by_name("Fafard"));

  int res = MSG_main();
  XBT_INFO("Bye (simulation time %g)", MSG_get_clock());

  return !(res == MSG_OK);
}
