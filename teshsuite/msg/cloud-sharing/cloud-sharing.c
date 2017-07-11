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
  MSG_task_destroy(task);
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

  free(size);

  return 0;
}

static void run_test_process(const char* name, msg_host_t location, int size)
{
  int* data = xbt_new(int, 1);
  *data = size;
  MSG_process_create(name, computation_fun, data, location);
}

static void run_test(const char* chooser)
{
  msg_host_t pm0 = MSG_host_by_name("node-0.1core.org");
  msg_host_t pm1 = MSG_host_by_name("node-1.1core.org");
  msg_host_t pm2 = MSG_host_by_name("node-0.2cores.org"); // 2 cores
  msg_host_t pm4 = MSG_host_by_name("node-0.4cores.org");

  msg_vm_t vm0;
  xbt_assert(pm0, "Host node-0.1core.org does not seem to exist");
  xbt_assert(pm2, "Host node-0.2cores.org does not seem to exist");
  xbt_assert(pm4, "Host node-0.4cores.org does not seem to exist");

  // syntax of the process name:
  // "( )1" means PM with one core; "( )2" means PM with 2 cores
  // "(  [  ]2  )4" means a VM with 2 cores, on a PM with 4 cores.
  // "o" means another process is there
  // "X" means the process which holds this name

  if (!strcmp(chooser, "(o)1")) {
    XBT_INFO("### Test '%s'. A task on a regular PM", chooser);
    run_test_process("(X)1", pm0, flop_amount);
    MSG_process_sleep(2);

  } else if (!strcmp(chooser, "(oo)1")) {
    XBT_INFO("### Test '%s'. 2 tasks on a regular PM", chooser);
    run_test_process("(Xo)1", pm0, flop_amount / 2);
    run_test_process("(oX)1", pm0, flop_amount / 2);
    MSG_process_sleep(2);

  } else if (!strcmp(chooser, "(o)1 (o)1")) {
    XBT_INFO("### Test '%s'. 2 regular PMs, with a task each.", chooser);
    run_test_process("(X)1 (o)1", pm0, flop_amount);
    run_test_process("(o)1 (X)1", pm1, flop_amount);
    MSG_process_sleep(2);

  } else if (!strcmp(chooser, "( [o]1 )1")) {
    XBT_INFO("### Test '%s'. A task in a VM on a PM.", chooser);
    vm0 = MSG_vm_create_core(pm0, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [X]1 )1", (msg_host_t)vm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]1 )1")) {
    XBT_INFO("### Test '%s'. 2 tasks co-located in a VM on a PM.", chooser);
    vm0 = MSG_vm_create_core(pm0, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [Xo]1 )1", (msg_host_t)vm0, flop_amount / 2);
    run_test_process("( [oX]1 )1", (msg_host_t)vm0, flop_amount / 2);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ ]1 o )1")) {
    XBT_INFO("### Test '%s'. 1 task collocated with an empty VM", chooser);
    vm0 = MSG_vm_create_core(pm0, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [ ]1 X )1", pm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]1 o )1")) {
    XBT_INFO("### Test '%s'. A task in a VM, plus a task", chooser);
    vm0 = MSG_vm_create_core(pm0, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [X]1 o )1", (msg_host_t)vm0, flop_amount / 2);
    run_test_process("( [o]1 X )1", pm0, flop_amount / 2);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]1 o )1")) {
    XBT_INFO("### Test '%s'. 2 tasks in a VM, plus a task", chooser);
    vm0 = MSG_vm_create_core(pm0, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [Xo]1 o )1", (msg_host_t)vm0, flop_amount / 4);
    run_test_process("( [oX]1 o )1", (msg_host_t)vm0, flop_amount / 4);
    run_test_process("( [oo]1 X )1", pm0, flop_amount / 2);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( o )2")) {
    XBT_INFO("### Test '%s'. A task on bicore PM", chooser);
    run_test_process("(X)2", pm2, flop_amount);
    MSG_process_sleep(2);

  } else if (!strcmp(chooser, "( oo )2")) {
    XBT_INFO("### Test '%s'. 2 tasks on a bicore PM", chooser);
    run_test_process("(Xx)2", pm2, flop_amount);
    run_test_process("(xX)2", pm2, flop_amount);
    MSG_process_sleep(2);

  } else if (!strcmp(chooser, "( ooo )2")) {
    XBT_INFO("### Test '%s'. 3 tasks on a bicore PM", chooser);
    run_test_process("(Xxx)2", pm2, flop_amount * 2 / 3);
    run_test_process("(xXx)2", pm2, flop_amount * 2 / 3);
    run_test_process("(xxX)2", pm2, flop_amount * 2 / 3);
    MSG_process_sleep(2);

  } else if (!strcmp(chooser, "( [o]1 )2")) {
    XBT_INFO("### Test '%s'. A task in a VM on a bicore PM", chooser);
    vm0 = MSG_vm_create_core(pm2, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [X]1 )2", (msg_host_t)vm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]1 )2")) {
    XBT_INFO("### Test '%s'. 2 tasks in a VM on a bicore PM", chooser);
    vm0 = MSG_vm_create_core(pm2, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [Xx]1 )2", (msg_host_t)vm0, flop_amount / 2);
    run_test_process("( [xX]1 )2", (msg_host_t)vm0, flop_amount / 2);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ ]1 o )2")) {
    XBT_INFO("### Put a VM on a PM, and put a task to the PM");
    vm0 = MSG_vm_create_core(pm2, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [ ]1 X )2", pm2, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]1 o )2")) {
    XBT_INFO("### Put a VM on a PM, put a task to the PM and a task to the VM");
    vm0 = MSG_vm_create_core(pm2, "VM0");
    MSG_vm_start(vm0);
    run_test_process("( [X]1 x )2", (msg_host_t)vm0, flop_amount);
    run_test_process("( [x]1 X )2", pm2, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]1 [ ]1 )2")) {
    XBT_INFO("### Put two VMs on a PM, and put a task to one VM");
    vm0          = MSG_vm_create_core(pm2, "VM0");
    msg_vm_t vm1 = MSG_vm_create_core(pm2, "VM1");
    MSG_vm_start(vm0);
    MSG_vm_start(vm1);
    run_test_process("( [X]1 [ ]1 )2", (msg_host_t)vm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);
    MSG_vm_destroy(vm1);

  } else if (!strcmp(chooser, "( [o]1 [o]1 )2")) {
    XBT_INFO("### Put two VMs on a PM, and put a task to each VM");
    vm0          = MSG_vm_create_core(pm2, "VM0");
    msg_vm_t vm1 = MSG_vm_create_core(pm2, "VM1");
    MSG_vm_start(vm0);
    MSG_vm_start(vm1);
    run_test_process("( [X]1 [x]1 )2", (msg_host_t)vm0, flop_amount);
    run_test_process("( [x]1 [X]1 )2", (msg_host_t)vm1, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);
    MSG_vm_destroy(vm1);

  } else if (!strcmp(chooser, "( [o]1 [o]1 [ ]1 )2")) {
    XBT_INFO("### Put three VMs on a PM, and put a task to two VMs");
    vm0          = MSG_vm_create_core(pm2, "VM0");
    msg_vm_t vm1 = MSG_vm_create_core(pm2, "VM1");
    msg_vm_t vm2 = MSG_vm_create_core(pm2, "VM2");
    MSG_vm_start(vm0);
    MSG_vm_start(vm1);
    MSG_vm_start(vm2);
    run_test_process("( [X]1 [x]1 [ ]1 )2", (msg_host_t)vm0, flop_amount);
    run_test_process("( [x]1 [X]1 [ ]1 )2", (msg_host_t)vm1, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);
    MSG_vm_destroy(vm1);
    MSG_vm_destroy(vm2);

  } else if (!strcmp(chooser, "( [o]1 [o]1 [o]1 )2")) {
    XBT_INFO("### Put three VMs on a PM, and put a task to each VM");
    vm0          = MSG_vm_create_core(pm2, "VM0");
    msg_vm_t vm1 = MSG_vm_create_core(pm2, "VM1");
    msg_vm_t vm2 = MSG_vm_create_core(pm2, "VM2");
    MSG_vm_start(vm0);
    MSG_vm_start(vm1);
    MSG_vm_start(vm2);
    run_test_process("( [X]1 [o]1 [o]1 )2", (msg_host_t)vm0, flop_amount * 2 / 3);
    run_test_process("( [o]1 [X]1 [o]1 )2", (msg_host_t)vm1, flop_amount * 2 / 3);
    run_test_process("( [o]1 [o]1 [X]1 )2", (msg_host_t)vm2, flop_amount * 2 / 3);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);
    MSG_vm_destroy(vm1);
    MSG_vm_destroy(vm2);

  } else if (!strcmp(chooser, "( [o]2 )2")) {
    XBT_INFO("### Put a VM on a PM, and put a task to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [X]2 )2", (msg_host_t)vm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]2 )2")) {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [Xo]2 )2", (msg_host_t)vm0, flop_amount);
    run_test_process("( [oX]2 )2", (msg_host_t)vm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ooo]2 )2")) {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [Xoo]2 )2", (msg_host_t)vm0, flop_amount * 2 / 3);
    run_test_process("( [oXo]2 )2", (msg_host_t)vm0, flop_amount * 2 / 3);
    run_test_process("( [ooX]2 )2", (msg_host_t)vm0, flop_amount * 2 / 3);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ ]2 o )2")) {
    XBT_INFO("### Put a VM on a PM, and put a task to the PM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [ ]2 X )2", pm2, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]2 o )2")) {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and one task to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [o]2 X )2", pm2, flop_amount);
    run_test_process("( [X]2 o )2", (msg_host_t)vm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]2 o )2")) {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and two tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [oo]2 X )2", pm2, flop_amount * 2 / 3);
    run_test_process("( [Xo]2 o )2", (msg_host_t)vm0, flop_amount * 2 / 3);
    run_test_process("( [oX]2 o )2", (msg_host_t)vm0, flop_amount * 2 / 3);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ooo]2 o )2")) {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and three tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [ooo]2 X )2", pm2, flop_amount * 2 / 3);
    run_test_process("( [Xoo]2 o )2", (msg_host_t)vm0, flop_amount * (2. / 3 * 2) / 3); // VM_share/3
    run_test_process("( [oXo]2 o )2", (msg_host_t)vm0, flop_amount * (2. / 3 * 2) / 3); // VM_share/3
    run_test_process("( [ooX]2 o )2", (msg_host_t)vm0, flop_amount * (2. / 3 * 2) / 3); // VM_share/3
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ ]2 oo )2")) {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the PM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [ ]2 Xo )2", pm2, flop_amount);
    run_test_process("( [ ]2 oX )2", pm2, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]2 oo )2")) {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and one task to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [o]2 Xo )2", pm2, flop_amount * 2 / 3);
    run_test_process("( [o]2 oX )2", pm2, flop_amount * 2 / 3);
    run_test_process("( [X]2 oo )2", (msg_host_t)vm0, flop_amount * 2 / 3);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]2 oo )2")) {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and two tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [oo]2 Xo )2", pm2, flop_amount / 2);
    run_test_process("( [oo]2 oX )2", pm2, flop_amount / 2);
    run_test_process("( [Xo]2 oo )2", (msg_host_t)vm0, flop_amount / 2);
    run_test_process("( [oX]2 oo )2", (msg_host_t)vm0, flop_amount / 2);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ooo]2 oo )2")) {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and three tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm2, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [ooo]2 Xo )2", pm2, flop_amount * 2 / 4);
    run_test_process("( [ooo]2 oX )2", pm2, flop_amount * 2 / 4);
    run_test_process("( [Xoo]2 oo )2", (msg_host_t)vm0, flop_amount / 3);
    run_test_process("( [oXo]2 oo )2", (msg_host_t)vm0, flop_amount / 3);
    run_test_process("( [ooX]2 oo )2", (msg_host_t)vm0, flop_amount / 3);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]2 )4")) {
    XBT_INFO("### Put a VM on a PM, and put a task to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [X]2 )4", (msg_host_t)vm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]2 )4")) {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [Xo]2 )4", (msg_host_t)vm0, flop_amount);
    run_test_process("( [oX]2 )4", (msg_host_t)vm0, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ooo]2 )4")) {
    XBT_INFO("### ( [ooo]2 )4: Put a VM on a PM, and put three tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [Xoo]2 )4", (msg_host_t)vm0, flop_amount * 2 / 3);
    run_test_process("( [oXo]2 )4", (msg_host_t)vm0, flop_amount * 2 / 3);
    run_test_process("( [ooX]2 )4", (msg_host_t)vm0, flop_amount * 2 / 3);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ ]2 o )4")) {
    XBT_INFO("### Put a VM on a PM, and put a task to the PM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [ ]2 X )4", pm4, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ ]2 oo )4")) {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the PM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [ ]2 Xo )4", pm4, flop_amount);
    run_test_process("( [ ]2 oX )4", pm4, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ ]2 ooo )4")) {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the PM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [ ]2 Xoo )4", pm4, flop_amount);
    run_test_process("( [ ]2 oXo )4", pm4, flop_amount);
    run_test_process("( [ ]2 ooX )4", pm4, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ ]2 oooo )4")) {
    XBT_INFO("### Put a VM on a PM, and put four tasks to the PM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [ ]2 Xooo )4", pm4, flop_amount);
    run_test_process("( [ ]2 oXoo )4", pm4, flop_amount);
    run_test_process("( [ ]2 ooXo )4", pm4, flop_amount);
    run_test_process("( [ ]2 oooX )4", pm4, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]2 o )4")) {
    XBT_INFO("### Put a VM on a PM, and put one task to the PM and one task to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [X]2 o )4", (msg_host_t)vm0, flop_amount);
    run_test_process("( [o]2 X )4", pm4, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]2 oo )4")) {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the PM and one task to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [X]2 oo )4", (msg_host_t)vm0, flop_amount);
    run_test_process("( [o]2 Xo )4", pm4, flop_amount);
    run_test_process("( [o]2 oX )4", pm4, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]2 oo )4")) {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the PM and two tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [Xo]2 oo )4", (msg_host_t)vm0, flop_amount);
    run_test_process("( [oX]2 oo )4", (msg_host_t)vm0, flop_amount);
    run_test_process("( [oo]2 Xo )4", pm4, flop_amount);
    run_test_process("( [oo]2 oX )4", pm4, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [o]2 ooo )4")) {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and one tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [X]2 ooo )4", (msg_host_t)vm0, flop_amount);
    run_test_process("( [o]2 Xoo )4", pm4, flop_amount);
    run_test_process("( [o]2 oXo )4", pm4, flop_amount);
    run_test_process("( [o]2 ooX )4", pm4, flop_amount);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [oo]2 ooo )4")) {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and two tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [Xo]2 ooo )4", (msg_host_t)vm0, flop_amount * 4 / 5);
    run_test_process("( [oX]2 ooo )4", (msg_host_t)vm0, flop_amount * 4 / 5);
    run_test_process("( [oo]2 Xoo )4", pm4, flop_amount * 4 / 5);
    run_test_process("( [oo]2 oXo )4", pm4, flop_amount * 4 / 5);
    run_test_process("( [oo]2 ooX )4", pm4, flop_amount * 4 / 5);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else if (!strcmp(chooser, "( [ooo]2 ooo )4")) {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and three tasks to the VM");
    vm0 = MSG_vm_create_multicore(pm4, "VM0", 2);
    MSG_vm_start(vm0);
    run_test_process("( [Xoo]2 ooo )4", (msg_host_t)vm0, flop_amount * (8. / 5) * 1 / 3); // The VM has 8/5 of the PM
    run_test_process("( [oXo]2 ooo )4", (msg_host_t)vm0, flop_amount * (8. / 5) * 1 / 3);
    run_test_process("( [ooX]2 ooo )4", (msg_host_t)vm0, flop_amount * (8. / 5) * 1 / 3);

    run_test_process("( [ooo]2 Xoo )4", pm4, flop_amount * 4 / 5);
    run_test_process("( [ooo]2 oXo )4", pm4, flop_amount * 4 / 5);
    run_test_process("( [ooo]2 ooX )4", pm4, flop_amount * 4 / 5);
    MSG_process_sleep(2);
    MSG_vm_destroy(vm0);

  } else {
    xbt_die("Unknown chooser: %s", chooser);
  }
}
static int master_main(int argc, char* argv[])
{
  XBT_INFO("# TEST ON SINGLE-CORE PMs");
  XBT_INFO("## Check computation on regular PMs");
  run_test("(o)1");
  run_test("(oo)1");
  run_test("(o)1 (o)1");
  XBT_INFO("# TEST ON SINGLE-CORE PMs AND SINGLE-CORE VMs");

  XBT_INFO("## Check the impact of running tasks inside a VM (no degradation for the moment)");
  run_test("( [o]1 )1");
  run_test("( [oo]1 )1");

  XBT_INFO("## Check impact of running tasks collocated with VMs (no VM noise for the moment)");
  run_test("( [ ]1 o )1");
  run_test("( [o]1 o )1");
  run_test("( [oo]1 o )1");

  XBT_INFO("# TEST ON TWO-CORE PMs");
  XBT_INFO("## Check computation on 2 cores PMs");
  run_test("( o )2");
  run_test("( oo )2");
  run_test("( ooo )2");

  XBT_INFO("# TEST ON TWO-CORE PMs AND SINGLE-CORE VMs");
  XBT_INFO("## Check impact of a single VM (no degradation for the moment)");
  run_test("( [o]1 )2");
  run_test("( [oo]1 )2");
  run_test("( [ ]1 o )2");
  run_test("( [o]1 o )2");

  XBT_INFO("## Check impact of a several VMs (there is no degradation for the moment)");
  run_test("( [o]1 [ ]1 )2");
  run_test("( [o]1 [o]1 )2");
  run_test("( [o]1 [o]1 [ ]1 )2");
  run_test("( [o]1 [o]1 [o]1 )2");

  XBT_INFO("# TEST ON TWO-CORE PMs AND TWO-CORE VMs");

  XBT_INFO("## Check impact of a single VM (there is no degradation for the moment)");
  run_test("( [o]2 )2");
  run_test("( [oo]2 )2");
  run_test("( [ooo]2 )2");

  XBT_INFO("## Check impact of a single VM collocated with a task (there is no degradation for the moment)");
  run_test("( [ ]2 o )2");
  run_test("( [o]2 o )2");
  run_test("( [oo]2 o )2");
  run_test("( [ooo]2 o )2");
  run_test("( [ ]2 oo )2");
  run_test("( [o]2 oo )2");
  run_test("( [oo]2 oo )2");
  run_test("( [ooo]2 oo )2");

  XBT_INFO("# TEST ON FOUR-CORE PMs AND TWO-CORE VMs");
  XBT_INFO("## Check impact of a single VM");
  run_test("( [o]2 )4");
  run_test("( [oo]2 )4");
  run_test("( [ooo]2 )4");

  XBT_INFO("## Check impact of a single empty VM collocated with tasks");
  run_test("( [ ]2 o )4");
  run_test("( [ ]2 oo )4");
  run_test("( [ ]2 ooo )4");
  run_test("( [ ]2 oooo )4");

  XBT_INFO("## Check impact of a single working VM collocated with tasks");
  run_test("( [o]2 o )4");
  run_test("( [o]2 oo )4");
  run_test("( [oo]2 oo )4");
  run_test("( [o]2 ooo )4");
  run_test("( [oo]2 ooo )4");
  run_test("( [ooo]2 ooo )4");

  XBT_INFO("   ");
  XBT_INFO("   ");
  XBT_INFO("## %d test failed", failed_test);
  XBT_INFO("   ");
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

  msg_host_t pm0 = MSG_host_by_name("node-0.1core.org");
  xbt_assert(pm0, "Host 'node-0.1core.org' not found");
  MSG_process_create("master", master_main, NULL, pm0);

  return MSG_main() != MSG_OK || failed_test;
}
