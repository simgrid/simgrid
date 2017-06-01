/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int computation_fun(int argc, char* argv[])
{
  msg_task_t task = MSG_task_create("Task", 100000000, 0, NULL);

  double clock_sta = MSG_get_clock();
  MSG_task_execute(task);
  double clock_end = MSG_get_clock();

  XBT_INFO("Task took %gs to execute", clock_end - clock_sta);

  MSG_task_destroy(task);

  return 0;
}

static int master_main(int argc, char* argv[])
{
  msg_host_t pm0 = MSG_host_by_name("node-0.acme.org");
  msg_host_t pm1 = MSG_host_by_name("node-1.acme.org");
  xbt_assert(pm0, "Host node-0.acme.org does not seem to exist");

  XBT_INFO("## Test 1 (started): check computation on normal PMs");

  XBT_INFO("### Put a task on a PM");
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);

  XBT_INFO("### Put two tasks on a PM");
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);

  XBT_INFO("### Put a task on each PM");
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm1);
  MSG_process_sleep(2);

  XBT_INFO("## Test 1 (ended)");

  XBT_INFO("## Test 2 (started): check impact of running a task inside a VM (there is no degradation for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  msg_vm_t vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 2 (ended)");

  XBT_INFO(
      "## Test 3 (started): check impact of running a task collocated with a VM (there is no VM noise for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 3 (ended)");

  XBT_INFO("## Test 4 (started): compare the cost of running two tasks inside two different VMs collocated or not (for"
           " the moment, there is no degradation for the VMs. Hence, the time should be equals to the time of test 1");

  XBT_INFO("### Put two VMs on a PM, and put a task to each VM");
  vm0          = MSG_vm_create_core(pm0, "VM0");
  msg_vm_t vm1 = MSG_vm_create_core(pm0, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm1);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);

  XBT_INFO("### Put a VM on each PM, and put a task to each VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  vm1 = MSG_vm_create_core(pm1, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm1);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  XBT_INFO("## Test 4 (ended)");

  return 0;
}

int main(int argc, char* argv[])
{
  /* Get the arguments */
  MSG_init(&argc, argv);

  /* load the platform file */
  const char* platform = "../../platforms/cluster.xml";
  if (argc == 2)
    platform = argv[1];
  MSG_create_environment(platform);

  msg_host_t pm0 = MSG_host_by_name("node-0.acme.org");
  xbt_assert(pm0, "Host 'node-0.acme.org' not found");
  MSG_process_create("master", master_main, NULL, pm0);

  return MSG_main() != MSG_OK;
}
