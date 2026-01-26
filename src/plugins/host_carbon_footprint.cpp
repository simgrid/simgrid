/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/plugins/carbon_footprint.h"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "simgrid/simcall.hpp"
#include "src/simgrid/module.hpp"
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "xbt/asserts.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>
#include <map>
#include <sstream>
#include <unordered_map>

SIMGRID_REGISTER_PLUGIN(host_carbon_footprint, "Host carbon footprint", &sg_host_carbon_footprint_plugin_init)

/** @addtogroup plugin_carbon_footprint

This is the carbon footprint plugin, enabling the simulation of CO₂ emissions associated with the energy consumption of
hosts in the simulated platform. It calculates the total CO₂ emissions of each host based on its energy consumption and
the CO₂ emission rate. To activate this plugin, first call :cpp:func:`sg_host_carbon_footprint_plugin_init()` before
loading your platform. Then, use :cpp:func:`sg_host_get_carbon_footprint()` to retrieve the total CO₂ emissions of a
given host.

The carbon footprint is calculated dynamically during the simulation, taking into account the energy consumed by the
host over time and the carbon emission rate (in grams per kWh). The emission rate can be configured for each host using
:cpp:func:`sg_host_set_carbon_intensity()`.

As a result, the carbon footprint model requires the following parameters:

  - **Energy consumption**: The energy consumed by the host, which is calculated by the energy plugin based on the
host's power profile and activity.
  - **Carbon emission rate**: The amount of CO₂ emitted (in grams per kWh) of energy consumed, which can vary depending
on the location of the host.

Here is an example of XML declaration:

.. code-block:: xml

   <host id="Host1" speed="100.0Mf, 1e-9Mf, 0.5f, 0.05f" pstate="0">
       <prop id="wattage_per_state" value="30.0:30.0:100.0, 9.75:9.75:9.75, 200.996721311:200.996721311:200.996721311,
425.1743849:425.1743849:425.1743849" /> <prop id="wattage_off" value="9.75" /> <prop id="carbon_intensity" value="30" />
   </host>

In this example:
- The `wattage_per_state` property defines the power consumption (in watts) for each pstate of the host.
- The `wattage_off` property defines the power consumption when the host is turned off.
- The `carbon_intensity` property defines the CO₂ emission rate (in grams per kWh) for the host.


### How accurate are these models?
This model is still a work in progress and may not fully reflect real-world CO₂ emissions. The accuracy of the results
depends on the quality of the input parameters, such as the energy profile of the host and the CO₂ emission rate
(carbon_intensity). Further improvements and evaluations of the model are needed. Keep this in mind when using this
plugin.

*/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(host_carbon_footprint, kernel, "Logging specific to the host carbon footprint plugin");

// Global storage for carbon intensity traces
static std::unordered_map<std::string, std::map<std::string, double>> carbon_intensity_traces;
static bool trace_file_loaded = false;

namespace simgrid::plugin {

class HostCarbonFootprint {

public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, HostCarbonFootprint> EXTENSION_ID;

  explicit HostCarbonFootprint(simgrid::s4u::Host* ptr);
  ~HostCarbonFootprint();

  double getHostCarbonFootprint();
  double getHostCarbonIntensity();
  double get_last_update_time() const { return last_updated; }
  void setHostCarbonIntensity(double carbon_intensity);
  void update();

private:
  simgrid::s4u::Host* host_ = nullptr;

