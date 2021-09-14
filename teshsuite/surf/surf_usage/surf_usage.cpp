/* A few basic tests for the surf library                                   */

/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/host.h"
#include "simgrid/kernel/routing/NetZoneImpl.hpp" // full type for NetZoneImpl object
#include "simgrid/zone.h"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "surf/surf.hpp"
#include "xbt/config.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");

static const char* string_action(simgrid::kernel::resource::Action::State state)
{
  switch (state) {
    case simgrid::kernel::resource::Action::State::INITED:
      return "SURF_ACTION_INITED";
    case simgrid::kernel::resource::Action::State::STARTED:
      return "SURF_ACTION_RUNNING";
    case simgrid::kernel::resource::Action::State::FAILED:
      return "SURF_ACTION_FAILED";
    case simgrid::kernel::resource::Action::State::FINISHED:
      return "SURF_ACTION_DONE";
    case simgrid::kernel::resource::Action::State::IGNORED:
      return "SURF_ACTION_IGNORED";
    default:
      return "INVALID STATE";
  }
}

int main(int argc, char** argv)
{
  surf_init(&argc, argv); /* Initialize some common structures */
  simgrid::config::set_parse("cpu/model:Cas01");
  simgrid::config::set_parse("network/model:CM02");

  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  parse_platform_file(argv[1]);

  const_sg_netzone_t as_zone = sg_zone_get_by_name("AS0");
  auto net_model             = as_zone->get_impl()->get_network_model();
  auto cpu_model_pm          = as_zone->get_impl()->get_cpu_pm_model();

  XBT_DEBUG("CPU model: %p", cpu_model_pm.get());
  XBT_DEBUG("Network model: %p", net_model.get());
  simgrid::s4u::Host* hostA = sg_host_by_name("Cpu A");
  simgrid::s4u::Host* hostB = sg_host_by_name("Cpu B");

  /* Let's do something on it */
  const simgrid::kernel::resource::Action* actionA = hostA->get_cpu()->execution_start(1000.0, -1);
  const simgrid::kernel::resource::Action* actionB = hostB->get_cpu()->execution_start(1000.0, -1);
  const simgrid::kernel::resource::Action* actionC = hostB->get_cpu()->sleep(7.32);

  simgrid::kernel::resource::Action::State stateActionA = actionA->get_state();
  simgrid::kernel::resource::Action::State stateActionB = actionB->get_state();
  simgrid::kernel::resource::Action::State stateActionC = actionC->get_state();

  /* And just look at the state of these tasks */
  XBT_INFO("actionA state: %s", string_action(stateActionA));
  XBT_INFO("actionB state: %s", string_action(stateActionB));
  XBT_INFO("actionC state: %s", string_action(stateActionC));

  /* Let's do something on it */
  net_model->communicate(hostA, hostB, 150.0, -1.0);

  surf_solve(-1.0);
  do {
    XBT_INFO("Next Event : %g", surf_get_clock());
    XBT_DEBUG("\t CPU actions");

    simgrid::kernel::resource::Action::StateSet* action_list = cpu_model_pm->get_failed_action_set();
    while (not action_list->empty()) {
      simgrid::kernel::resource::Action& action = action_list->front();
      XBT_INFO("   CPU Failed action");
      XBT_DEBUG("\t * Failed : %p", &action);
      action.unref();
    }

    action_list = cpu_model_pm->get_finished_action_set();
    while (not action_list->empty()) {
      simgrid::kernel::resource::Action& action = action_list->front();
      XBT_INFO("   CPU Done action");
      XBT_DEBUG("\t * Done : %p", &action);
      action.unref();
    }

    action_list = net_model->get_failed_action_set();
    while (not action_list->empty()) {
      simgrid::kernel::resource::Action& action = action_list->front();
      XBT_INFO("   Network Failed action");
      XBT_DEBUG("\t * Failed : %p", &action);
      action.unref();
    }

    action_list = net_model->get_finished_action_set();
    while (not action_list->empty()) {
      simgrid::kernel::resource::Action& action = action_list->front();
      XBT_INFO("   Network Done action");
      XBT_DEBUG("\t * Done : %p", &action);
      action.unref();
    }
  } while ((net_model->get_started_action_set()->size() || cpu_model_pm->get_started_action_set()->size()) &&
           surf_solve(-1.0) >= 0.0);

  XBT_DEBUG("Simulation Terminated");

  return 0;
}
