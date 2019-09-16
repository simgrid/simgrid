/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/include/surf/surf.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/surf/cpu_interface.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

SIMGRID_REGISTER_PLUGIN(host_energy, "Cpu energy consumption.", &sg_host_energy_plugin_init)

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

  - @b Idle: instantaneous consumption (in Watt) when your host is up and running, but without anything to do.
  - @b OneCore: instantaneous consumption (in Watt) when only one core is active, at 100%.
  - @b AllCores: instantaneous consumption (in Watt) when all cores of the host are at 100%.
  - @b Off: instantaneous consumption (in Watt) when the host is turned off.

Here is an example of XML declaration:

@code{.xml}
<host id="HostA" power="100.0Mf" cores="4">
    <prop id="watt_per_state" value="100.0:120.0:200.0" />
    <prop id="watt_off" value="10" />
</host>
@endcode

This example gives the following parameters: @b Off is 10 Watts; @b Idle is 100 Watts; @b OneCore is 120 Watts and @b
AllCores is 200 Watts.
This is enough to compute the consumption as a function of the amount of loaded cores:

<table>
<tr><th>@#Cores loaded</th><th>Consumption</th><th>Explanation</th></tr>
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

In this case, the parameters @b OneCore and @b AllCores are obviously the same.
Actually, SimGrid expect an energetic profile formatted as 'Idle:Running' for mono-cores hosts.
If you insist on passing 3 parameters in this case, then you must have the same value for @b OneCore and @b AllCores.

@code{.xml}
<host id="HostC" power="100.0Mf" cores="1">
    <prop id="watt_per_state" value="95.0:200.0" /> <!-- we may have used '95:200:200' instead -->
    <prop id="watt_off" value="10" />
</host>
@endcode

### How does DVFS interact with the host energy model?

If your host has several DVFS levels (several pstates), then you should give the energetic profile of each pstate level:

@code{.xml}
<host id="HostC" power="100.0Mf,50.0Mf,20.0Mf" cores="4">
    <prop id="watt_per_state" value="95.0:120.0:200.0, 93.0:115.0:170.0, 90.0:110.0:150.0" />
    <prop id="watt_off" value="10" />
</host>
@endcode

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

// Forwards declaration needed to make this function a friend (because friends have external linkage by default)
static void on_simulation_end();

namespace simgrid {
namespace plugin {

class PowerRange {
public:
  double idle_;
  double min_;
  double max_;

  PowerRange(double idle, double min, double max) : idle_(idle), min_(min), max_(max) {}
};

class HostEnergy {
  friend void ::on_simulation_end(); // For access to host_was_used_
public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, HostEnergy> EXTENSION_ID;

  explicit HostEnergy(simgrid::s4u::Host* ptr);
  ~HostEnergy();

  double get_current_watts_value();
  double get_current_watts_value(double cpu_load);
  double get_consumed_energy();
  double get_idle_consumption();
  double get_watt_min_at(int pstate);
  double get_watt_max_at(int pstate);
  void update();

private:
  void init_watts_range_list();
  simgrid::s4u::Host* host_ = nullptr;
  /*< List of (min_power,max_power) pairs corresponding to each cpu pstate */
  std::vector<PowerRange> power_range_watts_list_;

  /* We need to keep track of what pstate has been used, as we will sometimes be notified only *after* a pstate has been
   * used (but we need to update the energy consumption with the old pstate!)
   */
  int pstate_           = 0;
  const int pstate_off_ = -1;