  double total_carbon_footprint = 0.0; /*< Total CO2 emitted by the host */
  double carbon_intensity       = 0.0; /*< Grams of CO2 emitted per kWh consumed by the host */
  double last_updated           = -1;  // Initially negative in the case there is CO2 data for t = 0

public:
  double last_energy; /*< Amount of energy used so far (kwh) >*/
};

simgrid::xbt::Extension<simgrid::s4u::Host, HostCarbonFootprint> HostCarbonFootprint::EXTENSION_ID;

void HostCarbonFootprint::update()
{
  double start_time  = last_updated;
  double finish_time = simgrid::s4u::Engine::get_clock();

  if (start_time < finish_time) {
    double previous_carbon_footprint = total_carbon_footprint;
    double total_carbon_this_step    = 0.0;
    double current_time              = start_time < 0.0 ? 0.0 : start_time;

    // Process carbon intensity changes during the time period
    if (trace_file_loaded) {
      std::string host_name = host_->get_cname();
      auto host_traces      = carbon_intensity_traces.find(host_name);

      XBT_DEBUG("[%s] UPDATE started with trace file: period=[%.8f-%.8f] s, duration=%.8f s, "
                "initial_carbon_intensity=%.2f g/kWh, previous_carbon_footprint=%.8f g",
                host_name.c_str(), start_time, finish_time, finish_time - start_time, this->carbon_intensity,
                previous_carbon_footprint);

      if (host_traces != carbon_intensity_traces.end()) {
        // Get all carbon intensity changes that occurred during this period
        std::vector<std::pair<double, double>> intensity_changes; // (time, intensity)

        for (const auto& trace_entry : host_traces->second) {
          double event_time = std::stod(trace_entry.first);
          if (event_time > start_time && event_time <= finish_time) {
            intensity_changes.push_back({event_time, trace_entry.second});
          }
        }

        // Sort by time
        std::sort(intensity_changes.begin(), intensity_changes.end());

        XBT_DEBUG("[%s] Found %zu carbon_intensity changes in period [%.8f-%.8f] s", host_name.c_str(),
                  intensity_changes.size(), start_time, finish_time);

        // Add the current intensity at the end if no changes occurred
        if (intensity_changes.empty()) {
          intensity_changes.push_back({finish_time, this->carbon_intensity});
          XBT_DEBUG("[%s] No intensity change found, using current intensity: %.2f g/kWh", host_name.c_str(),
                    this->carbon_intensity);
        } else {
          // Use the last intensity that was active before finish_time
          double last_intensity = this->carbon_intensity;
          for (const auto& change : intensity_changes) {
            if (change.first <= finish_time) {
              last_intensity = change.second;
            }
          }
          intensity_changes.push_back({finish_time, last_intensity});

          XBT_DEBUG("[%s] Intensity changes found:", host_name.c_str());
          for (const auto& change : intensity_changes) {
            XBT_DEBUG("[%s]   t=%.8f s -> carbon_intensity=%.2f g/kWh", host_name.c_str(), change.first, change.second);
          }
        }

        // Calculate carbon footprint for each time segment
        double current_intensity = this->carbon_intensity;
        int segment_number       = 0;

        XBT_DEBUG("[%s] Calculating carbon footprint by segments:", host_name.c_str());

        for (const auto& change : intensity_changes) {
          double segment_end_time = change.first;
          double segment_duration = segment_end_time - current_time;

          if (segment_duration > 0) {
            double instantaneous_power_consumption = sg_host_get_current_consumption(host_);
            double energy_this_segment             = instantaneous_power_consumption * segment_duration;
            double energy_this_segment_kwh         = energy_this_segment / 3.6e6;
            double carbon_this_segment             = energy_this_segment_kwh * current_intensity;

            total_carbon_this_step += carbon_this_segment;
            segment_number++;

            XBT_DEBUG("[carbon_footprint_segment of %s] period=[%.8f-%.8f]; power=%.2f W; carbon rate=%.2f g/kWh; "
                      "carbon this segment: %.8f g",
                      host_->get_cname(), current_time, segment_end_time, instantaneous_power_consumption,
                      current_intensity, carbon_this_segment);
          }

          current_time      = segment_end_time;
          current_intensity = change.second;
        }

        // Update the current carbon intensity to the final value
        this->carbon_intensity = current_intensity;

        XBT_DEBUG("[%s] Total calculated in this update: carbon_this_step=%.8f g, final_carbon_intensity=%.2f g/kWh, "
                  "segments_processed=%d",
                  host_name.c_str(), total_carbon_this_step, this->carbon_intensity, segment_number);

      } else {
        // No traces for this host, use standard calculation
        double instantaneous_power_consumption = sg_host_get_current_consumption(host_);

        double energy_this_step     = instantaneous_power_consumption * (finish_time - current_time);
        double energy_this_step_kwh = energy_this_step / 3.6e6;
        total_carbon_this_step      = energy_this_step_kwh * this->carbon_intensity;

        XBT_DEBUG("[%s] No trace found for this host, using standard calculation: power=%.2f W, period=%.8f s, "
                  "energy=%.8f kWh, carbon=%.8f g",
                  host_name.c_str(), instantaneous_power_consumption, finish_time - current_time, energy_this_step_kwh,
                  total_carbon_this_step);
      }
    } else {
      // No trace file loaded, use standard calculation
      double instantaneous_power_consumption = sg_host_get_current_consumption(host_);
      double energy_this_step                = instantaneous_power_consumption * (finish_time - current_time);
      double energy_this_step_kwh            = energy_this_step / 3.6e6;
      total_carbon_this_step                 = energy_this_step_kwh * this->carbon_intensity;
    }

    total_carbon_footprint = previous_carbon_footprint + total_carbon_this_step;
    last_updated           = finish_time;

    if (trace_file_loaded) {
      XBT_DEBUG("[%s] UPDATE finished: previous_carbon_footprint=%.8f g + carbon_added=%.8f g = "
                "total_carbon_footprint=%.8f g, last_updated=%.8f s",
                host_->get_cname(), previous_carbon_footprint, total_carbon_this_step, total_carbon_footprint,
                last_updated);
    }

    XBT_DEBUG("[update_carbon_footprint of %s] period=[%.8f-%.8f]; total carbon footprint before: %.8f g -> added now: "
              "%.8f g",
              host_->get_cname(), start_time, finish_time, previous_carbon_footprint, total_carbon_this_step);
  }
}

HostCarbonFootprint::HostCarbonFootprint(simgrid::s4u::Host* ptr) : host_(ptr)
{
  this->last_energy = sg_host_get_consumed_energy(host_);

  const char* carbon_intensity_str = host_->get_property("carbon_intensity");
  if (carbon_intensity_str != nullptr) {
    try {
      this->carbon_intensity = std::stod(carbon_intensity_str);
    } catch (const std::invalid_argument&) {
      XBT_WARN("Invalid carbon_intensity value for host %s. Using default value: %f g/kWh.", host_->get_cname(),
               this->carbon_intensity);
    }
  } else {
    XBT_WARN("Host '%s': Missing value for property 'carbon_intensity', using default value (%.2f).",
             host_->get_cname(), this->carbon_intensity);
  }

  XBT_DEBUG("Creating HostCarbonFootprint for host %s with carbon rate: %f.", host_->get_cname(),
            this->carbon_intensity);
}

double HostCarbonFootprint::getHostCarbonFootprint()
{
  if (last_updated < simgrid::s4u::Engine::get_clock()) // We need to simcall this as it modifies the environment
    simgrid::kernel::actor::simcall_answered(std::bind(&HostCarbonFootprint::update, this));

  return total_carbon_footprint;
}

double HostCarbonFootprint::getHostCarbonIntensity()
{
  if (last_updated < simgrid::s4u::Engine::get_clock()) // We need to simcall this as it modifies the environment
    simgrid::kernel::actor::simcall_answered(std::bind(&HostCarbonFootprint::update, this));

  return carbon_intensity;
}

void HostCarbonFootprint::setHostCarbonIntensity(double carbon_intensity)
{
  if (last_updated < simgrid::s4u::Engine::get_clock()) // We need to simcall this as it modifies the environment
    simgrid::kernel::actor::simcall_answered(std::bind(&HostCarbonFootprint::update, this));

  this->carbon_intensity = carbon_intensity;
}

HostCarbonFootprint::~HostCarbonFootprint() = default;

} // namespace simgrid::plugin

