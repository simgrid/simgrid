/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/energy.h"
#include "simgrid/simix.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/surf/cpu_interface.hpp"

#include "simgrid/s4u/Engine.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <utility>
#include <vector>

/** @addtogroup SURF_plugin_energy


This is the energy plugin, enabling to account not only for computation time,
but also for the dissipated energy in the simulated platform.

The energy consumption of a CPU depends directly of its current load. Specify that consumption in your platform file as
follows:

\verbatim
<host id="HostA" power="100.0Mf" cores="8">
    <prop id="watt_per_state" value="100.0:120.0:200.0" />
    <prop id="watt_off" value="10" />
</host>
\endverbatim

The first property means that when your host is up and running, but without anything to do, it will dissipate 100 Watts.
If only one care is active, it will dissipate 120 Watts. If it's fully loaded, it will dissipate 200 Watts. If its load is at 50%, then it will dissipate 153.33 Watts.
The second property means that when your host is turned off, it will dissipate only 10 Watts (please note that these
values are arbitrary).

If your CPU is using pstates, then you can provide one consumption interval per pstate.

\verbatim
<host id="HostB" power="100.0Mf,50.0Mf,20.0Mf" pstate="0" >
    <prop id="watt_per_state" value="95.0:120.0:200.0, 93.0:115.0:170.0, 90.0:110.0:150.0" />
    <prop id="watt_off" value="10" />
</host>
\endverbatim

That host has 3 levels of performance with the following performance: 100 Mflop/s, 50 Mflop/s or 20 Mflop/s.
It starts at pstate 0 (ie, at 100 Mflop/s). In this case, you have to specify one interval per pstate in the
watt_per_state property.
In this example, the idle consumption is 95 Watts, 93 Watts and 90 Watts in each pstate while the CPU burn consumption
are at 200 Watts, 170 Watts, and 150 Watts respectively. If only one core is active, this machine consumes 120 / 115 / 110 watts.

To change the pstate of a given CPU, use the following functions:
#MSG_host_get_nb_pstates(), simgrid#s4u#Host#setPstate(), #MSG_host_get_power_peak_at().

To simulate the energy-related elements, first call the simgrid#energy#sg_energy_plugin_init() before your #MSG_init(),
and then use the following function to retrieve the consumption of a given host: MSG_host_get_consumed_energy().
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_energy, surf, "Logging specific to the SURF energy plugin");

namespace simgrid {
namespace energy {

class PowerRange {
public:
  double idle;
  double min;
  double max;

  PowerRange(double idle, double min, double max) : idle(idle), min(min), max(max) {}
};

class HostEnergy {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, HostEnergy> EXTENSION_ID;

  explicit HostEnergy(simgrid::s4u::Host* ptr);
  ~HostEnergy();

  double getCurrentWattsValue(double cpu_load);
  double getConsumedEnergy();
  double getWattMinAt(int pstate);
  double getWattMaxAt(int pstate);
  void update();

private:
  void initWattsRangeList();
  simgrid::s4u::Host* host = nullptr;
  std::vector<PowerRange>
      power_range_watts_list; /*< List of (min_power,max_power) pairs corresponding to each cpu pstate */

  /* We need to keep track of what pstate has been used, as we will sometimes
   * be notified only *after* a pstate has been used (but we need to update the energy consumption
   * with the old pstate!)
   */
  int pstate = 0;

public:
  double watts_off    = 0.0; /*< Consumption when the machine is turned off (shutdown) */
  double total_energy = 0.0; /*< Total energy consumed by the host */
  double last_updated;       /*< Timestamp of the last energy update event*/
};

simgrid::xbt::Extension<simgrid::s4u::Host, HostEnergy> HostEnergy::EXTENSION_ID;

/* Computes the consumption so far.  Called lazily on need. */
void HostEnergy::update()
{
  double start_time  = this->last_updated;
  double finish_time = surf_get_clock();
  double cpu_load;
  double current_speed = host->speed();

  if (start_time < finish_time) {
    // We may have start == finish if the past consumption was updated since the simcall was started
    // for example if 2 actors requested to update the same host's consumption in a given scheduling round.
    //
    // Even in this case, we need to save the pstate for the next call (after this big if),
    // which may have changed since that recent update.

    if (current_speed <= 0)
      // Some users declare a pstate of speed 0 flops (e.g., to model boot time).
      // We consider that the machine is then fully loaded. That's arbitrary but it avoids a NaN
      cpu_load = 1;
    else
      cpu_load = lmm_constraint_get_usage(host->pimpl_cpu->constraint()) / current_speed;

    /** Divide by the number of cores here **/
    cpu_load /= host->pimpl_cpu->coreCount();

    if (cpu_load > 1) // A machine with a load > 1 consumes as much as a fully loaded machine, not more
      cpu_load = 1;

    /* The problem with this model is that the load is always 0 or 1, never something less.
     * Another possibility could be to model the total energy as
     *
     *   X/(X+Y)*W_idle + Y/(X+Y)*W_burn
     *
     * where X is the amount of idling cores, and Y the amount of computing cores.
     */

    double previous_energy = this->total_energy;

    double instantaneous_consumption;
    if (this->pstate == -1) // The host was off at the beginning of this time interval
      instantaneous_consumption = this->watts_off;
    else
      instantaneous_consumption = this->getCurrentWattsValue(cpu_load);

    double energy_this_step = instantaneous_consumption * (finish_time - start_time);

    // TODO Trace: Trace energy_this_step from start_time to finish_time in host->name()

    this->total_energy = previous_energy + energy_this_step;
    this->last_updated = finish_time;

    XBT_DEBUG("[update_energy of %s] period=[%.2f-%.2f]; current power peak=%.0E flop/s; consumption change: %.2f J -> "
              "%.2f J",
              host->cname(), start_time, finish_time, host->pimpl_cpu->speed_.peak, previous_energy, energy_this_step);
  }

  /* Save data for the upcoming time interval: whether it's on/off and the pstate if it's on */
  this->pstate = host->isOn() ? host->pstate() : -1;
}

