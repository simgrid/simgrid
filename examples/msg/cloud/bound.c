/* Copyright (c) 2007-2013. The SimGrid Team. All rights reserved. */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** @addtogroup MSG_examples
 *
 * - <b>priority/priority.c</b>: Demonstrates the use of @ref
 *   MSG_task_set_bound to change the computation priority of a
 *   given task.
 *
 */

static int worker_main(int argc, char *argv[])
{
  double computation_amount = atof(argv[1]);
  int use_bound = atoi(argv[2]);
  double bound = atof(argv[3]);

  {
    double clock_sta = MSG_get_clock();

    msg_task_t task = MSG_task_create("Task", computation_amount, 0, NULL);
    if (use_bound)
      MSG_task_set_bound(task, bound);
    MSG_task_execute(task);
    MSG_task_destroy(task);

    double clock_end = MSG_get_clock();
    double duration = clock_end - clock_sta;
    double flops_per_sec = computation_amount / duration;

    if (use_bound)
      XBT_INFO("bound to %f => duration %f (%f flops/s)", bound, duration, flops_per_sec);
    else
      XBT_INFO("not bound => duration %f (%f flops/s)", duration, flops_per_sec);
  }

  return 0;
}

static void launch_worker(msg_host_t host, const char *pr_name, double computation_amount, int use_bound, double bound)
{
  char **argv = xbt_new(char *, 5);
  argv[0] = xbt_strdup(pr_name);
  argv[1] = bprintf("%lf", computation_amount);
  argv[2] = bprintf("%d", use_bound);
  argv[3] = bprintf("%lf", bound);
  argv[4] = NULL;

  MSG_process_create_with_arguments(pr_name, worker_main, NULL, host, 4, argv);
}



static int worker_busy_loop_main(int argc, char *argv[])
{
  msg_task_t *task = MSG_process_get_data(MSG_process_self());
  for (;;)
    MSG_task_execute(*task);

  return 0;
}

#define DOUBLE_MAX 100000000000L

static void test_dynamic_change(void)
{
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);

  msg_host_t vm0 = MSG_vm_create_core(pm0, "VM0");
  msg_host_t vm1 = MSG_vm_create_core(pm0, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);

  msg_task_t task0 = MSG_task_create("Task0", DOUBLE_MAX, 0, NULL);
  msg_task_t task1 = MSG_task_create("Task1", DOUBLE_MAX, 0, NULL);
  msg_process_t pr0 = MSG_process_create("worker0", worker_busy_loop_main, &task0, vm0);
  msg_process_t pr1 = MSG_process_create("worker1", worker_busy_loop_main, &task1, vm1);


  double task0_remain_prev = MSG_task_get_remaining_computation(task0);
  double task1_remain_prev = MSG_task_get_remaining_computation(task1);

  {
    const double cpu_speed = MSG_get_host_speed(pm0);
    int i = 0;
    for (i = 0; i < 10; i++) {
      double new_bound = (cpu_speed / 10) * i;
      XBT_INFO("set bound of VM1 to %f", new_bound);
      MSG_vm_set_bound(vm1, new_bound);
      MSG_process_sleep(100);

      double task0_remain_now = MSG_task_get_remaining_computation(task0);
      double task1_remain_now = MSG_task_get_remaining_computation(task1);

      double task0_flops_per_sec = task0_remain_prev - task0_remain_now;
      double task1_flops_per_sec = task1_remain_prev - task1_remain_now;

      XBT_INFO("VM0: %f flops/s", task0_flops_per_sec / 100);
      XBT_INFO("VM1: %f flops/s", task1_flops_per_sec / 100);

      task0_remain_prev = task0_remain_now;
      task1_remain_prev = task1_remain_now;
    }
  }

  MSG_process_kill(pr0);
  MSG_process_kill(pr1);
  
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
}



