/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef SIMGRID_PLUGINS_SOLAR_PANEL_H_
#define SIMGRID_PLUGINS_SOLAR_PANEL_H_

#include <simgrid/kernel/resource/Model.hpp>
#include <simgrid/s4u/Activity.hpp>
#include <xbt/Extendable.hpp>

namespace simgrid::plugins {

/** @ingroup plugin_solar_panel */
class SolarPanel;
/** @ingroup plugin_solar_panel */
using SolarPanelPtr = boost::intrusive_ptr<SolarPanel>;
XBT_PUBLIC void intrusive_ptr_release(SolarPanel* o);
XBT_PUBLIC void intrusive_ptr_add_ref(SolarPanel* o);

class SolarPanel {

  std::string name_;
  double area_m2_;
  double conversion_efficiency_;
  double solar_irradiance_w_per_m2_;
  double min_power_w_;
  double max_power_w_;
  double power_w_ = -1;

  explicit SolarPanel(std::string name, double area_m2, double conversion_efficiency, double solar_irradiance_w_per_m2,
                      double min_power_w, double max_power_w);
  void update();

  std::atomic_int_fast32_t refcount_{0};
#ifndef DOXYGEN
  friend void intrusive_ptr_release(SolarPanel* o)
  {
    if (o->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete o;
    }
  }
  friend void intrusive_ptr_add_ref(SolarPanel* o) { o->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif

  static xbt::signal<void(SolarPanel*)> on_power_change;
  xbt::signal<void(SolarPanel*)> on_this_power_change;

public:
  static SolarPanelPtr init(const std::string& name, double area_m2, double conversion_efficiency,
                            double solar_irradiance_w_per_m2, double min_power_w, double max_power_w);

  SolarPanelPtr set_name(std::string name);
  SolarPanelPtr set_area(double area_m2);
  SolarPanelPtr set_conversion_efficiency(double e);
  SolarPanelPtr set_solar_irradiance(double solar_irradiance_w_per_m2);
  SolarPanelPtr set_min_power(double power_w);
  SolarPanelPtr set_max_power(double power_w);

  std::string get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  double get_area() const { return area_m2_; }
  double get_conversion_efficiency() const { return conversion_efficiency_; }
  double get_solar_irradiance() const { return solar_irradiance_w_per_m2_; }
  double get_min_power() const { return min_power_w_; }
  double get_max_power() const { return max_power_w_; }
  double get_power() const { return power_w_; }

  /** Add a callback fired after this solar panel power changed. */
  void on_this_power_change_cb(const std::function<void(SolarPanel*)>& func) { on_this_power_change.connect(func); };
  /** Add a callback fired after a solar panel power changed.
   * Triggered after the on_this_power_change function.**/
  static void on_power_change_cb(const std::function<void(SolarPanel*)>& cb) { on_power_change.connect(cb); }
};
} // namespace simgrid::plugins
#endif
