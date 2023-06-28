/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <simgrid/Exception.hpp>
#include <simgrid/plugins/battery.hpp>
#include <simgrid/plugins/energy.h>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/simix.hpp>
#include <xbt/asserts.h>
#include <xbt/log.h>

#include "src/kernel/resource/CpuImpl.hpp"
#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(battery, "Battery management", nullptr)
/** @defgroup plugin_battery plugin_battery Plugin Battery

  @beginrst

This is the battery plugin

  @endrst
 */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Battery, kernel, "Logging specific to the battery plugin");

namespace simgrid::plugins {

/* BatteryModel */

BatteryModel::BatteryModel() : Model("BatteryModel") {}

void BatteryModel::add_battery(BatteryPtr b)
{
  batteries_.push_back(b);
}

void BatteryModel::update_actions_state(double now, double delta)
{
  for (auto battery : batteries_)
    battery->update();
}

double BatteryModel::next_occurring_event(double now)
{
  double time_delta = -1;
  for (auto battery : batteries_) {
    double time_delta_battery = battery->next_occurring_event();
    time_delta                = time_delta == -1 or time_delta_battery < time_delta ? time_delta_battery : time_delta;
  }
  return time_delta;
}

/* Event */

Battery::Event::Event(double state_of_charge, Flow flow, std::function<void()> callback, bool repeat)
    : state_of_charge_(state_of_charge), flow_(flow), callback_(callback), repeat_(repeat)
{
}

std::shared_ptr<Battery::Event> Battery::Event::init(double state_of_charge, Flow flow, std::function<void()> callback,
                                                     bool repeat)
{
  return std::make_shared<Battery::Event>(state_of_charge, flow, callback, repeat);
}

/* Battery */

std::shared_ptr<BatteryModel> Battery::battery_model_;

void Battery::init_plugin()
{
  auto model = std::make_shared<BatteryModel>();
  simgrid::s4u::Engine::get_instance()->add_model(model);
  Battery::battery_model_ = model;
}

void Battery::update()
{
  kernel::actor::simcall_answered([this] {
    double now          = s4u::Engine::get_clock();
    double time_delta_s = now - last_updated_;

    // Nothing to update
    if (time_delta_s <= 0)
      return;

    // Calculate energy provided / consumed during this step
    double provided_power_w = 0;
    double consumed_power_w = 0;
    for (auto const& [host, active] : host_loads_)
      provided_power_w += active ? sg_host_get_current_consumption(host) : 0;
    for (auto const& [name, load] : named_loads_) {
      if (load > 0)
        provided_power_w += load;
      else
        consumed_power_w += -load;
    }
    double energy_lost_delta_j   = provided_power_w / discharge_efficiency_ * time_delta_s;
    double energy_gained_delta_j = consumed_power_w * charge_efficiency_ * time_delta_s;

    // Check that the provided/consumed energy is valid
    energy_lost_delta_j = std::min(energy_lost_delta_j, energy_stored_j_ + energy_gained_delta_j);
    /* Charging deteriorate the capacity, but the capacity is used to evaluate the maximum charge so
       we need to evaluate the theorethical new capacity in the worst case when we fully charge the battery */
    double new_tmp_capacity_wh =
        (initial_capacity_wh_ *
         (1 - (energy_provided_j_ + energy_lost_delta_j * discharge_efficiency_ + energy_consumed_j_ -
               (energy_stored_j_ + energy_lost_delta_j) / charge_efficiency_) /
                  energy_budget_j_)) /
        (1 + 3600 * initial_capacity_wh_ / (charge_efficiency_ * energy_budget_j_));
    energy_gained_delta_j =
        std::min(energy_gained_delta_j, (3600 * new_tmp_capacity_wh) - energy_stored_j_ + energy_lost_delta_j);

    // Updating battery
    energy_provided_j_ += energy_lost_delta_j * discharge_efficiency_;
    energy_consumed_j_ += energy_gained_delta_j / charge_efficiency_;
    capacity_wh_ = initial_capacity_wh_ * (1 - (energy_provided_j_ + energy_consumed_j_) / energy_budget_j_);
    energy_stored_j_ += energy_gained_delta_j - energy_lost_delta_j;
    energy_stored_j_ = std::min(energy_stored_j_, 3600 * capacity_wh_);
    last_updated_    = now;

    std::vector<std::shared_ptr<Event>> to_delete = {};
    for (auto event : events_) {
      if (abs(event->time_delta_ - time_delta_s) < 0.000000001) {
        event->callback_();
        if (event->repeat_)
          event->time_delta_ = -1;
        else
          to_delete.push_back(event);
      }
    }
    for (auto event : to_delete)
      delete_event(event);
  });
}

double Battery::next_occurring_event()
{
  double provided_power_w = 0;
  double consumed_power_w = 0;
  for (auto const& [host, active] : host_loads_)
    provided_power_w += active ? sg_host_get_current_consumption(host) : 0;
  for (auto const& [name, load] : named_loads_) {
    if (load > 0)
      provided_power_w += load;
    else
      consumed_power_w += -load;
  }

  double time_delta = -1;
  for (auto& event : events_) {
    double lost_power_w   = provided_power_w / discharge_efficiency_;
    double gained_power_w = consumed_power_w * charge_efficiency_;
    // Event cannot happen
    if ((lost_power_w == gained_power_w) or (event->state_of_charge_ == energy_stored_j_ / (3600 * capacity_wh_)) or
        (lost_power_w > gained_power_w and event->flow_ == Flow::CHARGE) or
        (lost_power_w < gained_power_w and event->flow_ == Flow::DISCHARGE)) {
      continue;
    }
    // Evaluate time until event happen
    else {
      /* The time to reach a state of charge depends on the capacity, but charging and discharging deteriorate the
       * capacity, so we need to evaluate the time considering a capacity that also depends on time
       */
      event->time_delta_ = (3600 * event->state_of_charge_ * initial_capacity_wh_ *
                                (1 - (energy_provided_j_ + energy_consumed_j_) / energy_budget_j_) -
                            energy_stored_j_) /
                           (gained_power_w - lost_power_w +
                            3600 * event->state_of_charge_ * initial_capacity_wh_ *
                                (consumed_power_w + provided_power_w) / energy_budget_j_);
      if ((time_delta == -1 or event->time_delta_ < time_delta) and abs(event->time_delta_) > 0.000000001)
        time_delta = event->time_delta_;
    }
  }
  return time_delta;
}

Battery::Battery(const std::string& name, double state_of_charge, double charge_efficiency, double discharge_efficiency,
                 double initial_capacity_wh, int cycles, double depth_of_discharge)
    : name_(name)
    , energy_stored_j_(state_of_charge * 3600 * initial_capacity_wh)
    , charge_efficiency_(charge_efficiency)
    , discharge_efficiency_(discharge_efficiency)
    , initial_capacity_wh_(initial_capacity_wh)
    , capacity_wh_(initial_capacity_wh)
    , cycles_(cycles)
    , depth_of_discharge_(depth_of_discharge)
    , energy_budget_j_(initial_capacity_wh * depth_of_discharge * 3600 * cycles * 2)
{
  xbt_assert(state_of_charge >= 0 and state_of_charge <= 1, " : state of charge should be in [0, 1] (provided: %f)",
             state_of_charge);
  xbt_assert(charge_efficiency > 0 and charge_efficiency <= 1, " : charge efficiency should be in [0,1] (provided: %f)",
             charge_efficiency);
  xbt_assert(discharge_efficiency > 0 and discharge_efficiency <= 1,
             " : discharge efficiency should be in [0,1] (provided: %f)", discharge_efficiency);
  xbt_assert(initial_capacity_wh > 0, " : initial capacity should be > 0 (provided: %f)", initial_capacity_wh);
  xbt_assert(cycles > 0, " : cycles should be > 0 (provided: %d)", cycles);
  xbt_assert(depth_of_discharge > 0 and depth_of_discharge <= 1,
             " : depth of discharge should be in ]0, 1] (provided: %f)", depth_of_discharge);
}

BatteryPtr Battery::init(const std::string& name, double state_of_charge, double charge_efficiency,
                         double discharge_efficiency, double initial_capacity_wh, int cycles, double depth_of_discharge)
{
  static bool plugin_inited = false;
  if (not plugin_inited) {
    init_plugin();
    plugin_inited = true;
  }
  auto battery = BatteryPtr(new Battery(name, state_of_charge, charge_efficiency, discharge_efficiency,
                                        initial_capacity_wh, cycles, depth_of_discharge));
  battery_model_->add_battery(battery);
  return battery;
}

void Battery::set_load(const std::string& name, double power_w)
{
  named_loads_[name] = power_w;
}

void Battery::connect_host(s4u::Host* h, bool active)
{
  host_loads_[h] = active;
}

double Battery::get_state_of_charge()
{
  return energy_stored_j_ / (3600 * capacity_wh_);
}

double Battery::get_state_of_health()
{
  return 1 - ((energy_provided_j_ + energy_consumed_j_) / energy_budget_j_);
}

double Battery::get_capacity()
{
  return capacity_wh_;
}

double Battery::get_energy_provided()
{
  return energy_provided_j_;
}

double Battery::get_energy_consumed()
{
  return energy_consumed_j_;
}

double Battery::get_energy_stored(std::string unit)
{
  if (unit == "J")
    return energy_stored_j_;
  else if (unit == "Wh")
    return energy_stored_j_ / 3600;
  else
    xbt_die("Invalid unit. Valid units are J (default) or Wh.");
}

std::shared_ptr<Battery::Event> Battery::create_event(double state_of_charge, Flow flow, std::function<void()> callback,
                                                      bool repeat)
{
  auto event = Event::init(state_of_charge, flow, callback, repeat);
  events_.push_back(event);
  return event;
}

std::vector<std::shared_ptr<Battery::Event>> Battery::get_events()
{
  return events_;
}

void Battery::delete_event(std::shared_ptr<Event> event)
{
  events_.erase(
      std::remove_if(events_.begin(), events_.end(), [&event](std::shared_ptr<Event> e) { return event == e; }),
      events_.end());
}
} // namespace simgrid::plugins