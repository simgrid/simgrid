/* Copyright (c) 2007-2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "simgrid/msg.h"
#include "simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(energy_vm, "Messages of this example");

static int worker_func() {
  msg_task_t task1 = MSG_task_create("t1", 300E6, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);
  XBT_INFO("This worker is done.");
  return 0;
}

static int dvfs(int argc, char *argv[])
{
  msg_host_t host1 = MSG_host_by_name("MyHost1");
  msg_host_t host2 = MSG_host_by_name("MyHost2");
  msg_host_t host3 = MSG_host_by_name("MyHost3");

  /* Host 1 */
  XBT_INFO("Creating and starting two VMs");
  msg_vm_t vm_host1 = MSG_vm_create(host1, "vm_host1", 4, 2048, 100, NULL, 1024 * 20, 10,50);
  MSG_vm_start(vm_host1);
  msg_vm_t vm_host3 = MSG_vm_create(host3, "vm_host3", 4, 2048, 100, NULL, 1024 * 20, 10,50);
  MSG_vm_start(vm_host3);

  XBT_INFO("Create two tasks on Host1: one inside a VM, the other directly on the host");
  MSG_process_create("p11", worker_func, NULL, vm_host1);
  MSG_process_create("p12", worker_func, NULL, host1);

  XBT_INFO("Create two tasks on Host2: both directly on the host");
  MSG_process_create("p21", worker_func, NULL, host2);
  MSG_process_create("p22", worker_func, NULL, host2);

  XBT_INFO("Create two tasks on Host3: both inside a VM");
  MSG_process_create("p31", worker_func, NULL, vm_host3);
  MSG_process_create("p32", worker_func, NULL, vm_host3);

  XBT_INFO("Wait 5 seconds. The tasks are still running (they run for 3 seconds, but 2 tasks are co-located, "
           "so they run for 6 seconds)");
  MSG_process_sleep(5);
  XBT_INFO("Wait another 5 seconds. The tasks stop at some point in between");
  MSG_process_sleep(5);

  MSG_vm_shutdown(vm_host1);
  MSG_vm_shutdown(vm_host3);

  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;
  sg_energy_plugin_init();
  MSG_init(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_process_create("dvfs",dvfs,NULL,MSG_host_by_name("MyHost1"));

  res = MSG_main();

  XBT_INFO("Total simulation time: %.2f; All hosts must have the exact same energy consumption.", MSG_get_clock());

  return res != MSG_OK;
}

