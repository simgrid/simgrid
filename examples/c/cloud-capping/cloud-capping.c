/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/exec.h"
#include "simgrid/host.h"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/vm.h"

#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/str.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_capping, "Messages specific for this example");

static void worker_main(int argc, char* argv[])
{
  xbt_assert(argc == 4);
  double computation_amount = xbt_str_parse_double(argv[1], "Invalid computation amount: %s");
  int use_bound             = !!xbt_str_parse_int(argv[2], "Second parameter (use_bound) should be 0 or 1 but is: %s");
  double bound              = xbt_str_parse_double(argv[3], "Invalid bound: %s");

  double clock_sta = simgrid_get_clock();

  sg_exec_t exec = sg_actor_exec_init(computation_amount);
  if (use_bound) {
    if (bound < 1e-12) /* close enough to 0 without any floating precision surprise */
      XBT_INFO("bound == 0 means no capping (i.e., unlimited).");
    sg_exec_set_bound(exec, bound);
  }
  sg_exec_wait(exec);

  double clock_end     = simgrid_get_clock();
  double duration      = clock_end - clock_sta;
  double flops_per_sec = computation_amount / duration;

  if (use_bound)
    XBT_INFO("bound to %f => duration %f (%f flops/s)", bound, duration, flops_per_sec);
  else
    XBT_INFO("not bound => duration %f (%f flops/s)", duration, flops_per_sec);
}

static void launch_worker(sg_host_t host, const char* pr_name, double computation_amount, int use_bound, double bound)
{
  char* argv1        = bprintf("%f", computation_amount);
  char* argv2        = bprintf("%d", use_bound);
  char* argv3        = bprintf("%f", bound);
  const char* argv[] = {pr_name, argv1, argv2, argv3, NULL};

  sg_actor_create_(pr_name, host, worker_main, 4, argv);

  free(argv1);
  free(argv2);
  free(argv3);
}

static void worker_busy_loop(int argc, char* argv[])
{
  xbt_assert(argc > 2);
  char* name              = argv[1];
  double speed            = xbt_str_parse_double(argv[2], "Invalid speed value");
  double exec_remain_prev = 1e11;

  sg_exec_t exec = sg_actor_exec_async(exec_remain_prev);
  for (int i = 0; i < 10; i++) {
    if (speed > 0) {
      double new_bound = (speed / 10) * i;
      XBT_INFO("set bound of VM1 to %f", new_bound);
      sg_vm_set_bound(((sg_vm_t)sg_actor_get_host(sg_actor_self())), new_bound);
    }
    sg_actor_sleep_for(100);
    double exec_remain_now = sg_exec_get_remaining(exec);
    double flops_per_sec   = exec_remain_prev - exec_remain_now;
    XBT_INFO("%s@%s: %.0f flops/s", name, sg_host_get_name(sg_actor_get_host(sg_actor_self())), flops_per_sec / 100);
    exec_remain_prev = exec_remain_now;
    sg_actor_sleep_for(1);
  }

  sg_exec_wait(exec);
}

static void test_dynamic_change()
{
  sg_host_t pm0 = sg_host_by_name("Fafard");

  sg_vm_t vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_t vm1 = sg_vm_create_core(pm0, "VM1");
  sg_vm_start(vm0);
  sg_vm_start(vm1);

  const char* w0_argv[] = {"worker0", "Task0", "-1.0", NULL};
  sg_actor_create_("worker0", (sg_host_t)vm0, worker_busy_loop, 3, w0_argv);

  char* speed           = bprintf("%f", sg_host_get_speed(pm0));
  const char* w1_argv[] = {"worker1", "Task1", speed, NULL};
  sg_actor_create_("worker1", (sg_host_t)vm1, worker_busy_loop, 3, w1_argv);

  sg_actor_sleep_for(3000); // let the tasks end

  sg_vm_destroy(vm0);
  sg_vm_destroy(vm1);
  free(speed);
}

static void test_one_task(sg_host_t hostA)
{
  const double cpu_speed          = sg_host_get_speed(hostA);
  const double computation_amount = cpu_speed * 10;
  const char* hostA_name          = sg_host_get_name(hostA);

  XBT_INFO("### Test: with/without sg_exec_set_bound");

  XBT_INFO("### Test: no bound for Task1@%s", hostA_name);
  launch_worker(hostA, "worker0", computation_amount, 0, 0);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: 50%% for Task1@%s", hostA_name);
  launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed / 2);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: 33%% for Task1@%s", hostA_name);
  launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed / 3);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: zero for Task1@%s (i.e., unlimited)", hostA_name);
  launch_worker(hostA, "worker0", computation_amount, 1, 0);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: 200%% for Task1@%s (i.e., meaningless)", hostA_name);
  launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed * 2);

  sg_actor_sleep_for(1000);
}

