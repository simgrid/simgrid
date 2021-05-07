/* Copyright (c) 2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"

#include <limits>

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "Messages specific to this example");

/* Test for issue https://github.com/simgrid/simgrid/issues/322: Issue when an actor kills his host vm
 *
 * A VM is created, and destroyed either from the outside or from the inside.
 *
 * The latter case initially failed because:
 * - a call to "vm->destroy()" implicitly does a "vm->shutdown()" first;
 * - when an actor wanted to destroy its own VM, it was killed by the shutdown phase,
 *   and could not finish the destruction.
 */

/* Output infos about physical hosts first: name, load, actors present in them
 * and then output infos about vm hosts: name, physical host, load, state, actors present in them
 */
static void print_status(const std::vector<simgrid::s4u::Host*>& hosts)
{
  XBT_INFO("---- HOSTS and VMS STATUS ----");
  XBT_INFO("--- HOSTS ---");
  for (auto const& host : hosts) {
    XBT_INFO("+ Name:%s Load:%f", host->get_cname(), host->get_load());
    for (auto const& actor : host->get_all_actors())
      XBT_INFO("++ actor: %s", actor->get_cname());
  }
  XBT_INFO("--- VMS ---");
  for (auto const& host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    if (auto vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host)) {
      XBT_INFO("+ Name:%s Host:%s Load:%f State: %s", vm->get_cname(), vm->get_pm()->get_cname(), vm->get_load(),
               simgrid::s4u::VirtualMachine::to_c_str(vm->get_state()));
      for (auto const& actor : host->get_all_actors())
        XBT_INFO("++ actor: %s", actor->get_cname());
    }
  }
  XBT_INFO("------------------------------");
}

/* actor launching an infinite task on all the cores of its vm
 */
static void life_cycle_manager()
{
  simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(simgrid::s4u::this_actor::get_host());

  for (int i = 0; i < vm->get_core_count(); i++)
    simgrid::s4u::this_actor::exec_async(std::numeric_limits<double>::max());

  simgrid::s4u::this_actor::sleep_for(50);
  XBT_INFO("I'm done sleeping, time to kill myself");
  vm->destroy();
}

/* master actor (placed on hosts[0])
 *
 * create a vm on hoss[1] and hosts[2]
 * launch an actor creating an infinite task (exec_async) inside this vm
 *
 * task is either killed by this master or by the actor himself
 */
static void master(const std::vector<simgrid::s4u::Host*>& hosts)
{
  for (int i = 1; i <= 2; i++) {
    simgrid::s4u::VirtualMachine* vm = new simgrid::s4u::VirtualMachine("test_vm", hosts.at(i), 4);
    vm->start();
    simgrid::s4u::Actor::create("life_cycle_manager-" + std::to_string(i), vm, life_cycle_manager);

    simgrid::s4u::this_actor::sleep_for(10);
    print_status(hosts);

    simgrid::s4u::this_actor::sleep_for(30);
    if (i == 1) {
      XBT_INFO("Time to kill VM from master");
      vm->destroy();
    }

    simgrid::s4u::this_actor::sleep_for(20);
    print_status(hosts);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform.xml\n", argv[0]);

  e.load_platform(argv[1]);
  std::vector<simgrid::s4u::Host*> hosts = e.get_all_hosts();
  xbt_assert(hosts.size() > 3);
  hosts.resize(3);
  simgrid::s4u::Actor::create("test_master", hosts[0], master, hosts);

  e.run();
  return 0;
}
