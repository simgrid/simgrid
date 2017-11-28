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

/** @addtogroup plugin_energy

This is the energy plugin, enabling to account not only for computation time, but also for the dissipated energy in the
simulated platform.
To activate this plugin, first call sg_host_energy_plugin_init() before your #MSG_init(), and then use
MSG_host_get_consumed_energy() to retrieve the consumption of a given host.

When the host is on, this energy consumption naturally depends on both the current CPU load and the host energy profile.
According to our measurements, the consumption is somehow linear in the amount of cores at full speed, with an
abnormality when all the cores are idle. The full details are in
<a href="https://hal.inria.fr/hal-01523608">our scientific paper</a> on that topic.

As a result, our energy model takes 4 parameters:

  - \b Idle: instantaneous consumption (in Watt) when your host is up and running, but without anything to do.
  - \b OneCore: instantaneous consumption (in Watt) when only one core is active, at 100%.
  - \b AllCores: instantaneous consumption (in Watt) when all cores of the host are at 100%.
  - \b Off: instantaneous consumption (in Watt) when the host is turned off.

Here is an example of XML declaration:

\code{.xml}
<host id="HostA" power="100.0Mf" cores="4">
    <prop id="watt_per_state" value="100.0:120.0:200.0" />
    <prop id="watt_off" value="10" />
</host>
\endcode

This example gives the following parameters: \b Off is 10 Watts; \b Idle is 100 Watts; \b OneCore is 120 Watts and \b
AllCores is 200 Watts.
This is enough to compute the consumption as a function of the amount of loaded cores:

<table>
<tr><th>#Cores loaded</th><th>Consumption</th><th>Explanation</th></tr>
<tr><td>0</td><td> 100 Watts</td><td>Idle value</td></tr>
<tr><td>1</td><td> 120 Watts</td><td>OneCore value</td></tr>
<tr><td>2</td><td> 147 Watts</td><td>linear extrapolation between OneCore and AllCores</td></tr>
<tr><td>3</td><td> 173 Watts</td><td>linear extrapolation between OneCore and AllCores</td></tr>
<tr><td>4</td><td> 200 Watts</td><td>AllCores value</td></tr>
</table>

### What if a given core is only at load 50%?

This is impossible in SimGrid because we recompute everything each time that the CPU starts or stops doing something.
So if a core is at load 50% over a period, it means that it is at load 100% half of the time and at load 0% the rest of
the time, and our model holds.

### What if the host has only one core?

In this case, the parameters \b OneCore and \b AllCores are obviously the same.
Actually, SimGrid expect an energetic profile formatted as 'Idle:Running' for mono-cores hosts.
If you insist on passing 3 parameters in this case, then you must have the same value for \b OneCore and \b AllCores.

\code{.xml}
<host id="HostC" power="100.0Mf" cores="1">
    <prop id="watt_per_state" value="95.0:200.0" /> <!-- we may have used '95:200:200' instead -->
    <prop id="watt_off" value="10" />
</host>
\endcode

### How does DVFS interact with the host energy model?

If your host has several DVFS levels (several pstates), then you should give the energetic profile of each pstate level:

\code{.xml}
<host id="HostC" power="100.0Mf,50.0Mf,20.0Mf" cores="4">
    <prop id="watt_per_state" value="95.0:120.0:200.0, 93.0:115.0:170.0, 90.0:110.0:150.0" />
    <prop id="watt_off" value="10" />
</host>
\endcode

This encodes the following values
<table>
<tr><th>pstate</th><th>Performance</th><th>Idle</th><th>OneCore</th><th>AllCores</th></tr>
<tr><td>0</td><td>100 Mflop/s</td><td>95 Watts</td><td>120 Watts</td><td>200 Watts</td></tr>
<tr><td>1</td><td>50 Mflop/s</td><td>93 Watts</td><td>115 Watts</td><td>170 Watts</td></tr>
<tr><td>2</td><td>20 Mflop/s</td><td>90 Watts</td><td>110 Watts</td><td>150 Watts</td></tr>
</table>

To change the pstate of a given CPU, use the following functions:
#MSG_host_get_nb_pstates(), simgrid#s4u#Host#setPstate(), #MSG_host_get_power_peak_at().

### How accurate are these models?

This model cannot be more accurate than your instantiation: with the default values, your result will not be accurate at
all. You can still get accurate energy prediction, provided that you carefully instantiate the model.
The first step is to ensure that your timing prediction match perfectly. But this is only the first step of the path,
and you really want to read <a href="https://hal.inria.fr/hal-01523608">this paper</a> to see all what you need to do
before you can get accurate energy predictions.
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_energy, surf, "Logging specific to the SURF energy plugin");

namespace simgrid {
namespace plugin {

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

  /* We need to keep track of what pstate has been used, as we will sometimes be notified only *after* a pstate has been
   * used (but we need to update the energy consumption with the old pstate!)
   */
  int pstate = 0;
  const int pstate_off = -1;

