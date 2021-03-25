/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

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

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_migration, "Messages specific for this example");

static void vm_migrate(sg_vm_t vm, sg_host_t dst_pm)
{
  const_sg_host_t src_pm = sg_vm_get_pm(vm);
  double mig_sta         = simgrid_get_clock();
  sg_vm_migrate(vm, dst_pm);
  double mig_end = simgrid_get_clock();

  XBT_INFO("%s migrated: %s->%s in %g s", sg_vm_get_name(vm), sg_host_get_name(src_pm), sg_host_get_name(dst_pm),
           mig_end - mig_sta);
}

static void migration_worker_main(int argc, char* argv[])
{
  xbt_assert(argc == 3);
  const char* vm_name     = argv[1];
  const char* dst_pm_name = argv[2];

  sg_vm_t vm       = (sg_vm_t)sg_host_by_name(vm_name);
  sg_host_t dst_pm = sg_host_by_name(dst_pm_name);

  vm_migrate(vm, dst_pm);
}

static void vm_migrate_async(const_sg_vm_t vm, const_sg_host_t dst_pm)
{
  const char* vm_name     = sg_vm_get_name(vm);
  const char* dst_pm_name = sg_host_get_name(dst_pm);

  const char* argv[] = {"mig_work", vm_name, dst_pm_name, NULL};
  sg_actor_create_("mig_wrk", sg_host_self(), migration_worker_main, 3, argv);
}

static void master_main(int argc, char* argv[])
{
  sg_host_t pm0       = sg_host_by_name("Fafard");
  sg_host_t pm1       = sg_host_by_name("Tremblay");
  const_sg_host_t pm2 = sg_host_by_name("Bourassa");

  sg_vm_t vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_set_ramsize(vm0, 1e9); // 1Gbytes
  sg_vm_start(vm0);

  XBT_INFO("Test: Migrate a VM with %zu Mbytes RAM", sg_vm_get_ramsize(vm0) / 1000 / 1000);
  vm_migrate(vm0, pm1);

  sg_vm_destroy(vm0);

  vm0 = sg_vm_create_core(pm0, "VM0");
  sg_vm_set_ramsize(vm0, 1e8); // 100Mbytes
  sg_vm_start(vm0);

  XBT_INFO("Test: Migrate a VM with %zu Mbytes RAM", sg_vm_get_ramsize(vm0) / 1000 / 1000);
  vm_migrate(vm0, pm1);

  sg_vm_destroy(vm0);

  vm0         = sg_vm_create_core(pm0, "VM0");
  sg_vm_t vm1 = sg_vm_create_core(pm0, "VM1");

  sg_vm_set_ramsize(vm0, 1e9); // 1Gbytes
  sg_vm_set_ramsize(vm1, 1e9); // 1Gbytes
  sg_vm_start(vm0);
  sg_vm_start(vm1);

  XBT_INFO("Test: Migrate two VMs at once from PM0 to PM1");
  vm_migrate_async(vm0, pm1);
  vm_migrate_async(vm1, pm1);
  sg_actor_sleep_for(10000);

  sg_vm_destroy(vm0);
  sg_vm_destroy(vm1);

  vm0 = sg_vm_create_core(pm0, "VM0");
  vm1 = sg_vm_create_core(pm0, "VM1");

  sg_vm_set_ramsize(vm0, 1e9); // 1Gbytes
  sg_vm_set_ramsize(vm1, 1e9); // 1Gbytes
  sg_vm_start(vm0);
  sg_vm_start(vm1);

  XBT_INFO("Test: Migrate two VMs at once to different PMs");
  vm_migrate_async(vm0, pm1);
  vm_migrate_async(vm1, pm2);
  sg_actor_sleep_for(10000);

  sg_vm_destroy(vm0);
  sg_vm_destroy(vm1);
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
  XBT_INFO("Bye (simulation time %g)", simgrid_get_clock());

  return 0;
}
