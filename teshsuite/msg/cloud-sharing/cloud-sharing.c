/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

const int FAIL_ON_ERROR = 0;
const int flop_amount = 100000000;
int failed_test = 0;

static int computation_fun(int argc, char* argv[])
{
  int *size = MSG_process_get_data(MSG_process_self());
  msg_task_t task = MSG_task_create("Task", *size, 0, NULL);

  double begin = MSG_get_clock();
  MSG_task_execute(task);
  double end = MSG_get_clock();

  if (0.1 - (end - begin) > 0.001) {
    xbt_assert(! FAIL_ON_ERROR, "%s with %.4g load (%dflops) took %.4fs instead of 0.1s",
        MSG_process_get_name(MSG_process_self()), ((double)*size/flop_amount),*size, (end-begin));
    XBT_INFO("FAILED TEST: %s with %.4g load (%dflops) took %.4fs instead of 0.1s",
        MSG_process_get_name(MSG_process_self()),((double)*size/flop_amount), *size, (end-begin));
    failed_test ++;
  } else {
    XBT_INFO("Passed: %s with %.4g load (%dflops) took 0.1s as expected",
        MSG_process_get_name(MSG_process_self()), ((double)*size/flop_amount), *size);
  }

  MSG_task_destroy(task);
  free(size);

  return 0;
}

static void run_test(const char *name, msg_host_t location, int size) {
  int* data = xbt_new(int, 1);
  *data = size;
  MSG_process_create(name, computation_fun, data, location);
}

