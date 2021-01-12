/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_cloud_interrupt_migration, "Messages specific for this example");

static void vm_migrate(simgrid::s4u::VirtualMachine* vm, simgrid::s4u::Host* dst_pm)
{
  const simgrid::s4u::Host* src_pm = vm->get_pm();
  double mig_sta             = simgrid::s4u::Engine::get_clock();
  sg_vm_migrate(vm, dst_pm);
  double mig_end = simgrid::s4u::Engine::get_clock();

  XBT_INFO("%s migrated: %s->%s in %g s", vm->get_cname(), src_pm->get_cname(), dst_pm->get_cname(), mig_end - mig_sta);
}

static simgrid::s4u::ActorPtr vm_migrate_async(simgrid::s4u::VirtualMachine* vm, simgrid::s4u::Host* dst_pm)
{
  return simgrid::s4u::Actor::create("mig_wrk", simgrid::s4u::Host::current(), vm_migrate, vm, dst_pm);
}

static void master_main()
{
  simgrid::s4u::Host* pm0 = simgrid::s4u::Host::by_name("Fafard");
  simgrid::s4u::Host* pm1 = simgrid::s4u::Host::by_name("Tremblay");

  auto* vm0 = new simgrid::s4u::VirtualMachine("VM0", pm0, 1);
  vm0->set_ramsize(1e9); // 1Gbytes
  vm0->start();

  XBT_INFO("Start the migration of %s from %s to %s", vm0->get_cname(), pm0->get_cname(), pm1->get_cname());
  vm_migrate_async(vm0, pm1);

  simgrid::s4u::this_actor::sleep_for(2);
  XBT_INFO("Wait! change my mind, shutdown %s. This ends the migration", vm0->get_cname());
  vm0->shutdown();

  simgrid::s4u::this_actor::sleep_for(8);

  XBT_INFO("Start again the migration of %s from %s to %s", vm0->get_cname(), pm0->get_cname(), pm1->get_cname());

  vm0->start();
  vm_migrate_async(vm0, pm1);

  XBT_INFO("Wait for the completion of the migration this time");
  simgrid::s4u::this_actor::sleep_for(200);
  vm0->destroy();
}

int main(int argc, char* argv[])
{
  /* Get the arguments */
  simgrid::s4u::Engine e(&argc, argv);
  sg_vm_live_migration_plugin_init();

  /* load the platform file */
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("master_", simgrid::s4u::Host::by_name("Fafard"), master_main);

  e.run();

  XBT_INFO("Bye (simulation time %g)", simgrid::s4u::Engine::get_clock());

  return 0;
}
