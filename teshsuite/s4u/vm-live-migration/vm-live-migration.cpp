/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u.hpp"
#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "Messages specific for this example");

static void task_executor()
{
  simgrid::s4u::ExecPtr ptr = simgrid::s4u::this_actor::exec_init(3000000000.0)->start();

  // ptr->test(); // Harmless
  simgrid::s4u::this_actor::sleep_for(3); // Breaks everything

  ptr->wait();
  const auto* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(ptr->get_host());
  xbt_assert(vm != nullptr, "Hey, I expected to run on a VM");
  XBT_INFO("Task done. It's running on %s@%s that runs at %.0ef/s. host2 runs at %.0ef/s", vm->get_cname(),
           vm->get_pm()->get_cname(), vm->get_speed(), simgrid::s4u::Host::by_name("host2")->get_speed());
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  simgrid::s4u::Engine::set_config("network/model:CM02"); // Much less realistic, but easier to compute manually

  sg_vm_live_migration_plugin_init();
  sg_host_energy_plugin_init();

  xbt_assert(argc == 2, "Usage: %s platform.xml\n", argv[0]);

  e.load_platform(argv[1]);
  auto* pm = simgrid::s4u::Host::by_name("host1");
  auto* vm = new simgrid::s4u::VirtualMachine("VM0", pm, 1 /*nCores*/);
  vm->set_ramsize(1250000000)->start();
  simgrid::s4u::Actor::create("executor", vm, task_executor);

  simgrid::s4u::Actor::create("migration", pm, [vm]() {
    XBT_INFO("%s migration started", vm->get_cname());
    const auto* old = vm->get_pm();

    sg_vm_migrate(vm, simgrid::s4u::Host::by_name("host2"));

    XBT_INFO("VM '%s' migrated from '%s' to '%s'", vm->get_cname(), old->get_cname(), vm->get_pm()->get_cname());
  });

  e.run();
  return 0;
}
