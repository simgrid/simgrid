/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u/VirtualMachine.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_cloud_migration, "Messages specific for this example");
namespace sg4 = simgrid::s4u;

static void vm_migrate(sg4::VirtualMachine* vm, sg4::Host* dst_pm)
{
  const sg4::Host* src_pm = vm->get_pm();
  double mig_sta          = sg4::Engine::get_clock();
  sg_vm_migrate(vm, dst_pm);
  double mig_end = sg4::Engine::get_clock();

  XBT_INFO("%s migrated: %s->%s in %g s", vm->get_cname(), src_pm->get_cname(), dst_pm->get_cname(), mig_end - mig_sta);
}

static void vm_migrate_async(sg4::VirtualMachine* vm, sg4::Host* dst_pm)
{
  sg4::Actor::create("mig_wrk", sg4::Host::current(), vm_migrate, vm, dst_pm);
}

static void master_main()
{
  sg4::Host* pm0 = sg4::Host::by_name("Fafard");
  sg4::Host* pm1 = sg4::Host::by_name("Tremblay");
  sg4::Host* pm2 = sg4::Host::by_name("Bourassa");

  auto* vm0 = pm0->create_vm("VM0", 1);
  vm0->set_ramsize(1e9); // 1Gbytes
  vm0->start();

  XBT_INFO("Test: Migrate a VM with %zu Mbytes RAM", vm0->get_ramsize() / 1000 / 1000);
  vm_migrate(vm0, pm1);

  vm0->destroy();

  vm0 = pm0->create_vm("VM0", 1);
  vm0->set_ramsize(1e8); // 100Mbytes
  vm0->start();

  XBT_INFO("Test: Migrate a VM with %zu Mbytes RAM", vm0->get_ramsize() / 1000 / 1000);
  vm_migrate(vm0, pm1);

  vm0->destroy();

  vm0       = pm0->create_vm("VM0", 1);
  auto* vm1 = pm0->create_vm("VM1", 1);

  vm0->set_ramsize(1e9); // 1Gbytes
  vm1->set_ramsize(1e9); // 1Gbytes
  vm0->start();
  vm1->start();

  XBT_INFO("Test: Migrate two VMs at once from PM0 to PM1");
  vm_migrate_async(vm0, pm1);
  vm_migrate_async(vm1, pm1);
  sg4::this_actor::sleep_for(10000);

  vm0->destroy();
  vm1->destroy();

  vm0 = pm0->create_vm("VM0", 1);
  vm1 = pm0->create_vm("VM1", 1);

  vm0->set_ramsize(1e9); // 1Gbytes
  vm1->set_ramsize(1e9); // 1Gbytes
  vm0->start();
  vm1->start();

  XBT_INFO("Test: Migrate two VMs at once to different PMs");
  vm_migrate_async(vm0, pm1);
  vm_migrate_async(vm1, pm2);
  sg4::this_actor::sleep_for(10000);

  vm0->destroy();
  vm1->destroy();
}

int main(int argc, char* argv[])
{
  /* Get the arguments */
  sg4::Engine e(&argc, argv);
  sg_vm_live_migration_plugin_init();

  /* load the platform file */
  e.load_platform(argv[1]);

  sg4::Actor::create("master_", sg4::Host::by_name("Fafard"), master_main);

  e.run();

  XBT_INFO("Bye (simulation time %g)", sg4::Engine::get_clock());

  return 0;
}
