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
	
  XBT_INFO("# TEST ON SINGLE-CORE PMs");

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
  
  XBT_INFO("# TEST ON SINGLE-CORE PMs AND SINGLE-CORE VMs");

  XBT_INFO("## Test 2 (started): check impact of running tasks inside a VM (there is no degradation for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  msg_vm_t vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two task to the VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 2 (ended)");
  
  XBT_INFO("## Test 3 (started): check impact of running tasks collocated with VMs (there is no VM noise for the moment)");

  XBT_INFO("### Put a task on a PM collocated with an empty VM");

  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put a task to the PM and a task to the VM");

  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put a task to the PM and two tasks to the VM");

  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 3 (ended)");
  
  XBT_INFO("# TEST ON TWO-CORE PMs");
  
  pm0 = MSG_host_by_name("node-0.acme2.org");
  xbt_assert(pm0, "Host node-0.acme2.org does not seem to exist");
  
  XBT_INFO("## Test 4 (started): check computation on 2 cores PMs");

  XBT_INFO("### Put a task on a PM");
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);

  XBT_INFO("### Put two tasks on a PM");
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  
  XBT_INFO("### Put three tasks on a PM");
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);

  XBT_INFO("## Test 4 (ended)");
  
  XBT_INFO("# TEST ON TWO-CORE PMs AND SINGLE-CORE VMs");
  
  XBT_INFO("## Test 5 (started): check impact of a single VM (there is no degradation for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put a task to the PM and a task to the VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 5 (ended)");
  
  XBT_INFO("## Test 6 (started): check impact of a several VMs (there is no degradation for the moment)");

  XBT_INFO("### Put two VMs on a PM, and put a task to one VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  msg_vm_t vm1 = MSG_vm_create_core(pm0, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  
  XBT_INFO("### Put two VMs on a PM, and put a task to each VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  vm1 = MSG_vm_create_core(pm0, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm1);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  
  XBT_INFO("### Put three VMs on a PM, and put a task to two VMs");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  vm1 = MSG_vm_create_core(pm0, "VM1");
  msg_vm_t vm2 = MSG_vm_create_core(pm0, "VM2");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_vm_start(vm2);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm1);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  MSG_vm_destroy(vm2);
  
  XBT_INFO("### Put three VMs on a PM, and put a task to each VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  vm1 = MSG_vm_create_core(pm0, "VM1");
  vm2 = MSG_vm_create_core(pm0, "VM2");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_vm_start(vm2);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm1);
  MSG_process_create("compute", computation_fun, NULL, vm2);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  MSG_vm_destroy(vm2);
  
  XBT_INFO("## Test 6 (ended)");
  
  XBT_INFO("# TEST ON TWO-CORE PMs AND TWO-CORE VMs");
  
  XBT_INFO("## Test 7 (started): check impact of a single VM (there is no degradation for the moment)");
  
  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 7 (ended)");
  
  XBT_INFO("## Test 8 (started): check impact of a single VM collocated with a task (there is no degradation for the moment)");
  
  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put one task to the PM and one task to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put one task to the PM and two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put one task to the PM and three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 8 (ended)");
  
  XBT_INFO("# TEST ON FOUR-CORE PMs AND TWO-CORE VMs");
  
  pm0 = MSG_host_by_name("node-0.acme4.org");
  xbt_assert(pm0, "Host node-0.acme4.org does not seem to exist");
  
  XBT_INFO("## Test 9 (started): check impact of a single VM");
  
  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 9 (ended)");
  
  XBT_INFO("## Test 10 (started): check impact of a single emtpy VM collocated with tasks");
  
  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the PM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the PM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put four tasks to the PM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 10 (ended)");
  
  XBT_INFO("## Test 11 (started): check impact of a single working VM collocated with tasks");
  
  XBT_INFO("### Put a VM on a PM, and put one task to the PM and one task to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the PM and one task to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the PM and two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and one tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm0, "VM0",2);
  MSG_vm_start(vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, vm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_create("compute", computation_fun, NULL, pm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 11 (ended)");

  return 0;
}

int main(int argc, char* argv[])
{
  /* Get the arguments */
  MSG_init(&argc, argv);

  /* load the platform file */
  const char* platform = "../../../platforms/cloud-sharing.xml";
  if (argc == 2)
    platform = argv[1];
  MSG_create_environment(platform);

  msg_host_t pm0 = MSG_host_by_name("node-0.acme.org");
  xbt_assert(pm0, "Host 'node-0.acme.org' not found");
  MSG_process_create("master", master_main, NULL, pm0);

  return MSG_main() != MSG_OK;
}
