/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef SIMGRID_PLUGINS_SOLAR_PANEL_H_
#define SIMGRID_PLUGINS_SOLAR_PANEL_H_

#include <simgrid/kernel/resource/Model.hpp>
#include <simgrid/s4u/Activity.hpp>
#include <xbt/Extendable.hpp>

namespace simgrid::plugins {

class SolarPanel;
using SolarPanelPtr = boost::intrusive_ptr<SolarPanel>;
XBT_PUBLIC void intrusive_ptr_release(SolarPanel* o);
XBT_PUBLIC void intrusive_ptr_add_ref(SolarPanel* o);

class SolarPanelModel : public kernel::resource::Model {
  std::vector<SolarPanelPtr> solar_panels_;

public:
  explicit SolarPanelModel();

  void add_solar_panel(SolarPanelPtr b);
  void update_actions_state(double now, double delta) override;
  double next_occurring_event(double now) override;
};

class SolarPanel {

  friend SolarPanelModel;

private:
  static std::shared_ptr<SolarPanelModel> solar_panel_model_;

  std::string name_;
  double area_m2_;
  double conversion_efficiency_;
  double solar_irradiance_w_per_m2_;
  double min_power_w_;
  double max_power_w_;

  double power_w_      = 0;
  double last_updated_ = 0;

  explicit SolarPanel(std::string name, double area_m2, double conversion_efficiency, double solar_irradiance_w_per_m2,
                      double min_power_w, double max_power_w);
  static void init_plugin();
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

public:
  static SolarPanelPtr init(const std::string& name, double area_m2, double conversion_efficiency,
                            double solar_irradiance_w_per_m2, double min_power_w, double max_power_w);

  SolarPanelPtr set_name(std::string name);
  SolarPanelPtr set_area(double area_m2);
  SolarPanelPtr set_conversion_efficiency(double e);
  SolarPanelPtr set_solar_irradiance(double solar_irradiance_w_per_m2);
  SolarPanelPtr set_min_power(double power_w);
  SolarPanelPtr set_max_power(double power_w);

  std::string get_name() { return name_; }
  const char* get_cname() { return name_.c_str(); }
  double get_area() { return area_m2_; }
  double get_conversion_efficiency() { return conversion_efficiency_; }
  double get_solar_irradiance() { return solar_irradiance_w_per_m2_; }
  double get_min_power() { return min_power_w_; }
  double get_max_power() { return max_power_w_; }
  double get_power() { return power_w_; }
};
} // namespace simgrid::plugins
#endif
