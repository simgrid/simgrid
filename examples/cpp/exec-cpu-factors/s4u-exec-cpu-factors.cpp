/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to simulate variability for CPUs, using multiplicative factors
 */

#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

/*************************************************************************************************/
static void runner()
{
  double computation_amount = sg4::this_actor::get_host()->get_speed();

  XBT_INFO("Executing 1 tasks of %g flops, should take 1 second.", computation_amount);
  sg4::this_actor::execute(computation_amount);
  XBT_INFO("Executing 1 tasks of %g flops, it would take .001s without factor. It'll take .002s",
           computation_amount / 10);
  sg4::this_actor::execute(computation_amount / 1000);

  XBT_INFO("Finished executing. Goodbye now!");
}
/*************************************************************************************************/
/** @brief Variability for CPU */
static double cpu_variability(const sg4::Host* host, double flops)
{
  /* creates variability for tasks smaller than 1% of CPU power.
   * unrealistic, for learning purposes */
  double factor = 1.0;
  double speed  = host->get_speed();
  if (flops < speed / 100) {
    factor = 0.5;
  }
  XBT_INFO("Host %s, task with %lf flops, new factor %lf", host->get_cname(), flops, factor);
  return factor;
}

/** @brief Create a simple 1-host platform */
static void load_platform()
{
  auto* zone        = sg4::create_empty_zone("Zone1");
  auto* runner_host = zone->create_host("runner", 1e6);
  runner_host->set_factor_cb(std::bind(&cpu_variability, runner_host, std::placeholders::_1))->seal();
  zone->seal();

  /* create actor runner */
  sg4::Actor::create("runner", runner_host, runner);
}

/*************************************************************************************************/
int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  /* create platform */
  load_platform();

  /* runs the simulation */
  e.run();

  return 0;
}
