/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <simgrid/Exception.hpp>
#include <simgrid/plugins/photovoltaic.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/VirtualMachine.hpp>
#include <simgrid/simix.hpp>

#include "src/kernel/resource/CpuImpl.hpp"
#include "src/simgrid/module.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

SIMGRID_REGISTER_PLUGIN(photovoltaic, "Photovoltaic management", &sg_photovoltaic_plugin_init)

/** @defgroup plugin_photovoltaic plugin_photovoltaic Plugin photovoltaic

  @beginrst

This is the photovoltaic plugin, enabling management of photovoltaic panels on hosts.
To activate this plugin, first call :cpp:func:`sg_photovoltaic_plugin_init()`.

This plugin allows evaluation of photovoltaic panels power generation during simulation depending on size, solar
irradiance and conversion factor.

The power model is taken from the paper `"Reinforcement Learning Based Load Balancing for
Geographically Distributed Data Centres" <https://dro.dur.ac.uk/33395/1/33395.pdf?DDD280+kkgc95+vbdv77>`_ by Max Mackie et. al.

The cost model is taken from the chapter 4 of the thesis `Sizing and Operation of Multi-Energy Hydrogen-Based
Microgrids <https://tel.archives-ouvertes.fr/tel-02077668/document>`_ by Bei Li.

Photovoltaic Panel properties
....................

Properties of panels are defined as properties of hosts in the platform XML file.

Here is an example of XML declaration where we consider a host as a photovoltaic panel:

.. code-block:: xml

   <host id="pv_panel" speed="0f">
      <prop id="photovoltaic_area" value="4" />
      <prop id="photovoltaic_conversion_efficiency" value="0.2" />
    </host>

The different properties are:

- ``photovoltaic_area``: Set the area of the panel in m² (default=0)
- ``photovoltaic_conversion_efficiency``: Set the conversion efficiency of the panel (default=0)
- ``photovoltaic_solar_irradiance``: Set the initial solar irradiance in W/m² (default=0)
- ``photovoltaic_min_power``: Set the minimum power of the panel in W (default=-1). Power generation below this value is automatically 0. Value is ignored if negative
- ``photovoltaic_max_power``: Set the maximal power of the panel in W (default=-1). Power generation above this value is automatically truncated. Value is ignored if negative
- ``photovoltaic_eval_cost``: Evaluate the cost of the panel during the simulation if set to 1 (defaulf=0)
- ``photovoltaic_lifespan``: Set the lifespan of the panel in years (default=0)
- ``photovoltaic_investment_cost``: Set the investment cost of the panel (default=0)
- ``photovoltaic_maintenance_cost``: Set the maintenance cost of the panel (default=0)

  @endrst
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(photovoltaic, kernel, "Logging specific to the photovoltaic plugin");

namespace simgrid::plugin {
class Photovoltaic {
private:
  simgrid::s4u::Host* host_ = nullptr;

  double area_m2_                   = 0;
  double conversion_efficiency_     = 0;
  double solar_irradiance_w_per_m2_ = 0;
  double min_power_w_               = -1;
  double max_power_w_               = -1;

  double power_w_      = 0;
  double last_updated_ = 0;

  bool eval_cost_                 = false;
  double cumulative_cost_         = 0;
  int lifespan_years_             = 0;
  double investment_cost_per_w_   = 0;
  double maintenance_cost_per_wh_ = 0;

  void init_photovoltaic_params();
  void init_cost_params();
  void update();

  Photovoltaic* set_area(double a);
  Photovoltaic* set_conversion_efficiency(double e);
  Photovoltaic* set_min_power(double p);
  Photovoltaic* set_max_power(double p);
  Photovoltaic* set_eval_cost(bool eval);
  Photovoltaic* set_lifespan(int l);
  Photovoltaic* set_investment_cost(double c);
  Photovoltaic* set_maintenance_cost(double c);

public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, Photovoltaic> EXTENSION_ID;

  explicit Photovoltaic(simgrid::s4u::Host* host);
  ~Photovoltaic();

  Photovoltaic* set_solar_irradiance(double s);

  double get_power();
};

Photovoltaic* Photovoltaic::set_area(double a)
{
  xbt_assert(a > 0, " : area should be > 0 (provided: %f)", a);
  simgrid::kernel::actor::simcall_answered([this, a] { area_m2_ = a; });
  return this;
}

Photovoltaic* Photovoltaic::set_conversion_efficiency(double e)
{
  xbt_assert(e > 0 and e <= 1, " : conversion efficiency should be in [0,1] (provided: %f)", e);
  simgrid::kernel::actor::simcall_answered([this, e] { conversion_efficiency_ = e; });
  return this;
}

Photovoltaic* Photovoltaic::set_solar_irradiance(double s)
{
  xbt_assert(s > 0, " : solar irradiance should be > 0 (provided: %f)", s);
  simgrid::kernel::actor::simcall_answered([this, s] { solar_irradiance_w_per_m2_ = s; });
  return this;
}

Photovoltaic* Photovoltaic::set_min_power(double p)
{
  simgrid::kernel::actor::simcall_answered([this, p] { min_power_w_ = p; });
  return this;
}

Photovoltaic* Photovoltaic::set_max_power(double p)
{
  simgrid::kernel::actor::simcall_answered([this, p] { max_power_w_ = p; });
  return this;
}

Photovoltaic* Photovoltaic::set_eval_cost(bool e)
{
  simgrid::kernel::actor::simcall_answered([this, e] { eval_cost_ = e; });
  return this;
}

Photovoltaic* Photovoltaic::set_lifespan(int l)
{
  xbt_assert(l > 0, " : lifespan should be > 0 (provided: %d)", l);
  simgrid::kernel::actor::simcall_answered([this, l] { lifespan_years_ = l; });
  return this;
}

Photovoltaic* Photovoltaic::set_investment_cost(double c)
{
  xbt_assert(c > 0, " : investment cost should be > 0 (provided: %f)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { investment_cost_per_w_ = c; });
  return this;
}

Photovoltaic* Photovoltaic::set_maintenance_cost(double c)
{
  xbt_assert(c > 0, " : maintenance cost hould be > 0 (provided: %f)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { maintenance_cost_per_wh_ = c; });
  return this;
}

double Photovoltaic::get_power()
{
  update();
  return power_w_;
}

simgrid::xbt::Extension<simgrid::s4u::Host, Photovoltaic> Photovoltaic::EXTENSION_ID;

void Photovoltaic::init_photovoltaic_params()
{
  const char* prop_chars;
  prop_chars = host_->get_property("photovoltaic_area");
  if (prop_chars)
    set_area(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("photovoltaic_conversion_efficiency");
  if (prop_chars)
    set_conversion_efficiency(
        xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("photovoltaic_solar_irradiance");
  if (prop_chars)
    set_solar_irradiance(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("photovoltaic_min_power");
  if (prop_chars)
    set_min_power(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("photovoltaic_max_power");
  if (prop_chars)
    set_max_power(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("photovoltaic_eval_cost");
  if (prop_chars)
    set_eval_cost(xbt_str_parse_int(prop_chars, ("cannot parse int: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("photovoltaic_lifespan");
  if (prop_chars)
    set_lifespan(xbt_str_parse_int(prop_chars, ("cannot parse int: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("photovoltaic_investment_cost");
  if (prop_chars)
    set_investment_cost(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("photovoltaic_maintenance_cost");
  if (prop_chars)
    set_maintenance_cost(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  simgrid::kernel::actor::simcall_answered([this] { last_updated_ = simgrid::s4u::Engine::get_clock(); });
}

void Photovoltaic::update()
{
  simgrid::kernel::actor::simcall_answered([this] {
    double now = simgrid::s4u::Engine::get_clock();
    if (now <= last_updated_)
      return;
    double power_w = conversion_efficiency_ * area_m2_ * solar_irradiance_w_per_m2_;
    if (min_power_w_ > 0 and power_w_ < min_power_w_)
      power_w = 0;
    if (max_power_w_ > 0 and power_w_ > max_power_w_)
      power_w = max_power_w_;
    power_w_ = power_w;
    if (eval_cost_) {
      xbt_assert(max_power_w_ > 0, " : max power must be > 0 (provided: %f)", max_power_w_);
      cumulative_cost_ += max_power_w_ * (now - last_updated_) *
                          (investment_cost_per_w_ / (lifespan_years_ * 8760 * 3600) + maintenance_cost_per_wh_ / 3600);
    }
    last_updated_ = now;
  });
}

Photovoltaic::Photovoltaic(simgrid::s4u::Host* host) : host_(host)
{
  init_photovoltaic_params();
}

Photovoltaic::~Photovoltaic() = default;
} // namespace simgrid::plugin

using simgrid::plugin::Photovoltaic;

/* **************************** events  callback *************************** */

