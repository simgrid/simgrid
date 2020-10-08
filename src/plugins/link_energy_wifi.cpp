/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/network_wifi.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "xbt/config.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <map>

SIMGRID_REGISTER_PLUGIN(link_energy_wifi, "Energy wifi test", &sg_wifi_energy_plugin_init);
/** @degroup plugin_link_energy_wifi Plugin WiFi energy
 * 
 * This is the WiFi energy plugin, accounting for the dissipated energy of WiFi links.
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(link_energy_wifi, surf, "Logging specific to the link energy wifi plugin");

namespace simgrid {
namespace plugin {

class XBT_PRIVATE LinkEnergyWifi {

public:
  static simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergyWifi> EXTENSION_ID;

  explicit LinkEnergyWifi(simgrid::s4u::Link* ptr) : link_(ptr) {}

  ~LinkEnergyWifi() = default;
  LinkEnergyWifi()  = delete;

  /**
   * Update the energy consumed by link_ when transmissions start or end
   */
  void update(const simgrid::kernel::resource::NetworkAction &);
  
  /**
   * Update the energy consumed when link_ is destroyed
   */
  void update_destroy();

  /**
   * Fetches energy consumption values from the platform file.
   * The user can specify:
   *  - wifi_watt_values: energy consumption in each state (IDLE:Tx:Rx:SLEEP)
   *      default: 0.82:1.14:0.94:0.10
   *  - controlDuration: duration of active beacon transmissions per second
   *      default: 0.0036
   */
  void init_watts_range_list();

  double get_consumed_energy(void) { return eDyn_ + eStat_; }
  /** Get the dynamic part of the energy for this link */
  double get_energy_dynamic(void) { return eDyn_; }
  double get_energy_static(void) { return eStat_; }
  double get_duration_comm(void) { return dur_TxRx_; }
  double get_duration_idle(void) { return dur_idle_; }

  /** Set the power consumed by this link while idle */
  void set_power_idle(double value) { pIdle_ = value; }
  /** Set the power consumed by this link while transmitting */
  void set_power_tx(double value) { pTx_ = value; }
  /** Set the power consumed by this link while receiving */
  void set_power_rx(double value) { pRx_ = value; }
  /** Set the power consumed by this link while sleeping */
  void set_power_sleep(double value) { pSleep_ = value; }

private:
  // associative array keeping size of data already sent for a given flow (required for interleaved actions)
  std::map<simgrid::kernel::resource::NetworkWifiAction*, std::pair<int, double>> flowTmp{};

  // WiFi link the plugin instance is attached to
  s4u::Link* link_{};

  // dynamic energy accumulated since the simulation start (active durations consumption)
  double eDyn_{0.0};
  // static energy (no activity consumption)
  double eStat_{0.0};

  // duration since previous energy update
  double prev_update_{0.0};

  // Same energy calibration values as ns3 by default
  // https://www.nsnam.org/docs/release/3.30/doxygen/classns3_1_1_wifi_radio_energy_model.html#details
  double pIdle_{0.82};
  double pTx_{1.14};
  double pRx_{0.94};
  double pSleep_{0.10};

  // constant taking beacons into account (can be specified by the user)
  double control_duration_{0.0036};

  // Measurements for report
  double dur_TxRx_{0}; // Duration of transmission
  double dur_idle_{0}; // Duration of idle time
  bool valuesInit_{false};
};

xbt::Extension<s4u::Link, LinkEnergyWifi> LinkEnergyWifi::EXTENSION_ID;

void LinkEnergyWifi::update_destroy()
{
  simgrid::kernel::resource::NetworkWifiLink* wifi_link =
    static_cast<simgrid::kernel::resource::NetworkWifiLink*>(link_->get_impl());
  double duration = surf_get_clock() - prev_update_;
  prev_update_    = surf_get_clock();

  dur_idle_ += duration;

  // add IDLE energy usage, as well as beacons consumption since previous update
  eDyn_ += duration * control_duration_ * wifi_link->get_host_count() * pRx_;
  eStat_ += (duration - (duration * control_duration_)) * pIdle_ * (wifi_link->get_host_count() + 1);

  XBT_DEBUG("finish eStat_ += %f * %f * (%d+1) | eStat = %f", duration, pIdle_, wifi_link->get_host_count(), eStat_);
}

