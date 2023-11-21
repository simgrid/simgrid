/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <simgrid/Exception.hpp>
#include <simgrid/plugins/chiller.hpp>
#include <simgrid/plugins/energy.h>
#include <simgrid/simix.hpp>
#include <xbt/asserts.h>
#include <xbt/log.h>

#include "src/kernel/resource/CpuImpl.hpp"
#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(chiller, "Chiller management", nullptr)

/** @defgroup plugin_chiller Plugin Chiller

  @beginrst

This is the chiller plugin, enabling management of chillers.

Chiller
.......

A chiller is placed inside a room with several machines. The role of the chiller is to keep the temperature of the room
below a threshold. This plugin and its equations are based on the paper "Co-simulation of FMUs and Distributed
Applications with SimGrid" by Camus et al. (https://hal.science/hal-01762540).

The heat generated inside the room :math:`Q_{room}` depends on the heat from the machines :math:`Q_{machines}` and
from the heat of the other devices, such as lighing, accounted using a factor :math:`\alpha` such as:

.. math::

  Q_{room} = (1 + \alpha) \times Q_{machines}

This energy heats the input temperature :math:`T_{in}` and gives an output temperature :math:`T_{out}` based on the
mass of air inside the room :math:`m_{air}` and its specific heat :math:`C_{p}`:

.. math::

  T_{out} = T_{in} + {Q_{room} \over m_{air} \times C_{p}}

If the output temperature is above the goal temperature :math:`T_{goal}` the chiller compensates the excessive heat
using electrical energy :math:`Q_{cooling}` depending on its cooling efficiency :math:`\eta_{cooling}` :

.. math::

  Q_{cooling} = (T_{out} - T_{goal}) \times m_{air} \times C_{p} / \eta_{cooling}

The chiller has a power threshold that cannot be exceeded. If the power needed is above this threshold, or if the
chiller is not active, the temperature of the room increases.

  @endrst
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Chiller, kernel, "Logging specific to the solar panel plugin");

namespace simgrid::plugins {
xbt::signal<void(Chiller*)> Chiller::on_power_change; // initialisation of static field

/* ChillerModel */

ChillerModel::ChillerModel() : Model("ChillerModel") {}

void ChillerModel::add_chiller(ChillerPtr c)
{
  chillers_.push_back(c);
}

void ChillerModel::update_actions_state(double now, double delta)
{
  for (auto chiller : chillers_)
    chiller->update();
}

double ChillerModel::next_occurring_event(double now)
{
  static bool init = false;
  if (not init) {
    init = true;
    return 0;
  } else
    return -1;
}

/* Chiller */

std::shared_ptr<ChillerModel> Chiller::chiller_model_;

void Chiller::init_plugin()
{
  auto model = std::make_shared<ChillerModel>();
  simgrid::s4u::Engine::get_instance()->add_model(model);
  Chiller::chiller_model_ = model;
}

void Chiller::update()
{
  simgrid::kernel::actor::simcall_answered([this] {
    double now          = s4u::Engine::get_clock();
    double time_delta_s = now - last_updated_;

    if (time_delta_s <= 0)
      return;

    double hosts_power_w = 0;
    for (auto const& host : hosts_) {
      hosts_power_w += sg_host_get_current_consumption(host);
    }

    double heat_generated_j = hosts_power_w * (1 + alpha_) * time_delta_s;
    temp_out_c_             = temp_in_c_ + heat_generated_j / (air_mass_kg_ * specific_heat_j_per_kg_per_c_);
    double cooling_demand_w =
        std::max(temp_out_c_ - goal_temp_c_, 0.0) * air_mass_kg_ * specific_heat_j_per_kg_per_c_ / time_delta_s;
    if (not active_)
      power_w_ = 0;
    else
      power_w_ = std::min(max_power_w_, cooling_demand_w / cooling_efficiency_);
    temp_in_c_ =
        temp_out_c_ - (power_w_ * time_delta_s * cooling_efficiency_) / (air_mass_kg_ * specific_heat_j_per_kg_per_c_);
    energy_consumed_j_ += power_w_ * time_delta_s;
    last_updated_ = now;
  });
}

Chiller::Chiller(const std::string& name, double air_mass_kg, double specific_heat_j_per_kg_per_c, double alpha,
                 double cooling_efficiency, double initial_temp_c, double goal_temp_c, double max_power_w)
    : name_(name)
    , air_mass_kg_(air_mass_kg)
    , specific_heat_j_per_kg_per_c_(specific_heat_j_per_kg_per_c)
    , alpha_(alpha)
    , cooling_efficiency_(cooling_efficiency)
    , temp_in_c_(initial_temp_c)
    , temp_out_c_(initial_temp_c)
    , goal_temp_c_(goal_temp_c)
    , max_power_w_(max_power_w)
{
  xbt_assert(air_mass_kg > 0, ": air mass must be > 0 (provided: %f)", air_mass_kg);
  xbt_assert(specific_heat_j_per_kg_per_c > 0, ": specific heat must be > 0 (provided: %f)",
             specific_heat_j_per_kg_per_c);
  xbt_assert(alpha >= 0, ": alpha must be >= 0 (provided: %f)", alpha);
  xbt_assert(cooling_efficiency >= 0 and cooling_efficiency <= 1,
             ": cooling efficiency must be in [0,1] (provided: %f)", cooling_efficiency);
  xbt_assert(max_power_w >= 0, ": maximal power must be >=0 (provided: %f)", max_power_w);
}

