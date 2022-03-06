/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u/VirtualMachine.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void computation_fun()
{
  double clock_sta = sg4::Engine::get_clock();
  sg4::this_actor::execute(1000000);
  double clock_end = sg4::Engine::get_clock();

  XBT_INFO("%s:%s executed %g", sg4::this_actor::get_host()->get_cname(), sg4::this_actor::get_cname(),
           clock_end - clock_sta);
}

static void launch_computation_worker(s4u_Host* host)
{
  sg4::Actor::create("compute", host, computation_fun);
}

struct s_payload {
  s4u_Host* tx_host;
  const char* tx_actor_name;
  double clock_sta;
};

static void communication_tx_fun(std::vector<std::string> args)
{
  sg4::Mailbox* mbox            = sg4::Mailbox::by_name(args.at(0));
  auto* payload                 = new s_payload;
  payload->tx_actor_name        = sg4::Actor::self()->get_cname();
  payload->tx_host              = sg4::this_actor::get_host();
  payload->clock_sta            = sg4::Engine::get_clock();

  mbox->put(payload, 1000000);
}

static void communication_rx_fun(std::vector<std::string> args)
{
  const char* actor_name = sg4::Actor::self()->get_cname();
  const char* host_name  = sg4::this_actor::get_host()->get_cname();
  sg4::Mailbox* mbox     = sg4::Mailbox::by_name(args.at(0));

  auto payload     = mbox->get_unique<struct s_payload>();
  double clock_end = sg4::Engine::get_clock();

  XBT_INFO("%s:%s to %s:%s => %g sec", payload->tx_host->get_cname(), payload->tx_actor_name, host_name, actor_name,
           clock_end - payload->clock_sta);
}

static void launch_communication_worker(s4u_Host* tx_host, s4u_Host* rx_host)
{
  std::string mbox_name = std::string("MBOX:") + tx_host->get_cname() + "-" + rx_host->get_cname();
  std::vector<std::string> args;
  args.push_back(mbox_name);

  sg4::Actor::create("comm_tx", tx_host, communication_tx_fun, args);

  sg4::Actor::create("comm_rx", rx_host, communication_rx_fun, args);
}

