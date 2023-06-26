/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <simgrid/Exception.hpp>
#include <simgrid/plugins/battery.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/VirtualMachine.hpp>
#include <simgrid/simix.hpp>

#include "src/kernel/resource/CpuImpl.hpp"
#include "src/simgrid/module.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

SIMGRID_REGISTER_PLUGIN(battery, "Battery management", &sg_battery_plugin_init)

/** @defgroup plugin_battery plugin_battery Plugin Battery

  @beginrst

This is the battery plugin, enabling management of batteries on hosts.
To activate this plugin, first call :cpp:func:`sg_battery_plugin_init()`.

We consider a constant energy exchange model.

Properties of batteries such as State of Charge and State of Health are lazily updated, ie., when reading values and
when the power is modified.

State of Charge (SoC)
.....................

If the power of a battery is set to a negative value then the battery will act as a load and fill over time until it
reaches its maximal SoC. Conversely, if the power of a battery is set to a positive value then the battery will act as a
generator and deplete over time until it reaches its minimal SoC. When reaching either its maximal or minimal SoC it
will set its power to 0.

The natural depletion of batteries over time is not taken into account.

State of Health (SoH)
.....................

A battery starts with an energy budget :math:`E` such as:

.. math::

  E = C \times U \times D \times N \times 2

Where :math:`C` is the initial capacity, :math:`U` is the ratio of usable capacity, :math:`D` is the depth of discharge
and :math:`N` is the number of cycles of the battery.

The SoH represents the consumption of this energy budget during the lifetime of the battery.
Use the battery reduces its SoH and its capacity in consequence.
When the SoH reaches 0, the battery becomes unusable.

.. warning::

  Due to the decrease of the battery capacity with the SoH, a large usable capacity leads to very tiny battery capacity
  when reaching low SoH. This may results in charge and discharge cycles too short to be evaluated by the simulator. To
  avoid this situation you should not try to reach a SoH of 0 with a usable capacity set to 1.

Plotting the output of the example "battery-degradation" highlights the linear decrease of the SoH due to a continuous
use of the battery and the decreasing cycle duration as its capacity reduces:

.. image:: /img/battery_degradation.svg
   :align: center

Batteries properties
....................

Properties of the batteries are defined as properties of hosts in the platform XML file.

Here is an example of XML declaration:

.. code-block:: xml

   <host id="Host" speed="100.0Mf" core="1">
       <prop id="battery_active" value="1" />
       <prop id="battery_capacity" value="10" />
       <prop id="battery_cycles" value="200" />
       <prop id="battery_state_of_charge" value="0.8" />
   </host>

The different properties are:

- ``battery_active``: Set the battery as active if set to 1 (default=0)
- ``battery_power``: Set the initial power of the battery in W (default=0). A negative value indicates that the battery act as a load and charge. A positive value indicates that the battery act as a generator and discharge
- ``battery_state_of_charge``: Set the initial state of charge of the battery (default=0)
- ``battery_state_of_charge_min``: Set the minimal state of charge of the battery (default=0.2)
- ``battery_state_of_charge_max``: Set the maximal state of charge of the battery (default=0.8)
- ``battery_capacity``: Set the initial capacity of the battery in Wh (default=0)
- ``battery_usable_capacity``: Set the ratio of usable capacity of the battery (default=0.8)
- ``battery_depth_of_discharge``: Set the depth of discharge of the battery (default=0.9)
- ``battery_charge_efficiency``: Set the charge efficiency of the battery (default=1)
- ``battery_discharge_efficiency``: Set the charge efficiency of the battery (default=1)
- ``battery_cycles``: Set the number of cycle of the battery (default=1)
- ``battery_depth_of_discharge``: Set the number of cycle of the battery (default=1)
- ``battery_eval_cost``: Evaluate the cost of the battery during the simulation if set to 1 (defaulf=0)
- ``battery_investment_cost``: Set the investment cost of the battery (default=0)
- ``battery_static_maintenance_cost``: Set the static maintenance cost of the battery (default=0)
- ``battery_variable_maintenance_cost``: Set the variable maintenance cost of the battery (default=0)

  @endrst
 */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(battery, kernel, "Logging specific to the battery plugin");