static void test_two_tasks(sg_host_t hostA, sg_host_t hostB)
{
  const double cpu_speed = sg_host_get_speed(hostA);
  xbt_assert(cpu_speed == sg_host_get_speed(hostB));
  const double computation_amount = cpu_speed * 10;
  const char* hostA_name          = sg_host_get_name(hostA);
  const char* hostB_name          = sg_host_get_name(hostB);

  XBT_INFO("### Test: no bound for Task1@%s, no bound for Task2@%s", hostA_name, hostB_name);
  launch_worker(hostA, "worker0", computation_amount, 0, 0);
  launch_worker(hostB, "worker1", computation_amount, 0, 0);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: 0 for Task1@%s, 0 for Task2@%s (i.e., unlimited)", hostA_name, hostB_name);
  launch_worker(hostA, "worker0", computation_amount, 1, 0);
  launch_worker(hostB, "worker1", computation_amount, 1, 0);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: 50%% for Task1@%s, 50%% for Task2@%s", hostA_name, hostB_name);
  launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed / 2);
  launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed / 2);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: 25%% for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
  launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed / 4);
  launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed / 4);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: 75%% for Task1@%s, 100%% for Task2@%s", hostA_name, hostB_name);
  launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed * 0.75);
  launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: no bound for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
  launch_worker(hostA, "worker0", computation_amount, 0, 0);
  launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed / 4);

  sg_actor_sleep_for(1000);

  XBT_INFO("### Test: 75%% for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
  launch_worker(hostA, "worker0", computation_amount, 1, cpu_speed * 0.75);
  launch_worker(hostB, "worker1", computation_amount, 1, cpu_speed / 4);

  sg_actor_sleep_for(1000);
}

static void master_main(int argc, char* argv[])
{
  sg_host_t pm0 = sg_host_by_name("Fafard");
  sg_host_t pm1 = sg_host_by_name("Fafard");

  XBT_INFO("# 1. Put a single task on a PM. ");
  test_one_task(pm0);
  XBT_INFO(" ");

  XBT_INFO("# 2. Put two tasks on a PM.");
  test_two_tasks(pm0, pm0);
  XBT_INFO(" ");

  sg_vm_t vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_start(vm0);

  XBT_INFO("# 3. Put a single task on a VM. ");
  test_one_task((sg_host_t)vm0);
  XBT_INFO(" ");

  XBT_INFO("# 4. Put two tasks on a VM.");
  test_two_tasks((sg_host_t)vm0, (sg_host_t)vm0);
  XBT_INFO(" ");

  sg_vm_destroy(vm0);

  vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_start(vm0);

  XBT_INFO("# 6. Put a task on a PM and a task on a VM.");
  test_two_tasks(pm0, (sg_host_t)vm0);
  XBT_INFO(" ");

  sg_vm_destroy(vm0);

  vm0              = sg_vm_create_core(pm0, "VM0");
  double cpu_speed = sg_host_get_speed(pm0);
  sg_vm_set_bound(vm0, cpu_speed / 10);
  sg_vm_start(vm0);

  XBT_INFO("# 7. Put a single task on the VM capped by 10%%.");
  test_one_task((sg_host_t)vm0);
  XBT_INFO(" ");

  XBT_INFO("# 8. Put two tasks on the VM capped by 10%%.");
  test_two_tasks((sg_host_t)vm0, (sg_host_t)vm0);
  XBT_INFO(" ");

  XBT_INFO("# 9. Put a task on a PM and a task on the VM capped by 10%%.");
  test_two_tasks(pm0, (sg_host_t)vm0);
  XBT_INFO(" ");

  sg_vm_destroy(vm0);

  vm0 = sg_vm_create_core(pm0, "VM0");

  sg_vm_set_ramsize(vm0, 1e9); // 1GB
  sg_vm_start(vm0);

  cpu_speed = sg_host_get_speed(pm0);
  sg_vm_start(vm0);

  XBT_INFO("# 10. Test migration");
  const double computation_amount = cpu_speed * 10;

  XBT_INFO("# 10. (a) Put a task on a VM without any bound.");
  launch_worker((sg_host_t)vm0, "worker0", computation_amount, 0, 0);
  sg_actor_sleep_for(1000);
  XBT_INFO(" ");

  XBT_INFO("# 10. (b) set 10%% bound to the VM, and then put a task on the VM.");
  sg_vm_set_bound(vm0, cpu_speed / 10);
  launch_worker((sg_host_t)vm0, "worker0", computation_amount, 0, 0);
  sg_actor_sleep_for(1000);
  XBT_INFO(" ");

  XBT_INFO("# 10. (c) migrate");
  sg_vm_migrate(vm0, pm1);
  XBT_INFO(" ");

  XBT_INFO("# 10. (d) Put a task again on the VM.");
  launch_worker((sg_host_t)vm0, "worker0", computation_amount, 0, 0);
  sg_actor_sleep_for(1000);
  XBT_INFO(" ");

  sg_vm_destroy(vm0);

  XBT_INFO("# 11. Change a bound dynamically.");
  test_dynamic_change();
}

int main(int argc, char* argv[])
{
  /* Get the arguments */
  simgrid_init(&argc, argv);
  sg_vm_live_migration_plugin_init();

  /* load the platform file */
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  sg_actor_create("master_", sg_host_by_name("Fafard"), master_main, 0, NULL);

  simgrid_run();
  XBT_INFO("Bye (simulation time %g)", simgrid_get_clock());

  return 0;
}