HostEnergy::HostEnergy(simgrid::s4u::Host* ptr) : host(ptr), last_updated(surf_get_clock())
{
  initWattsRangeList();

  const char* off_power_str = host->property("watt_off");
  if (off_power_str != nullptr) {
    char* msg       = bprintf("Invalid value for property watt_off of host %s: %%s", host->cname());
    this->watts_off = xbt_str_parse_double(off_power_str, msg);
    xbt_free(msg);
  }
  /* watts_off is 0 by default */
}

HostEnergy::~HostEnergy() = default;

double HostEnergy::getWattMinAt(int pstate)
{
  xbt_assert(not power_range_watts_list.empty(), "No power range properties specified for host %s", host->cname());
  return power_range_watts_list[pstate].min;
}

double HostEnergy::getWattMaxAt(int pstate)
{
  xbt_assert(not power_range_watts_list.empty(), "No power range properties specified for host %s", host->cname());
  return power_range_watts_list[pstate].max;
}

/** @brief Computes the power consumed by the host according to the current pstate and processor load */
double HostEnergy::getCurrentWattsValue(double cpu_load)
{
  xbt_assert(not power_range_watts_list.empty(), "No power range properties specified for host %s", host->cname());

  /* min_power corresponds to the power consumed when only one core is active */
  /* max_power is the power consumed at 100% cpu load       */
  auto range           = power_range_watts_list.at(this->pstate);
  double current_power = 0;
  double min_power     = 0;
  double max_power     = 0;
  double power_slope   = 0;

  if (cpu_load > 0) { /* Something is going on, the machine is not idle */
    double min_power = range.min;
    double max_power = range.max;

    /**
     * The min_power states how much we consume when only one single
     * core is working. This means that when cpu_load == 1/coreCount, then
     * current_power == min_power.
     *
     * The maximum must be reached when all cores are working (but 1 core was
     * already accounted for by min_power)
     * i.e., we need min_power + (maxCpuLoad-1/coreCount)*power_slope == max_power
     * (maxCpuLoad is by definition 1)
     */
    double power_slope;
    int coreCount         = host->coreCount();
    double coreReciprocal = static_cast<double>(1) / static_cast<double>(coreCount);
    if (coreCount > 1)
      power_slope = (max_power - min_power) / (1 - coreReciprocal);
    else
      power_slope = 0; // Should be 0, since max_power == min_power (in this case)

    current_power = min_power + (cpu_load - coreReciprocal) * power_slope;
  } else { /* Our machine is idle, take the dedicated value! */
    current_power = range.idle;
  }

  XBT_DEBUG("[get_current_watts] min_power=%f, max_power=%f, slope=%f", min_power, max_power, power_slope);
  XBT_DEBUG("[get_current_watts] Current power (watts) = %f, load = %f", current_power, cpu_load);

  return current_power;
}

double HostEnergy::getConsumedEnergy()
{
  if (last_updated < surf_get_clock()) // We need to simcall this as it modifies the environment
    simgrid::simix::kernelImmediate(std::bind(&HostEnergy::update, this));

  return total_energy;
}

void HostEnergy::initWattsRangeList()
{
  const char* all_power_values_str = host->property("watt_per_state");
  if (all_power_values_str == nullptr)
    return;

  std::vector<std::string> all_power_values;
  boost::split(all_power_values, all_power_values_str, boost::is_any_of(","));

  int i = 0;
  for (auto current_power_values_str : all_power_values) {
    /* retrieve the power values associated with the current pstate */
    std::vector<std::string> current_power_values;
    boost::split(current_power_values, current_power_values_str, boost::is_any_of(":"));
    xbt_assert(current_power_values.size() == 3, "Power properties incorrectly defined - "
                                                 "could not retrieve idle, min and max power values for host %s",
               host->cname());

    /* min_power corresponds to the idle power (cpu load = 0) */
    /* max_power is the power consumed at 100% cpu load       */
    char* msg_idle = bprintf("Invalid idle value for pstate %d on host %s: %%s", i, host->cname());
    char* msg_min  = bprintf("Invalid min value for pstate %d on host %s: %%s", i, host->cname());
    char* msg_max  = bprintf("Invalid max value for pstate %d on host %s: %%s", i, host->cname());
    PowerRange range(xbt_str_parse_double((current_power_values.at(0)).c_str(), msg_idle),
                     xbt_str_parse_double((current_power_values.at(1)).c_str(), msg_min),
                     xbt_str_parse_double((current_power_values.at(2)).c_str(), msg_max));
    power_range_watts_list.push_back(range);
    xbt_free(msg_idle);
    xbt_free(msg_min);
    xbt_free(msg_max);
    i++;
  }
}
}
}