  /* Only used to split total energy into unused/used hosts.
   * If you want to get this info for something else, rather use the host_load plugin
   */
  bool host_was_used_  = false;
public:
  double watts_off_    = 0.0; /*< Consumption when the machine is turned off (shutdown) */
  double total_energy_ = 0.0; /*< Total energy consumed by the host */
  double last_updated_;       /*< Timestamp of the last energy update event*/
};

simgrid::xbt::Extension<simgrid::s4u::Host, HostEnergy> HostEnergy::EXTENSION_ID;

/* Computes the consumption so far. Called lazily on need. */
void HostEnergy::update()
{
  double start_time  = this->last_updated_;
  double finish_time = surf_get_clock();
  //
  // We may have start == finish if the past consumption was updated since the simcall was started
  // for example if 2 actors requested to update the same host's consumption in a given scheduling round.
  //
  // Even in this case, we need to save the pstate for the next call (after this if),
  // which may have changed since that recent update.
  if (start_time < finish_time) {
    double previous_energy = this->total_energy_;

    double instantaneous_consumption = this->get_current_watts_value();

    double energy_this_step = instantaneous_consumption * (finish_time - start_time);

    // TODO Trace: Trace energy_this_step from start_time to finish_time in host->getName()

    this->total_energy_ = previous_energy + energy_this_step;
    this->last_updated_ = finish_time;

    XBT_DEBUG("[update_energy of %s] period=[%.8f-%.8f]; current speed=%.2E flop/s (pstate %i); total consumption before: consumption change: %.8f J -> added now: %.8f J",
              host_->get_cname(), start_time, finish_time, host_->pimpl_cpu->get_pstate_peak_speed(this->pstate_), this->pstate_, previous_energy,
              energy_this_step);
  }

  /* Save data for the upcoming time interval: whether it's on/off and the pstate if it's on */
  this->pstate_ = host_->is_on() ? host_->get_pstate() : pstate_off_;
}

HostEnergy::HostEnergy(simgrid::s4u::Host* ptr) : host_(ptr), last_updated_(surf_get_clock())
{
  init_watts_range_list();

  const char* off_power_str = host_->get_property("watt_off");
  if (off_power_str != nullptr) {
    try {
      this->watts_off_ = std::stod(std::string(off_power_str));
    } catch (const std::invalid_argument&) {
      throw std::invalid_argument(std::string("Invalid value for property watt_off of host ") + host_->get_cname() +
                                  ": " + off_power_str);
    }
  }
  /* watts_off is 0 by default */
}

HostEnergy::~HostEnergy() = default;

double HostEnergy::get_idle_consumption()
{
  xbt_assert(not power_range_watts_list_.empty(), "No power range properties specified for host %s",
             host_->get_cname());

  return power_range_watts_list_[0].idle_;
}

double HostEnergy::get_watt_min_at(int pstate)
{
  xbt_assert(not power_range_watts_list_.empty(), "No power range properties specified for host %s",
             host_->get_cname());
  return power_range_watts_list_[pstate].min_;
}

double HostEnergy::get_watt_max_at(int pstate)
{
  xbt_assert(not power_range_watts_list_.empty(), "No power range properties specified for host %s",
             host_->get_cname());
  return power_range_watts_list_[pstate].max_;
}

/** @brief Computes the power consumed by the host according to the current situation
 *
 * - If the host is off, that's the watts_off value
 * - if it's on, take the current pstate and the current processor load into account */
double HostEnergy::get_current_watts_value()
{
  if (this->pstate_ == pstate_off_) // The host is off (or was off at the beginning of this time interval)
    return this->watts_off_;

  double current_speed = host_->get_pstate_speed(this->pstate_);

  double cpu_load;

  if (current_speed <= 0)
    // Some users declare a pstate of speed 0 flops (e.g., to model boot time).
    // We consider that the machine is then fully loaded. That's arbitrary but it avoids a NaN
    cpu_load = 1;
  else {
    cpu_load = host_->pimpl_cpu->get_constraint()->get_usage() / current_speed;

    /** Divide by the number of cores here **/
    cpu_load /= host_->pimpl_cpu->get_core_count();

    if (cpu_load > 1) // A machine with a load > 1 consumes as much as a fully loaded machine, not more
      cpu_load = 1;
    if (cpu_load > 0)
      host_was_used_ = true;
  }

  /* The problem with this model is that the load is always 0 or 1, never something less.
   * Another possibility could be to model the total energy as
   *
   *   X/(X+Y)*W_idle + Y/(X+Y)*W_burn
   *
   * where X is the amount of idling cores, and Y the amount of computing cores.
   */
  return get_current_watts_value(cpu_load);
}

/** @brief Computes the power that the host would consume at the provided processor load
 *
 * Whether the host is ON or OFF is not taken into account.
 */
double HostEnergy::get_current_watts_value(double cpu_load)
{
  xbt_assert(not power_range_watts_list_.empty(), "No power range properties specified for host %s",
             host_->get_cname());

  /* Return watts_off if pstate == pstate_off (ie, if the host is off) */
  if (this->pstate_ == pstate_off_) {
    return watts_off_;
  }

  /* min_power corresponds to the power consumed when only one core is active */
  /* max_power is the power consumed at 100% cpu load       */
  auto range           = power_range_watts_list_.at(this->pstate_);
  double current_power;
  double min_power;
  double max_power;
  double power_slope;

  if (cpu_load > 0) { /* Something is going on, the machine is not idle */
    min_power = range.min_;
    max_power = range.max_;

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
    int coreCount         = host_->get_core_count();
    double coreReciprocal = 1.0 / coreCount;
    if (coreCount > 1)
      power_slope = (max_power - min_power) / (1 - coreReciprocal);
    else
      power_slope = 0; // Should be 0, since max_power == min_power (in this case)

    current_power = min_power + (cpu_load - coreReciprocal) * power_slope;
  } else { /* Our machine is idle, take the dedicated value! */
    min_power     = 0;
    max_power     = 0;
    power_slope   = 0;
    current_power = range.idle_;
  }

  XBT_DEBUG("[get_current_watts] pstate=%i, min_power=%f, max_power=%f, slope=%f", this->pstate_, min_power, max_power, power_slope);
  XBT_DEBUG("[get_current_watts] Current power (watts) = %f, load = %f", current_power, cpu_load);

  return current_power;
}

double HostEnergy::get_consumed_energy()
{
  if (last_updated_ < surf_get_clock()) // We need to simcall this as it modifies the environment
    simgrid::kernel::actor::simcall(std::bind(&HostEnergy::update, this));

  return total_energy_;
}

void HostEnergy::init_watts_range_list()
{
  const char* all_power_values_str = host_->get_property("watt_per_state");
  if (all_power_values_str == nullptr)
    return;

  std::vector<std::string> all_power_values;
  boost::split(all_power_values, all_power_values_str, boost::is_any_of(","));
  XBT_DEBUG("%s: profile: %s, cores: %d", host_->get_cname(), all_power_values_str, host_->get_core_count());

  int i = 0;
  for (auto const& current_power_values_str : all_power_values) {
    /* retrieve the power values associated with the current pstate */
    std::vector<std::string> current_power_values;
    boost::split(current_power_values, current_power_values_str, boost::is_any_of(":"));
    if (host_->get_core_count() == 1) {
      xbt_assert(current_power_values.size() == 2 || current_power_values.size() == 3,
                 "Power properties incorrectly defined for host %s."
                 "It should be 'Idle:FullSpeed' power values because you have one core only.",
                 host_->get_cname());
      if (current_power_values.size() == 2) {
        // In this case, 1core == AllCores
        current_power_values.push_back(current_power_values.at(1));
      } else { // size == 3
        current_power_values[1] = current_power_values.at(2);
        current_power_values[2] = current_power_values.at(2);
        static bool displayed_warning = false;
        if (not displayed_warning) { // Otherwise we get in the worst case no_pstate*no_hosts warnings
          XBT_WARN("Host %s is a single-core machine and part of the power profile is '%s'"
                   ", which is in the 'Idle:OneCore:AllCores' format."
                   " Here, only the value for 'AllCores' is used.", host_->get_cname(), current_power_values_str.c_str());
          displayed_warning = true;
        }
      }
    } else {
      xbt_assert(current_power_values.size() == 3,
                 "Power properties incorrectly defined for host %s."
                 "It should be 'Idle:OneCore:AllCores' power values because you have more than one core.",
                 host_->get_cname());
    }

    /* min_power corresponds to the idle power (cpu load = 0) */
    /* max_power is the power consumed at 100% cpu load       */
    char* msg_idle = bprintf("Invalid idle value for pstate %d on host %s: %%s", i, host_->get_cname());
    char* msg_min  = bprintf("Invalid OneCore value for pstate %d on host %s: %%s", i, host_->get_cname());
    char* msg_max  = bprintf("Invalid AllCores value for pstate %d on host %s: %%s", i, host_->get_cname());
    PowerRange range(xbt_str_parse_double((current_power_values.at(0)).c_str(), msg_idle),
                     xbt_str_parse_double((current_power_values.at(1)).c_str(), msg_min),
                     xbt_str_parse_double((current_power_values.at(2)).c_str(), msg_max));
    power_range_watts_list_.push_back(range);
    xbt_free(msg_idle);
    xbt_free(msg_min);
    xbt_free(msg_max);
    i++;
  }
}
} // namespace plugin
} // namespace simgrid