void LinkEnergyWifi::update(const simgrid::kernel::resource::NetworkAction&)
{
  init_watts_range_list();

  double duration = surf_get_clock() - prev_update_;
  prev_update_    = surf_get_clock();

  // we don't update for null durations
  if(duration < 1e-6)
    return;

  simgrid::kernel::resource::NetworkWifiLink* wifi_link =
      static_cast<simgrid::kernel::resource::NetworkWifiLink*>(link_->get_impl());
  
  const kernel::lmm::Variable* var;
  const kernel::lmm::Element* elem = nullptr;

  /**
   * We update the energy consumed by each flow active on the link since the previous update.
   * To do this, we need to know how much time each flow has been effectively sending data on the WiFi link since the previous update (durUsage). 
   * We compute this value using the size of the flow, 
   * the amount of data already spent (using flowTmp), 
   * as well as the bandwidth used by the flow since the previous update (using LMM variables)
   * Since flows are sharing the medium, the total active duration on the link is equal to the transmission/reception duration used by the flow with the longest active time since the previous update 
   */
  double durUsage = 0;
  while((var = wifi_link->get_constraint()->get_variable(&elem))) {
    auto* action = static_cast<kernel::resource::NetworkWifiAction*>(var->get_id());
    XBT_DEBUG("cost: %f action value: %f link rate 1: %f link rate 2: %f", action->get_cost(), action->get_variable()->get_value(), wifi_link->get_host_rate(&action->get_src()),wifi_link->get_host_rate(&action->get_dst()));
    action->get_variable();
    
    double du = 0; // durUsage on the current flow
    std::map<simgrid::kernel::resource::NetworkWifiAction *, std::pair<int, double>>::iterator it;

    if(action->get_variable()->get_value()) {
      it = flowTmp.find(action);
      
      // if the flow has not been registered, initialize it: 0 bytes sent, and not updated since its creation timestamp
      if(it == flowTmp.end())
        flowTmp[action] = std::pair<int,double>(0, action->get_start_time());

      it = flowTmp.find(action);

      /**
       * The active duration of the link is equal to the amount of data it had to send divided by the bandwidth on the link.
       * If this is longer than the duration since the previous update, active duration = now - previous_update
       */
      du = (action->get_cost()-it->second.first) / action->get_variable()->get_value();

      if(du > surf_get_clock()-it->second.second)
        du = surf_get_clock()-it->second.second;

      // if the flow has been more active than the others
      if(du > durUsage)
        durUsage = du;

      // update the amount of data already sent by the flow
      it->second.first += du*action->get_variable()->get_value();
      it->second.second =  surf_get_clock();

      // important: if the transmission finished, remove it (needed for performance and multi-message flows)
      if(it->second.first >= action->get_cost())
        flowTmp.erase (it);
    }
  }

  XBT_DEBUG("durUsage: %f", durUsage);

  // beacons cost
  eDyn_ += duration * control_duration_ * wifi_link->get_host_count() * pRx_;

  /**
   * Same principle as ns3:
   *  - if tx or rx, update P_{dyn}
   *  - if idle i.e. get_usage = 0, update P_{stat}
   * P_{tot} = P_{dyn}+P_{stat}
   */
  if(link_->get_usage()){
    eDyn_ += /*duration * */ durUsage * ((wifi_link->get_host_count() * pRx_) + pTx_);
    eStat_ += (duration - durUsage) * pIdle_ * (wifi_link->get_host_count() + 1);
    XBT_DEBUG("eDyn +=  %f * ((%d * %f) + %f) | eDyn = %f (durusage =%f)", durUsage, wifi_link->get_host_count(), pRx_,
              pTx_, eDyn_, durUsage);
    dur_TxRx_ += duration;
  }else{
    dur_idle_ += duration;
    eStat_ += (duration - (duration * control_duration_)) * pIdle_ * (wifi_link->get_host_count() + 1);
  }

  XBT_DEBUG("eStat_ += %f * %f * (%d+1) | eStat = %f", duration, pIdle_, wifi_link->get_host_count(), eStat_);
}

