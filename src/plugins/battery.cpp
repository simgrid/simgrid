/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <simgrid/Exception.hpp>
#include <simgrid/plugins/battery.hpp>
#include <simgrid/plugins/energy.h>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/simix.hpp>

#include "src/kernel/resource/CpuImpl.hpp"
#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(battery, "Battery management", nullptr)
/** @defgroup plugin_battery plugin_battery Plugin Battery

  @beginrst

This is the battery plugin, enabling management of batteries.

Batteries
.........

A battery has an initial State of Charge :math:`SoC`, a nominal charge power, a nominal discharge power, a charge
efficiency :math:`\eta_{charge}`, a discharge efficiency :math:`\eta_{discharge}`, an initial capacity
:math:`C_{initial}` and a number of cycle :math:`N`.

The nominal charge(discharge) power is the maximum power the Battery can consume(provide), before application of the
charge(discharge) efficiency factor. For instance, if a load provides(consumes) 100W to(from) the Battery with a nominal
charge(discharge) power of 50W and a charge(discharge) efficiency of 0.9, the Battery will only gain(provide) 45W.

We distinguish the energy provided :math:`E_{provided}` / consumed :math:`E_{consumed}` from the energy lost
:math:`E_{lost}` / gained :math:`E_{gained}`. The energy provided / consumed shows the external point of view, and the
energy lost / gained shows the internal point of view:

.. math::

  E_{lost} = {E_{provided} \over \eta_{discharge}}

  E_{gained} = E_{consumed} \times \eta_{charge}

For instance, if you apply a load of 100W to a battery for 10s with a discharge efficiency of 0.8, the energy provided
will be equal to 10kJ, and the energy lost will be equal to 12.5kJ.

All the energies are positive, but loads connected to a Battery may be positive or negative, as explained in the next
section.

Use the battery reduces its State of Health :math:`SoH` and its capacity :math:`C` linearly in consequence:

.. math::

  SoH = 1 - {E_{lost} + E_{gained} \over E_{budget}}

  C = C_{initial} \times SoH

With:

.. math::

  E_{budget} = C_{initial} \times N \times 2

Plotting the output of the example "battery-degradation" highlights the linear decrease of the :math:`SoH` due to a
continuous use of the battery alternating between charge and discharge:

.. image:: /img/battery_degradation.svg
   :align: center

The natural depletion of batteries over time is not taken into account.

Loads & Hosts
.............

You can add named loads to a battery. Those loads may be positive and consume energy from the battery, or negative and
provide energy to the battery. You can also connect hosts to a battery. Theses hosts will consume their energy from the
battery until the battery is empty or until the connection between the hosts and the battery is set inactive.

Handlers
........

You can schedule handlers that will happen at specific SoC of the battery and trigger a callback.
Theses handlers may be recurrent, for instance you may want to always set all loads to zero and deactivate all hosts
connections when the battery reaches 20% SoC.

Connector
.........

A Battery can act as a connector to connect Solar Panels direcly to loads. Such Battery is created without any
parameter, cannot store energy and has a transfer efficiency of 100%.

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
  static bool init = false;
  if (!init) {
    init = true;
    return 0;
  }
  double time_delta = -1;
  for (auto battery : batteries_) {
    double time_delta_battery = battery->next_occurring_handler();
    time_delta                = time_delta == -1 or time_delta_battery < time_delta ? time_delta_battery : time_delta;
  }
  return time_delta;
}

/* Handler */

Battery::Handler::Handler(double state_of_charge, Flow flow, Persistancy p, std::function<void()> callback)
    : state_of_charge_(state_of_charge), flow_(flow), callback_(callback), persistancy_(p)
{
}