/** @ingroup plugin_chiller
 *  @param name The name of the Chiller.
 *  @param air_mass_kg The air mass of the room managed by the Chiller in kg (> 0).
 *  @param specific_heat_j_per_kg_per_c The specific heat of air in J per kg per °C (> 0).
 *  @param alpha The ratio of the other devices in the total heat dissipation (e.g. lighting, Power Distribution Unit)
 * (>= 0).
 *  @param cooling_efficiency The cooling efficiency of the Chiller [0, 1].
 *  @param initial_temp_c The initial temperature of the room managed by the Chiller.
 *  @param goal_temp_c The goal temperature of the room. The Chiller is idle below this temperature.
 *  @param max_power_w The maximal power delivered by the Chiller in W (> 0). If this power is reached the room
 * temperature will raise above the goal temperature.
 *  @return A ChillerPtr pointing to the new Chiller.
 */
ChillerPtr Chiller::init(const std::string& name, double air_mass_kg, double specific_heat_j_per_kg_per_c, double alpha,
                         double cooling_efficiency, double initial_temp_c, double goal_temp_c, double max_power_w)
{
  static bool plugin_inited = false;
  if (not plugin_inited) {
    init_plugin();
    plugin_inited = true;
  }
  auto chiller = ChillerPtr(new Chiller(name, air_mass_kg, specific_heat_j_per_kg_per_c, alpha, cooling_efficiency,
                                        initial_temp_c, goal_temp_c, max_power_w));
  chiller_model_->add_chiller(chiller);
  return chiller;
}

/** @ingroup plugin_chiller
 *  @param name The new name of the Chiller.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::set_name(std::string name)
{
  simgrid::kernel::actor::simcall_answered([this, name] { name_ = name; });
  return this;
}

/** @ingroup plugin_chiller
 *  @param air_mass_kg The new air mass of the Chiller in kg.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::set_air_mass(double air_mass_kg)
{
  xbt_assert(air_mass_kg > 0, ": air mass must be > 0 (provided: %f)", air_mass_kg);
  simgrid::kernel::actor::simcall_answered([this, air_mass_kg] { air_mass_kg_ = air_mass_kg; });
  return this;
}

/** @ingroup plugin_chiller
 *  @param specific_heat_j_per_kg_per_c The specific heat of the Chiller in J per kg per °C.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::set_specific_heat(double specific_heat_j_per_kg_per_c)
{
  xbt_assert(specific_heat_j_per_kg_per_c > 0, ": specific heat must be > 0 (provided: %f)",
             specific_heat_j_per_kg_per_c);
  simgrid::kernel::actor::simcall_answered(
      [this, specific_heat_j_per_kg_per_c] { specific_heat_j_per_kg_per_c_ = specific_heat_j_per_kg_per_c; });
  return this;
}

/** @ingroup plugin_chiller
 *  @param alpha The new alpha of the Chiller.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::set_alpha(double alpha)
{
  xbt_assert(alpha >= 0, ": alpha must be >= 0 (provided: %f)", alpha);
  simgrid::kernel::actor::simcall_answered([this, alpha] { alpha_ = alpha; });
  return this;
}

/** @ingroup plugin_chiller
 *  @param cooling_efficiency The new coolingefficiency of the Chiller.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::set_cooling_efficiency(double cooling_efficiency)
{
  xbt_assert(cooling_efficiency >= 0 and cooling_efficiency <= 1,
             ": cooling efficiency must be in [0,1] (provided: %f)", cooling_efficiency);
  simgrid::kernel::actor::simcall_answered([this, cooling_efficiency] { cooling_efficiency_ = cooling_efficiency; });
  return this;
}

/** @ingroup plugin_chiller
 *  @param goal_temp_c The new goal temperature of the Chiller in °C.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::set_goal_temp(double goal_temp_c)
{
  simgrid::kernel::actor::simcall_answered([this, goal_temp_c] { goal_temp_c_ = goal_temp_c; });
  return this;
}

/** @ingroup plugin_chiller
 *  @param max_power_w The new maximal power of the Chiller in W.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::set_max_power(double max_power_w)
{
  xbt_assert(max_power_w >= 0, ": maximal power must be >=0 (provided: %f)", max_power_w);
  simgrid::kernel::actor::simcall_answered([this, max_power_w] { max_power_w_ = max_power_w; });
  return this;
}

/** @ingroup plugin_chiller
 *  @param active The new active status of the Chiller.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::set_active(bool active)
{
  simgrid::kernel::actor::simcall_answered([this, active] { active_ = active; });
  return this;
}

/** @ingroup plugin_chiller
 *  @param host The host to add to the room managed by the Chiller.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::add_host(s4u::Host* host)
{
  simgrid::kernel::actor::simcall_answered([this, host] { hosts_.insert(host); });
  return this;
}

/** @ingroup plugin_chiller
 *  @param host The host to remove from the room managed by the Chiller.
 *  @return A ChillerPtr pointing to the modified Chiller.
 */
ChillerPtr Chiller::remove_host(s4u::Host* host)
{
  simgrid::kernel::actor::simcall_answered([this, host] { hosts_.erase(host); });
  return this;
}

/** @ingroup plugin_chiller
 *  @return The time to reach to goal temp, assuming that the system remain in the same state.
 */
double Chiller::get_time_to_goal_temp() const
{
  if (goal_temp_c_ == temp_in_c_)
    return 0;

  double heat_power_w = 0;
  for (auto const& host : hosts_)
    heat_power_w += sg_host_get_current_consumption(host);
  heat_power_w = heat_power_w * (1 + alpha_);

  if (temp_in_c_ < goal_temp_c_)
    return air_mass_kg_ * (goal_temp_c_ - temp_in_c_) * specific_heat_j_per_kg_per_c_ / heat_power_w;

  if (not active_)
    return -1;
  else
    return air_mass_kg_ * (temp_in_c_ - goal_temp_c_) * specific_heat_j_per_kg_per_c_ /
           (power_w_ * cooling_efficiency_ - heat_power_w);
}
} // namespace simgrid::plugins
