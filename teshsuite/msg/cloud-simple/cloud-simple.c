/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "simgrid/plugins/live_migration.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int computation_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  const char* pr_name   = MSG_process_get_name(MSG_process_self());
  const char* host_name = MSG_host_get_name(MSG_host_self());

  msg_task_t task = MSG_task_create("Task", 1000000, 1000000, NULL);

  double clock_sta = MSG_get_clock();
  MSG_task_execute(task);
  double clock_end = MSG_get_clock();

  XBT_INFO("%s:%s task executed %g", host_name, pr_name, clock_end - clock_sta);

  MSG_task_destroy(task);

  return 0;
}

static void launch_computation_worker(msg_host_t host)
{
  const char* pr_name = "compute";
  char** argv         = xbt_new(char*, 2);
  argv[0]             = xbt_strdup(pr_name);
  argv[1]             = NULL;

  MSG_process_create_with_arguments(pr_name, computation_fun, NULL, host, 1, argv);
}

struct task_priv {
  msg_host_t tx_host;
  msg_process_t tx_proc;
  double clock_sta;
};

static int communication_tx_fun(int argc, char* argv[])
{
  xbt_assert(argc == 2);
  const char* mbox = argv[1];

  msg_task_t task = MSG_task_create("Task", 1000000, 1000000, NULL);

  struct task_priv* priv = xbt_new(struct task_priv, 1);
  priv->tx_proc          = MSG_process_self();
  priv->tx_host          = MSG_host_self();
  priv->clock_sta        = MSG_get_clock();

  MSG_task_set_data(task, priv);

  MSG_task_send(task, mbox);

  return 0;
}

static int communication_rx_fun(int argc, char* argv[])
{
  const char* pr_name   = MSG_process_get_name(MSG_process_self());
  const char* host_name = MSG_host_get_name(MSG_host_self());
  xbt_assert(argc == 2);
  const char* mbox = argv[1];

  msg_task_t task = NULL;
  MSG_task_recv(&task, mbox);

  struct task_priv* priv = MSG_task_get_data(task);
  double clock_end       = MSG_get_clock();

  XBT_INFO("%s:%s to %s:%s => %g sec", MSG_host_get_name(priv->tx_host), MSG_process_get_name(priv->tx_proc), host_name,
           pr_name, clock_end - priv->clock_sta);

  xbt_free(priv);
  MSG_task_destroy(task);

  return 0;
}

static void launch_communication_worker(msg_host_t tx_host, msg_host_t rx_host)
{
  char* mbox             = bprintf("MBOX:%s-%s", MSG_host_get_name(tx_host), MSG_host_get_name(rx_host));
  const char* pr_name_tx = "comm_tx";

  char** argv = xbt_new(char*, 3);
  argv[0]     = xbt_strdup(pr_name_tx);
  argv[1]     = xbt_strdup(mbox);
  argv[2]     = NULL;

  MSG_process_create_with_arguments(pr_name_tx, communication_tx_fun, NULL, tx_host, 2, argv);

  const char* pr_name_rx = "comm_rx";
  argv                   = xbt_new(char*, 3);
  argv[0]                = xbt_strdup(pr_name_rx);
  argv[1]                = xbt_strdup(mbox);
  argv[2]                = NULL;

  MSG_process_create_with_arguments(pr_name_rx, communication_rx_fun, NULL, rx_host, 2, argv);

  xbt_free(mbox);
}

