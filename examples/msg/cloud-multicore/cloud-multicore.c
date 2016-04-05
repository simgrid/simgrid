/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int worker_main(int argc, char *argv[])
{
  msg_task_t task = MSG_process_get_data(MSG_process_self());
  MSG_task_execute(task);

  XBT_INFO("task %p bye", task);

  return 0;
}

struct task_data {
  msg_task_t task;
  double prev_computation_amount;
  double prev_clock;
};

static void task_data_init_clock(struct task_data *t)
{
  t->prev_computation_amount = MSG_task_get_flops_amount(t->task);
  t->prev_clock = MSG_get_clock();
}

static void task_data_get_clock(struct task_data *t)
{
  double now_computation_amount = MSG_task_get_flops_amount(t->task);
  double now_clock = MSG_get_clock();

  double done = t->prev_computation_amount - now_computation_amount;
  double duration = now_clock - t->prev_clock;

  XBT_INFO("%s: %f fops/s", MSG_task_get_name(t->task), done / duration);

  t->prev_computation_amount = now_computation_amount;
  t->prev_clock = now_clock;
}

static void test_pm_pin(void)
{
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);
  msg_host_t pm1 = xbt_dynar_get_as(hosts_dynar, 1, msg_host_t);
  msg_host_t pm2 = xbt_dynar_get_as(hosts_dynar, 2, msg_host_t);

  struct task_data t1;
  struct task_data t2;
  struct task_data t3;
  struct task_data t4;

  t1.task = MSG_task_create("Task1", 1e16, 0, NULL);
  t2.task = MSG_task_create("Task2", 1e16, 0, NULL);
  t3.task = MSG_task_create("Task3", 1e16, 0, NULL);
  t4.task = MSG_task_create("Task4", 1e16, 0, NULL);

  MSG_process_create("worker1", worker_main, t1.task, pm1);
  MSG_process_create("worker2", worker_main, t2.task, pm1);
  MSG_process_create("worker3", worker_main, t3.task, pm1);
  MSG_process_create("worker4", worker_main, t4.task, pm1);

  XBT_INFO("## 1. start 4 tasks on PM1 (2 cores)");
  task_data_init_clock(&t1);
  task_data_init_clock(&t2);
  task_data_init_clock(&t3);
  task_data_init_clock(&t4);

  MSG_process_sleep(10);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);
  task_data_get_clock(&t4);

  XBT_INFO("## 2. pin all tasks to CPU0");
  MSG_task_set_affinity(t1.task, pm1, 0x01);
  MSG_task_set_affinity(t2.task, pm1, 0x01);
  MSG_task_set_affinity(t3.task, pm1, 0x01);
  MSG_task_set_affinity(t4.task, pm1, 0x01);

  MSG_process_sleep(10);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);
  task_data_get_clock(&t4);

  XBT_INFO("## 3. clear the affinity of task4");
  MSG_task_set_affinity(t4.task, pm1, 0);

  MSG_process_sleep(10);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);
  task_data_get_clock(&t4);

  XBT_INFO("## 4. clear the affinity of task3");
  MSG_task_set_affinity(t3.task, pm1, 0);

  MSG_process_sleep(10);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);
  task_data_get_clock(&t4);

  XBT_INFO("## 5. clear the affinity of task2");
  MSG_task_set_affinity(t2.task, pm1, 0);

  MSG_process_sleep(10);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);
  task_data_get_clock(&t4);

  XBT_INFO("## 6. pin all tasks to CPU0 of another PM (no effect now)");
  MSG_task_set_affinity(t1.task, pm0, 0);
  MSG_task_set_affinity(t2.task, pm0, 0);
  MSG_task_set_affinity(t3.task, pm2, 0);
  MSG_task_set_affinity(t4.task, pm2, 0);

  MSG_process_sleep(10);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);
  task_data_get_clock(&t4);

  MSG_task_cancel(t1.task);
  MSG_task_cancel(t2.task);
  MSG_task_cancel(t3.task);
  MSG_task_cancel(t4.task);
  MSG_process_sleep(10);
  MSG_task_destroy(t1.task);
  MSG_task_destroy(t2.task);
  MSG_task_destroy(t3.task);
  MSG_task_destroy(t4.task);
}