static void master_main()
{
  s4u_Host* pm0 = sg4::Host::by_name("Fafard");
  s4u_Host* pm1 = sg4::Host::by_name("Tremblay");
  s4u_Host* pm2 = sg4::Host::by_name("Bourassa");

  XBT_INFO("## Test 1 (started): check computation on normal PMs");

  XBT_INFO("### Put an activity on a PM");
  launch_computation_worker(pm0);
  sg4::this_actor::sleep_for(2);

  XBT_INFO("### Put two activities on a PM");
  launch_computation_worker(pm0);
  launch_computation_worker(pm0);
  sg4::this_actor::sleep_for(2);

  XBT_INFO("### Put an activity on each PM");
  launch_computation_worker(pm0);
  launch_computation_worker(pm1);
  sg4::this_actor::sleep_for(2);

  XBT_INFO("## Test 1 (ended)");

  XBT_INFO(
      "## Test 2 (started): check impact of running an activity inside a VM (there is no degradation for the moment)");

  XBT_INFO("### Put a VM on a PM, and put an activity to the VM");
  auto* vm0 = pm0->create_vm("VM0", 1);
  vm0->start();
  launch_computation_worker(vm0);
  sg4::this_actor::sleep_for(2);
  vm0->destroy();

  XBT_INFO("## Test 2 (ended)");

  XBT_INFO("## Test 3 (started): check impact of running an activity collocated with a VM (there is no VM noise for "
           "the moment)");

  XBT_INFO("### Put a VM on a PM, and put an activity to the PM");
  vm0 = pm0->create_vm("VM0", 1);
  vm0->start();
  launch_computation_worker(pm0);
  sg4::this_actor::sleep_for(2);
  vm0->destroy();
  XBT_INFO("## Test 3 (ended)");

  XBT_INFO(
      "## Test 4 (started): compare the cost of running two activities inside two different VMs collocated or not (for"
      " the moment, there is no degradation for the VMs. Hence, the time should be equals to the time of test 1");

  XBT_INFO("### Put two VMs on a PM, and put an activity to each VM");
  vm0 = pm0->create_vm("VM0", 1);
  vm0->start();
  auto* vm1 = pm0->create_vm("VM1", 1);
  launch_computation_worker(vm0);
  launch_computation_worker(vm1);
  sg4::this_actor::sleep_for(2);
  vm0->destroy();
  vm1->destroy();

  XBT_INFO("### Put a VM on each PM, and put an activity to each VM");
  vm0 = pm0->create_vm("VM0", 1);
  vm1 = pm1->create_vm("VM1", 1);
  vm0->start();
  vm1->start();
  launch_computation_worker(vm0);
  launch_computation_worker(vm1);
  sg4::this_actor::sleep_for(2);
  vm0->destroy();
  vm1->destroy();
  XBT_INFO("## Test 4 (ended)");

  XBT_INFO("## Test 5  (started): Analyse network impact");
  XBT_INFO("### Make a connection between PM0 and PM1");
  launch_communication_worker(pm0, pm1);
  sg4::this_actor::sleep_for(5);

  XBT_INFO("### Make two connection between PM0 and PM1");
  launch_communication_worker(pm0, pm1);
  launch_communication_worker(pm0, pm1);
  sg4::this_actor::sleep_for(5);

  XBT_INFO("### Make a connection between PM0 and VM0@PM0");
  vm0 = pm0->create_vm("VM0", 1);
  vm0->start();
  launch_communication_worker(pm0, vm0);
  sg4::this_actor::sleep_for(5);
  vm0->destroy();

  XBT_INFO("### Make a connection between PM0 and VM0@PM1");
  vm0 = pm1->create_vm("VM0", 1);
  launch_communication_worker(pm0, vm0);
  sg4::this_actor::sleep_for(5);
  vm0->destroy();

  XBT_INFO("### Make two connections between PM0 and VM0@PM1");
  vm0 = pm1->create_vm("VM0", 1);
  vm0->start();
  launch_communication_worker(pm0, vm0);
  launch_communication_worker(pm0, vm0);
  sg4::this_actor::sleep_for(5);
  vm0->destroy();

  XBT_INFO("### Make a connection between PM0 and VM0@PM1, and also make a connection between PM0 and PM1");
  vm0 = pm1->create_vm("VM0", 1);
  vm0->start();
  launch_communication_worker(pm0, vm0);
  launch_communication_worker(pm0, pm1);
  sg4::this_actor::sleep_for(5);
  vm0->destroy();

  XBT_INFO("### Make a connection between VM0@PM0 and PM1@PM1, and also make a connection between VM0@PM0 and VM1@PM1");
  vm0 = pm0->create_vm("VM0", 1);
  vm1 = pm1->create_vm("VM1", 1);
  vm0->start();
  vm1->start();
  launch_communication_worker(vm0, vm1);
  launch_communication_worker(vm0, vm1);
  sg4::this_actor::sleep_for(5);
  vm0->destroy();
  vm1->destroy();

  XBT_INFO("## Test 5 (ended)");
  XBT_INFO("## Test 6 (started): Check migration impact (not yet implemented neither on the CPU resource nor on the"
           " network one");
  XBT_INFO("### Relocate VM0 between PM0 and PM1");
  vm0 = pm0->create_vm("VM0", 1);
  vm0->set_ramsize(1L * 1024 * 1024 * 1024); // 1GiB

  vm0->start();
  launch_communication_worker(vm0, pm2);
  sg4::this_actor::sleep_for(0.01);
  sg_vm_migrate(vm0, pm1);
  sg4::this_actor::sleep_for(0.01);
  sg_vm_migrate(vm0, pm0);
  sg4::this_actor::sleep_for(5);
  vm0->destroy();
  XBT_INFO("## Test 6 (ended)");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  sg_vm_live_migration_plugin_init();
  e.load_platform(argv[1]); /* - Load the platform description */

  sg4::Actor::create("master_", e.host_by_name("Fafard"), master_main);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