using simgrid::energy::HostEnergy;

/* **************************** events  callback *************************** */
static void onCreation(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  //TODO Trace: set to zero the energy variable associated to host->name()

  host.extension_set(new HostEnergy(&host));
}

static void onActionStateChange(simgrid::surf::CpuAction* action, simgrid::surf::Action::State previous)
{
  for (simgrid::surf::Cpu* cpu : action->cpus()) {
    simgrid::s4u::Host* host = cpu->getHost();
    if (host != nullptr) {

      // If it's a VM, take the corresponding PM
      simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host);
      if (vm) // If it's a VM, take the corresponding PM
        host = vm->pimpl_vm_->getPm();

      // Get the host_energy extension for the relevant host
      HostEnergy* host_energy = host->extension<HostEnergy>();

      if (host_energy->last_updated < surf_get_clock())
        host_energy->update();
    }
  }
}

/* This callback is fired either when the host changes its state (on/off) ("onStateChange") or its speed
 * (because the user changed the pstate, or because of external trace events) ("onSpeedChange") */
static void onHostChange(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  HostEnergy* host_energy = host.extension<HostEnergy>();

  host_energy->update();
}

static void onHostDestruction(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  HostEnergy* host_energy = host.extension<HostEnergy>();
  host_energy->update();
  XBT_INFO("Energy consumption of host %s: %f Joules", host.cname(), host_energy->getConsumedEnergy());
}

static void onSimulationEnd()
{
  sg_host_t* host_list     = sg_host_list();
  int host_count           = sg_host_count();
  double total_energy      = 0.0; // Total energy consumption (whole platform)
  double used_hosts_energy = 0.0; // Energy consumed by hosts that computed something
  for (int i = 0; i < host_count; i++) {
    if (dynamic_cast<simgrid::s4u::VirtualMachine*>(host_list[i]) == nullptr) { // Ignore virtual machines

      bool host_was_used = (host_list[i]->extension<HostEnergy>()->last_updated != 0);
      double energy      = host_list[i]->extension<HostEnergy>()->getConsumedEnergy();
      total_energy      += energy;
      if (host_was_used)
        used_hosts_energy += energy;
    }
  }
  XBT_INFO("Total energy consumption: %f Joules (used hosts: %f Joules; unused/idle hosts: %f)",
           total_energy, used_hosts_energy, total_energy - used_hosts_energy);
  xbt_free(host_list);
}

/* **************************** Public interface *************************** */
SG_BEGIN_DECL()

/** \ingroup SURF_plugin_energy
 * \brief Enable host energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. Call this function before #MSG_init().
 */
void sg_host_energy_plugin_init()
{
  if (HostEnergy::EXTENSION_ID.valid())
    return;

  HostEnergy::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostEnergy>();

  simgrid::s4u::Host::onCreation.connect(&onCreation);
  simgrid::s4u::Host::onStateChange.connect(&onHostChange);
  simgrid::s4u::Host::onSpeedChange.connect(&onHostChange);
  simgrid::s4u::Host::onDestruction.connect(&onHostDestruction);
  simgrid::s4u::onSimulationEnd.connect(&onSimulationEnd);
  simgrid::surf::CpuAction::onStateChange.connect(&onActionStateChange);
}

/** @brief updates the consumption of all hosts
 *
 * After this call, sg_host_get_consumed_energy() will not interrupt your process
 * (until after the next clock update).
 */
void sg_host_energy_update_all()
{
  simgrid::simix::kernelImmediate([]() {
    std::vector<simgrid::s4u::Host*> list;
    simgrid::s4u::Engine::instance()->hostList(&list);
    for (auto host : list)
      host->extension<HostEnergy>()->update();
  });
}

/** @brief Returns the total energy consumed by the host so far (in Joules)
 *
 *  Please note that since the consumption is lazily updated, it may require a simcall to update it.
 *  The result is that the actor requesting this value will be interrupted,
 *  the value will be updated in kernel mode before returning the control to the requesting actor.
 *
 *  See also @ref SURF_plugin_energy.
 */
double sg_host_get_consumed_energy(sg_host_t host)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getConsumedEnergy();
}

/** @brief Get the amount of watt dissipated at the given pstate when the host is idling */
double sg_host_get_wattmin_at(sg_host_t host, int pstate)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getWattMinAt(pstate);
}
/** @brief  Returns the amount of watt dissipated at the given pstate when the host burns CPU at 100% */
double sg_host_get_wattmax_at(sg_host_t host, int pstate)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getWattMaxAt(pstate);
}

SG_END_DECL()
