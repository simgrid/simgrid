/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_BATTERY_HPP_
#define SIMGRID_PLUGINS_BATTERY_HPP_

#include <cmath>
#include <memory>
#include <simgrid/kernel/resource/Model.hpp>
#include <simgrid/s4u/Activity.hpp>
#include <xbt/Extendable.hpp>

namespace simgrid::plugins {

class Battery;
using BatteryPtr = boost::intrusive_ptr<Battery>;
XBT_PUBLIC void intrusive_ptr_release(Battery* o);
XBT_PUBLIC void intrusive_ptr_add_ref(Battery* o);

class BatteryModel : public kernel::resource::Model {
  std::vector<BatteryPtr> batteries_;

public:
  explicit BatteryModel();

  void add_battery(BatteryPtr b);
  void update_actions_state(double now, double delta) override;
  double next_occurring_event(double now) override;
};

class Battery {

  friend BatteryModel;

public:
  enum Flow { CHARGE, DISCHARGE };

  class Handler {
    friend Battery;

  public:
    enum Persistancy { PERSISTANT, ONESHOT };

  private:
    double state_of_charge_;
    Flow flow_;
    double time_delta_ = -1;
    std::function<void()> callback_;
    Persistancy persistancy_;

  public:
    Handler(double state_of_charge, Flow flow, Persistancy p, std::function<void()> callback);
    static std::shared_ptr<Handler> init(double state_of_charge, Flow flow, Persistancy p,
                                         std::function<void()> callback);

    /** @ingroup plugin_battery
     *  @return The state of charge at which the Handler will happen.
     *  @note For Battery::Handler objects
     */
    double get_state_of_charge() { return state_of_charge_; }
    /** @ingroup plugin_battery
     *  @return The flow in which the Handler will happen, either when the Battery is charging or discharging.
     *  @note For Battery::Handler objects
     */
    Flow get_flow() { return flow_; }
    /** @ingroup plugin_battery
     *  @return The time delta until the Handler happen.
     -1 means that is will never happen with the current state the Battery,
     for instance when there is no load connected to the Battery.
     *  @note For Battery::Handler objects
    */
    double get_time_delta() { return time_delta_; }
    /** @ingroup plugin_battery
     *  @return The callback to trigger when the Handler happen.
     *  @note For Battery::Handler objects
     */
    std::function<void()> get_callback() { return callback_; }
    /** @ingroup plugin_battery
     *  @return true if its a recurrent Handler.
     *  @note For Battery::Handler objects
     */
    Persistancy get_persistancy() { return persistancy_; }
  };

private:
  static std::shared_ptr<BatteryModel> battery_model_;

  std::string name_;
  double nominal_charge_power_w_    = -INFINITY;
  double nominal_discharge_power_w_ = INFINITY;
  double charge_efficiency_         = 1;
  double discharge_efficiency_      = 1;
  double initial_capacity_wh_       = 0;
  double energy_budget_j_           = 0;

  std::map<const s4u::Host*, bool> host_loads_                      = {};
  std::map<const std::string, std::pair<bool, double>> named_loads_ = {};
  std::vector<std::shared_ptr<Handler>> handlers_;

  double capacity_wh_       = 0;
  double energy_stored_j_   = 0;
  double energy_provided_j_ = 0;
  double energy_consumed_j_ = 0;
  double last_updated_      = 0;

  explicit Battery() = default;
  explicit Battery(const std::string& name, double state_of_charge, double nominal_charge_power_w,
                   double nominal_discharge_power_w, double charge_efficiency, double discharge_efficiency,
                   double initial_capacity_wh, int cycles);
  static void init_plugin();
  void update();
  double next_occurring_handler();

  std::atomic_int_fast32_t refcount_{0};
#ifndef DOXYGEN
  friend void intrusive_ptr_release(Battery* o)
  {
    if (o->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete o;
    }
  }
  friend void intrusive_ptr_add_ref(Battery* o) { o->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif

public:
  static BatteryPtr init();
  static BatteryPtr init(const std::string& name, double state_of_charge, double nominal_charge_power_w,
                         double nominal_discharge_power_w, double charge_efficiency, double discharge_efficiency,
                         double initial_capacity_wh, int cycles);
  void set_load(const std::string& name, double power_w);
  void set_load(const std::string& name, bool active);
  void connect_host(s4u::Host* host, bool active = true);
  std::string get_name() const { return name_; }
  double get_state_of_charge();
  double get_state_of_health();
  double get_capacity();
  double get_energy_provided();
  double get_energy_consumed();
  double get_energy_stored(std::string unit = "J");
  std::shared_ptr<Handler> schedule_handler(double state_of_charge, Flow flow, Handler::Persistancy p,
                                            std::function<void()> callback);
  std::vector<std::shared_ptr<Handler>> get_handlers();
  void delete_handler(std::shared_ptr<Handler> handler);
};
} // namespace simgrid::plugins
#endif