static int master_main(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_host_t pm0 = MSG_host_by_name("Fafard");
  msg_host_t pm1 = MSG_host_by_name("Tremblay");
  msg_host_t pm2 = MSG_host_by_name("Bourassa");

  XBT_INFO("## Test 1 (started): check computation on normal PMs");

  XBT_INFO("### Put a task on a PM");
  launch_computation_worker(pm0);
  MSG_process_sleep(2);

  XBT_INFO("### Put two tasks on a PM");
  launch_computation_worker(pm0);
  launch_computation_worker(pm0);
  MSG_process_sleep(2);

  XBT_INFO("### Put a task on each PM");
  launch_computation_worker(pm0);
  launch_computation_worker(pm1);
  MSG_process_sleep(2);

  XBT_INFO("## Test 1 (ended)");

  XBT_INFO("## Test 2 (started): check impact of running a task inside a VM (there is no degradation for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  msg_vm_t vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  launch_computation_worker((msg_host_t)vm0);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);

  XBT_INFO("## Test 2 (ended)");

  XBT_INFO(
      "## Test 3 (started): check impact of running a task collocated with a VM (there is no VM noise for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  launch_computation_worker(pm0);
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
  launch_computation_worker((msg_host_t)vm0);
  launch_computation_worker((msg_host_t)vm1);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);

  XBT_INFO("### Put a VM on each PM, and put a task to each VM");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  vm1 = MSG_vm_create_core(pm1, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  launch_computation_worker((msg_host_t)vm0);
  launch_computation_worker((msg_host_t)vm1);
  MSG_process_sleep(2);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);
  XBT_INFO("## Test 4 (ended)");

  XBT_INFO("## Test 5  (started): Analyse network impact");
  XBT_INFO("### Make a connection between PM0 and PM1");
  launch_communication_worker(pm0, pm1);
  MSG_process_sleep(5);

  XBT_INFO("### Make two connection between PM0 and PM1");
  launch_communication_worker(pm0, pm1);
  launch_communication_worker(pm0, pm1);
  MSG_process_sleep(5);

  XBT_INFO("### Make a connection between PM0 and VM0@PM0");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_start(vm0);
  launch_communication_worker(pm0, (msg_host_t)vm0);
  MSG_process_sleep(5);
  MSG_vm_destroy(vm0);

  XBT_INFO("### Make a connection between PM0 and VM0@PM1");
  vm0 = MSG_vm_create_core(pm1, "VM0");
  MSG_vm_start(vm0);
  launch_communication_worker(pm0, (msg_host_t)vm0);
  MSG_process_sleep(5);
  MSG_vm_destroy(vm0);

  XBT_INFO("### Make two connections between PM0 and VM0@PM1");
  vm0 = MSG_vm_create_core(pm1, "VM0");
  MSG_vm_start(vm0);
  launch_communication_worker(pm0, (msg_host_t)vm0);
  launch_communication_worker(pm0, (msg_host_t)vm0);
  MSG_process_sleep(5);
  MSG_vm_destroy(vm0);

  XBT_INFO("### Make a connection between PM0 and VM0@PM1, and also make a connection between PM0 and PM1");
  vm0 = MSG_vm_create_core(pm1, "VM0");
  MSG_vm_start(vm0);
  launch_communication_worker(pm0, (msg_host_t)vm0);
  launch_communication_worker(pm0, pm1);
  MSG_process_sleep(5);
  MSG_vm_destroy(vm0);

  XBT_INFO("### Make a connection between VM0@PM0 and PM1@PM1, and also make a connection between VM0@PM0 and VM1@PM1");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  vm1 = MSG_vm_create_core(pm1, "VM1");
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);
  launch_communication_worker((msg_host_t)vm0, (msg_host_t)vm1);
  launch_communication_worker((msg_host_t)vm0, (msg_host_t)vm1);
  MSG_process_sleep(5);
  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);

  XBT_INFO("## Test 5 (ended)");

  XBT_INFO("## Test 6 (started): Check migration impact (not yet implemented neither on the CPU resource nor on the"
           " network one");
  XBT_INFO("### Relocate VM0 between PM0 and PM1");
  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_set_ramsize(vm0, 1L * 1024 * 1024 * 1024); // 1GiB

  MSG_vm_start(vm0);
  launch_communication_worker((msg_host_t)vm0, pm2);
  MSG_process_sleep(0.01);
  MSG_vm_migrate(vm0, pm1);
  MSG_process_sleep(0.01);
  MSG_vm_migrate(vm0, pm0);
  MSG_process_sleep(5);
  MSG_vm_destroy(vm0);
  XBT_INFO("## Test 6 (ended)");

  return 0;
}

static void launch_master(msg_host_t host)
{
  const char* pr_name = "master_";
  char** argv         = xbt_new(char*, 2);
  argv[0]             = xbt_strdup(pr_name);
  argv[1]             = NULL;

  MSG_process_create_with_arguments(pr_name, master_main, NULL, host, 1, argv);
}

int main(int argc, char* argv[])
{
  /* Get the arguments */
  MSG_init(&argc, argv);
  sg_vm_live_migration_plugin_init();

  /* load the platform file */
  const char* platform = "../../platforms/small_platform.xml";
  if (argc == 2)
    platform = argv[1];
  MSG_create_environment(platform);

  msg_host_t pm0 = MSG_host_by_name("Fafard");
  launch_master(pm0);

  int res = MSG_main();
  XBT_INFO("Bye (simulation time %g)", MSG_get_clock());

  return !(res == MSG_OK);
}