namespace simgrid::plugin {
class Battery {
private:
  simgrid::s4u::Host* host_ = nullptr;

  double state_of_charge_min_  = 0.2;
  double state_of_charge_max_  = 0.8;
  double charge_efficiency_    = 1;
  double discharge_efficiency_ = 1;
  double initial_capacity_wh_  = 0;
  double cycles_ = 1; // total complete cycles (charge + discharge) the battery can do before complete depletion.
  double depth_of_discharge_ = 0.9;
  double usable_capacity_    = 0.8;
  double energy_budget_j_    = 0;

  bool active_               = false;
  double power_w_            = 0; // NEGATIVE when charging (consumes power) POSITIVE when discharging (generates power)
  double state_of_charge_    = 0;
  double capacity_wh_        = 0;
  double next_event_         = -1;
  double energy_exchanged_j_ = 0;
  double last_updated_       = 0;

  // Calculation of costs from Bei Li thesis (link :https://tel.archives-ouvertes.fr/tel-02077668/document) (chapter 4)
  bool eval_cost_                                = false;
  double cumulative_cost_                        = 0;
  double investment_cost_per_wh_                 = 0;
  double static_maintenance_cost_per_wh_times_h_ = 0;
  double variable_maintenance_cost_per_wh_       = 0;

  void init_battery_params();
  void init_cost_params();
  void update();

  void set_state_of_charge(double soc);
  void set_state_of_charge_min(double soc);
  void set_state_of_charge_max(double soc);
  void set_capacity(double c);
  void set_initial_capacity(double c);
  void set_cycles(int c);
  void set_depth_of_discharge(double d);
  void set_usable_capacity(double c);
  void set_charge_efficiency(double e);
  void set_discharge_efficiency(double e);
  void set_eval_cost(bool eval);
  void set_investment_cost(double c);
  void set_static_maintenance_cost(double c);
  void set_variable_maintenance_cost(double c);

  bool is_charging();

public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, Battery> EXTENSION_ID;

  explicit Battery(simgrid::s4u::Host* host);
  ~Battery();

  void set_state(bool state);
  void set_power(const double power);