static int master_main(int argc, char* argv[])
{
	
  XBT_INFO("# TEST ON SINGLE-CORE PMs");

  msg_host_t pm0 = MSG_host_by_name("node-0.acme.org");
  msg_host_t pm1 = MSG_host_by_name("node-1.acme.org");
  msg_host_t vm0;
  xbt_assert(pm0, "Host node-0.acme.org does not seem to exist");
  
  // syntax of the process name:
  // "( )1" means PM with one core; "( )2" means PM with 2 cores
  // "(  [  ]2  )4" means a VM with 2 cores, on a PM with 4 cores.
  // "o" means another process is there
  // "X" means the process which holds this name

  XBT_INFO("## Test 1 (started): check computation on normal PMs");

  XBT_INFO("### Put a task on a PM");
  run_test("(X)1", pm0, flop_amount);
  MSG_process_sleep(2);

  XBT_INFO("### Put two tasks on a PM");
  run_test("(Xo)1", pm0, flop_amount/2);
  run_test("(oX)1", pm0, flop_amount/2);
  MSG_process_sleep(2);

  XBT_INFO("### Put a task on each PM");
  run_test("(X)1 (o)1", pm0, flop_amount);
  run_test("(o)1 (X)1", pm1, flop_amount);
  MSG_process_sleep(2);

  XBT_INFO("## Test 1 (ended)");
  XBT_INFO("# TEST ON SINGLE-CORE PMs AND SINGLE-CORE VMs");

  XBT_INFO("## Test 2 (started): check impact of running tasks inside a VM (there is no degradation for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  run_test("( [X]1 )1", vm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two task to the VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  run_test("( [Xo]1 )1", vm0, flop_amount/2);
  run_test("( [oX]1 )1", vm0, flop_amount/2);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 2 (ended)");
  
  XBT_INFO("## Test 3 (started): check impact of running tasks collocated with VMs (there is no VM noise for the moment)");

  XBT_INFO("### Put a task on a PM collocated with an empty VM");

  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  run_test("( [ ]1 X )1", pm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put a task to the PM and a task to the VM");

  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  run_test("( [X]1 o )1", vm0, flop_amount/2);
  run_test("( [o]1 X )1", pm0, flop_amount/2);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put a task to the PM and two tasks to the VM");

  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  run_test("( [Xo]1 o )1", vm0, flop_amount/4);
  run_test("( [oX]1 o )1", vm0, flop_amount/4);
  run_test("( [oo]1 X )1", pm0, flop_amount/2);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  XBT_INFO("## Test 3 (ended)");
  
  XBT_INFO("# TEST ON TWO-CORE PMs");
  
  msg_host_t pm2 = MSG_host_by_name("node-0.acme2.org"); // 2 cores
  xbt_assert(pm2, "Host node-0.acme2.org does not seem to exist");
  
  XBT_INFO("## Test 4 (started): check computation on 2 cores PMs");

  XBT_INFO("### Put a task on a PM");
  run_test("(X)2", pm2, flop_amount);
  MSG_process_sleep(2);

  XBT_INFO("### Put two tasks on a PM");
  run_test("(Xx)2", pm2, flop_amount);
  run_test("(xX)2", pm2, flop_amount);
  MSG_process_sleep(2);
  
  XBT_INFO("### Put three tasks on a PM");
  run_test("(Xxx)2", pm2, flop_amount*2/3);
  run_test("(xXx)2", pm2, flop_amount*2/3);
  run_test("(xxX)2", pm2, flop_amount*2/3);
  MSG_process_sleep(2);

  XBT_INFO("## Test 4 (ended)");
  
  XBT_INFO("# TEST ON TWO-CORE PMs AND SINGLE-CORE VMs");
  
  XBT_INFO("## Test 5 (started): check impact of a single VM (there is no degradation for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  vm0 = MSG_vm_create_core(pm2, "VM0");
  MSG_vm_start(vm0);
  run_test("( [X]1 )2", vm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
  vm0 = MSG_vm_create_core(pm2, "VM0");
  MSG_vm_start(vm0);
  run_test("( [Xx]1 )2", vm0, flop_amount/2);
  run_test("( [xX]1 )2", vm0, flop_amount/2);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = MSG_vm_create_core(pm2, "VM0");
  MSG_vm_start(vm0);
  run_test("( [ ]1 X )2", pm2, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put a task to the PM and a task to the VM");
  vm0 = MSG_vm_create_core(pm2, "VM0");
  MSG_vm_start(vm0);
  run_test("( [X]1 x )2", vm0, flop_amount);
  run_test("( [x]1 X )2", pm2, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 5 (ended)");
  
  XBT_INFO("## Test 6 (started): check impact of a several VMs (there is no degradation for the moment)");

  XBT_INFO("### Put two VMs on a PM, and put a task to one VM");
  vm0 = MSG_vm_create_core(pm2, "VM0");
  msg_vm_t vm1 = MSG_vm_create_core(pm2, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  run_test("( [X]1 [ ]1 )2", vm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  
  XBT_INFO("### Put two VMs on a PM, and put a task to each VM");
  vm0 = MSG_vm_create_core(pm2, "VM0");
  vm1 = MSG_vm_create_core(pm2, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  run_test("( [X]1 [x]1 )2", vm0, flop_amount);
  run_test("( [x]1 [X]1 )2", vm1, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  
  XBT_INFO("### Put three VMs on a PM, and put a task to two VMs");
  vm0 = MSG_vm_create_core(pm2, "VM0");
  vm1 = MSG_vm_create_core(pm2, "VM1");
  msg_vm_t vm2 = MSG_vm_create_core(pm2, "VM2");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_vm_start(vm2);
  run_test("( [X]1 [x]1 [ ]1 )2", vm0, flop_amount);
  run_test("( [x]1 [X]1 [ ]1 )2", vm1, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  MSG_vm_destroy(vm2);
  
  XBT_INFO("### Put three VMs on a PM, and put a task to each VM");
  vm0 = MSG_vm_create_core(pm2, "VM0");
  vm1 = MSG_vm_create_core(pm2, "VM1");
  vm2 = MSG_vm_create_core(pm2, "VM2");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_vm_start(vm2);
  run_test("( [X]1 [o]1 [o]1 )2", vm0, flop_amount*2/3);
  run_test("( [o]1 [X]1 [o]1 )2", vm1, flop_amount*2/3);
  run_test("( [o]1 [o]1 [X]1 )2", vm2, flop_amount*2/3);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  MSG_vm_destroy(vm2);
  
  XBT_INFO("## Test 6 (ended)");
  
  XBT_INFO("# TEST ON TWO-CORE PMs AND TWO-CORE VMs");
  
  XBT_INFO("## Test 7 (started): check impact of a single VM (there is no degradation for the moment)");
  
  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [X]2 )2", vm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [Xo]2 )2", vm0, flop_amount);
  run_test("( [oX]2 )2", vm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [Xoo]2 )2", vm0, flop_amount*2/3);
  run_test("( [oXo]2 )2", vm0, flop_amount*2/3);
  run_test("( [ooX]2 )2", vm0, flop_amount*2/3);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 7 (ended)");
  
  XBT_INFO("## Test 8 (started): check impact of a single VM collocated with a task (there is no degradation for the moment)");
  
  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [ ]2 X )2", pm2, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put one task to the PM and one task to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [o]2 X )2", pm2, flop_amount);
  run_test("( [X]2 o )2", vm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put one task to the PM and two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [oo]2 X )2", pm2, flop_amount*2/3);
  run_test("( [Xo]2 o )2", vm0, flop_amount*2/3);
  run_test("( [oX]2 o )2", vm0, flop_amount*2/3);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, put one task to the PM and three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [ooo]2 X )2", pm2, flop_amount*2/3);
  run_test("( [Xoo]2 o )2", vm0, flop_amount*2/9);
  run_test("( [oXo]2 o )2", vm0, flop_amount*2/9);
  run_test("( [ooX]2 o )2", vm0, flop_amount*2/9);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the PM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [ ]2 Xo )2", pm2, flop_amount);
  run_test("( [ ]2 oX )2", pm2, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("### Put a VM on a PM, put one task to the PM and one task to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [o]2 Xo )2", pm2, flop_amount*2/3);
  run_test("( [o]2 oX )2", pm2, flop_amount*2/3);
  run_test("( [X]2 oo )2", vm0, flop_amount*2/3);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("### Put a VM on a PM, put one task to the PM and two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [oo]2 Xo )2", pm2, flop_amount/2);
  run_test("( [oo]2 oX )2", pm2, flop_amount/2);
  run_test("( [Xo]2 oo )2", vm0, flop_amount/2);
  run_test("( [oX]2 oo )2", vm0, flop_amount/2);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("### Put a VM on a PM, put one task to the PM and three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
  MSG_vm_start(vm0);
  run_test("( [ooo]2 Xo )2", pm2, flop_amount*2/4);
  run_test("( [ooo]2 oX )2", pm2, flop_amount*2/4);
  run_test("( [Xoo]2 oo )2", vm0, flop_amount/3);
  run_test("( [oXo]2 oo )2", vm0, flop_amount/3);
  run_test("( [ooX]2 oo )2", vm0, flop_amount/3);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 8 (ended)");
  
  XBT_INFO("# TEST ON FOUR-CORE PMs AND TWO-CORE VMs");
  
  msg_host_t pm4 = MSG_host_by_name("node-0.acme4.org");
  xbt_assert(pm4, "Host node-0.acme4.org does not seem to exist");
  
  XBT_INFO("## Test 9 (started): check impact of a single VM");
  
  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
  MSG_vm_start(vm0);
  run_test("( [X]2 )4", vm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [Xo]2 )4", vm0, flop_amount);
  run_test("( [oX]2 )4", vm0, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### ( [ooo]2 )4: Put a VM on a PM, and put three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [Xoo]2 )4", vm0, flop_amount*2/3);
  run_test("( [oXo]2 )4", vm0, flop_amount*2/3);
  run_test("( [ooX]2 )4", vm0, flop_amount*2/3);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 9 (ended)");
  
  XBT_INFO("## Test 10 (started): check impact of a single emtpy VM collocated with tasks");
  
  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [ ]2 X )4", pm4, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the PM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [ ]2 Xo )4", pm4, flop_amount);
  run_test("( [ ]2 oX )4", pm4, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the PM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [ ]2 Xoo )4", pm4, flop_amount);
  run_test("( [ ]2 oXo )4", pm4, flop_amount);
  run_test("( [ ]2 ooX )4", pm4, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put four tasks to the PM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [ ]2 Xooo )4", pm4, flop_amount);
  run_test("( [ ]2 oXoo )4", pm4, flop_amount);
  run_test("( [ ]2 ooXo )4", pm4, flop_amount);
  run_test("( [ ]2 oooX )4", pm4, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 10 (ended)");
  
  XBT_INFO("## Test 11 (started): check impact of a single working VM collocated with tasks");
  
  XBT_INFO("### Put a VM on a PM, and put one task to the PM and one task to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
  MSG_vm_start(vm0);
  run_test("( [X]2 o )4", vm0, flop_amount);
  run_test("( [o]2 X )4", pm4, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the PM and one task to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [X]2 oo )4", vm0, flop_amount);
  run_test("( [o]2 Xo )4", pm4, flop_amount);
  run_test("( [o]2 oX )4", pm4, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put two tasks to the PM and two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [Xo]2 oo )4", vm0, flop_amount);
  run_test("( [oX]2 oo )4", vm0, flop_amount);
  run_test("( [oo]2 Xo )4", pm4, flop_amount);
  run_test("( [oo]2 oX )4", pm4, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and one tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [X]2 ooo )4", vm0, flop_amount);
  run_test("( [o]2 Xoo )4", pm4, flop_amount);
  run_test("( [o]2 oXo )4", pm4, flop_amount);
  run_test("( [o]2 ooX )4", pm4, flop_amount);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and two tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [Xo]2 ooo )4", vm0, flop_amount*4/5);
  run_test("( [oX]2 ooo )4", vm0, flop_amount*4/5);
  run_test("( [oo]2 Xoo )4", pm4, flop_amount*4/5);
  run_test("( [oo]2 oXo )4", pm4, flop_amount*4/5);
  run_test("( [oo]2 ooX )4", pm4, flop_amount*4/5);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and three tasks to the VM");
  vm0 = MSG_vm_create_multicore(pm4, "VM0",2);
  MSG_vm_start(vm0);
  run_test("( [Xoo]2 ooo )4", vm0, flop_amount*(8/5) * 1/3); // The VM has 8/5 of the PM
  run_test("( [oXo]2 ooo )4", vm0, flop_amount*(8/5) * 1/3);
  run_test("( [ooX]2 ooo )4", vm0, flop_amount*(8/5) * 1/3);

  run_test("( [ooo]2 Xoo )4", pm4, flop_amount*4/5);
  run_test("( [ooo]2 oXo )4", pm4, flop_amount*4/5);
  run_test("( [ooo]2 ooX )4", pm4, flop_amount*4/5);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  
  XBT_INFO("## Test 11 (ended)");

  XBT_INFO("## %d test failed", failed_test);
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