public:
  double watts_off    = 0.0; /*< Consumption when the machine is turned off (shutdown) */
  double total_energy = 0.0; /*< Total energy consumed by the host */
  double last_updated;       /*< Timestamp of the last energy update event*/
};

simgrid::xbt::Extension<simgrid::s4u::Host, HostEnergy> HostEnergy::EXTENSION_ID;

/* Computes the consumption so far. Called lazily on need. */
void HostEnergy::update()
{
  double start_time  = this->last_updated;
  double finish_time = surf_get_clock();
  double current_speed = host->getSpeed();

  if (start_time < finish_time) {
    double cpu_load;
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
      cpu_load = host->pimpl_cpu->constraint()->get_usage() / current_speed;

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
    if (this->pstate == pstate_off) // The host was off at the beginning of this time interval
      instantaneous_consumption = this->watts_off;
    else
      instantaneous_consumption = this->getCurrentWattsValue(cpu_load);

    double energy_this_step = instantaneous_consumption * (finish_time - start_time);

    // TODO Trace: Trace energy_this_step from start_time to finish_time in host->getName()

    this->total_energy = previous_energy + energy_this_step;
    this->last_updated = finish_time;

    XBT_DEBUG("[update_energy of %s] period=[%.2f-%.2f]; current power peak=%.0E flop/s; consumption change: %.2f J -> "
              "%.2f J",
              host->getCname(), start_time, finish_time, host->pimpl_cpu->speed_.peak, previous_energy,
              energy_this_step);
  }

  /* Save data for the upcoming time interval: whether it's on/off and the pstate if it's on */
  this->pstate = host->isOn() ? host->getPstate() : pstate_off;
}

HostEnergy::HostEnergy(simgrid::s4u::Host* ptr) : host(ptr), last_updated(surf_get_clock())
{
  initWattsRangeList();

  const char* off_power_str = host->getProperty("watt_off");
  if (off_power_str != nullptr) {
    try {
      this->watts_off = std::stod(std::string(off_power_str));
    } catch (std::invalid_argument& ia) {
      throw std::invalid_argument(std::string("Invalid value for property watt_off of host ") + host->getCname() +
                                  ": " + off_power_str);
    }
  }
  /* watts_off is 0 by default */
}

HostEnergy::~HostEnergy() = default;

double HostEnergy::getWattMinAt(int pstate)
{
  xbt_assert(not power_range_watts_list.empty(), "No power range properties specified for host %s", host->getCname());
  return power_range_watts_list[pstate].min;
}

double HostEnergy::getWattMaxAt(int pstate)
{
  xbt_assert(not power_range_watts_list.empty(), "No power range properties specified for host %s", host->getCname());
  return power_range_watts_list[pstate].max;
}