static void test_vm_pin(void)
{
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t); // 1 cores
  msg_host_t pm1 = xbt_dynar_get_as(hosts_dynar, 1, msg_host_t); // 2 cores
  msg_host_t pm2 = xbt_dynar_get_as(hosts_dynar, 2, msg_host_t); // 4 cores

  /* set up VMs on PM2 (4 cores) */
  msg_vm_t vm0 = MSG_vm_create_core(pm2, "VM0");
  msg_vm_t vm1 = MSG_vm_create_core(pm2, "VM1");
  msg_vm_t vm2 = MSG_vm_create_core(pm2, "VM2");
  msg_vm_t vm3 = MSG_vm_create_core(pm2, "VM3");

  s_vm_params_t params;
  memset(&params, 0, sizeof(params));
  params.ramsize = 1L * 1024 * 1024;
  params.skip_stage1 = 1;
  params.skip_stage2 = 1;
  //params.mig_speed = 1L * 1024 * 1024;
  MSG_host_set_params(vm0, &params);
  MSG_host_set_params(vm1, &params);
  MSG_host_set_params(vm2, &params);
  MSG_host_set_params(vm3, &params);

  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  MSG_vm_start(vm2);
  MSG_vm_start(vm3);

  /* set up tasks and processes */
  struct task_data t0;
  struct task_data t1;
  struct task_data t2;
  struct task_data t3;

  t0.task = MSG_task_create("Task0", 1e16, 0, NULL);
  t1.task = MSG_task_create("Task1", 1e16, 0, NULL);
  t2.task = MSG_task_create("Task2", 1e16, 0, NULL);
  t3.task = MSG_task_create("Task3", 1e16, 0, NULL);

  MSG_process_create("worker0", worker_main, t0.task, vm0);
  MSG_process_create("worker1", worker_main, t1.task, vm1);
  MSG_process_create("worker2", worker_main, t2.task, vm2);
  MSG_process_create("worker3", worker_main, t3.task, vm3);

  /* start experiments */
  XBT_INFO("## 1. start 4 VMs on PM2 (4 cores)");
  task_data_init_clock(&t0);
  task_data_init_clock(&t1);
  task_data_init_clock(&t2);
  task_data_init_clock(&t3);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  XBT_INFO("## 2. pin all VMs to CPU0 of PM2");
  MSG_vm_set_affinity(vm0, pm2, 0x01);
  MSG_vm_set_affinity(vm1, pm2, 0x01);
  MSG_vm_set_affinity(vm2, pm2, 0x01);
  MSG_vm_set_affinity(vm3, pm2, 0x01);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  XBT_INFO("## 3. pin all VMs to CPU0 of PM1 (no effect at now)");
  /* Because VMs are on PM2, the below operations do not effect computation now. */
  MSG_vm_set_affinity(vm0, pm1, 0x01);
  MSG_vm_set_affinity(vm1, pm1, 0x01);
  MSG_vm_set_affinity(vm2, pm1, 0x01);
  MSG_vm_set_affinity(vm3, pm1, 0x01);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  XBT_INFO("## 4. unpin VM0, and pin VM2 and VM3 to CPU1 of PM2");
  MSG_vm_set_affinity(vm0, pm2, 0x00);
  MSG_vm_set_affinity(vm2, pm2, 0x02);
  MSG_vm_set_affinity(vm3, pm2, 0x02);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  XBT_INFO("## 5. migrate all VMs to PM0 (only 1 CPU core)");
  MSG_vm_migrate(vm0, pm0);
  MSG_vm_migrate(vm1, pm0);
  MSG_vm_migrate(vm2, pm0);
  MSG_vm_migrate(vm3, pm0);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  XBT_INFO("## 6. migrate all VMs to PM1 (2 CPU cores, with affinity settings)");
  MSG_vm_migrate(vm0, pm1);
  MSG_vm_migrate(vm1, pm1);
  MSG_vm_migrate(vm2, pm1);
  MSG_vm_migrate(vm3, pm1);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);


  XBT_INFO("## 7. clear affinity settings on PM1");
  MSG_vm_set_affinity(vm0, pm1, 0);
  MSG_vm_set_affinity(vm1, pm1, 0);
  MSG_vm_set_affinity(vm2, pm1, 0);
  MSG_vm_set_affinity(vm3, pm1, 0);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  MSG_process_sleep(10);
  task_data_get_clock(&t0);
  task_data_get_clock(&t1);
  task_data_get_clock(&t2);
  task_data_get_clock(&t3);

  /* clean up everything */
  MSG_task_cancel(t0.task);
  MSG_task_cancel(t1.task);
  MSG_task_cancel(t2.task);
  MSG_task_cancel(t3.task);
  MSG_process_sleep(10);
  MSG_task_destroy(t0.task);
  MSG_task_destroy(t1.task);
  MSG_task_destroy(t2.task);
  MSG_task_destroy(t3.task);

  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  MSG_vm_destroy(vm2);
  MSG_vm_destroy(vm3);
}

static int master_main(int argc, char *argv[])
{
  XBT_INFO("=== Test PM (set affinity) ===");
  test_pm_pin();

  XBT_INFO("=== Test VM (set affinity) ===");
  test_vm_pin();

  return 0;
}

int main(int argc, char *argv[])
{
  /* Get the arguments */
  MSG_init(&argc, argv);

  /* load the platform file */
  if (argc != 2) {
    printf("Usage: %s examples/msg/cloud/multicore_plat.xml\n", argv[0]);
    return 1;
  }

  MSG_create_environment(argv[1]);

  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);
  msg_host_t pm1 = xbt_dynar_get_as(hosts_dynar, 1, msg_host_t);
  msg_host_t pm2 = xbt_dynar_get_as(hosts_dynar, 2, msg_host_t);

  XBT_INFO("%s: %d core(s), %f flops/s per each", MSG_host_get_name(pm0), MSG_host_get_core_number(pm0),
           MSG_host_get_speed(pm0));
  XBT_INFO("%s: %d core(s), %f flops/s per each", MSG_host_get_name(pm1), MSG_host_get_core_number(pm1),
           MSG_host_get_speed(pm1));
  XBT_INFO("%s: %d core(s), %f flops/s per each", MSG_host_get_name(pm2), MSG_host_get_core_number(pm2),
           MSG_host_get_speed(pm2));

  MSG_process_create("master", master_main, NULL, pm0);

  int res = MSG_main();
  XBT_INFO("Bye (simulation time %g)", MSG_get_clock());

  return !(res == MSG_OK);
}
