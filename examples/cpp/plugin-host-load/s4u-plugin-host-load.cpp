/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/load.h"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void execute_load_test()
{
  s4u_Host* host = sg4::Host::by_name("MyHost1");

  XBT_INFO("Initial peak speed: %.0E flop/s; number of flops computed so far: %.0E (should be 0) and current average "
           "load: %.5f (should be 0)",
           host->get_speed(), sg_host_get_computed_flops(host), sg_host_get_avg_load(host));

  double start = sg4::Engine::get_clock();
  XBT_INFO("Sleep for 10 seconds");
  sg4::this_actor::sleep_for(10);

  double speed = host->get_speed();
  XBT_INFO("Done sleeping %.2fs; peak speed: %.0E flop/s; number of flops computed so far: %.0E (nothing should have "
           "changed)",
           sg4::Engine::get_clock() - start, host->get_speed(), sg_host_get_computed_flops(host));

  // Run an activity
  start = sg4::Engine::get_clock();
  XBT_INFO("Run an activity of %.0E flops at current speed of %.0E flop/s", 200E6, host->get_speed());
  sg4::this_actor::execute(200E6);

  XBT_INFO(
      "Done working on my activity; this took %.2fs; current peak speed: %.0E flop/s (when I started the computation, "
      "the speed was set to %.0E flop/s); number of flops computed so "
      "far: %.2E, average load as reported by the HostLoad plugin: %.5f (should be %.5f)",
      sg4::Engine::get_clock() - start, host->get_speed(), speed, sg_host_get_computed_flops(host),
      sg_host_get_avg_load(host),
      200E6 / (10.5 * speed * host->get_core_count() +
               (sg4::Engine::get_clock() - start - 0.5) * host->get_speed() * host->get_core_count()));

  // ========= Change power peak =========
  int pstate = 1;
  host->set_pstate(pstate);
  XBT_INFO(
      "========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s, average load is %.5f)",
      pstate, host->get_pstate_speed(pstate), host->get_speed(), sg_host_get_avg_load(host));

  // Run a second activity
  start = sg4::Engine::get_clock();
  XBT_INFO("Run an activity of %.0E flops", 100E6);
  sg4::this_actor::execute(100E6);
  XBT_INFO("Done working on my activity; this took %.2fs; current peak speed: %.0E flop/s; number of flops computed so "
           "far: %.2E",
           sg4::Engine::get_clock() - start, host->get_speed(), sg_host_get_computed_flops(host));

  start = sg4::Engine::get_clock();
  XBT_INFO("========= Requesting a reset of the computation and load counters");
  sg_host_load_reset(host);
  XBT_INFO("After reset: %.0E flops computed; load is %.5f", sg_host_get_computed_flops(host),
           sg_host_get_avg_load(host));
  XBT_INFO("Sleep for 4 seconds");
  sg4::this_actor::sleep_for(4);
  XBT_INFO("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
           sg4::Engine::get_clock() - start, host->get_speed(), sg_host_get_computed_flops(host));

  // =========== Turn the other host off ==========
  s4u_Host* host2 = sg4::Host::by_name("MyHost2");
  XBT_INFO("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 computed %.0f flops so far and has an "
           "average load of %.5f.",
           sg_host_get_computed_flops(host2), sg_host_get_avg_load(host2));
  host2->turn_off();
  start = sg4::Engine::get_clock();
  sg4::this_actor::sleep_for(10);
  XBT_INFO("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
           sg4::Engine::get_clock() - start, host->get_speed(), sg_host_get_computed_flops(host));
}

static void change_speed()
{
  s4u_Host* host = sg4::Host::by_name("MyHost1");
  sg4::this_actor::sleep_for(10.5);
  XBT_INFO("I slept until now, but now I'll change the speed of this host "
           "while the other actor is still computing! This should slow the computation down.");
  host->set_pstate(2);
}

int main(int argc, char* argv[])
{
  sg_host_load_plugin_init();
  sg4::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/energy_platform.xml\n", argv[0], argv[0]);
  e.load_platform(argv[1]);

  e.host_by_name("MyHost1")->add_actor("load_test", execute_load_test);
  e.host_by_name("MyHost1")->add_actor("change_speed", change_speed);

  e.run();

  XBT_INFO("Total simulation time: %.2f", e.get_clock());

  return 0;
}
