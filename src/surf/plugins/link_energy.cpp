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


 This is the energy plugin, enabling to account for the dissipated energy in the simulated platform.

 The energy consumption of a link depends directly on its current traffic load. Specify that consumption in your
 platform file as follows:

 \verbatim
 <link id="SWITCH1" bandwidth="125000000" latency="5E-5" sharing_policy="SHARED" >
 <prop id="watts" value="100.0:200.0" />
 <prop id="watt_off" value="10" />
 </link>
 \endverbatim

 The first property means that when your link is switched on, but without anything to do, it will dissipate 100 Watts.
 If it's fully loaded, it will dissipate 200 Watts. If its load is at 50%, then it will dissipate 150 Watts.
 The second property means that when your host is turned off, it will dissipate only 10 Watts (please note that these
 values are arbitrary).

 To simulate the energy-related elements, first call the simgrid#energy#sg_link_energy_plugin_init() before your
 #MSG_init(),
 and then use the following function to retrieve the consumption of a given link: MSG_link_get_consumed_energy().
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(link_energy, surf, "Logging specific to the SURF LinkEnergy plugin");

namespace simgrid {
namespace plugin {

class LinkPowerRange {
public:
  double idle;
  double busy;

  LinkPowerRange(double idle, double busy) : idle(idle), busy(busy) {}
};

class LinkEnergy {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> EXTENSION_ID;

  explicit LinkEnergy(simgrid::s4u::Link* ptr);
  ~LinkEnergy();

  double getALinkTotalPower();
  void initWattsRangeList();
  double getTotalEnergy();
  void update();

private:
  double getPower();

  simgrid::s4u::Link* link{};

  std::vector<LinkPowerRange> power_range_watts_list{};

  double total_energy{0.0};
  double last_updated{0.0}; /*< Timestamp of the last energy update event*/
};

simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> LinkEnergy::EXTENSION_ID;

LinkEnergy::LinkEnergy(simgrid::s4u::Link* ptr) : link(ptr), last_updated(surf_get_clock())
{
}

LinkEnergy::~LinkEnergy() = default;

void LinkEnergy::update()
{
  double power = getPower();
  double now   = surf_get_clock();
  total_energy += power * (now - last_updated);
  last_updated = now;
}

void LinkEnergy::initWattsRangeList()
{

  if (!power_range_watts_list.empty())
    return;

  const char* all_power_values_str = this->link->getProperty("watt_range");

  if (all_power_values_str == nullptr)
    return;

  std::vector<std::string> all_power_values;
  boost::split(all_power_values, all_power_values_str, boost::is_any_of(","));

  for (auto current_power_values_str : all_power_values) {
    /* retrieve the power values associated */
    std::vector<std::string> current_power_values;
    boost::split(current_power_values, current_power_values_str, boost::is_any_of(":"));
    xbt_assert(current_power_values.size() == 2, "Power properties incorrectly defined - "
                                                 "could not retrieve idle and busy power values for link %s",
               this->link->getCname());

    /* min_power corresponds to the idle power (link load = 0) */
    /* max_power is the power consumed at 100% link load       */
    char* idle = bprintf("Invalid idle power value for link%s", this->link->getCname());
    char* busy = bprintf("Invalid busy power value for %s", this->link->getCname());

    double idleVal = xbt_str_parse_double((current_power_values.at(0)).c_str(), idle);

    double busyVal = xbt_str_parse_double((current_power_values.at(1)).c_str(), busy);

    this->power_range_watts_list.push_back(LinkPowerRange(idleVal, busyVal));

    xbt_free(idle);
    xbt_free(busy);
    update();
  }
}

double LinkEnergy::getPower()
{

  if (power_range_watts_list.empty())
    return 0.0;

  auto range = power_range_watts_list[0];

  double busy = range.busy;
  double idle = range.idle;

  double power_slope = busy - idle;

  double normalized_link_usage = link->getUsage() / link->bandwidth();
  double dynamic_power         = power_slope * normalized_link_usage;

  return idle + dynamic_power;
}

double LinkEnergy::getTotalEnergy()
{
  update();
  return this->total_energy;
}
}
}

using simgrid::plugin::LinkEnergy;

/* **************************** events  callback *************************** */
static void onCreation(simgrid::s4u::Link& link)
{
  XBT_DEBUG("onCreation is called for link: %s", link.getCname());
  link.extension_set(new LinkEnergy(&link));
}

static void onCommunicate(simgrid::surf::NetworkAction* action, simgrid::s4u::Host* src, simgrid::s4u::Host* dst)
{
  XBT_DEBUG("onCommunicate is called");
  for (simgrid::surf::LinkImpl* link : action->links()) {

    if (link == nullptr)
      continue;

    XBT_DEBUG("Update link %s", link->getCname());
    // Get the link_energy extension for the relevant link
    LinkEnergy* link_energy = link->piface_.extension<LinkEnergy>();
    link_energy->initWattsRangeList();
    link_energy->update();
  }
}

static void onActionStateChange(simgrid::surf::NetworkAction* action)
{
  XBT_DEBUG("onActionStateChange is called");
  for (simgrid::surf::LinkImpl* link : action->links()) {

    if (link == nullptr)
      continue;

    // Get the link_energy extension for the relevant link
    LinkEnergy* link_energy = link->piface_.extension<LinkEnergy>();
    link_energy->update();
  }
}

static void onLinkStateChange(simgrid::s4u::Link& link)
{
  XBT_DEBUG("onLinkStateChange is called for link: %s", link.getCname());

  LinkEnergy* link_energy = link.extension<LinkEnergy>();
  link_energy->update();
}

static void onLinkDestruction(simgrid::s4u::Link& link)
{
  XBT_DEBUG("onLinkDestruction is called for link: %s", link.getCname());

  LinkEnergy* link_energy = link.extension<LinkEnergy>();
  link_energy->update();
}

static void computeAndDisplayTotalEnergy()
{
  std::vector<simgrid::s4u::Link*> link_list;
  simgrid::s4u::Engine::getInstance()->getLinkList(&link_list);
  double total_energy = 0.0; // Total dissipated energy (whole platform)
  for (const auto link : link_list) {
    LinkEnergy* link_energy = link->extension<LinkEnergy>();

    double a_link_total_energy = link_energy->getTotalEnergy();
    total_energy += a_link_total_energy;
    const char* name = link->getCname();
    if (strcmp(name, "__loopback__"))
      XBT_INFO("Link '%s' total consumption: %f", name, a_link_total_energy);
  }

  XBT_INFO("Total energy over all links: %f", total_energy);
}

static void onSimulationEnd()
{
  computeAndDisplayTotalEnergy();
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

  simgrid::s4u::Link::onCreation.connect(&onCreation);
  simgrid::s4u::Link::onStateChange.connect(&onLinkStateChange);
  simgrid::s4u::Link::onDestruction.connect(&onLinkDestruction);
  simgrid::s4u::Link::onCommunicationStateChange.connect(&onActionStateChange);
  simgrid::s4u::Link::onCommunicate.connect(&onCommunicate);
  simgrid::s4u::onSimulationEnd.connect(&onSimulationEnd);
}

SG_END_DECL()
