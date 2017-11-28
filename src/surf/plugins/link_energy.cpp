/* Copyright (c) 2017. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/simix.hpp"
#include "src/surf/network_interface.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <map>
#include <string>
#include <utility>
#include <vector>

/** @addtogroup SURF_plugin_energy


 This is the link energy plugin, accounting for the dissipated energy in the simulated platform.

 The energy consumption of a link depends directly on its current traffic load. Specify that consumption in your
 platform file as follows:

 \verbatim
 <link id="SWITCH1" bandwidth="125Mbps" latency="5us" sharing_policy="SHARED" >
 <prop id="watt_range" value="100.0:200.0" />
 <prop id="watt_off" value="10" />
 </link>
 \endverbatim

 The first property means that when your link is switched on, but without anything to do, it will dissipate 100 Watts.
 If it's fully loaded, it will dissipate 200 Watts. If its load is at 50%, then it will dissipate 150 Watts.
 The second property means that when your host is turned off, it will dissipate only 10 Watts (please note that these
 values are arbitrary).

 To simulate the energy-related elements, first call the simgrid#energy#sg_link_energy_plugin_init() before your
 #MSG_init(),
 and then use the following function to retrieve the consumption of a given link: sg_link_get_consumed_energy().
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(link_energy, surf, "Logging specific to the SURF LinkEnergy plugin");

namespace simgrid {
namespace plugin {

class LinkEnergy {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> EXTENSION_ID;

  explicit LinkEnergy(simgrid::s4u::Link* ptr);
  ~LinkEnergy();

  void initWattsRangeList();
  double getConsumedEnergy();
  void update();

private:
  double getPower();

  simgrid::s4u::Link* link_{};

  bool inited_{false};
  double idle_{0.0};
  double busy_{0.0};

  double totalEnergy_{0.0};
  double lastUpdated_{0.0}; /*< Timestamp of the last energy update event*/
};

simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> LinkEnergy::EXTENSION_ID;

LinkEnergy::LinkEnergy(simgrid::s4u::Link* ptr) : link_(ptr), lastUpdated_(surf_get_clock())
{
}

LinkEnergy::~LinkEnergy() = default;

void LinkEnergy::update()
{
  double power = getPower();
  double now   = surf_get_clock();
  totalEnergy_ += power * (now - lastUpdated_);
  lastUpdated_ = now;
}

void LinkEnergy::initWattsRangeList()
{

  if (inited_)
    return;
  inited_ = true;

  const char* all_power_values_str = this->link_->getProperty("watt_range");

  if (all_power_values_str == nullptr)
    return;

  std::vector<std::string> all_power_values;
  boost::split(all_power_values, all_power_values_str, boost::is_any_of(","));

  for (auto current_power_values_str : all_power_values) {
    /* retrieve the power values associated */
    std::vector<std::string> current_power_values;
    boost::split(current_power_values, current_power_values_str, boost::is_any_of(":"));
    xbt_assert(current_power_values.size() == 2,
               "Power properties incorrectly defined - could not retrieve idle and busy power values for link %s",
               this->link_->getCname());

    /* min_power corresponds to the idle power (link load = 0) */
    /* max_power is the power consumed at 100% link load       */
    char* idleMsg = bprintf("Invalid idle power value for link%s", this->link_->getCname());
    char* busyMsg = bprintf("Invalid busy power value for %s", this->link_->getCname());

    idle_ = xbt_str_parse_double((current_power_values.at(0)).c_str(), idleMsg);
    busy_ = xbt_str_parse_double((current_power_values.at(1)).c_str(), busyMsg);

    xbt_free(idleMsg);
    xbt_free(busyMsg);
    update();
  }
}

double LinkEnergy::getPower()
{

  if (!inited_)
    return 0.0;

  double power_slope = busy_ - idle_;

  double normalized_link_usage = link_->getUsage() / link_->bandwidth();
  double dynamic_power         = power_slope * normalized_link_usage;

  return idle_ + dynamic_power;
}

double LinkEnergy::getConsumedEnergy()
{
  if (lastUpdated_ < surf_get_clock()) // We need to simcall this as it modifies the environment
    simgrid::simix::kernelImmediate(std::bind(&LinkEnergy::update, this));
  return this->totalEnergy_;
}
}
}

using simgrid::plugin::LinkEnergy;

/* **************************** events  callback *************************** */
static void onCommunicate(simgrid::surf::NetworkAction* action, simgrid::s4u::Host* src, simgrid::s4u::Host* dst)
{
  XBT_DEBUG("onCommunicate is called");
  for (simgrid::surf::LinkImpl* link : action->links()) {

    if (link == nullptr)
      continue;

    XBT_DEBUG("Update link %s", link->getCname());
    LinkEnergy* link_energy = link->piface_.extension<LinkEnergy>();
    link_energy->initWattsRangeList();
    link_energy->update();
  }
}

static void onSimulationEnd()
{
  std::vector<simgrid::s4u::Link*> links;
  simgrid::s4u::Engine::getInstance()->getLinkList(&links);

  double total_energy = 0.0; // Total dissipated energy (whole platform)
  for (const auto link : links) {
    double link_energy = link->extension<LinkEnergy>()->getConsumedEnergy();
    total_energy += link_energy;
  }

  XBT_INFO("Total energy over all links: %f", total_energy);
}
/* **************************** Public interface *************************** */
SG_BEGIN_DECL()
/** \ingroup SURF_plugin_energy
 * \brief Enable energy plugin
 * \details Enable energy plugin to get joules consumption of each cpu. You should call this function before
 * #MSG_init().
 */
void sg_link_energy_plugin_init()
{

  if (LinkEnergy::EXTENSION_ID.valid())
    return;
  LinkEnergy::EXTENSION_ID = simgrid::s4u::Link::extension_create<LinkEnergy>();

  simgrid::s4u::Link::onCreation.connect([](simgrid::s4u::Link& link) {
    link.extension_set(new LinkEnergy(&link));
  });

  simgrid::s4u::Link::onStateChange.connect([](simgrid::s4u::Link& link) {
    link.extension<LinkEnergy>()->update();
  });

  simgrid::s4u::Link::onDestruction.connect([](simgrid::s4u::Link& link) {
    if (strcmp(link.getCname(), "__loopback__"))
      XBT_INFO("Energy consumption of link '%s': %f Joules", link.getCname(),
               link.extension<LinkEnergy>()->getConsumedEnergy());
  });

  simgrid::s4u::Link::onCommunicationStateChange.connect([](simgrid::surf::NetworkAction* action) {
    for (simgrid::surf::LinkImpl* link : action->links()) {
      if (link != nullptr)
        link->piface_.extension<LinkEnergy>()->update();
    }
  });

  simgrid::s4u::Link::onCommunicate.connect(&onCommunicate);
  simgrid::s4u::onSimulationEnd.connect(&onSimulationEnd);
}

/** @ingroup plugin_energy
 *  @brief Returns the total energy consumed by the link so far (in Joules)
 *
 *  Please note that since the consumption is lazily updated, it may require a simcall to update it.
 *  The result is that the actor requesting this value will be interrupted,
 *  the value will be updated in kernel mode before returning the control to the requesting actor.
 */
double sg_link_get_consumed_energy(sg_link_t link)
{
  return link->extension<LinkEnergy>()->getConsumedEnergy();
}
SG_END_DECL()
