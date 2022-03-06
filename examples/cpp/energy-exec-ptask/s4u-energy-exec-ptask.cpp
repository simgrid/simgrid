/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void runner()
{
  sg4::Host* host1 = sg4::Host::by_name("MyHost1");
  sg4::Host* host2 = sg4::Host::by_name("MyHost2");
  std::vector<sg4::Host*> hosts{host1, host2};

  double old_energy_host1 = sg_host_get_consumed_energy(host1);
  double old_energy_host2 = sg_host_get_consumed_energy(host2);

  XBT_INFO("[%s] Energetic profile: %s", host1->get_cname(), host1->get_property("wattage_per_state"));
  XBT_INFO("[%s] Initial peak speed=%.0E flop/s; Total energy dissipated =%.0E J", host1->get_cname(), host1->get_speed(),
           old_energy_host1);
  XBT_INFO("[%s] Energetic profile: %s", host2->get_cname(), host2->get_property("wattage_per_state"));
  XBT_INFO("[%s] Initial peak speed=%.0E flop/s; Total energy dissipated =%.0E J", host2->get_cname(), host2->get_speed(),
           old_energy_host2);

  double start = sg4::Engine::get_clock();
  XBT_INFO("Sleep for 10 seconds");
  sg4::this_actor::sleep_for(10);

  double new_energy_host1 = sg_host_get_consumed_energy(host1);
  double new_energy_host2 = sg_host_get_consumed_energy(host2);
  XBT_INFO("Done sleeping (duration: %.2f s).\n"
           "[%s] Current peak speed=%.0E; Energy dissipated during this step=%.2f J; Total energy dissipated=%.2f J\n"
           "[%s] Current peak speed=%.0E; Energy dissipated during this step=%.2f J; Total energy dissipated=%.2f J\n",
           sg4::Engine::get_clock() - start, host1->get_cname(), host1->get_speed(),
           (new_energy_host1 - old_energy_host1), sg_host_get_consumed_energy(host1), host2->get_cname(),
           host2->get_speed(), (new_energy_host2 - old_energy_host2), sg_host_get_consumed_energy(host2));

  old_energy_host1 = new_energy_host1;
  old_energy_host2 = new_energy_host2;


  // ========= Execute something =========
  start             = sg4::Engine::get_clock();
  double flopAmount = 1E9;
  std::vector<double> cpu_amounts{flopAmount, flopAmount};
  std::vector<double> com_amounts{0, 0, 0, 0};
  XBT_INFO("Run a task of %.0E flops on two hosts", flopAmount);
  sg4::this_actor::parallel_execute(hosts, cpu_amounts, com_amounts);

  new_energy_host1 = sg_host_get_consumed_energy(host1);
  new_energy_host2 = sg_host_get_consumed_energy(host2);
  XBT_INFO(
      "Task done (duration: %.2f s).\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f "
      "J\n",
      sg4::Engine::get_clock() - start, host1->get_cname(), host1->get_speed(), (new_energy_host1 - old_energy_host1),
      sg_host_get_consumed_energy(host1), host2->get_cname(), host2->get_speed(), (new_energy_host2 - old_energy_host2),
      sg_host_get_consumed_energy(host2));

  old_energy_host1 = new_energy_host1;
  old_energy_host2 = new_energy_host2;


  // ========= Change power peak =========
  int pstate = 2;
  host1->set_pstate(pstate);
  host2->set_pstate(pstate);
  XBT_INFO("========= Requesting pstate %d for both hosts (speed should be of %.0E flop/s and is of %.0E flop/s)", pstate,
           host1->get_pstate_speed(pstate), host1->get_speed());


  // ========= Run another ptask =========
  start = sg4::Engine::get_clock();
  std::vector<double> cpu_amounts2{flopAmount, flopAmount};
  std::vector<double> com_amounts2{0, 0, 0, 0};
  XBT_INFO("Run a task of %.0E flops on %s and %.0E flops on %s.", flopAmount, host1->get_cname(), flopAmount, host2->get_cname());
  sg4::this_actor::parallel_execute(hosts, cpu_amounts2, com_amounts2);

  new_energy_host1 = sg_host_get_consumed_energy(host1);
  new_energy_host2 = sg_host_get_consumed_energy(host2);
  XBT_INFO(
      "Task done (duration: %.2f s).\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f "
      "J\n",
      sg4::Engine::get_clock() - start, host1->get_cname(), host1->get_speed(), (new_energy_host1 - old_energy_host1),
      sg_host_get_consumed_energy(host1), host2->get_cname(), host2->get_speed(), (new_energy_host2 - old_energy_host2),
      sg_host_get_consumed_energy(host2));

  old_energy_host1 = new_energy_host1;
  old_energy_host2 = new_energy_host2;


  // ========= A new ptask with computation and communication =========
  start            = sg4::Engine::get_clock();
  double comAmount = 1E7;
  std::vector<double> cpu_amounts3{flopAmount, flopAmount};
  std::vector<double> com_amounts3{0, comAmount, comAmount, 0};
  XBT_INFO("Run a task with computation and communication on two hosts.");
  sg4::this_actor::parallel_execute(hosts, cpu_amounts3, com_amounts3);

  new_energy_host1 = sg_host_get_consumed_energy(host1);
  new_energy_host2 = sg_host_get_consumed_energy(host2);
  XBT_INFO(
      "Task done (duration: %.2f s).\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f "
      "J\n",
      sg4::Engine::get_clock() - start, host1->get_cname(), host1->get_speed(), (new_energy_host1 - old_energy_host1),
      sg_host_get_consumed_energy(host1), host2->get_cname(), host2->get_speed(), (new_energy_host2 - old_energy_host2),
      sg_host_get_consumed_energy(host2));

  old_energy_host1 = new_energy_host1;
  old_energy_host2 = new_energy_host2;


  // ========= A new ptask with communication only =========
  start = sg4::Engine::get_clock();
  std::vector<double> cpu_amounts4{0, 0};
  std::vector<double> com_amounts4{0, comAmount, comAmount, 0};
  XBT_INFO("Run a task with only communication on two hosts.");
  sg4::this_actor::parallel_execute(hosts, cpu_amounts4, com_amounts4);

  new_energy_host1 = sg_host_get_consumed_energy(host1);
  new_energy_host2 = sg_host_get_consumed_energy(host2);
  XBT_INFO(
      "Task done (duration: %.2f s).\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f "
      "J\n",
      sg4::Engine::get_clock() - start, host1->get_cname(), host1->get_speed(), (new_energy_host1 - old_energy_host1),
      sg_host_get_consumed_energy(host1), host2->get_cname(), host2->get_speed(), (new_energy_host2 - old_energy_host2),
      sg_host_get_consumed_energy(host2));

  old_energy_host1 = new_energy_host1;
  old_energy_host2 = new_energy_host2;

  // ========= A new ptask with computation and a timeout =========
  start = sg4::Engine::get_clock();
  std::vector<double> cpu_amounts5{flopAmount, flopAmount};
  std::vector<double> com_amounts5{0, 0, 0, 0};
  XBT_INFO("Run a task with computation on two hosts and a timeout of 20s.");
  try {
    sg4::this_actor::exec_init(hosts, cpu_amounts5, com_amounts5)->wait_for(20);
  } catch (const simgrid::TimeoutException &){
    XBT_INFO("Finished WITH timeout");
  }

  new_energy_host1 = sg_host_get_consumed_energy(host1);
  new_energy_host2 = sg_host_get_consumed_energy(host2);
  XBT_INFO(
      "Task ended (duration: %.2f s).\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
      "[%s] Current peak speed=%.0E flop/s; Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f "
      "J\n",
      sg4::Engine::get_clock() - start, host1->get_cname(), host1->get_speed(), (new_energy_host1 - old_energy_host1),
      sg_host_get_consumed_energy(host1), host2->get_cname(), host2->get_speed(), (new_energy_host2 - old_energy_host2),
      sg_host_get_consumed_energy(host2));

  XBT_INFO("Now is time to quit!");
}

int main(int argc, char* argv[])
{
  sg_host_energy_plugin_init();
  sg4::Engine e(&argc, argv);
  sg4::Engine::set_config("host/model:ptask_L07");

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/energy_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);
  sg4::Actor::create("energy_ptask_test", e.host_by_name("MyHost1"), runner);

  e.run();
  XBT_INFO("End of simulation.");
  return 0;
}