/** @brief Computes the power consumed by the host according to the current pstate and processor load */
double HostEnergy::getCurrentWattsValue(double cpu_load)
{
  xbt_assert(not power_range_watts_list.empty(), "No power range properties specified for host %s", host->getCname());

 /*
  *    * Return watts_off if pstate == pstate_off
  *       * this happens when host is off
  */
  if (this->pstate == pstate_off) {
    return watts_off;
  }

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
    int coreCount         = host->getCoreCount();
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
  const char* all_power_values_str = host->getProperty("watt_per_state");
  if (all_power_values_str == nullptr)
    return;

  std::vector<std::string> all_power_values;
  boost::split(all_power_values, all_power_values_str, boost::is_any_of(","));
  XBT_DEBUG("%s: profile: %s, cores: %d", host->getCname(), all_power_values_str, host->getCoreCount());

  int i = 0;
  for (auto const& current_power_values_str : all_power_values) {
    /* retrieve the power values associated with the current pstate */
    std::vector<std::string> current_power_values;
    boost::split(current_power_values, current_power_values_str, boost::is_any_of(":"));
    if (host->getCoreCount() == 1) {
      xbt_assert(current_power_values.size() == 2 || current_power_values.size() == 3,
                 "Power properties incorrectly defined for host %s."
                 "It should be 'Idle:FullSpeed' power values because you have one core only.",
                 host->getCname());
      if (current_power_values.size() == 2) {
        // In this case, 1core == AllCores
        current_power_values.push_back(current_power_values.at(1));
      } else { // size == 3
        xbt_assert((current_power_values.at(1)) == (current_power_values.at(2)),
                   "Power properties incorrectly defined for host %s.\n"
                   "The energy profile of mono-cores should be formatted as 'Idle:FullSpeed' only.\n"
                   "If you go for a 'Idle:OneCore:AllCores' power profile on mono-cores, then OneCore and AllCores "
                   "must be equal.",
                   host->getCname());
      }
    } else {
      xbt_assert(current_power_values.size() == 3,
                 "Power properties incorrectly defined for host %s."
                 "It should be 'Idle:OneCore:AllCores' power values because you have more than one core.",
                 host->getCname());
    }

    /* min_power corresponds to the idle power (cpu load = 0) */
    /* max_power is the power consumed at 100% cpu load       */
    char* msg_idle = bprintf("Invalid idle value for pstate %d on host %s: %%s", i, host->getCname());
    char* msg_min  = bprintf("Invalid OneCore value for pstate %d on host %s: %%s", i, host->getCname());
    char* msg_max  = bprintf("Invalid AllCores value for pstate %d on host %s: %%s", i, host->getCname());
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

using simgrid::plugin::HostEnergy;

/* **************************** events  callback *************************** */
static void onCreation(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  // TODO Trace: set to zero the energy variable associated to host->getName()

  host.extension_set(new HostEnergy(&host));
}

static void onActionStateChange(simgrid::surf::CpuAction* action, simgrid::surf::Action::State previous)
{
  for (simgrid::surf::Cpu* const& cpu : action->cpus()) {
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

  XBT_INFO("Energy consumption of host %s: %f Joules", host.getCname(),
           host.extension<HostEnergy>()->getConsumedEnergy());
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
extern "C" {

/** \ingroup plugin_energy
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

/** @ingroup plugin_energy
 *  @brief updates the consumption of all hosts
 *
 * After this call, sg_host_get_consumed_energy() will not interrupt your process
 * (until after the next clock update).
 */
void sg_host_energy_update_all()
{
  simgrid::simix::kernelImmediate([]() {
    std::vector<simgrid::s4u::Host*> list;
    simgrid::s4u::Engine::getInstance()->getHostList(&list);
    for (auto const& host : list)
      if (dynamic_cast<simgrid::s4u::VirtualMachine*>(host) == nullptr) // Ignore virtual machines
        host->extension<HostEnergy>()->update();
  });
}

/** @ingroup plugin_energy
 *  @brief Returns the total energy consumed by the host so far (in Joules)
 *
 *  Please note that since the consumption is lazily updated, it may require a simcall to update it.
 *  The result is that the actor requesting this value will be interrupted,
 *  the value will be updated in kernel mode before returning the control to the requesting actor.
 */
double sg_host_get_consumed_energy(sg_host_t host)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getConsumedEnergy();
}

/** @ingroup plugin_energy
 *  @brief Get the amount of watt dissipated at the given pstate when the host is idling
 */
double sg_host_get_wattmin_at(sg_host_t host, int pstate)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getWattMinAt(pstate);
}
/** @ingroup plugin_energy
 *  @brief  Returns the amount of watt dissipated at the given pstate when the host burns CPU at 100%
 */
double sg_host_get_wattmax_at(sg_host_t host, int pstate)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->getWattMaxAt(pstate);
}

/** @ingroup plugin_energy
 *  @brief Returns the current consumption of the host
 */
double sg_host_get_current_consumption(sg_host_t host)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  double cpu_load = host->pimpl_cpu->constraint()->get_usage() / host->getSpeed();
  return host->extension<HostEnergy>()->getCurrentWattsValue(cpu_load);
}
}
