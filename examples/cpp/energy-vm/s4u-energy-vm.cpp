/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/VirtualMachine.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(energy_vm, "Messages of this example");
namespace sg4 = simgrid::s4u;

static void executor()
{
  sg4::this_actor::execute(300E6);
  XBT_INFO("This worker is done.");
}

static void dvfs()
{
  sg4::Host* host1 = sg4::Host::by_name("MyHost1");
  sg4::Host* host2 = sg4::Host::by_name("MyHost2");
  sg4::Host* host3 = sg4::Host::by_name("MyHost3");

  /* Host 1 */
  XBT_INFO("Creating and starting two VMs");
  auto* vm_host1 = host1->create_vm("vm_host1", 1);
  vm_host1->start();
  auto* vm_host2 = host2->create_vm("vm_host2", 1);
  vm_host2->start();

  XBT_INFO("Create two activities on Host1: both inside a VM");
  sg4::Actor::create("p11", vm_host1, executor);
  sg4::Actor::create("p12", vm_host1, executor);

  XBT_INFO("Create two activities on Host2: one inside a VM, the other directly on the host");
  sg4::Actor::create("p21", vm_host2, executor);
  sg4::Actor::create("p22", host2, executor);

  XBT_INFO("Create two activities on Host3: both directly on the host");
  sg4::Actor::create("p31", host3, executor);
  sg4::Actor::create("p32", host3, executor);

  XBT_INFO("Wait 5 seconds. The activities are still running (they run for 3 seconds, but 2 activities are co-located, "
           "so they run for 6 seconds)");
  sg4::this_actor::sleep_for(5);
  XBT_INFO("Wait another 5 seconds. The activities stop at some point in between");
  sg4::this_actor::sleep_for(5);

  vm_host1->destroy();
  vm_host2->destroy();
}

int main(int argc, char* argv[])
{
  sg_host_energy_plugin_init();
  sg4::Engine e(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s ../platforms/energy_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  sg4::Actor::create("dvfs", e.host_by_name("MyHost1"), dvfs);

  e.run();

  XBT_INFO("Total simulation time: %.2f; Host2 and Host3 must have the exact same energy consumption; Host1 is "
           "multi-core and will differ.",
           sg4::Engine::get_clock());

  return 0;
}
