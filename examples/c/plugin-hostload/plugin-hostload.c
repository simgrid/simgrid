/* Copyright (c) 2007-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/plugins/load.h"

#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(hostload_test, "Messages specific for this example");

static void execute_load_test(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  sg_host_t host = sg_host_by_name("MyHost1");

  XBT_INFO("Initial peak speed: %.0E flop/s; number of flops computed so far: %.0E (should be 0) and current average "
           "load: %.5f (should be 0)",
           sg_host_speed(host), sg_host_get_computed_flops(host), sg_host_get_avg_load(host));

  double start = simgrid_get_clock();
  XBT_INFO("Sleep for 10 seconds");
  sg_actor_sleep_for(10);

  double speed = sg_host_speed(host);
  XBT_INFO("Done sleeping %.2fs; peak speed: %.0E flop/s; number of flops computed so far: %.0E (nothing should have "
           "changed)",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_computed_flops(host));

  // Run a task
  start = simgrid_get_clock();
  XBT_INFO("Run a task of %.0E flops at current speed of %.0E flop/s", 200e6, sg_host_speed(host));
  sg_actor_execute(200e6);

  XBT_INFO("Done working on my task; this took %.2fs; current peak speed: %.0E flop/s (when I started the computation, "
           "the speed was set to %.0E flop/s); number of flops computed so "
           "far: %.2E, average load as reported by the HostLoad plugin: %.5f (should be %.5f)",
           simgrid_get_clock() - start, sg_host_speed(host), speed, sg_host_get_computed_flops(host),
           sg_host_get_avg_load(host),
           200E6 / (10.5 * speed * sg_host_core_count(host) +
                    (simgrid_get_clock() - start - 0.5) * sg_host_speed(host) * sg_host_core_count(host)));

  // ========= Change power peak =========
  int pstate = 1;
  sg_host_set_pstate(host, pstate);
  XBT_INFO(
      "========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s, average load is %.5f)",
      pstate, sg_host_get_pstate_speed(host, pstate), sg_host_speed(host), sg_host_get_avg_load(host));

  // Run a second task
  start = simgrid_get_clock();
  XBT_INFO("Run a task of %.0E flops", 100e6);
  sg_actor_execute(100e6);
  XBT_INFO("Done working on my task; this took %.2fs; current peak speed: %.0E flop/s; number of flops computed so "
           "far: %.2E",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_computed_flops(host));

  start = simgrid_get_clock();
  XBT_INFO("========= Requesting a reset of the computation and load counters");
  sg_host_load_reset(host);
  XBT_INFO("After reset: %.0E flops computed; load is %.5f", sg_host_get_computed_flops(host),
           sg_host_get_avg_load(host));
  XBT_INFO("Sleep for 4 seconds");
  sg_actor_sleep_for(4);
  XBT_INFO("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_computed_flops(host));

  // =========== Turn the other host off ==========
  XBT_INFO("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 computed %.0f flops so far and has an "
           "average load of %.5f.",
           sg_host_get_computed_flops(sg_host_by_name("MyHost2")), sg_host_get_avg_load(sg_host_by_name("MyHost2")));
  sg_host_turn_off(sg_host_by_name("MyHost2"));
  start = simgrid_get_clock();
  sg_actor_sleep_for(10);
  XBT_INFO("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_computed_flops(host));
}

static void change_speed(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  sg_host_t host = sg_host_by_name("MyHost1");
  sg_actor_sleep_for(10.5);
  XBT_INFO("I slept until now, but now I'll change the speed of this host "
           "while the other process is still computing! This should slow the computation down.");
  sg_host_set_pstate(host, 2);
}

int main(int argc, char* argv[])
{
  sg_host_load_plugin_init();
  simgrid_init(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);
  sg_actor_t actor = sg_actor_init("load_test", sg_host_by_name("MyHost1"));
  sg_actor_start(actor, execute_load_test, 0, NULL);

  sg_actor_t actor2 = sg_actor_init("change_speed", sg_host_by_name("MyHost1"));
  sg_actor_start(actor2, change_speed, 0, NULL);

  simgrid_run();

  XBT_INFO("Total simulation time: %.2f", simgrid_get_clock());

  return 0;
}