void LinkEnergyWifi::init_watts_range_list()
{
  if (valuesInit_)
    return;
  valuesInit_                      = true;

  /* beacons factor
  Set to 0 if you do not want to compute beacons,
  otherwise to the duration of beacons transmissions per second
  */
  const char* beacons_factor = this->link_->get_property("control_duration");
  if(beacons_factor != nullptr) {
    try {
      control_duration_ = std::stod(beacons_factor);
    } catch (const std::invalid_argument&) {
      throw std::invalid_argument(std::string("Invalid beacons factor value for link ") + this->link_->get_cname());
    }
  }

  const char* all_power_values_str = this->link_->get_property("wifi_watt_values");
  if (all_power_values_str != nullptr)
  {
    std::vector<std::string> all_power_values;
    boost::split(all_power_values, all_power_values_str, boost::is_any_of(","));

    for (auto current_power_values_str : all_power_values) {
      /* retrieve the power values associated */
      std::vector<std::string> current_power_values;
      boost::split(current_power_values, current_power_values_str, boost::is_any_of(":"));
      xbt_assert(current_power_values.size() == 4,
                "Power properties incorrectly defined - could not retrieve idle, Tx, Rx, Sleep power values for link %s",
                this->link_->get_cname());

      /* min_power corresponds to the idle power (link load = 0) */
      /* max_power is the power consumed at 100% link load       */
      try {
        pSleep_ = std::stod(current_power_values.at(3));
      } catch (const std::invalid_argument&) {
        throw std::invalid_argument(std::string("Invalid idle power value for link ") + this->link_->get_cname());
      }
      try {
        pRx_ = std::stod(current_power_values.at(2));
      } catch (const std::invalid_argument&) {
        throw std::invalid_argument(std::string("Invalid idle power value for link ") + this->link_->get_cname());
      }
      try {
        pTx_ = std::stod(current_power_values.at(1));
      } catch (const std::invalid_argument&) {
        throw std::invalid_argument(std::string("Invalid idle power value for link ") + this->link_->get_cname());
      }
      try {
        pIdle_ = std::stod(current_power_values.at(0));
      } catch (const std::invalid_argument&) {
        throw std::invalid_argument(std::string("Invalid busy power value for link ") + this->link_->get_cname());
      }

      XBT_DEBUG("Values aa initialized with: pSleep=%f pIdle=%f pTx=%f pRx=%f", pSleep_, pIdle_, pTx_, pRx_);
    }
  }
}

} // namespace plugin
} // namespace simgrid

using simgrid::plugin::LinkEnergyWifi;

void sg_wifi_energy_plugin_init()
{
  if (LinkEnergyWifi::EXTENSION_ID.valid())
    return;

  XBT_INFO("Activating the wifi_energy plugin.");
  LinkEnergyWifi::EXTENSION_ID = simgrid::s4u::Link::extension_create<LinkEnergyWifi>();

  /**
   * Attaching to events:
   * - on_creation to initialize the plugin
   * - on_destruction to produce final energy results
   * - on_communication_state_change: to account the energy when communications are updated
   * - on_communicate: ''
   */
  simgrid::s4u::Link::on_creation.connect([](simgrid::s4u::Link& link) {
    // verify the link is appropriate to WiFi energy computations
    if (link.get_sharing_policy() == simgrid::s4u::Link::SharingPolicy::WIFI) {
      XBT_DEBUG("Wifi Link: %s, initialization of wifi energy plugin", link.get_cname());
      LinkEnergyWifi* plugin = new LinkEnergyWifi(&link);
      link.extension_set(plugin);
    } else {
      XBT_DEBUG("Not Wifi Link: %s, wifi energy on link not computed", link.get_cname());
    }
  });

  simgrid::s4u::Link::on_destruction.connect([](simgrid::s4u::Link const& link) {
    // output energy values if WiFi link
    if (link.get_sharing_policy() == simgrid::s4u::Link::SharingPolicy::WIFI) {
      link.extension<LinkEnergyWifi>()->update_destroy();
      XBT_INFO(
          "Link %s destroyed, consumed: %f J dyn: %f stat: %f durIdle: %f durTxRx: %f", link.get_cname(),
          link.extension<LinkEnergyWifi>()->get_consumed_energy(),
          link.extension<LinkEnergyWifi>()->get_energy_dynamic(), link.extension<LinkEnergyWifi>()->get_energy_static(),
          link.extension<LinkEnergyWifi>()->get_duration_idle(), link.extension<LinkEnergyWifi>()->get_duration_comm());
    }
  });

  simgrid::s4u::Link::on_communication_state_change.connect(
      [](simgrid::kernel::resource::NetworkAction const& action, simgrid::kernel::resource::Action::State previous) {
        // update WiFi links encountered during the communication
        for (simgrid::kernel::resource::LinkImpl* link : action.get_links()) {
          if (link != nullptr && link->get_sharing_policy() == simgrid::s4u::Link::SharingPolicy::WIFI) {
            link->get_iface()->extension<LinkEnergyWifi>()->update(action);
          }
        }
      });

  simgrid::s4u::Link::on_communicate.connect([](const simgrid::kernel::resource::NetworkAction& action) {
    const simgrid::kernel::resource::NetworkWifiAction* actionWifi = dynamic_cast<const simgrid::kernel::resource::NetworkWifiAction*>(&action);

    if(actionWifi == nullptr) 
      return;

    auto link_src  = actionWifi->get_src_link();
    auto link_dst = actionWifi->get_dst_link();

    if(link_src != nullptr)
      link_src->get_iface()->extension<LinkEnergyWifi>()->update(action);
    if(link_dst != nullptr)
      link_dst->get_iface()->extension<LinkEnergyWifi>()->update(action);

  });

}