std::shared_ptr<Battery::Handler> Battery::Handler::init(double state_of_charge, Flow flow, Persistancy p,
                                                         std::function<void()> callback)
{
  return std::make_shared<Battery::Handler>(state_of_charge, flow, p, callback);
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
    for (auto const& [name, pair] : named_loads_) {
      if (not pair.first)
        continue;
      if (pair.second > 0)
        provided_power_w += pair.second;
      else
        consumed_power_w += -pair.second;
    }

    provided_power_w = std::min(provided_power_w, nominal_discharge_power_w_ * discharge_efficiency_);
    consumed_power_w = std::min(consumed_power_w, -nominal_charge_power_w_);

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

    // This battery is a simple connector, we only update energy provided and consumed
    if (energy_budget_j_ == 0) {
      energy_consumed_j_ = energy_provided_j_;
      last_updated_      = now;
      return;
    }

    capacity_wh_ =
        initial_capacity_wh_ *
        (1 - (energy_provided_j_ / discharge_efficiency_ + energy_consumed_j_ * charge_efficiency_) / energy_budget_j_);
    energy_stored_j_ += energy_gained_delta_j - energy_lost_delta_j;
    energy_stored_j_ = std::min(energy_stored_j_, 3600 * capacity_wh_);
    last_updated_    = now;

    auto handlers_2 = handlers_;
    for (auto handler : handlers_2) {
      if (abs(handler->time_delta_ - time_delta_s) < 0.000000001) {
        handler->callback_();
        if (handler->persistancy_ == Handler::Persistancy::PERSISTANT)
          handler->time_delta_ = -1;
        else
          delete_handler(handler);
      }
    }
  });
}

double Battery::next_occurring_handler()
{
  double provided_power_w = 0;
  double consumed_power_w = 0;
  for (auto const& [host, active] : host_loads_)
    provided_power_w += active ? sg_host_get_current_consumption(host) : 0;
  for (auto const& [name, pair] : named_loads_) {
    if (not pair.first)
      continue;
    if (pair.second > 0)
      provided_power_w += pair.second;
    else
      consumed_power_w += -pair.second;
  }

  provided_power_w = std::min(provided_power_w, nominal_discharge_power_w_ * discharge_efficiency_);
  consumed_power_w = std::min(consumed_power_w, -nominal_charge_power_w_);

  double time_delta = -1;
  for (auto& handler : handlers_) {
    double lost_power_w   = provided_power_w / discharge_efficiency_;
    double gained_power_w = consumed_power_w * charge_efficiency_;
    if ((lost_power_w == gained_power_w) or (handler->state_of_charge_ == get_state_of_charge()) or
        (lost_power_w > gained_power_w and
         (handler->flow_ == Flow::CHARGE or handler->state_of_charge_ > get_state_of_charge())) or
        (lost_power_w < gained_power_w and
         (handler->flow_ == Flow::DISCHARGE or handler->state_of_charge_ < get_state_of_charge()))) {
      continue;
    }
    // Evaluate time until handler happen
    else {
      /* The time to reach a state of charge depends on the capacity, but charging and discharging deteriorate the
       * capacity, so we need to evaluate the time considering a capacity that also depends on time
       */
      handler->time_delta_ =
          (3600 * handler->state_of_charge_ * initial_capacity_wh_ *
               (1 - (energy_provided_j_ / discharge_efficiency_ + energy_consumed_j_ * charge_efficiency_) /
                        energy_budget_j_) -
           energy_stored_j_) /
          (gained_power_w - lost_power_w +
           3600 * handler->state_of_charge_ * initial_capacity_wh_ * (gained_power_w + lost_power_w) /
               energy_budget_j_);
      if ((time_delta == -1 or handler->time_delta_ < time_delta) and abs(handler->time_delta_) > 0.000000001)
        time_delta = handler->time_delta_;
    }
  }
  return time_delta;
}

