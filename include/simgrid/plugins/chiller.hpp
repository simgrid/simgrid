/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef SIMGRID_PLUGINS_CHILLER_H_
#define SIMGRID_PLUGINS_CHILLER_H_

#include <simgrid/kernel/resource/Model.hpp>
#include <simgrid/s4u/Activity.hpp>
#include <xbt/Extendable.hpp>

namespace simgrid::plugins {

/** @ingroup plugin_chiller */
class Chiller;
/** @ingroup plugin_chiller */
using ChillerPtr = boost::intrusive_ptr<Chiller>;
XBT_PUBLIC void intrusive_ptr_release(Chiller* o);
XBT_PUBLIC void intrusive_ptr_add_ref(Chiller* o);

class ChillerModel : public kernel::resource::Model {
  std::vector<ChillerPtr> chillers_;

public:
  explicit ChillerModel();

  void add_chiller(ChillerPtr b);
  void update_actions_state(double now, double delta) override;
  double next_occurring_event(double now) override;
};

class Chiller {

  friend ChillerModel;

private:
  static std::shared_ptr<ChillerModel> chiller_model_;

  std::string name_;
  double air_mass_kg_;
  double specific_heat_j_per_kg_per_c_;
  double alpha_;
  double cooling_efficiency_;
  double temp_in_c_;
  double temp_out_c_;
  double goal_temp_c_;
  double max_power_w_;

  std::set<const s4u::Host*> hosts_ = {};
  bool active_                      = true;
  double power_w_                   = 0;
  double energy_consumed_j_         = 0;
  double last_updated_              = 0;

  explicit Chiller(const std::string& name, double air_mass_kg, double specific_heat_j_per_kg_per_c, double alpha,
                   double cooling_efficiency, double initial_temp_c, double goal_temp_c, double max_power_w);

  static void init_plugin();
  void update();

  std::atomic_int_fast32_t refcount_{0};
#ifndef DOXYGEN
  friend void intrusive_ptr_release(Chiller* o)
  {
    if (o->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete o;
    }
  }
  friend void intrusive_ptr_add_ref(Chiller* o) { o->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif

  static xbt::signal<void(Chiller*)> on_power_change;
  xbt::signal<void(Chiller*)> on_this_power_change;

public:
  static ChillerPtr init(const std::string& name, double air_mass_kg, double specific_heat_j_per_kg_per_c, double alpha,
                         double cooling_efficiency, double initial_temp_c, double goal_temp_c, double max_power_w);

  ChillerPtr set_name(std::string name);
  ChillerPtr set_air_mass(double air_mass_kg);
  ChillerPtr set_specific_heat(double specific_heat_j_per_kg_per_c);
  ChillerPtr set_alpha(double alpha);
  ChillerPtr set_cooling_efficiency(double cooling_efficiency);
  ChillerPtr set_goal_temp(double goal_temp_c);
  ChillerPtr set_max_power(double max_power_w);
  ChillerPtr set_active(bool active);
  ChillerPtr add_host(simgrid::s4u::Host* host);
  ChillerPtr remove_host(simgrid::s4u::Host* host);

  std::string get_name() { return name_; }
  const char* get_cname() { return name_.c_str(); }
  double get_air_mass() { return air_mass_kg_; }
  double get_specific_heat() { return specific_heat_j_per_kg_per_c_; }
  double get_alpha() { return alpha_; }
  double get_cooling_efficiency() { return cooling_efficiency_; }
  double get_goal_temp() { return goal_temp_c_; }
  double get_max_power() { return max_power_w_; }
  bool is_active() { return active_; }
  double get_temp_in() { return temp_in_c_; }
  double get_power() { return power_w_; }
  double get_energy_consumed() { return energy_consumed_j_; }
  double get_time_to_goal_temp() const;
};

} // namespace simgrid::plugins
#endif