using simgrid::plugin::HostCarbonFootprint;

/* **************************** events  callback *************************** */

static void on_creation(simgrid::s4u::Host& host)
{

  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  if (not host.extension<HostCarbonFootprint>()) {
    host.extension_set(new HostCarbonFootprint(&host));
  } else {
    return;
  }
}

static void on_action_state_change(simgrid::kernel::resource::CpuAction const& action,
                                   simgrid::kernel::resource::Action::State /*previous*/)
{
  for (simgrid::kernel::resource::CpuImpl const* cpu : action.cpus()) {
    simgrid::s4u::Host* host = cpu->get_iface();
    if (host != nullptr) {
      // If it's a VM, take the corresponding PM
      if (const auto* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(host))
        host = vm->get_pm();

      // Get the host_carbon_footprint extension for the relevant host
      auto* host_carbon_footprint = host->extension<HostCarbonFootprint>();

      if (host_carbon_footprint->get_last_update_time() < simgrid::s4u::Engine::get_clock())
        host_carbon_footprint->update();
    }
  }
}

/* This callback is fired either when the host changes its state (on/off) ("onStateChange") or its speed
 * (because the user changed the pstate, or because of external trace events) ("onSpeedChange") */
static void on_host_change(simgrid::s4u::Host const& h)
{
  const auto* host = &h;
  if (const auto* vm = dynamic_cast<simgrid::s4u::VirtualMachine const*>(host)) // Take the PM of virtual machines
    host = vm->get_pm();

  host->extension<HostCarbonFootprint>()->update();
}

