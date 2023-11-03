/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <simgrid/Exception.hpp>
#include <simgrid/plugins/solar_panel.hpp>
#include <simgrid/simix.hpp>
#include <xbt/asserts.h>
#include <xbt/log.h>

#include "src/kernel/resource/CpuImpl.hpp"
#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(solar_panel, "Solar Panel management", nullptr)

/** @defgroup plugin_solar_panel Plugin Solar Panel

  @beginrst

This is the solar panel plugin, enabling management of solar panels on hosts.

This plugin allows the use of solar panels to generate power during simulation depending on size, solar
irradiance and conversion factor.

The power model is taken from the paper `"Reinforcement Learning Based Load Balancing for
Geographically Distributed Data Centres" <https://dro.dur.ac.uk/33395/1/33395.pdf?DDD280+kkgc95+vbdv77>`_ by Max Mackie
et. al.

Solar Panel
....................

A solar panel has an area :math:`A` in m², a conversion efficiency :math:`\eta` and a solar irradiance :math:`S` in
W/m². The power generated :math:`P` in W by a solar panel is given by the following equation:

.. math::

  P = A \times \eta \times S

  @endrst
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(SolarPanel, kernel, "Logging specific to the solar panel plugin");

namespace simgrid::plugins {

xbt::signal<void(SolarPanel*)> SolarPanel::on_power_change;

/* SolarPanel */

void SolarPanel::update()
{
  simgrid::kernel::actor::simcall_answered([this] {
    double power_w = conversion_efficiency_ * area_m2_ * solar_irradiance_w_per_m2_;
    if (power_w < min_power_w_)
      power_w = 0;
    if (power_w > max_power_w_)
      power_w = max_power_w_;
    auto previous_power_w = power_w_;
    power_w_              = power_w;
    if (previous_power_w != power_w_) {
      on_this_power_change(this);
      on_power_change(this);
    }
  });
}

SolarPanel::SolarPanel(std::string name, double area_m2, double conversion_efficiency, double solar_irradiance_w_per_m2,
                       double min_power_w, double max_power_w)
    : name_(name)
    , area_m2_(area_m2)
    , conversion_efficiency_(conversion_efficiency)
    , solar_irradiance_w_per_m2_(solar_irradiance_w_per_m2)
    , min_power_w_(min_power_w)
    , max_power_w_(max_power_w)
{
  xbt_assert(area_m2 >= 0, " : area must be >= 0 (provided: %f)", area_m2);
  xbt_assert(conversion_efficiency >= 0 and conversion_efficiency <= 1,
             " : conversion efficiency must be in [0,1] (provided: %f)", conversion_efficiency);
  xbt_assert(solar_irradiance_w_per_m2 >= 0, " : solar irradiance must be >= 0 (provided: %f)",
             solar_irradiance_w_per_m2);
  xbt_assert(min_power_w >= 0, " : minimal power must be >= 0 (provided: %f)", min_power_w);
  xbt_assert(max_power_w > 0, " : maximal power must be > 0 (provided: %f)", max_power_w);
  xbt_assert(max_power_w > min_power_w, " : maximal power must be above minimal power (provided: %f, %f)", max_power_w,
             min_power_w);
}

/** @ingroup plugin_solar_panel
 *  @param name The name of the Solar Panel.
 *  @param area_m2 The area of the Solar Panel in m² (> 0).
 *  @param conversion_efficiency The conversion efficiency of the Solar Panel [0,1].
 *  @param solar_irradiance_w_per_m2 The solar irradiance of the Solar Panel in W/m² (> 0).
 *  @param min_power_w The minimal power delivered by the Solar Panel in W (> 0 and < max_power_w).
 *  @param max_power_w The maximal power delivered by the Solar Panel in W (> 0 and > min_power_w).
 *  @return A SolarPanelPtr pointing to the new SolarPanel.
 */
SolarPanelPtr SolarPanel::init(const std::string& name, double area_m2, double conversion_efficiency,
                               double solar_irradiance_w_per_m2, double min_power_w, double max_power_w)
{
  auto solar_panel = SolarPanelPtr(
      new SolarPanel(name, area_m2, conversion_efficiency, solar_irradiance_w_per_m2, min_power_w, max_power_w));
  solar_panel->update();
  return solar_panel;
}

/** @ingroup plugin_solar_panel
 *  @param name The new name of the Solar Panel.
 *  @return A SolarPanelPtr pointing to the modified SolarPanel.
 */
SolarPanelPtr SolarPanel::set_name(std::string name)
{
  kernel::actor::simcall_answered([this, name] { name_ = name; });
  return this;
}

/** @ingroup plugin_solar_panel
 *  @param area_m2 The new area of the Solar Panel in m².
 *  @return A SolarPanelPtr pointing to the modified SolarPanel.
 */
SolarPanelPtr SolarPanel::set_area(double area_m2)
{
  xbt_assert(area_m2 >= 0, " : area must be > 0 (provided: %f)", area_m2);
  kernel::actor::simcall_answered([this, area_m2] { area_m2_ = area_m2; });
  update();
  return this;
}

/** @ingroup plugin_solar_panel
 *  @param e The new convesion efficiency of the Solar Panel.
 *  @return A SolarPanelPtr pointing to the modified SolarPanel.
 */
SolarPanelPtr SolarPanel::set_conversion_efficiency(double e)
{
  xbt_assert(e >= 0 and e <= 1, " : conversion efficiency must be in [0,1] (provided: %f)", e);
  kernel::actor::simcall_answered([this, e] { conversion_efficiency_ = e; });
  update();
  return this;
}

/** @ingroup plugin_solar_panel
 *  @param solar_irradiance_w_per_m2 The new solar irradiance of the Solar Panel in W/m².
 *  @return A SolarPanelPtr pointing to the modified SolarPanel.
 */
SolarPanelPtr SolarPanel::set_solar_irradiance(double solar_irradiance_w_per_m2)
{
  xbt_assert(solar_irradiance_w_per_m2 >= 0, " : solar irradiance must be >= 0 (provided: %f)",
             solar_irradiance_w_per_m2);
  kernel::actor::simcall_answered(
      [this, solar_irradiance_w_per_m2] { solar_irradiance_w_per_m2_ = solar_irradiance_w_per_m2; });
  update();
  return this;
}

/** @ingroup plugin_solar_panel
 *  @param power_w The new minimal power of the Solar Panel in W.
 *  @return A SolarPanelPtr pointing to the modified SolarPanel.
 */
SolarPanelPtr SolarPanel::set_min_power(double power_w)
{
  xbt_assert(power_w >= 0, " : minimal power must be >= 0 (provided: %f)", power_w);
  xbt_assert(max_power_w_ > power_w, " : maximal power must be above minimal power (provided: %f, max: %f)", power_w,
             max_power_w_);
  kernel::actor::simcall_answered([this, power_w] { min_power_w_ = power_w; });
  update();
  return this;
}

/** @ingroup plugin_solar_panel
 *  @param power_w The new maximal power of the Solar Panel in W.
 *  @return A SolarPanelPtr pointing to the modified SolarPanel.
 */
SolarPanelPtr SolarPanel::set_max_power(double power_w)
{
  xbt_assert(power_w > 0, " : maximal power must be > 0 (provided: %f)", power_w);
  xbt_assert(min_power_w_ < power_w, " : maximal power must be above minimal power (provided: %f, min: %f)", power_w,
             min_power_w_);
  kernel::actor::simcall_answered([this, power_w] { max_power_w_ = power_w; });
  update();
  return this;
}

} // namespace simgrid::plugins