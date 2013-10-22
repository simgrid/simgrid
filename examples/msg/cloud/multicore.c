/* Copyright (c) 2007-2013. The SimGrid Team. All rights reserved. */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");


struct worker_data {
	double computation_amount;
};


static int worker_main(int argc, char *argv[])
{
  struct worker_data *params = MSG_process_get_data(MSG_process_self());
  double computation_amount = params->computation_amount;

  {
    double clock_sta = MSG_get_clock();

    msg_task_t task = MSG_task_create("Task", computation_amount, 0, NULL);
    MSG_task_execute(task);
    MSG_task_destroy(task);

    double clock_end = MSG_get_clock();

    double duration = clock_end - clock_sta;
    double flops_per_sec = computation_amount / duration;

    XBT_INFO("%s: amount %f duration %f (%f flops/s)",
		    MSG_host_get_name(MSG_host_self()), computation_amount, duration, flops_per_sec);
  }



  xbt_free(params);

  return 0;
}




static void test_one_task(msg_host_t hostA, double computation)
{

  struct worker_data *params = xbt_new(struct worker_data, 1);
  params->computation_amount = computation;

  MSG_process_create("worker", worker_main, params, hostA);

  //xbt_free(params);
}

#if 0
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
#endif

static void test_pm(void)
{
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);
  msg_host_t pm1 = xbt_dynar_get_as(hosts_dynar, 1, msg_host_t);
  msg_host_t pm2 = xbt_dynar_get_as(hosts_dynar, 2, msg_host_t);

  const double cpu_speed = MSG_get_host_speed(pm0);
  const double computation_amount = cpu_speed * 10;

  {
    XBT_INFO("# 1. Put a single task on each PM. ");
    test_one_task(pm0, computation_amount);
    MSG_process_sleep(100);
    test_one_task(pm1, computation_amount);
    MSG_process_sleep(100);
    test_one_task(pm2, computation_amount);
  }

  MSG_process_sleep(100);

  {
    XBT_INFO("# 2. Put 2 tasks on each PM. ");
    test_one_task(pm0, computation_amount);
    test_one_task(pm0, computation_amount);
    MSG_process_sleep(100);

    test_one_task(pm1, computation_amount);
    test_one_task(pm1, computation_amount);
    MSG_process_sleep(100);

    test_one_task(pm2, computation_amount);
    test_one_task(pm2, computation_amount);
  }

  MSG_process_sleep(100);

  {
    XBT_INFO("# 3. Put 4 tasks on each PM. ");
    test_one_task(pm0, computation_amount);
    test_one_task(pm0, computation_amount);
    test_one_task(pm0, computation_amount);
    test_one_task(pm0, computation_amount);
    MSG_process_sleep(100);

    test_one_task(pm1, computation_amount);
    test_one_task(pm1, computation_amount);
    test_one_task(pm1, computation_amount);
    test_one_task(pm1, computation_amount);
    MSG_process_sleep(100);

    test_one_task(pm2, computation_amount);
    test_one_task(pm2, computation_amount);
    test_one_task(pm2, computation_amount);
    test_one_task(pm2, computation_amount);
  }

  MSG_process_sleep(100);
}


