/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/mailbox.h"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/vm.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_simple, "Messages specific for this example");

static void computation_fun(int argc, char* argv[])
{
  const char* pr_name   = sg_actor_get_name(sg_actor_self());
  const char* host_name = sg_host_get_name(sg_host_self());

  double clock_sta = simgrid_get_clock();
  sg_actor_execute(1000000);
  double clock_end = simgrid_get_clock();

  XBT_INFO("%s:%s task executed %g", host_name, pr_name, clock_end - clock_sta);
}

static void launch_computation_worker(sg_host_t host)
{
  sg_actor_create("compute", host, computation_fun, 0, NULL);
}

struct task_priv {
  sg_host_t tx_host;
  sg_actor_t tx_proc;
  double clock_sta;
};

static void communication_tx_fun(int argc, char* argv[])
{
  xbt_assert(argc == 2);
  sg_mailbox_t mbox = sg_mailbox_by_name(argv[1]);

  struct task_priv* priv = xbt_new(struct task_priv, 1);
  priv->tx_proc          = sg_actor_self();
  priv->tx_host          = sg_host_self();
  priv->clock_sta        = simgrid_get_clock();

  sg_mailbox_put(mbox, priv, 1000000);
}

static void communication_rx_fun(int argc, char* argv[])
{
  const char* pr_name   = sg_actor_get_name(sg_actor_self());
  const char* host_name = sg_host_get_name(sg_host_self());
  xbt_assert(argc == 2);
  sg_mailbox_t mbox = sg_mailbox_by_name(argv[1]);

  struct task_priv* priv = (struct task_priv*)sg_mailbox_get(mbox);
  double clock_end       = simgrid_get_clock();

  XBT_INFO("%s:%s to %s:%s => %g sec", sg_host_get_name(priv->tx_host), sg_actor_get_name(priv->tx_proc), host_name,
           pr_name, clock_end - priv->clock_sta);

  free(priv);
}

static void launch_communication_worker(sg_host_t tx_host, sg_host_t rx_host)
{
  char* mbox = bprintf("MBOX:%s-%s", sg_host_get_name(tx_host), sg_host_get_name(rx_host));

  const char* tx_argv[] = {"comm_tx", mbox, NULL};
  sg_actor_create_("comm_tx", tx_host, communication_tx_fun, 2, tx_argv);

  const char* rx_argv[] = {"comm_rx", mbox, NULL};
  sg_actor_create_("comm_rx", rx_host, communication_rx_fun, 2, rx_argv);

  xbt_free(mbox);
}