static void on_creation(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;
  host.extension_set(new Photovoltaic(&host));
}

/* **************************** Public interface *************************** */

static void ensure_plugin_inited()
{
  if (not Photovoltaic::EXTENSION_ID.valid())
    throw simgrid::xbt::InitializationError(
        "The Photovoltaic plugin is not active. Please call sg_photovoltaic_plugin_init() "
        "before calling any function related to that plugin.");
}

/** @ingroup plugin_photovoltaic
 *  @brief Enable photovoltaic plugin.
 */
void sg_photovoltaic_plugin_init()
{
  if (Photovoltaic::EXTENSION_ID.valid())
    return;
  Photovoltaic::EXTENSION_ID = simgrid::s4u::Host::extension_create<Photovoltaic>();
  simgrid::s4u::Host::on_creation_cb(&on_creation);
}

/** @ingroup plugin_photovoltaic
 *  @param s The solar irradiance to set in W/m².
 */
void sg_photovoltaic_set_solar_irradiance(const_sg_host_t host, double s)
{
  ensure_plugin_inited();
  host->extension<Photovoltaic>()->set_solar_irradiance(s);
}

/** @ingroup plugin_photovoltaic
 *  @return Power generation in W.
 */
double sg_photovoltaic_get_power(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Photovoltaic>()->get_power();
}