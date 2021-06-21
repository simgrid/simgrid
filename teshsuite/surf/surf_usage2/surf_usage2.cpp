/* A few basic tests for the surf library                                   */

/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/host.h"
#include "simgrid/kernel/routing/NetZoneImpl.hpp" // full type for NetZoneImpl object
#include "simgrid/zone.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"
#include "xbt/config.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");

int main(int argc, char** argv)
{
  int running;

  surf_init(&argc, argv); /* Initialize some common structures */

  simgrid::config::set_parse("network/model:CM02");
  simgrid::config::set_parse("cpu/model:Cas01");

  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  parse_platform_file(argv[1]);

  /*********************** HOST ***********************************/
  simgrid::s4u::Host* hostA = sg_host_by_name("Cpu A");
  simgrid::s4u::Host* hostB = sg_host_by_name("Cpu B");

  /* Let's do something on it */
  hostA->get_cpu()->execution_start(1000.0);
  hostB->get_cpu()->execution_start(1000.0);
  hostB->get_cpu()->sleep(7.32);

  const_sg_netzone_t as_zone = sg_zone_get_by_name("AS0");
  auto net_model             = as_zone->get_impl()->get_network_model();
  net_model->communicate(hostA, hostB, 150.0, -1.0);

  surf_solve(-1.0); /* Takes traces into account. Returns 0.0 */
  do {
    simgrid::kernel::resource::Action* action = nullptr;
    running                                   = 0;

    double now = surf_get_clock();
    XBT_INFO("Next Event : %g", now);

    for (auto const& model : simgrid::kernel::EngineImpl::get_instance()->get_all_models()) {
      if (not model->get_started_action_set()->empty()) {
        XBT_DEBUG("\t Running that model");
        running = 1;
      }

      action = model->extract_failed_action();
      while (action != nullptr) {
        XBT_INFO("   * Done Action");
        XBT_DEBUG("\t * Failed Action: %p", action);
        action->unref();
        action = model->extract_failed_action();
      }

      action = model->extract_done_action();
      while (action != nullptr) {
        XBT_INFO("   * Done Action");
        XBT_DEBUG("\t * Done Action: %p", action);
        action->unref();
        action = model->extract_done_action();
      }
    }
  } while (running && surf_solve(-1.0) >= 0.0);

  XBT_INFO("Simulation Terminated");
  return 0;
}
