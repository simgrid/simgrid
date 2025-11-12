/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

 #include <simgrid/plugins/load.h>
 #include <simgrid/s4u/Engine.hpp>
 #include <simgrid/s4u/Exec.hpp>
 #include <simgrid/s4u/Host.hpp>
 #include <simgrid/s4u/VirtualMachine.hpp>
 
 #include "src/kernel/activity/ExecImpl.hpp"
 #include "src/simgrid/module.hpp" // SIMGRID_REGISTER_PLUGIN
 
 // Makes sure that this plugin can be activated from the command line with ``--cfg=plugin:vm_load``
 SIMGRID_REGISTER_PLUGIN(vm_load, "VM CPU load", &sg_vm_load_plugin_init)
 
 /** @defgroup plugin_vm_load Simple plugin that monitors the current load for each VM. 

  @beginrst
This plugin is similar to host_load but for VMs.
It attaches an extension to each VM to store some data, and places callbacks in the following signals:

  - :cpp:func:`simgrid::s4u::VirtualMachine::on_creation_cb`: Attach a new extension to the newly created vm.
  - :cpp:func:`simgrid::s4u::Exec::on_start_cb`: Make note that a new execution started, increasing the load.
  - :cpp:func:`simgrid::s4u::Exec::on_completion_cb`: Make note that an execution completed, decreasing the load.
  - :cpp:func:`simgrid::s4u::Host::on_onoff_cb`: Do what is appropriate when the host of the VM gets turned off or on.
  - :cpp:func:`simgrid::s4u::Host::on_speed_change_cb`: Do what is appropriate when the DVFS is modified in the host of the VM.

  Note that extensions are automatically destroyed when the virtual machine gets destroyed.
  @endrst
*/
 
 XBT_LOG_NEW_DEFAULT_SUBCATEGORY(vm_load, plugin, "Logging specific to the VMLoad plugin");
 
 namespace simgrid::plugin {
 
 static const double activity_uninitialized_remaining_cost = -1;
 
 // This class stores the extra data needed by this plugin about a given vm
 class VMLoad {
 public:
  // It uses simgrid::s4u::Host because a VM is a extension of a Host
   static simgrid::xbt::Extension<simgrid::s4u::Host, VMLoad> EXTENSION_ID;
 
   explicit VMLoad(simgrid::s4u::VirtualMachine* ptr)
       : host_(ptr)
       , last_updated_(simgrid_get_clock())
       , last_reset_(simgrid_get_clock())
       , current_speed_(host_->get_pm()->get_speed())
       , current_flops_(host_->get_load())
   {
   }
   VMLoad() = delete;
   explicit VMLoad(simgrid::s4u::Host& ptr) = delete;
   explicit VMLoad(simgrid::s4u::Host&& ptr) = delete;
 
   double get_current_load() const;
   /** Get the the average load since last reset(), as a ratio
    *
    * That's the ratio (amount of flops that were actually computed) / (amount of flops that could have been computed at full speed)
    */
   double get_average_load() { update(); return (theor_max_flops_ == 0) ? 0 : computed_flops_ / theor_max_flops_; };
   /** Amount of flops computed since last reset() */
   double get_computed_flops() { update(); return computed_flops_; }
   /** Return idle time since last reset() */
   double get_idle_time() { update(); return idle_time_; }
   /** Return idle time over the whole simulation */
   double get_total_idle_time() { update(); return total_idle_time_; }
   void update();
   void add_activity(simgrid::kernel::activity::ExecImpl* activity);
   void reset();
 
 private:
   simgrid::s4u::VirtualMachine* host_ = nullptr;
   /* Stores all currently ongoing activities (computations) on this machine */
   std::map<simgrid::kernel::activity::ExecImpl*, /* cost still remaining*/ double> current_activities;
   double last_updated_      = 0;
   double last_reset_        = 0;
   /**
    * current_speed each core is running at; we need to store this as the speed
    * will already have changed once we get notified
    */
   double current_speed_     = 0;
   /**
    * How many flops are currently used by all the processes running on this VM
    */
   double current_flops_     = 0;
   double computed_flops_    = 0;
   double idle_time_         = 0;
   double total_idle_time_   = 0; /* This updated but never gets reset */
   double theor_max_flops_   = 0;
 };
 
 // Create the static field that the extension mechanism needs
 simgrid::xbt::Extension<simgrid::s4u::Host, VMLoad> VMLoad::EXTENSION_ID;
 
 void VMLoad::add_activity(simgrid::kernel::activity::ExecImpl* activity)
 {
   current_activities.insert({activity, activity_uninitialized_remaining_cost});
 }
 
 void VMLoad::update()
 {
   double now = simgrid_get_clock();
 
   // This loop updates the flops that the host executed for the ongoing computations
   auto iter = begin(current_activities);
   while (iter != end(current_activities)) {
     const auto& activity                   = iter->first;  // Just an alias
     auto& remaining_cost_after_last_update = iter->second; // Just an alias
     auto& action                           = activity->model_action_;
     auto current_iter                      = iter;
     ++iter;
 
     if (action != nullptr && action->get_finish_time() != now &&
         activity->get_state() == kernel::activity::State::RUNNING) {
       if (remaining_cost_after_last_update == activity_uninitialized_remaining_cost) {
         remaining_cost_after_last_update = action->get_cost();
       }
       double computed_flops_since_last_update = remaining_cost_after_last_update - /*remaining now*/activity->get_remaining();
       computed_flops_                        += computed_flops_since_last_update;
       remaining_cost_after_last_update        = activity->get_remaining();
     } else if (activity->get_state() == kernel::activity::State::DONE) {
       computed_flops_ += remaining_cost_after_last_update;
       current_activities.erase(current_iter);
     }
   }
 
   /* Current flop per second computed by the cpu; current_flops = k * pstate_speed_in_flops, k @in {0, 1, ..., cores-1}
    * designates number of active cores; will be 0 if CPU is currently idle */
   current_flops_ = host_->get_load();
 
   if (current_flops_ == 0) {
     idle_time_ += (now - last_updated_);
     total_idle_time_ += (now - last_updated_);
     XBT_DEBUG("[%s]: Currently idle -> Added %f seconds to idle time (totaling %fs)", host_->get_cname(), (now - last_updated_), idle_time_);
   }
 
   theor_max_flops_ += current_speed_ * host_->get_core_count() * (now - last_updated_);
   current_speed_ = host_->get_pm()->get_speed();
   last_updated_  = now;
 }
 
 // Get the current load as a ratio = achieved_flops / (core_current_speed * core_amount)
 double VMLoad::get_current_load() const
 {
   // We don't need to call update() here because it is called every time an action terminates or starts
   return current_flops_ / (host_->get_pm()->get_speed() * host_->get_core_count());
 }
 
 /*
  * Resets the counters
  */
 void VMLoad::reset()
 {
   last_updated_    = simgrid_get_clock();
   last_reset_      = simgrid_get_clock();
   idle_time_       = 0;
   computed_flops_  = 0;
   theor_max_flops_ = 0;
   current_flops_   = host_->get_load();
   current_speed_   = host_->get_pm()->get_speed();
 }
 } // namespace simgrid::plugin
 
 using simgrid::plugin::VMLoad;
 
 /* **************************** events  callback *************************** */
 /* This callback is fired either when the host changes its state (on/off) or its speed
  * (because the user changed the pstate, or because of external trace events) */
 static void on_vm_change(simgrid::s4u::Host const& host)
 {
    // If the host is a VM already, just update it
    if (dynamic_cast<simgrid::s4u::VirtualMachine const*>(&host)) // If it is a VM
    {
      host.extension<VMLoad>()->update();
    }
    else
    {
      // Verifies the VMs allocated in the host that changed
      const simgrid::s4u::Engine* e = simgrid::s4u::Engine::get_instance();
      for (auto& h : e->get_all_hosts()) {
        if (dynamic_cast<simgrid::s4u::VirtualMachine const*>(h))
        {
          if (dynamic_cast<simgrid::s4u::VirtualMachine const*>(h)->get_pm() == &host)
          {
            h->extension<VMLoad>()->update();
          }
        }
        
      }
  
    }
 }
 
 /* **************************** Public interface *************************** */
 
 /** @brief Initializes the HostLoad plugin
  *  @ingroup plugin_host_load
  */
 void sg_vm_load_plugin_init()
 {
   if (VMLoad::EXTENSION_ID.valid()) // Don't do the job twice
     return;
 
   // First register our extension of Hosts properly
   VMLoad::EXTENSION_ID = simgrid::s4u::Host::extension_create<VMLoad>();
 
   // Make sure that every future host also gets an extension (in case the platform is not loaded yet)
   simgrid::s4u::VirtualMachine::on_creation_cb(
       [](simgrid::s4u::VirtualMachine& host) { host.extension_set(new VMLoad(&host)); });

   simgrid::s4u::Exec::on_start_cb([](simgrid::s4u::Exec const& activity) {
     if (activity.get_host_number() == 1) { // We only run on one host
       simgrid::s4u::Host* host         = activity.get_host();
       if (!dynamic_cast<simgrid::s4u::VirtualMachine const*>(host)) // Ignore Hosts
        return;
       xbt_assert(host != nullptr);
       host->extension<VMLoad>()->add_activity(static_cast<simgrid::kernel::activity::ExecImpl*>(activity.get_impl()));
       host->extension<VMLoad>()->update(); // If the system was idle until now, we need to update *before*
                                              // this computation starts running so we can keep track of the
                                              // idle time. (Communication operations don't trigger this hook!)
     }
     else { // This runs on multiple hosts
       XBT_WARN("HostLoad plugin currently does not support executions on several hosts");
     }
   });
   simgrid::s4u::Exec::on_completion_cb([](simgrid::s4u::Exec const& exec) {
     if (exec.get_host_number() == 1) { // We only run on one host
       simgrid::s4u::Host* host = exec.get_host();
       if (!dynamic_cast<simgrid::s4u::VirtualMachine const*>(host)) // Ignore Hosts
        return;
       xbt_assert(host != nullptr);
       host->extension<VMLoad>()->update();
     } else { // This runs on multiple hosts
       XBT_WARN("HostLoad plugin currently does not support executions on several hosts");
     }
   });
   simgrid::s4u::Host::on_onoff_cb(&on_vm_change);
   simgrid::s4u::Host::on_speed_change_cb(&on_vm_change);
 }
 
 /** @brief Returns the current load of that host, as a ratio = achieved_flops / (core_current_speed * core_amount)
  *  @ingroup plugin_host_load
  */
 double sg_vm_get_current_load(const_sg_host_t host)
 {
   xbt_assert(VMLoad::EXTENSION_ID.valid(), "Please sg_vm_load_plugin_init() to initialize this plugin.");
 
   return host->extension<VMLoad>()->get_current_load();
 }
 
 /** @brief Returns the current load of that host
  *  @ingroup plugin_host_load
  */
 double sg_vm_get_avg_load(const_sg_host_t host)
 {
   xbt_assert(VMLoad::EXTENSION_ID.valid(), "Please sg_vm_load_plugin_init() to initialize this plugin.");
 
   return host->extension<VMLoad>()->get_average_load();
 }
 
 /** @brief Returns the time this host was idle since the last reset
  *  @ingroup plugin_host_load
  */
 double sg_vm_get_idle_time(const_sg_host_t host)
 {
   xbt_assert(VMLoad::EXTENSION_ID.valid(), "Please sg_vm_load_plugin_init() to initialize this plugin.");
 
   return host->extension<VMLoad>()->get_idle_time();
 }
 
 /** @brief Returns the time this host was idle since the beginning of the simulation
  *  @ingroup plugin_host_load
  */
 double sg_vm_get_total_idle_time(const_sg_host_t host)
 {
   xbt_assert(VMLoad::EXTENSION_ID.valid(), "Please sg_vm_load_plugin_init() to initialize this plugin.");
 
   return host->extension<VMLoad>()->get_total_idle_time();
 }
 
 /** @brief Returns the amount of flops computed by that host since the last reset
  *  @ingroup plugin_host_load
  */
 double sg_vm_get_computed_flops(const_sg_host_t host)
 {
   xbt_assert(VMLoad::EXTENSION_ID.valid(), "Please sg_vm_load_plugin_init() to initialize this plugin.");
 
   return host->extension<VMLoad>()->get_computed_flops();
 }
 
 /** @brief Resets the idle time and flops amount of that host
  *  @ingroup plugin_host_load
  */
 void sg_vm_load_reset(const_sg_host_t host)
 {
   xbt_assert(VMLoad::EXTENSION_ID.valid(), "Please sg_vm_load_plugin_init() to initialize this plugin.");
 
   host->extension<VMLoad>()->reset();
 }
 