using simgrid::plugin::HostEnergy;

/* **************************** events  callback *************************** */
static void on_creation(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  // TODO Trace: set to zero the energy variable associated to host->getName()

  host.extension_set(new HostEnergy(&host));
}

static void on_action_state_change(simgrid::kernel::resource::CpuAction const& action,
                                   simgrid::kernel::resource::Action::State /*previous*/)
{
  for (simgrid::kernel::resource::Cpu* const& cpu : action.cpus()) {
    simgrid::s4u::Host* host = cpu->get_host();
    if (host != nullptr) {

      // If it's a VM, take the corresponding PM
      simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host);
      if (vm) // If it's a VM, take the corresponding PM
        host = vm->get_pm();

      // Get the host_energy extension for the relevant host
      HostEnergy* host_energy = host->extension<HostEnergy>();

      if (host_energy->last_updated_ < surf_get_clock())
        host_energy->update();
    }
  }
}

/* This callback is fired either when the host changes its state (on/off) ("onStateChange") or its speed
 * (because the user changed the pstate, or because of external trace events) ("onSpeedChange") */
static void on_host_change(simgrid::s4u::Host const& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine const*>(&host)) // Ignore virtual machines
    return;

  HostEnergy* host_energy = host.extension<HostEnergy>();

  host_energy->update();
}