Battery::Battery(const std::string& name, double state_of_charge, double nominal_charge_power_w,
                 double nominal_discharge_power_w, double charge_efficiency, double discharge_efficiency,
                 double initial_capacity_wh, int cycles)
    : name_(name)
    , nominal_charge_power_w_(nominal_charge_power_w)
    , nominal_discharge_power_w_(nominal_discharge_power_w)
    , charge_efficiency_(charge_efficiency)
    , discharge_efficiency_(discharge_efficiency)
    , initial_capacity_wh_(initial_capacity_wh)
    , energy_budget_j_(initial_capacity_wh * 3600 * cycles * 2)
    , capacity_wh_(initial_capacity_wh)
    , energy_stored_j_(state_of_charge * 3600 * initial_capacity_wh)
{
  xbt_assert(nominal_charge_power_w <= 0, " : nominal charge power must be <= 0 (provided: %f)",
             nominal_charge_power_w);
  xbt_assert(nominal_discharge_power_w >= 0, " : nominal discharge power must be non-negative (provided: %f)",
             nominal_discharge_power_w);
  xbt_assert(state_of_charge >= 0 and state_of_charge <= 1, " : state of charge should be in [0, 1] (provided: %f)",
             state_of_charge);
  xbt_assert(charge_efficiency > 0 and charge_efficiency <= 1, " : charge efficiency should be in [0,1] (provided: %f)",
             charge_efficiency);
  xbt_assert(discharge_efficiency > 0 and discharge_efficiency <= 1,
             " : discharge efficiency should be in [0,1] (provided: %f)", discharge_efficiency);
  xbt_assert(initial_capacity_wh > 0, " : initial capacity should be > 0 (provided: %f)", initial_capacity_wh);
  xbt_assert(cycles > 0, " : cycles should be > 0 (provided: %d)", cycles);
}

/** @ingroup plugin_battery
 *  @brief Init a Battery with this constructor makes it only usable as a connector.
 *         A connector has no capacity and only delivers as much power as it receives
           with a transfer efficiency of 100%.
 *  @return A BatteryPtr pointing to the new Battery.
 */
BatteryPtr Battery::init()
{
  static bool plugin_inited = false;
  if (not plugin_inited) {
    init_plugin();
    plugin_inited = true;
  }
  auto battery = BatteryPtr(new Battery());
  battery_model_->add_battery(battery);
  return battery;
}

/** @ingroup plugin_battery
 *  @param name The name of the Battery.
 *  @param state_of_charge The initial state of charge of the Battery [0,1].
 *  @param nominal_charge_power_w The maximum power absorbed by the Battery in W (<= 0).
 *  @param nominal_discharge_power_w The maximum power delivered by the Battery in W (>= 0).
 *  @param charge_efficiency The charge efficiency of the Battery [0,1].
 *  @param discharge_efficiency The discharge efficiency of the Battery [0,1].
 *  @param initial_capacity_wh The initial capacity of the Battery in Wh (>0).
 *  @param cycles The number of charge-discharge cycles until complete depletion of the Battery capacity.
 *  @return A BatteryPtr pointing to the new Battery.
 */
BatteryPtr Battery::init(const std::string& name, double state_of_charge, double nominal_charge_power_w,
                         double nominal_discharge_power_w, double charge_efficiency, double discharge_efficiency,
                         double initial_capacity_wh, int cycles)
{
  static bool plugin_inited = false;
  if (not plugin_inited) {
    init_plugin();
    plugin_inited = true;
  }
  auto battery = BatteryPtr(new Battery(name, state_of_charge, nominal_charge_power_w, nominal_discharge_power_w,
                                        charge_efficiency, discharge_efficiency, initial_capacity_wh, cycles));
  battery_model_->add_battery(battery);
  return battery;
}

/** @ingroup plugin_battery
 *  @param name The name of the load
 *  @param power_w Power of the load in W. A positive value discharges the Battery while a negative value charges it.
 */
void Battery::set_load(const std::string& name, double power_w)
{
  kernel::actor::simcall_answered([this, &name, &power_w] {
    if (named_loads_.find(name) == named_loads_.end())
      named_loads_[name] = std::make_pair(true, power_w);
    else
      named_loads_[name].second = power_w;
  });
}