static void on_host_destruction(simgrid::s4u::Host const& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine const*>(&host)) // Ignore virtual machines
    return;

  XBT_INFO("Carbon emitted by host %s: %f g", host.get_cname(),
           host.extension<HostCarbonFootprint>()->getHostCarbonFootprint());
}

static void on_simulation_end()
{
  // Do nothing. Maybe do something in another version.
  return;
}

/* **************************** Public interface *************************** */

/** \ingroup plugin_carbon_footprint
 * \brief Enable host carbon footprint plugin
 * \details Enable carbon footprint plugin to get the carbon footprint of each cpu.
 */
void sg_host_carbon_footprint_plugin_init()
{
  if (HostCarbonFootprint::EXTENSION_ID.valid())
    return;

  sg_host_energy_plugin_init();
  HostCarbonFootprint::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostCarbonFootprint>();

  simgrid::s4u::Host::on_creation_cb(&on_creation);
  simgrid::s4u::Host::on_onoff_cb(&on_host_change);
  simgrid::s4u::Host::on_destruction_cb(&on_host_destruction);
  simgrid::s4u::Host::on_exec_state_change_cb(&on_action_state_change);
}

static void ensure_plugin_inited()
{
  if (not HostCarbonFootprint::EXTENSION_ID.valid())
    throw simgrid::xbt::InitializationError(
        "The Carbon Footprint plugin is not active. Please call sg_host_carbon_footprint_plugin_init() "
        "before calling any function related to that plugin.");
}

/** @ingroup plugin_carbon_footprint
 *  @brief Returns the total carbon footprint by the host so far (in g/kwh)
 *
 *  Please note that since the carbon footprint is lazily updated, it may require a simcall to update it.
 *  The result is that the actor requesting this value will be interrupted,
 *  the value will be updated in kernel mode before returning the control to the requesting actor.
 */

double sg_host_get_carbon_footprint(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<HostCarbonFootprint>()->getHostCarbonFootprint();
}

double sg_host_get_carbon_intensity(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<HostCarbonFootprint>()->getHostCarbonIntensity();
}

void sg_host_set_carbon_intensity(const_sg_host_t host, double carbon_intensity)
{
  ensure_plugin_inited();
  host->extension<HostCarbonFootprint>()->setHostCarbonIntensity(carbon_intensity);
}

void sg_host_carbon_footprint_load_trace_file(const char* trace_file)
{
  ensure_plugin_inited();

  if (trace_file == nullptr || strlen(trace_file) == 0) {
    XBT_WARN("Trace file path is null or empty.");
    return;
  }

  std::ifstream file(trace_file);
  xbt_enforce(file.is_open(), "Cannot open carbon trace file: %s", trace_file);

  std::string line;
  std::getline(file, line); // Skip header line

  while (std::getline(file, line)) {
    std::istringstream ss(line);
    std::string host_id, timestamp, carbon_intensity_str;

    try {
      if (std::getline(ss, host_id, ',') && std::getline(ss, timestamp, ',') &&
          std::getline(ss, carbon_intensity_str)) {
        double carbon_intensity                     = std::stod(carbon_intensity_str);
        carbon_intensity_traces[host_id][timestamp] = carbon_intensity;
      } else {
        XBT_WARN("Malformed line in carbon trace file: '%s'.", line.c_str());
      }
    } catch (const std::exception& e) {
      XBT_WARN("Error parsing line in carbon trace file: '%s'. Error: %s", line.c_str(), e.what());
    }
  }

  file.close();
  trace_file_loaded = true;

  XBT_INFO("Carbon trace file '%s' loaded successfully. Found traces for %zu hosts.", trace_file,
           carbon_intensity_traces.size());
}
