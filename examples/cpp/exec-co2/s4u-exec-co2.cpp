/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/carbon_footprint.h"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void test_execution()
{
  sg4::Host* host = sg4::this_actor::get_host();

  double host_speed_in_flops = host->get_speed();

  // Run one task (total 1 CPU core) for 1 hour
  double flopAmount_1_hour = host_speed_in_flops * 3600 * 1;

  sg4::ExecPtr activity = sg4::this_actor::exec_init(flopAmount_1_hour);
  activity->start();
  activity->wait();

  // Run two tasks (total of 2 CPU cores) for 2 hours
  sg4::ExecPtr activity_2_1 = sg4::this_actor::exec_init(flopAmount_1_hour * 2);
  sg4::ExecPtr activity_2_2 = sg4::this_actor::exec_init(flopAmount_1_hour * 2);

  activity_2_1->start();
  activity_2_2->start();

  activity_2_1->wait();
  activity_2_2->wait();

  // Run three tasks (total of 3 CPU cores) for 3 hours
  sg4::ExecPtr activity_3_1 = sg4::this_actor::exec_init(flopAmount_1_hour * 3);
  sg4::ExecPtr activity_3_2 = sg4::this_actor::exec_init(flopAmount_1_hour * 3);
  sg4::ExecPtr activity_3_3 = sg4::this_actor::exec_init(flopAmount_1_hour * 3);

  activity_3_1->start();
  activity_3_2->start();
  activity_3_3->start();

  activity_3_1->wait();
  activity_3_2->wait();
  activity_3_3->wait();

  // Run four tasks (total of 4 CPU cores) for 4 hours
  sg4::ExecPtr activity_4_1 = sg4::this_actor::exec_init(flopAmount_1_hour * 4);
  sg4::ExecPtr activity_4_2 = sg4::this_actor::exec_init(flopAmount_1_hour * 4);
  sg4::ExecPtr activity_4_3 = sg4::this_actor::exec_init(flopAmount_1_hour * 4);
  sg4::ExecPtr activity_4_4 = sg4::this_actor::exec_init(flopAmount_1_hour * 4);

  activity_4_1->start();
  activity_4_2->start();
  activity_4_3->start();
  activity_4_4->start();

  activity_4_1->wait();
  activity_4_2->wait();
  activity_4_3->wait();
  activity_4_4->wait();
}

int main(int argc, char* argv[])
{
  sg_host_carbon_footprint_plugin_init();
  sg_host_carbon_footprint_load_trace_file("co2.csv");

  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Host* host_br_static_co2  = sg4::Host::by_name("host_br_static_co2");
  sg4::Host* host_usa_static_co2 = sg4::Host::by_name("host_usa_static_co2");

  sg4::Host* host_br_dynamic_co2  = sg4::Host::by_name("host_br_dynamic_co2");
  sg4::Host* host_usa_dynamic_co2 = sg4::Host::by_name("host_usa_dynamic_co2");

  host_br_static_co2->add_actor("execution_static_br", test_execution);
  host_br_dynamic_co2->add_actor("execution_dynamic_br", test_execution);

  host_br_static_co2->add_actor(
      "turn_off usa static", [](sg4::Host* h) { h->turn_off(); }, host_usa_static_co2);
  host_br_dynamic_co2->add_actor(
      "turn_off usa dynamic", [](sg4::Host* h) { h->turn_off(); }, host_usa_dynamic_co2);

  e.run();

  return 0;
}
