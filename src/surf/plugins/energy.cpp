/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <utility>
#include <vector>

#include "simgrid/plugins/energy.h"
#include "simgrid/simix.hpp"
#include "src/surf/plugins/energy.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/virtual_machine.hpp"

/** @addtogroup SURF_plugin_energy


This is the energy plugin, enabling to account not only for computation time,
but also for the dissipated energy in the simulated platform.

The energy consumption of a CPU depends directly of its current load. Specify that consumption in your platform file as follows:

\verbatim
<host id="HostA" power="100.0Mf" >
    <prop id="watt_per_state" value="100.0:200.0" />
    <prop id="watt_off" value="10" />
</host>
\endverbatim

The first property means that when your host is up and running, but without anything to do, it will dissipate 100 Watts.
If it's fully loaded, it will dissipate 200 Watts. If its load is at 50%, then it will dissipate 150 Watts.
The second property means that when your host is turned off, it will dissipate only 10 Watts (please note that these values are arbitrary).

If your CPU is using pstates, then you can provide one consumption interval per pstate.

\verbatim
<host id="HostB" power="100.0Mf,50.0Mf,20.0Mf" pstate="0" >
    <prop id="watt_per_state" value="95.0:200.0, 93.0:170.0, 90.0:150.0" />
    <prop id="watt_off" value="10" />
</host>
\endverbatim

That host has 3 levels of performance with the following performance: 100 Mflop/s, 50 Mflop/s or 20 Mflop/s.
It starts at pstate 0 (ie, at 100 Mflop/s). In this case, you have to specify one interval per pstate in the watt_per_state property.
In this example, the idle consumption is 95 Watts, 93 Watts and 90 Watts in each pstate while the CPU burn consumption are at 200 Watts,
170 Watts and 150 Watts respectively.

To change the pstate of a given CPU, use the following functions: #MSG_host_get_nb_pstates(), simgrid#s4u#Host#set_pstate(), #MSG_host_get_power_peak_at().

To simulate the energy-related elements, first call the simgrid#energy#sg_energy_plugin_init() before your #MSG_init(),
and then use the following function to retrieve the consumption of a given host: MSG_host_get_consumed_energy().
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_energy, surf,
                                "Logging specific to the SURF energy plugin");

using simgrid::energy::HostEnergy;

namespace simgrid {
namespace energy {

simgrid::xbt::Extension<simgrid::s4u::Host, HostEnergy> HostEnergy::EXTENSION_ID;

/* Computes the consumption so far.  Called lazily on need. */
void HostEnergy::update()
{
  simgrid::surf::HostImpl* surf_host = host->extension<simgrid::surf::HostImpl>();
  double start_time = this->last_updated;
  double finish_time = surf_get_clock();
  double cpu_load;
  if (surf_host->p_cpu->speed_.peak == 0)
    // Some users declare a pstate of speed 0 flops (eg to model boot time).
    // We consider that the machine is then fully loaded. That's arbitrary but it avoids a NaN
    cpu_load = 1;
  else
    cpu_load = lmm_constraint_get_usage(surf_host->p_cpu->getConstraint())
                / surf_host->p_cpu->speed_.peak;

  if (cpu_load > 1) // A machine with a load > 1 consumes as much as a fully loaded machine, not mores
    cpu_load = 1;

  double previous_energy = this->total_energy;

  double instantaneous_consumption;
  if (host->isOff())
    instantaneous_consumption = this->watts_off;
  else
    instantaneous_consumption = this->getCurrentWattsValue(cpu_load);

  double energy_this_step = instantaneous_consumption*(finish_time-start_time);

  this->total_energy = previous_energy + energy_this_step;
  this->last_updated = finish_time;

  XBT_DEBUG("[update_energy of %s] period=[%.2f-%.2f]; current power peak=%.0E flop/s; consumption change: %.2f J -> %.2f J",
      surf_host->getName(), start_time, finish_time, surf_host->p_cpu->speed_.peak, previous_energy, energy_this_step);
}

HostEnergy::HostEnergy(simgrid::s4u::Host *ptr) :
  host(ptr), last_updated(surf_get_clock())
{
  initWattsRangeList();

  if (host->properties() != NULL) {
    char* off_power_str = (char*)xbt_dict_get_or_null(host->properties(), "watt_off");
    if (off_power_str != NULL)
      watts_off = xbt_str_parse_double(off_power_str,
          bprintf("Invalid value for property watt_off of host %s: %%s",host->name().c_str()));
    else
      watts_off = 0;
  }

}

HostEnergy::~HostEnergy()
{
}

double HostEnergy::getWattMinAt(int pstate)
{
  xbt_assert(!power_range_watts_list.empty(),
    "No power range properties specified for host %s", host->name().c_str());
  return power_range_watts_list[pstate].first;
}

double HostEnergy::getWattMaxAt(int pstate)
{
  xbt_assert(!power_range_watts_list.empty(),
    "No power range properties specified for host %s", host->name().c_str());
  return power_range_watts_list[pstate].second;
}

/** @brief Computes the power consumed by the host according to the current pstate and processor load */
double HostEnergy::getCurrentWattsValue(double cpu_load)
{
  xbt_assert(!power_range_watts_list.empty(),
    "No power range properties specified for host %s", host->name().c_str());

  /* min_power corresponds to the idle power (cpu load = 0) */
  /* max_power is the power consumed at 100% cpu load       */
  auto range = power_range_watts_list.at(host->pstate());
  double min_power = range.first;
  double max_power = range.second;
  double power_slope = max_power - min_power;
  double current_power = min_power + cpu_load * power_slope;

  XBT_DEBUG("[get_current_watts] min_power=%f, max_power=%f, slope=%f", min_power, max_power, power_slope);
  XBT_DEBUG("[get_current_watts] Current power (watts) = %f, load = %f", current_power, cpu_load);

  return current_power;
}

