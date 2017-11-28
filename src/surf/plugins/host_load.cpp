/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/load.h"
#include "simgrid/simix.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/surf/cpu_interface.hpp"

#include "simgrid/s4u/Engine.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <utility>
#include <vector>

/** @addtogroup plugin_load

This plugin makes it very simple for users to obtain the current load for each host.

*/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_plugin_load, surf, "Logging specific to the HostLoad plugin");

namespace simgrid {
namespace plugin {

class HostLoad {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, HostLoad> EXTENSION_ID;

  explicit HostLoad(simgrid::s4u::Host* ptr);
  ~HostLoad();

  double getCurrentLoad();
  double getComputedFlops();
  double getAverageLoad();
  void update();
  void reset();

private:
  simgrid::s4u::Host* host = nullptr;
  double last_updated      = 0;
  double last_reset        = 0;
  double current_flops     = 0;
  double computed_flops    = 0;
};

simgrid::xbt::Extension<simgrid::s4u::Host, HostLoad> HostLoad::EXTENSION_ID;

HostLoad::HostLoad(simgrid::s4u::Host* ptr)
    : host(ptr)
    , last_updated(surf_get_clock())
    , last_reset(surf_get_clock())
    , current_flops(host->pimpl_cpu->constraint()->get_usage())
{
}

HostLoad::~HostLoad() = default;

void HostLoad::update()
{
  double now = surf_get_clock();
  if (last_updated < now) {
    /* Current flop per second computed by the cpu; current_flops = k * pstate_speed_in_flops, k \in {0, 1, ..., cores}
     * number of active cores */
    current_flops = host->pimpl_cpu->constraint()->get_usage();

    /* flops == pstate_speed * cores_being_currently_used */
    computed_flops += (now - last_updated) * current_flops;
    last_updated = now;
  }
}

double HostLoad::getCurrentLoad()
{
  return current_flops / (host->getSpeed() * host->getCoreCount());
}

double HostLoad::getAverageLoad()
{
  return getComputedFlops() / (host->getSpeed() * host->getCoreCount() * (surf_get_clock() - last_reset));
}

double HostLoad::getComputedFlops()
{
  update();

  return computed_flops;
}

/*
 * Resets the counters
 */
void HostLoad::reset()
{
  last_updated   = surf_get_clock();
  last_reset     = surf_get_clock();
  computed_flops = 0;
}
}
}

using simgrid::plugin::HostLoad;

/* **************************** events  callback *************************** */
/* This callback is fired either when the host changes its state (on/off) or its speed
 * (because the user changed the pstate, or because of external trace events) */
static void onHostChange(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  host.extension<HostLoad>()->update();
}

/* This callback is called when an action (computation, idle, ...) terminates */
static void onActionStateChange(simgrid::surf::CpuAction* action, simgrid::surf::Action::State /*previous*/)
{
  for (simgrid::surf::Cpu* const& cpu : action->cpus()) {
    simgrid::s4u::Host* host = cpu->getHost();

    if (host != nullptr) {
      // Get the host_load extension for the relevant host
      HostLoad* host_load = host->extension<HostLoad>();
      host_load->update();
    }
  }
}

/* **************************** Public interface *************************** */
extern "C" {

/** \ingroup plugin_load
 * \brief Initializes the HostLoad plugin
 * \details The HostLoad plugin provides an API to get the current load of each host.
 */
void sg_host_load_plugin_init()
{
  if (HostLoad::EXTENSION_ID.valid())
    return;

  HostLoad::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostLoad>();

  /* When attaching a callback into a signal, you can use a lambda as follows, or a regular function as done below */

  simgrid::s4u::Host::onCreation.connect([](simgrid::s4u::Host& host) {
    if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
      return;
    host.extension_set(new HostLoad(&host));
  });

  simgrid::surf::CpuAction::onStateChange.connect(&onActionStateChange);
  simgrid::s4u::Host::onStateChange.connect(&onHostChange);
  simgrid::s4u::Host::onSpeedChange.connect(&onHostChange);
}

/** @brief Returns the current load of the host passed as argument
 *
 *  See also @ref plugin_load
 */
double sg_host_get_current_load(sg_host_t host)
{
  xbt_assert(HostLoad::EXTENSION_ID.valid(),
             "The Load plugin is not active. Please call sg_host_load_plugin_init() during initialization.");

  return host->extension<HostLoad>()->getCurrentLoad();
}

double sg_host_get_computed_flops(sg_host_t host)
{
  xbt_assert(HostLoad::EXTENSION_ID.valid(),
             "The Load plugin is not active. Please call sg_host_load_plugin_init() during initialization.");

  return host->extension<HostLoad>()->getComputedFlops();
}

void sg_host_load_reset(sg_host_t host)
{
  xbt_assert(HostLoad::EXTENSION_ID.valid(),
             "The Load plugin is not active. Please call sg_host_load_plugin_init() during initialization.");

  host->extension<HostLoad>()->reset();
}
}
