/* Copyright (c) 2007-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "simgrid/plugins/live_migration.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static void vm_migrate(msg_vm_t vm, msg_host_t dst_pm)
{
  const_sg_host_t src_pm = MSG_vm_get_pm(vm);
  double mig_sta    = MSG_get_clock();
  MSG_vm_migrate(vm, dst_pm);
  double mig_end = MSG_get_clock();

  XBT_INFO("%s migrated: %s->%s in %g s", MSG_vm_get_name(vm), MSG_host_get_name(src_pm), MSG_host_get_name(dst_pm),
           mig_end - mig_sta);
}

static int migration_worker_main(int argc, char* argv[])
{
  xbt_assert(argc == 3);
  const char* vm_name     = argv[1];
  const char* dst_pm_name = argv[2];

  msg_vm_t vm       = (msg_vm_t)MSG_host_by_name(vm_name);
  msg_host_t dst_pm = MSG_host_by_name(dst_pm_name);

  vm_migrate(vm, dst_pm);

  return 0;
}

static void vm_migrate_async(const_sg_vm_t vm, const_sg_host_t dst_pm)
{
  const char* vm_name     = MSG_vm_get_name(vm);
  const char* dst_pm_name = MSG_host_get_name(dst_pm);
  msg_host_t host         = MSG_host_self();

  const char* pr_name = "mig_wrk";
  char** argv         = xbt_new(char*, 4);
  argv[0]             = xbt_strdup(pr_name);
  argv[1]             = xbt_strdup(vm_name);
  argv[2]             = xbt_strdup(dst_pm_name);
  argv[3]             = NULL;

  MSG_process_create_with_arguments(pr_name, migration_worker_main, NULL, host, 3, argv);
}

static int master_main(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_host_t pm0 = MSG_host_by_name("Fafard");
  msg_host_t pm1 = MSG_host_by_name("Tremblay");
  const_sg_host_t pm2 = MSG_host_by_name("Bourassa");

  msg_vm_t vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_set_ramsize(vm0, 1e9); // 1Gbytes
  MSG_vm_start(vm0);

  XBT_INFO("Test: Migrate a VM with %zu Mbytes RAM", MSG_vm_get_ramsize(vm0) / 1000 / 1000);
  vm_migrate(vm0, pm1);

  MSG_vm_destroy(vm0);

  vm0 = MSG_vm_create_core(pm0, "VM0");
  MSG_vm_set_ramsize(vm0, 1e8); // 100Mbytes
  MSG_vm_start(vm0);

  XBT_INFO("Test: Migrate a VM with %zu Mbytes RAM", MSG_vm_get_ramsize(vm0) / 1000 / 1000);
  vm_migrate(vm0, pm1);

  MSG_vm_destroy(vm0);

  vm0          = MSG_vm_create_core(pm0, "VM0");
  msg_vm_t vm1 = MSG_vm_create_core(pm0, "VM1");

  MSG_vm_set_ramsize(vm0, 1e9); // 1Gbytes
  MSG_vm_set_ramsize(vm1, 1e9); // 1Gbytes
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);

  XBT_INFO("Test: Migrate two VMs at once from PM0 to PM1");
  vm_migrate_async(vm0, pm1);
  vm_migrate_async(vm1, pm1);
  MSG_process_sleep(10000);

  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);

  vm0 = MSG_vm_create_core(pm0, "VM0");
  vm1 = MSG_vm_create_core(pm0, "VM1");

  MSG_vm_set_ramsize(vm0, 1e9); // 1Gbytes
  MSG_vm_set_ramsize(vm1, 1e9); // 1Gbytes
  MSG_vm_start(vm0);
  MSG_vm_start(vm1);

  XBT_INFO("Test: Migrate two VMs at once to different PMs");
  vm_migrate_async(vm0, pm1);
  vm_migrate_async(vm1, pm2);
  MSG_process_sleep(10000);

  MSG_vm_destroy(vm0);
  MSG_vm_destroy(vm1);

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
  MSG_vm_live_migration_plugin_init();

  /* load the platform file */
  MSG_create_environment(argv[1]);

  msg_host_t pm0 = MSG_host_by_name("Fafard");
  launch_master(pm0);

  int res = MSG_main();
  XBT_INFO("Bye (simulation time %g)", MSG_get_clock());

  return !(res == MSG_OK);
}