double HostEnergy::getConsumedEnergy()
{
  if (last_updated < surf_get_clock()) // We need to simcall this as it modifies the environment
    simgrid::simix::kernel(std::bind(&HostEnergy::update, this));

  return total_energy;
}

void HostEnergy::initWattsRangeList()
{
  if (host->properties() == NULL)
    return;
  char* all_power_values_str =
    (char*)xbt_dict_get_or_null(host->properties(), "watt_per_state");
  if (all_power_values_str == NULL)
    return;

  xbt_dynar_t all_power_values = xbt_str_split(all_power_values_str, ",");
  int pstate_nb = xbt_dynar_length(all_power_values);

  for (int i=0; i< pstate_nb; i++)
  {
    /* retrieve the power values associated with the current pstate */
    xbt_dynar_t current_power_values = xbt_str_split(xbt_dynar_get_as(all_power_values, i, char*), ":");
    xbt_assert(xbt_dynar_length(current_power_values) > 1,
        "Power properties incorrectly defined - "
        "could not retrieve min and max power values for host %s",
        host->name().c_str());

    /* min_power corresponds to the idle power (cpu load = 0) */
    /* max_power is the power consumed at 100% cpu load       */
    power_range_watts_list.push_back(power_range(
      xbt_str_parse_double(xbt_dynar_get_as(current_power_values, 0, char*),
          bprintf("Invalid min value for pstate %d on host %s: %%s", i, host->name().c_str())),
      xbt_str_parse_double(xbt_dynar_get_as(current_power_values, 1, char*),
          bprintf("Invalid min value for pstate %d on host %s: %%s", i, host->name().c_str()))
    ));

    xbt_dynar_free(&current_power_values);
  }
  xbt_dynar_free(&all_power_values);
}

}
}

/* **************************** events  callback *************************** */
static void onCreation(simgrid::s4u::Host& host) {
  simgrid::surf::HostImpl* surf_host = host.extension<simgrid::surf::HostImpl>();
  if (dynamic_cast<simgrid::surf::VirtualMachine*>(surf_host)) // Ignore virtual machines
    return;
  host.extension_set(new HostEnergy(&host));
}

static void onActionStateChange(simgrid::surf::CpuAction *action, e_surf_action_state_t previous) {
  const char *name = getActionCpu(action)->getName();
  simgrid::surf::HostImpl *host = sg_host_by_name(name)->extension<simgrid::surf::HostImpl>();
  simgrid::surf::VirtualMachine *vm = dynamic_cast<simgrid::surf::VirtualMachine*>(host);
  if (vm) // If it's a VM, take the corresponding PM
    host = vm->getPm()->extension<simgrid::surf::HostImpl>();

  HostEnergy *host_energy = host->p_host->extension<HostEnergy>();

  if(host_energy->last_updated < surf_get_clock())
    host_energy->update();
}

static void onHostStateChange(simgrid::s4u::Host &host) {
  simgrid::surf::HostImpl* surf_host = host.extension<simgrid::surf::HostImpl>();
  if (dynamic_cast<simgrid::surf::VirtualMachine*>(surf_host)) // Ignore virtual machines
    return;

  HostEnergy *host_energy = host.extension<HostEnergy>();

  if(host_energy->last_updated < surf_get_clock())
    host_energy->update();
}

static void onHostDestruction(simgrid::s4u::Host& host) {
  // Ignore virtual machines
  simgrid::surf::HostImpl* surf_host = host.extension<simgrid::surf::HostImpl>();
  if (dynamic_cast<simgrid::surf::VirtualMachine*>(surf_host))
    return;
  HostEnergy *host_energy = host.extension<HostEnergy>();
  host_energy->update();
  XBT_INFO("Total energy of host %s: %f Joules",
    host.name().c_str(), host_energy->getConsumedEnergy());
}

/* **************************** Public interface *************************** */
/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. You should call this function before #MSG_init().
 */
void sg_energy_plugin_init(void)
{
  if (HostEnergy::EXTENSION_ID.valid())
    return;

  HostEnergy::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostEnergy>();

  simgrid::s4u::Host::onCreation.connect(&onCreation);
  simgrid::s4u::Host::onStateChange.connect(&onHostStateChange);
  simgrid::s4u::Host::onDestruction.connect(&onHostDestruction);
  simgrid::surf::CpuAction::onStateChange.connect(&onActionStateChange);
}

/** @brief Returns the total energy consumed by the host so far (in Joules)
 *
 *  See also @ref SURF_plugin_energy.
 */
double sg_host_get_consumed_energy(sg_host_t host) {
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
    "The Energy plugin is not active. "
    "Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getConsumedEnergy();
}

/** @brief Get the amount of watt dissipated at the given pstate when the host is idling */
double sg_host_get_wattmin_at(sg_host_t host, int pstate) {
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
    "The Energy plugin is not active. "
    "Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getWattMinAt(pstate);
}
/** @brief  Returns the amount of watt dissipated at the given pstate when the host burns CPU at 100% */
double sg_host_get_wattmax_at(sg_host_t host, int pstate) {
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
    "The Energy plugin is not active. "
    "Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getWattMaxAt(pstate);
}