static void on_host_destruction(simgrid::s4u::Host const& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine const*>(&host)) // Ignore virtual machines
    return;

  XBT_INFO("Energy consumption of host %s: %f Joules", host.get_cname(),
           host.extension<HostEnergy>()->get_consumed_energy());
}

static void on_simulation_end()
{
  std::vector<simgrid::s4u::Host*> hosts = simgrid::s4u::Engine::get_instance()->get_all_hosts();

  double total_energy      = 0.0; // Total energy consumption (whole platform)
  double used_hosts_energy = 0.0; // Energy consumed by hosts that computed something
  for (size_t i = 0; i < hosts.size(); i++) {
    if (dynamic_cast<simgrid::s4u::VirtualMachine*>(hosts[i]) == nullptr) { // Ignore virtual machines

      double energy      = hosts[i]->extension<HostEnergy>()->get_consumed_energy();
      total_energy += energy;
      if (hosts[i]->extension<HostEnergy>()->host_was_used_)
        used_hosts_energy += energy;
    }
  }
  XBT_INFO("Total energy consumption: %f Joules (used hosts: %f Joules; unused/idle hosts: %f)", total_energy,
           used_hosts_energy, total_energy - used_hosts_energy);
}

/* **************************** Public interface *************************** */

/** @ingroup plugin_energy
 * @brief Enable host energy plugin
 * @details Enable energy plugin to get joules consumption of each cpu. Call this function before #MSG_init().
 */
void sg_host_energy_plugin_init()
{
  if (HostEnergy::EXTENSION_ID.valid())
    return;

  HostEnergy::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostEnergy>();

  simgrid::s4u::Host::on_creation.connect(&on_creation);
  simgrid::s4u::Host::on_state_change.connect(&on_host_change);
  simgrid::s4u::Host::on_speed_change.connect(&on_host_change);
  simgrid::s4u::Host::on_destruction.connect(&on_host_destruction);
  simgrid::s4u::Engine::on_simulation_end.connect(&on_simulation_end);
  simgrid::kernel::resource::CpuAction::on_state_change.connect(&on_action_state_change);
  // We may only have one actor on a node. If that actor executes something like
  //   compute -> recv -> compute
  // the recv operation will not trigger a "CpuAction::on_state_change". This means
  // that the next trigger would be the 2nd compute, hence ignoring the idle time
  // during the recv call. By updating at the beginning of a compute, we can
  // fix that. (If the cpu is not idle, this is not required.)
  simgrid::kernel::activity::ExecImpl::on_creation.connect([](simgrid::kernel::activity::ExecImpl const& activity) {
    if (activity.get_host_number() == 1) { // We only run on one host
      simgrid::s4u::Host* host         = activity.get_host();
      simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host);
      if (vm != nullptr)
        host = vm->get_pm();
      xbt_assert(host != nullptr);
      host->extension<HostEnergy>()->update();
    }
  });
}

/** @ingroup plugin_energy
 *  @brief updates the consumption of all hosts
 *
 * After this call, sg_host_get_consumed_energy() will not interrupt your process
 * (until after the next clock update).
 */
void sg_host_energy_update_all()
{
  simgrid::kernel::actor::simcall([]() {
    std::vector<simgrid::s4u::Host*> list = simgrid::s4u::Engine::get_instance()->get_all_hosts();
    for (auto const& host : list)
      if (dynamic_cast<simgrid::s4u::VirtualMachine*>(host) == nullptr) { // Ignore virtual machines
        xbt_assert(host != nullptr);
        host->extension<HostEnergy>()->update();
      }
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
             "The Energy plugin is not active. Please call sg_host_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->get_consumed_energy();
}

/** @ingroup plugin_energy
 *  @brief Get the amount of watt dissipated when the host is idling
 */
double sg_host_get_idle_consumption(sg_host_t host)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_host_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->get_idle_consumption();
}

/** @ingroup plugin_energy
 *  @brief Get the amount of watt dissipated at the given pstate when the host is idling
 */
double sg_host_get_wattmin_at(sg_host_t host, int pstate)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_host_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->get_watt_min_at(pstate);
}
/** @ingroup plugin_energy
 *  @brief  Returns the amount of watt dissipated at the given pstate when the host burns CPU at 100%
 */
double sg_host_get_wattmax_at(sg_host_t host, int pstate)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_host_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->get_watt_max_at(pstate);
}

/** @ingroup plugin_energy
 *  @brief Returns the current consumption of the host
 */
double sg_host_get_current_consumption(sg_host_t host)
{
  xbt_assert(HostEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_host_energy_plugin_init() during initialization.");
  return host->extension<HostEnergy>()->get_current_watts_value();
}