/** @ingroup plugin_battery
 *  @param name The name of the load
 *  @param active Status of the load. If false then the load is ignored by the Battery.
 */
void Battery::set_load(const std::string& name, bool active)
{
  kernel::actor::simcall_answered([this, &name, &active] { named_loads_[name].first = active; });
}

/** @ingroup plugin_battery
 *  @param host The Host to connect.
 *  @param active Status of the connected Host (default true).
 *  @brief Connect a Host to the Battery with the status active. As long as the status is true the Host takes its energy
 from the Battery. To modify this status connect again the same Host with a different status.
    @warning Do NOT connect the same Host to multiple Batteries with the status true at the same time.
    In this case all Batteries would have the full consumption from this Host.
 */
void Battery::connect_host(s4u::Host* host, bool active)
{
  kernel::actor::simcall_answered([this, &host, &active] { host_loads_[host] = active; });
}

/** @ingroup plugin_battery
 *  @return The state of charge of the battery.
 */
double Battery::get_state_of_charge()
{
  return energy_stored_j_ / (3600 * capacity_wh_);
}

/** @ingroup plugin_battery
 *  @return The state of health of the Battery.
 */
double Battery::get_state_of_health()
{
  return 1 -
         ((energy_provided_j_ / discharge_efficiency_ + energy_consumed_j_ * charge_efficiency_) / energy_budget_j_);
}

/** @ingroup plugin_battery
 *  @return The current capacity of the Battery.
 */
double Battery::get_capacity()
{
  return capacity_wh_;
}

/** @ingroup plugin_battery
 *  @return The energy provided by the Battery.
 *  @note It is the energy provided from an external point of view, after application of the discharge efficiency.
          It means that the Battery lost more energy than it has provided.
 */
double Battery::get_energy_provided()
{
  return energy_provided_j_;
}

/** @ingroup plugin_battery
 *  @return The energy consumed by the Battery.
 *  @note It is the energy consumed from an external point of view, before application of the charge efficiency.
          It means that the Battery consumed more energy than is has absorbed.
 */
double Battery::get_energy_consumed()
{
  return energy_consumed_j_;
}

/** @ingroup plugin_battery
 *  @param unit Valid units are J (default) and Wh.
 *  @return Energy stored in the Battery.
 */
double Battery::get_energy_stored(std::string unit)
{
  if (unit == "J")
    return energy_stored_j_;
  else if (unit == "Wh")
    return energy_stored_j_ / 3600;
  else
    xbt_die("Invalid unit. Valid units are J (default) or Wh.");
}

/** @ingroup plugin_battery
 *  @brief Schedule a new Handler.
 *  @param state_of_charge The state of charge at which the Handler will happen.
 *  @param flow The flow in which the Handler will happen, either when the Battery is charging or discharging.
 *  @param callback The callable to trigger when the Handler happen.
 *  @param p If the Handler is recurrent or unique.
 *  @return A shared pointer of the new Handler.
 */
std::shared_ptr<Battery::Handler> Battery::schedule_handler(double state_of_charge, Flow flow, Handler::Persistancy p,
                                                            std::function<void()> callback)
{
  auto handler = Handler::init(state_of_charge, flow, p, callback);
  handlers_.push_back(handler);
  return handler;
}

/** @ingroup plugin_battery
 *  @return A vector containing the Handlers associated to the Battery.
 */
std::vector<std::shared_ptr<Battery::Handler>> Battery::get_handlers()
{
  return handlers_;
}

/** @ingroup plugin_battery
 *  @brief Remove an Handler from the Battery.
 */
void Battery::delete_handler(std::shared_ptr<Handler> handler)
{
  handlers_.erase(std::remove_if(handlers_.begin(), handlers_.end(),
                                 [&handler](std::shared_ptr<Handler> e) { return handler == e; }),
                  handlers_.end());
}
} // namespace simgrid::plugins