static void master_main(int argc, char* argv[])
{
  sg_host_t pm0 = sg_host_by_name("Fafard");
  sg_host_t pm1 = sg_host_by_name("Tremblay");
  sg_host_t pm2 = sg_host_by_name("Bourassa");

  XBT_INFO("## Test 1 (started): check computation on normal PMs");

  XBT_INFO("### Put a task on a PM");
  launch_computation_worker(pm0);
  sg_actor_sleep_for(2);

  XBT_INFO("### Put two tasks on a PM");
  launch_computation_worker(pm0);
  launch_computation_worker(pm0);
  sg_actor_sleep_for(2);

  XBT_INFO("### Put a task on each PM");
  launch_computation_worker(pm0);
  launch_computation_worker(pm1);
  sg_actor_sleep_for(2);

  XBT_INFO("## Test 1 (ended)");

  XBT_INFO("## Test 2 (started): check impact of running a task inside a VM (there is no degradation for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the VM");
  sg_vm_t vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_start(vm0);
  launch_computation_worker((sg_host_t)vm0);
  sg_actor_sleep_for(2);
  sg_vm_destroy(vm0);

  XBT_INFO("## Test 2 (ended)");

  XBT_INFO(
      "## Test 3 (started): check impact of running a task collocated with a VM (there is no VM noise for the moment)");

  XBT_INFO("### Put a VM on a PM, and put a task to the PM");
  vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_start(vm0);
  launch_computation_worker(pm0);
  sg_actor_sleep_for(2);
  sg_vm_destroy(vm0);

  XBT_INFO("## Test 3 (ended)");

  XBT_INFO("## Test 4 (started): compare the cost of running two tasks inside two different VMs collocated or not (for"
           " the moment, there is no degradation for the VMs. Hence, the time should be equals to the time of test 1");

  XBT_INFO("### Put two VMs on a PM, and put a task to each VM");
  vm0         = sg_vm_create_core(pm0, "VM0");
  sg_vm_t vm1 = sg_vm_create_core(pm0, "VM1");
  sg_vm_start(vm0);
  sg_vm_start(vm1);
  launch_computation_worker((sg_host_t)vm0);
  launch_computation_worker((sg_host_t)vm1);
  sg_actor_sleep_for(2);
  sg_vm_destroy(vm0);
  sg_vm_destroy(vm1);

  XBT_INFO("### Put a VM on each PM, and put a task to each VM");
  vm0 = sg_vm_create_core(pm0, "VM0");
  vm1 = sg_vm_create_core(pm1, "VM1");
  sg_vm_start(vm0);
  sg_vm_start(vm1);
  launch_computation_worker((sg_host_t)vm0);
  launch_computation_worker((sg_host_t)vm1);
  sg_actor_sleep_for(2);
  sg_vm_destroy(vm0);
  sg_vm_destroy(vm1);
  XBT_INFO("## Test 4 (ended)");

  XBT_INFO("## Test 5  (started): Analyse network impact");
  XBT_INFO("### Make a connection between PM0 and PM1");
  launch_communication_worker(pm0, pm1);
  sg_actor_sleep_for(5);

  XBT_INFO("### Make two connection between PM0 and PM1");
  launch_communication_worker(pm0, pm1);
  launch_communication_worker(pm0, pm1);
  sg_actor_sleep_for(5);

  XBT_INFO("### Make a connection between PM0 and VM0@PM0");
  vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_start(vm0);
  launch_communication_worker(pm0, (sg_host_t)vm0);
  sg_actor_sleep_for(5);
  sg_vm_destroy(vm0);

  XBT_INFO("### Make a connection between PM0 and VM0@PM1");
  vm0 = sg_vm_create_core(pm1, "VM0");
  sg_vm_start(vm0);
  launch_communication_worker(pm0, (sg_host_t)vm0);
  sg_actor_sleep_for(5);
  sg_vm_destroy(vm0);

  XBT_INFO("### Make two connections between PM0 and VM0@PM1");
  vm0 = sg_vm_create_core(pm1, "VM0");
  sg_vm_start(vm0);
  launch_communication_worker(pm0, (sg_host_t)vm0);
  launch_communication_worker(pm0, (sg_host_t)vm0);
  sg_actor_sleep_for(5);
  sg_vm_destroy(vm0);

  XBT_INFO("### Make a connection between PM0 and VM0@PM1, and also make a connection between PM0 and PM1");
  vm0 = sg_vm_create_core(pm1, "VM0");
  sg_vm_start(vm0);
  launch_communication_worker(pm0, (sg_host_t)vm0);
  launch_communication_worker(pm0, pm1);
  sg_actor_sleep_for(5);
  sg_vm_destroy(vm0);

  XBT_INFO("### Make a connection between VM0@PM0 and PM1@PM1, and also make a connection between VM0@PM0 and VM1@PM1");
  vm0 = sg_vm_create_core(pm0, "VM0");
  vm1 = sg_vm_create_core(pm1, "VM1");
  sg_vm_start(vm0);
  sg_vm_start(vm1);
  launch_communication_worker((sg_host_t)vm0, (sg_host_t)vm1);
  launch_communication_worker((sg_host_t)vm0, (sg_host_t)vm1);
  sg_actor_sleep_for(5);
  sg_vm_destroy(vm0);
  sg_vm_destroy(vm1);

  XBT_INFO("## Test 5 (ended)");

  XBT_INFO("## Test 6 (started): Check migration impact (not yet implemented neither on the CPU resource nor on the"
           " network one");
  XBT_INFO("### Relocate VM0 between PM0 and PM1");
  vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_set_ramsize(vm0, 1L * 1024 * 1024 * 1024); // 1GiB

  sg_vm_start(vm0);
  launch_communication_worker((sg_host_t)vm0, pm2);
  sg_actor_sleep_for(0.01);
  sg_vm_migrate(vm0, pm1);
  sg_actor_sleep_for(0.01);
  sg_vm_migrate(vm0, pm0);
  sg_actor_sleep_for(5);
  sg_vm_destroy(vm0);
  XBT_INFO("## Test 6 (ended)");
}

int main(int argc, char* argv[])
{
  /* Get the arguments */
  simgrid_init(&argc, argv);
  sg_vm_live_migration_plugin_init();

  /* load the platform file */
  simgrid_load_platform(argv[1]);

  sg_actor_create("master_", sg_host_by_name("Fafard"), master_main, 0, NULL);

  simgrid_run();
  XBT_INFO("Simulation time %g", simgrid_get_clock());

  return 0;
}