static void test_one_task(msg_host_t hostA)
{
  const double cpu_speed = MSG_get_host_speed(hostA);
  const double computation_amount = cpu_speed * 10;
  const char *hostA_name = MSG_host_get_name(hostA);

  XBT_INFO("### Test: with/without MSG_task_set_bound");

#if 0
  /* Easy-to-understand code (without calling MSG_task_set_bound) */
  {
    double clock_sta = MSG_get_clock();

    msg_task_t task = MSG_task_create("Task", computation_amount, 0, NULL);
    MSG_task_execute(task);
    MSG_task_destroy(task);

    double clock_end = MSG_get_clock();
    double duration = clock_end - clock_sta;
    double flops_per_sec = computation_amount / duration;

    XBT_INFO("not bound => duration %f (%f flops/s)", duration, flops_per_sec);
  }

  /* Easy-to-understand code (with calling MSG_task_set_bound) */
  {
    double clock_sta = MSG_get_clock();

    msg_task_t task = MSG_task_create("Task", computation_amount, 0, NULL);
    MSG_task_set_bound(task, cpu_speed / 2);
    MSG_task_execute(task);
    MSG_task_destroy(task);

    double clock_end = MSG_get_clock();
    double duration = clock_end - clock_sta;
    double flops_per_sec = computation_amount / duration;

    XBT_INFO("bound to 0.5 => duration %f (%f flops/s)", duration, flops_per_sec);
  }
#endif

  {
    XBT_INFO("### Test: no bound for Task1@%s", hostA_name);
    launch_worker(hostA, "worker0", computation_amount, 0, 0);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: 50%% for Task1@%s", hostA_name);
    launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed / 2);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: 33%% for Task1@%s", hostA_name);
    launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed / 3);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: zero for Task1@%s (i.e., unlimited)", hostA_name);
    launch_worker(hostA, "worker0", computation_amount, 1, 0);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: 200%% for Task1@%s (i.e., meaningless)", hostA_name);
    launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed * 2);
  }

  MSG_process_sleep(1000);
}


static void test_two_tasks(msg_host_t hostA, msg_host_t hostB)
{
  const double cpu_speed = MSG_get_host_speed(hostA);
  xbt_assert(cpu_speed == MSG_get_host_speed(hostB));
  const double computation_amount = cpu_speed * 10;
  const char *hostA_name = MSG_host_get_name(hostA);
  const char *hostB_name = MSG_host_get_name(hostB);

  {
    XBT_INFO("### Test: no bound for Task1@%s, no bound for Task2@%s", hostA_name, hostB_name);
    launch_worker(hostA, "worker0", computation_amount, 0, 0);
    launch_worker(hostB, "worker1", computation_amount, 0, 0);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: 0 for Task1@%s, 0 for Task2@%s (i.e., unlimited)", hostA_name, hostB_name);
    launch_worker(hostA, "worker0", computation_amount, 1, 0);
    launch_worker(hostB, "worker1", computation_amount, 1, 0);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: 50%% for Task1@%s, 50%% for Task2@%s", hostA_name, hostB_name);
    launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed / 2);
    launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed / 2);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: 25%% for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
    launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed / 4);
    launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed / 4);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: 75%% for Task1@%s, 100%% for Task2@%s", hostA_name, hostB_name);
    launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed * 0.75);
    launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: no bound for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
    launch_worker(hostA, "worker0", computation_amount, 0, 0);
    launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed / 4);
  }

  MSG_process_sleep(1000);

  {
    XBT_INFO("### Test: 75%% for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
    launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed * 0.75);
    launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed / 4);
  }

  MSG_process_sleep(1000);
}

static int master_main(int argc, char *argv[])
{
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);


  {
    XBT_INFO("# 1. Put a single task on a PM. ");
    test_one_task(pm0);
    XBT_INFO(" ");


    XBT_INFO("# 2. Put two tasks on a PM.");
    test_two_tasks(pm0, pm0);
    XBT_INFO(" ");
  }


  {
    msg_host_t vm0 = MSG_vm_create_core(pm0, "VM0");
    MSG_vm_start(vm0);

    XBT_INFO("# 3. Put a single task on a VM. ");
    test_one_task(vm0);
    XBT_INFO(" ");

    XBT_INFO("# 4. Put two tasks on a VM.");
    test_two_tasks(vm0, vm0);
    XBT_INFO(" ");


    MSG_vm_destroy(vm0);
  }


  {
    msg_host_t vm0 = MSG_vm_create_core(pm0, "VM0");
    MSG_vm_start(vm0);

    XBT_INFO("# 6. Put a task on a PM and a task on a VM.");
    test_two_tasks(pm0, vm0);
    XBT_INFO(" ");


    MSG_vm_destroy(vm0);
  }


  {
    msg_host_t vm0 = MSG_vm_create_core(pm0, "VM0");
    const double cpu_speed = MSG_get_host_speed(pm0);
    MSG_vm_set_bound(vm0, cpu_speed / 10);
    MSG_vm_start(vm0);

    XBT_INFO("# 7. Put a single task on the VM capped by 10%%.");
    test_one_task(vm0);
    XBT_INFO(" ");

    XBT_INFO("# 8. Put two tasks on the VM capped by 10%%.");
    test_two_tasks(vm0, vm0);
    XBT_INFO(" ");

    XBT_INFO("# 9. Put a task on a PM and a task on the VM capped by 10%%.");
    test_two_tasks(pm0, vm0);
    XBT_INFO(" ");

    MSG_vm_destroy(vm0);
  }


  XBT_INFO("# 10. Change a bound dynamically");
  test_dynamic_change();

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