static void test_vm(void)
{
  xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
  msg_host_t pm0 = xbt_dynar_get_as(hosts_dynar, 0, msg_host_t);
  msg_host_t pm1 = xbt_dynar_get_as(hosts_dynar, 1, msg_host_t);
  msg_host_t pm2 = xbt_dynar_get_as(hosts_dynar, 2, msg_host_t);


  const double cpu_speed = MSG_get_host_speed(pm0);
  const double computation_amount = cpu_speed * 10;


  {
    msg_host_t vm0 = MSG_vm_create_core(pm0, "vm0");
    msg_host_t vm1 = MSG_vm_create_core(pm1, "vm1");
    msg_host_t vm2 = MSG_vm_create_core(pm2, "vm2");

    XBT_INFO("# 1. Put a single task on each VM.");
    test_one_task(vm0, computation_amount);
    MSG_process_sleep(100);

    test_one_task(vm1, computation_amount);
    MSG_process_sleep(100);

    test_one_task(vm2, computation_amount);
    MSG_process_sleep(100);

    MSG_vm_destroy(vm0);
    MSG_vm_destroy(vm1);
    MSG_vm_destroy(vm2);
  }


  {
    msg_host_t vm0 = MSG_vm_create_core(pm0, "vm0");
    msg_host_t vm1 = MSG_vm_create_core(pm1, "vm1");
    msg_host_t vm2 = MSG_vm_create_core(pm2, "vm2");

    XBT_INFO("# 2. Put 2 tasks on each VM.");
    test_one_task(vm0, computation_amount);
    test_one_task(vm0, computation_amount);
    MSG_process_sleep(100);

    test_one_task(vm1, computation_amount);
    test_one_task(vm1, computation_amount);
    MSG_process_sleep(100);

    test_one_task(vm2, computation_amount);
    test_one_task(vm2, computation_amount);
    MSG_process_sleep(100);

    MSG_vm_destroy(vm0);
    MSG_vm_destroy(vm1);
    MSG_vm_destroy(vm2);
  }


  {
    msg_host_t vm0 = MSG_vm_create_core(pm0, "vm0");
    msg_host_t vm1 = MSG_vm_create_core(pm1, "vm1");
    msg_host_t vm2 = MSG_vm_create_core(pm2, "vm2");

    XBT_INFO("# 3. Put a task on each VM, and put a task on its PM.");
    test_one_task(vm0, computation_amount);
    test_one_task(pm0, computation_amount);
    MSG_process_sleep(100);

    test_one_task(vm1, computation_amount);
    test_one_task(pm1, computation_amount);
    MSG_process_sleep(100);

    test_one_task(vm2, computation_amount);
    test_one_task(pm2, computation_amount);
    MSG_process_sleep(100);

    MSG_vm_destroy(vm0);
    MSG_vm_destroy(vm1);
    MSG_vm_destroy(vm2);
  }


  {
    {
       /* 1-core PM */
       XBT_INFO("# 4. Put 2 VMs on a 1-core PM.");
       msg_host_t vm0 = MSG_vm_create_core(pm0, "vm0");
       msg_host_t vm1 = MSG_vm_create_core(pm0, "vm1");
      
       test_one_task(vm0, computation_amount);
       test_one_task(vm1, computation_amount);
       MSG_process_sleep(100);
      
       MSG_vm_destroy(vm0);
       MSG_vm_destroy(vm1);
    }

    {
       /* 2-core PM */
       XBT_INFO("# 5. Put 2 VMs on a 2-core PM.");
       msg_host_t vm0 = MSG_vm_create_core(pm1, "vm0");
       msg_host_t vm1 = MSG_vm_create_core(pm1, "vm1");

       test_one_task(vm0, computation_amount);
       test_one_task(vm1, computation_amount);
       MSG_process_sleep(100);

       MSG_vm_destroy(vm0);
       MSG_vm_destroy(vm1);
    }

    {
       /* 2-core PM */
       XBT_INFO("# 6. Put 2 VMs on a 2-core PM and 1 task on the PM.");
       msg_host_t vm0 = MSG_vm_create_core(pm1, "vm0");
       msg_host_t vm1 = MSG_vm_create_core(pm1, "vm1");

       test_one_task(vm0, computation_amount);
       test_one_task(vm1, computation_amount);
       test_one_task(pm1, computation_amount);
       MSG_process_sleep(100);

       MSG_vm_destroy(vm0);
       MSG_vm_destroy(vm1);
    }

    {
       /* 2-core PM */
       XBT_INFO("# 7. Put 2 VMs and 2 tasks on a 2-core PM. Put two tasks on one of the VMs.");
       msg_host_t vm0 = MSG_vm_create_core(pm1, "vm0");
       msg_host_t vm1 = MSG_vm_create_core(pm1, "vm1");
       test_one_task(pm1, computation_amount);
       test_one_task(pm1, computation_amount);

       /* Reduce computation_amount to make all tasks finish at the same time. Simplify results. */
       test_one_task(vm0, computation_amount / 2);
       test_one_task(vm0, computation_amount / 2);
       test_one_task(vm1, computation_amount);
       MSG_process_sleep(100);

       MSG_vm_destroy(vm0);
       MSG_vm_destroy(vm1);
    }

    {
       /* 2-core PM */
       XBT_INFO("# 8. Put 2 VMs and a task on a 2-core PM. Cap the load of VM1 at 50%%.");
       /* This is a tricky case. The process schedular of the host OS may not work as expected. */
	
       /* VM0 gets 50%. VM1 and VM2 get 75%, respectively. */
       msg_host_t vm0 = MSG_vm_create_core(pm1, "vm0");
       MSG_vm_set_bound(vm0, cpu_speed / 2);
       msg_host_t vm1 = MSG_vm_create_core(pm1, "vm1");
       test_one_task(pm1, computation_amount);

       test_one_task(vm0, computation_amount);
       test_one_task(vm1, computation_amount);

       MSG_process_sleep(100);

       MSG_vm_destroy(vm0);
       MSG_vm_destroy(vm1);
    }


    /* In all the above cases, tasks finish at the same time.
     * TODO: more complex cases must be done.
     **/

#if 0
    {
       /* 2-core PM */
       XBT_INFO("# 8. Put 2 VMs and a task on a 2-core PM. Put two tasks on one of the VMs.");
       msg_host_t vm0 = MSG_vm_create_core(pm1, "vm0");
       msg_host_t vm1 = MSG_vm_create_core(pm1, "vm1");

       test_one_task(vm0, computation_amount);
       test_one_task(vm0, computation_amount);
       test_one_task(vm1, computation_amount);
       test_one_task(pm1, computation_amount);
       MSG_process_sleep(100);

       MSG_vm_destroy(vm0);
       MSG_vm_destroy(vm1);
    }
#endif
  }
}



static int master_main(int argc, char *argv[])
{
  XBT_INFO("=== Test PM ===");
  test_pm();

  XBT_INFO(" ");
  XBT_INFO(" ");
  XBT_INFO("=== Test VM ===");
  test_vm();

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


  XBT_INFO("%s: %d core(s), %f flops/s per each", MSG_host_get_name(pm0), MSG_get_host_core(pm0), MSG_get_host_speed(pm0));
  XBT_INFO("%s: %d core(s), %f flops/s per each", MSG_host_get_name(pm1), MSG_get_host_core(pm1), MSG_get_host_speed(pm1));
  XBT_INFO("%s: %d core(s), %f flops/s per each", MSG_host_get_name(pm2), MSG_get_host_core(pm2), MSG_get_host_speed(pm2));



  MSG_process_create("master", master_main, NULL, pm0);




  int res = MSG_main();
  XBT_INFO("Bye (simulation time %g)", MSG_get_clock());


  return !(res == MSG_OK);
}