  bool is_active() const;
  double get_power();
  double get_state_of_charge();
  double get_state_of_charge_min() const;
  double get_state_of_charge_max() const;
  double get_state_of_health();
  double get_capacity();
  double get_cumulative_cost();
  double get_next_event_date();
};

simgrid::xbt::Extension<simgrid::s4u::Host, Battery> Battery::EXTENSION_ID;

void Battery::set_power(const double p)
{
  update();
  xbt_assert(p == 0 or (p > 0 and state_of_charge_ > state_of_charge_min_) or
                 (p < 0 and state_of_charge_ < state_of_charge_max_),
             "Incoherent power and state of charge. A battery cannot charge(discharge) past its maximal(minimal) state "
             "of charge.");
  xbt_assert(p == 0 or energy_exchanged_j_ < energy_budget_j_, "Cannot set power of a fully used battery.");
  simgrid::kernel::actor::simcall_answered([this, p] {
    power_w_ = p;
    if (power_w_ == 0) {
      next_event_ = -1;
      return;
    }
    double soc_shutdown;
    double soh_shutdown;
    if (power_w_ > 0) {
      soc_shutdown = capacity_wh_ * 3600 * (state_of_charge_ - state_of_charge_min_) / (power_w_ * charge_efficiency_);
      soh_shutdown = (energy_budget_j_ - energy_exchanged_j_) / (power_w_ * charge_efficiency_);
    } else { // power_w_ < 0
      soc_shutdown =
          capacity_wh_ * 3600 * (state_of_charge_max_ - state_of_charge_) / (abs(power_w_) / discharge_efficiency_);
      soh_shutdown = (energy_budget_j_ - energy_exchanged_j_) / (abs(power_w_) / discharge_efficiency_);
    }
    if (soh_shutdown <= 0)
      next_event_ = simgrid::s4u::Engine::get_clock() + soc_shutdown;
    else
      next_event_ = simgrid::s4u::Engine::get_clock() + std::min(soc_shutdown, soh_shutdown);
  });
}

void Battery::set_state(bool state)
{
  update();
  simgrid::kernel::actor::simcall_answered([this, state] { active_ = state; });
}

void Battery::set_state_of_charge(double soc)
{
  xbt_assert(soc > 0 and soc <= 1, " : state of charge should be in ]0,1] (provided: %f)", soc);
  simgrid::kernel::actor::simcall_answered([this, soc] { state_of_charge_ = soc; });
}

void Battery::set_state_of_charge_min(double soc)
{
  xbt_assert(soc > 0 and soc <= 1 and soc < state_of_charge_max_,
             " : state of charge min should be in ]0,1] and below state of charge max (provided: %f)", soc);
  simgrid::kernel::actor::simcall_answered([this, soc] { state_of_charge_min_ = soc; });
}

void Battery::set_state_of_charge_max(double soc)
{
  xbt_assert(soc > 0 and soc <= 1 and soc > state_of_charge_min_,
             " : state of charge max should be in ]0,1] and above state of charge min (provided: %f)", soc);
  simgrid::kernel::actor::simcall_answered([this, soc] { state_of_charge_max_ = soc; });
}

void Battery::set_initial_capacity(double c)
{
  xbt_assert(c > 0, " : capacity should be > 0 (provided: %f)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { initial_capacity_wh_ = c; });
}

void Battery::set_capacity(double c)
{
  xbt_assert(c > 0, " : capacity should be > 0 (provided: %f)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { capacity_wh_ = c; });
}

void Battery::set_cycles(int c)
{
  xbt_assert(c > 0, " : cycles should be > 0 (provided: %d)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { cycles_ = c; });
}

void Battery::set_depth_of_discharge(double d)
{
  xbt_assert(d > 0 and d <= 1, " : depth of discharge should be in ]0, 1] (provided: %f)", d);
  simgrid::kernel::actor::simcall_answered([this, d] { depth_of_discharge_ = d; });
}

void Battery::set_usable_capacity(double c)
{
  xbt_assert(c > 0 and c <= 1, " : usable capacity should be in ]0, 1] (provided: %f)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { usable_capacity_ = c; });
}

void Battery::set_charge_efficiency(double e)
{
  xbt_assert(e > 0 and e <= 1, " : charge efficiency should be in [0,1] (provided: %f)", e);
  simgrid::kernel::actor::simcall_answered([this, e] { charge_efficiency_ = e; });
}

void Battery::set_discharge_efficiency(double e)
{
  xbt_assert(e > 0 and e <= 1, " : discharge efficiency should be in [0,1] (provided: %f)", e);
  simgrid::kernel::actor::simcall_answered([this, e] { discharge_efficiency_ = e; });
}

void Battery::set_eval_cost(bool eval)
{
  simgrid::kernel::actor::simcall_answered([this, eval] { eval_cost_ = eval; });
}

void Battery::set_investment_cost(double c)
{
  xbt_assert(c >= 0, " : investment cost should be >= 0 (provided: %f)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { investment_cost_per_wh_ = c; });
}

void Battery::set_static_maintenance_cost(double c)
{
  xbt_assert(c >= 0, " : static maintenance cost should be >= 0 (provided: %f)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { static_maintenance_cost_per_wh_times_h_ = c; });
}

void Battery::set_variable_maintenance_cost(double c)
{
  xbt_assert(c >= 0, " : variable maintenance cost should be >= 0 (provided: %f)", c);
  simgrid::kernel::actor::simcall_answered([this, c] { variable_maintenance_cost_per_wh_ = c; });
}

bool Battery::is_charging()
{
  update();
  return power_w_ < 0;
}

bool Battery::is_active() const
{
  return active_;
}

double Battery::get_power()
{
  update();
  return power_w_;
}

double Battery::get_state_of_charge()
{
  update();
  return state_of_charge_;
}

double Battery::get_state_of_charge_min() const
{
  return state_of_charge_min_;
}

double Battery::get_state_of_charge_max() const
{
  return state_of_charge_max_;
}

double Battery::get_state_of_health()
{
  update();
  return 1 - (energy_exchanged_j_ / energy_budget_j_);
}

double Battery::get_capacity()
{
  update();
  return capacity_wh_;
}

double Battery::get_cumulative_cost()
{
  update();
  return cumulative_cost_;
}

double Battery::get_next_event_date()
{
  update();
  return next_event_;
}

void Battery::init_battery_params()
{
  const char* prop_chars;
  prop_chars = host_->get_property("battery_capacity");
  if (prop_chars) {
    set_capacity(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
    set_initial_capacity(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  }
  prop_chars = host_->get_property("battery_usable_capacity");
  if (prop_chars)
    set_usable_capacity(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_depth_of_discharge");
  if (prop_chars)
    set_depth_of_discharge(
        xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_cycles");
  if (prop_chars)
    set_cycles(xbt_str_parse_int(prop_chars, ("cannot parse int: " + std::string(prop_chars)).c_str()));
  simgrid::kernel::actor::simcall_answered([this] {
    energy_budget_j_ = (initial_capacity_wh_ * usable_capacity_ * depth_of_discharge_ * 3600 * cycles_ * 2);
  });
  prop_chars = host_->get_property("battery_active");
  if (prop_chars)
    set_state(xbt_str_parse_int(prop_chars, ("cannot parse int: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_state_of_charge_min");
  if (prop_chars)
    set_state_of_charge_min(
        xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_state_of_charge_max");
  if (prop_chars)
    set_state_of_charge_max(
        xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_charge_efficiency");
  if (prop_chars)
    set_charge_efficiency(
        xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_discharge_efficiency");
  if (prop_chars)
    set_discharge_efficiency(
        xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_state_of_charge");
  if (prop_chars)
    set_state_of_charge(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_eval_cost");
  if (prop_chars)
    set_eval_cost(xbt_str_parse_int(prop_chars, ("cannot parse int: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_investment_cost");
  if (prop_chars)
    set_investment_cost(xbt_str_parse_int(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_static_maintenance_cost");
  if (prop_chars)
    set_static_maintenance_cost(
        xbt_str_parse_int(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_variable_maintenance_cost");
  if (prop_chars)
    set_variable_maintenance_cost(
        xbt_str_parse_int(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  prop_chars = host_->get_property("battery_power");
  if (prop_chars)
    set_power(xbt_str_parse_double(prop_chars, ("cannot parse double: " + std::string(prop_chars)).c_str()));
  simgrid::kernel::actor::simcall_answered([this] { last_updated_ = simgrid::s4u::Engine::get_clock(); });
}

void Battery::update()
{
  simgrid::kernel::actor::simcall_answered([this] {
    double now             = simgrid::s4u::Engine::get_clock();
    double time_delta_real = now - last_updated_;
    if (time_delta_real <= 0 or not is_active())
      return;
    double time_delta_until_event = next_event_ - last_updated_;
    bool event                    = next_event_ != -1 and time_delta_until_event <= time_delta_real;
    double power_real_w           = power_w_ < 0 ? power_w_ * charge_efficiency_ : power_w_ / discharge_efficiency_;
    state_of_charge_ -= power_real_w * (event ? time_delta_until_event : time_delta_real) / (3600 * capacity_wh_);
    energy_exchanged_j_ += (event ? time_delta_until_event : time_delta_real) * abs(power_real_w);
    capacity_wh_ = initial_capacity_wh_ * usable_capacity_ * (1 - (energy_exchanged_j_ / energy_budget_j_)) +
                   initial_capacity_wh_ * (1 - usable_capacity_);
    capacity_wh_ = std::max(capacity_wh_, 0.0);
    if (eval_cost_) {
      double usage_cost_per_wh =
          (investment_cost_per_wh_ / (depth_of_discharge_ * cycles_ * 2) + variable_maintenance_cost_per_wh_);
      double usage_cost =
          usage_cost_per_wh * abs(power_real_w) * (event ? time_delta_until_event : time_delta_real) / 3600;
      double static_maintenance_cost =
          static_maintenance_cost_per_wh_times_h_ * initial_capacity_wh_ * time_delta_real / 3600;
      cumulative_cost_ += usage_cost + static_maintenance_cost;
    }
    if (event) {
      power_w_    = 0;
      next_event_ = -1;
    }
    last_updated_ = now;
  });
}

Battery::Battery(simgrid::s4u::Host* host) : host_(host)
{
  init_battery_params();
}

Battery::~Battery() = default;
} // namespace simgrid::plugin

using simgrid::plugin::Battery;

/* **************************** events  callback *************************** */

static void on_creation(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;
  host.extension_set(new Battery(&host));
}

/* **************************** Public interface *************************** */

static void ensure_plugin_inited()
{
  if (not Battery::EXTENSION_ID.valid())
    throw simgrid::xbt::InitializationError("The Battery plugin is not active. Please call sg_battery_plugin_init() "
                                            "before calling any function related to that plugin.");
}

/** @ingroup plugin_battery
 *  @brief Enable battery plugin.
 */
void sg_battery_plugin_init()
{
  if (Battery::EXTENSION_ID.valid())
    return;
  Battery::EXTENSION_ID = simgrid::s4u::Host::extension_create<Battery>();
  simgrid::s4u::Host::on_creation_cb(&on_creation);
}

/** @ingroup plugin_battery
 *  @param state The state to set.
 *  @brief Set the state of the battery.
 * A battery set to inactive (false) will neither update its state of charge nor its state of health.
 */
void sg_battery_set_state(const_sg_host_t host, bool state)
{
  ensure_plugin_inited();
  host->extension<Battery>()->set_state(state);
}

/** @ingroup plugin_battery
 *  @param power The power to set in W.
 *  @brief Set the power of the battery.
 *  @note A negative value makes the battery act as a load and charge.
 * A positive value makes the battery act as a generator and discharge.
 */
void sg_battery_set_power(const_sg_host_t host, double power)
{
  ensure_plugin_inited();
  host->extension<Battery>()->set_power(power);
}

/** @ingroup plugin_battery
 *  @brief Return true if the battery is active.
 */
bool sg_battery_is_active(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->is_active();
}

/** @ingroup plugin_battery
 *  @brief Return the power of the battery in W.
 *  @note A negative value indicates that the battery act as a load and charge.
 * A positive value indicates that the battery act as a generator and discharge.
 */
double sg_battery_get_power(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->get_power();
}

/** @ingroup plugin_battery
 *  @brief Return the state of charge of the battery.
 */
double sg_battery_get_state_of_charge(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->get_state_of_charge();
}

/** @ingroup plugin_battery
 *  @brief Return the minimal state of charge of the battery.
 */
double sg_battery_get_state_of_charge_min(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->get_state_of_charge_min();
}

/** @ingroup plugin_battery
 *  @brief Return the maximal state of charge of the battery.
 */
double sg_battery_get_state_of_charge_max(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->get_state_of_charge_max();
}

/** @ingroup plugin_battery
 *  @brief Return the state of health of the battery.
 */
double sg_battery_get_state_of_health(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->get_state_of_health();
}

/** @ingroup plugin_battery
 *  @brief Return the capacity of the battery in Wh.
 *  @note capacity is reduced by the state of health of the battery.
 */
double sg_battery_get_capacity(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->get_capacity();
}

/** @ingroup plugin_battery
 *  @brief Return the cumulative cost of the battery.
 */
double sg_battery_get_cumulative_cost(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->get_cumulative_cost();
}

/** @ingroup plugin_battery
 *  @brief Return the date of the next event, i.e., when the battery will be empty or full.
 *  @note If power is null then return -1.
 */
double sg_battery_get_next_event_date(const_sg_host_t host)
{
  ensure_plugin_inited();
  return host->extension<Battery>()->get_next_event_date();
}
