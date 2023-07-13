/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_BATTERY_HPP_
#define SIMGRID_PLUGINS_BATTERY_HPP_

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

  class Event {
    friend Battery;

  private:
    double state_of_charge_;
    Flow flow_;
    double time_delta_ = -1;
    std::function<void()> callback_;
    bool repeat_;

  public:
    Event(double state_of_charge, Flow flow, std::function<void()> callback, bool repeat);
    static std::shared_ptr<Event> init(double state_of_charge, Flow flow, std::function<void()> callback, bool repeat);

    /** @ingroup plugin_battery
     *  @return The state of charge at which the Event will happen.
     *  @note For Battery::Event objects
     */
    double get_state_of_charge() { return state_of_charge_; }
    /** @ingroup plugin_battery
     *  @return The flow in which the Event will happen, either when the Battery is charging or discharging.
     *  @note For Battery::Event objects
     */
    Flow get_flow() { return flow_; }
    /** @ingroup plugin_battery
     *  @return The time delta until the Event happen.
     -1 means that is will never happen with the current state the Battery,
     for instance when there is no load connected to the Battery.
     *  @note For Battery::Event objects
    */
    double get_time_delta() { return time_delta_; }
    /** @ingroup plugin_battery
     *  @return The callback to trigger when the Event happen.
     *  @note For Battery::Event objects
     */
    std::function<void()> get_callback() { return callback_; }
    /** @ingroup plugin_battery
     *  @return true if its a recurrent Event.
     *  @note For Battery::Event objects
     */
    bool get_repeat() { return repeat_; }
  };

private:
  static std::shared_ptr<BatteryModel> battery_model_;

  std::string name_;
  double charge_efficiency_;
  double discharge_efficiency_;
  double initial_capacity_wh_;
  double energy_budget_j_;

  std::map<const s4u::Host*, bool> host_loads_     = {};
  std::map<const std::string, double> named_loads_ = {};
  std::vector<std::shared_ptr<Event>> events_;

  double capacity_wh_;
  double energy_stored_j_;
  double energy_provided_j_ = 0;
  double energy_consumed_j_ = 0;
  double last_updated_      = 0;

  explicit Battery(const std::string& name, double state_of_charge, double charge_efficiency,
                   double discharge_efficiency, double initial_capacity_wh, int cycles);
  static void init_plugin();
  void update();
  double next_occurring_event();

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
  static BatteryPtr init(const std::string& name, double state_of_charge, double charge_efficiency,
                         double discharge_efficiency, double initial_capacity_wh, int cycles);
  void set_load(const std::string& name, double power_w);
  void connect_host(s4u::Host* host, bool active = true);
  double get_state_of_charge();
  double get_state_of_health();
  double get_capacity();
  double get_energy_provided();
  double get_energy_consumed();
  double get_energy_stored(std::string unit = "J");
  std::shared_ptr<Event> create_event(double state_of_charge, Flow flow, std::function<void()> callback,
                                      bool repeat = false);
  std::vector<std::shared_ptr<Event>> get_events();
  void delete_event(std::shared_ptr<Event> event);
};
} // namespace simgrid::plugins
#endif