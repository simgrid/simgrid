/* Copyright (c) 2010, 2012-2016. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/link_energy.h"
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

  double getALinkTotalPower(sg_link_t link);
  void initWattsRangeList();
  double getLinkUsage();
  double getALinkTotalEnergy(sg_link_t link);
  void update();

private:
  double computeALinkPower();
  void computeALinkTotalEnergy();

  simgrid::s4u::Link* link{};
  //	simgrid::s4u::Link *up_link { };
  //	simgrid::s4u::Link *down_link { };

  std::vector<LinkPowerRange> power_range_watts_list{};

  double a_link_total_energy{0.0};

  double last_updated{0.0}; /*< Timestamp of the last energy update event*/
  double current_link_usage{0.0};
};

simgrid::xbt::Extension<simgrid::s4u::Link, LinkEnergy> LinkEnergy::EXTENSION_ID;

LinkEnergy::LinkEnergy(simgrid::s4u::Link* ptr) : link(ptr), last_updated(surf_get_clock())
{

  /*std::string lnk_name(this->link->name());
   size_t lnk_down = lnk_name.find("_DOWN");
   size_t lnk_up = lnk_name.find("_UP");

   if (lnk_down != std::string::npos) {

   this->down_link = this->link->byName(lnk_name.c_str());
   replace(lnk_name, "_DOWN", "_UP", lnk_down);
   this->up_link = this->link->byName(lnk_name.c_str());

   } else if (lnk_up != std::string::npos) {

   this->up_link = this->link->byName(lnk_name.c_str());
   replace(lnk_name, "_UP", "_DOWN", lnk_up);
   this->down_link = this->link->byName(lnk_name.c_str());

   } else {
   this->up_link = this->link;
   }*/
}

LinkEnergy::~LinkEnergy() = default;

void LinkEnergy::update()
{

  this->current_link_usage = this->link->getUsage();

  computeALinkTotalEnergy();

  /*
   double uplink_usage{0.0};
   double downlink_usage{0.0};

   std::string lnk_name(this->link->name());
   size_t lnk_down = lnk_name.find("_DOWN");
   size_t lnk_up = lnk_name.find("_UP");

   if (lnk_down != std::string::npos) {

   downlink_usage = lmm_constraint_get_usage(
   this->down_link->pimpl_->constraint());

   this->up_link->extension<LinkEnergy>()->updateLinkUsage();

   } else if (lnk_up != std::string::npos) {

   uplink_usage = lmm_constraint_get_usage(
   this->up_link->pimpl_->constraint());

   this->link_usage = downlink_usage + uplink_usage;

   } else {

   this->link_usage = lmm_constraint_get_usage(
   this->up_link->pimpl_->constraint());

   }*/
}

void LinkEnergy::initWattsRangeList()
{

  if (!power_range_watts_list.empty())
    return;

  xbt_assert(power_range_watts_list.empty(), "Power properties incorrectly defined - "
                                             "could not retrieve idle and busy power values for link %s",
             this->link->getCname());

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

    idleVal *= 2; // the idle value is multiplied by 2 because SimGrid's 1 link is mapped to 2 NetDevices in ECOFEN
    double busyVal = xbt_str_parse_double((current_power_values.at(1)).c_str(), busy);
    busyVal *= 2; // the busy value is multiplied by 2 because SimGrid's 1 link is mapped to 2 NetDevices in ECOFEN

    this->power_range_watts_list.push_back(LinkPowerRange(idleVal, busyVal));

    xbt_free(idle);
    xbt_free(busy);
    update();
  }
}

double LinkEnergy::computeALinkPower()
{

  double dynamic_power = 0.0;

  if (power_range_watts_list.empty()) {
    return 0.0;
  }

  auto range = power_range_watts_list[0];

  double busy = range.busy;
  double idle = range.idle;

  double power_slope = busy - idle;

  if (this->last_updated > 0) {

    double normalized_link_usage = this->current_link_usage / this->link->bandwidth();
    dynamic_power                = power_slope * normalized_link_usage;

  } else {
    dynamic_power = 0.0;
  }
  double current_power = idle + dynamic_power;

  return current_power;
}

void LinkEnergy::computeALinkTotalEnergy()
{
  double current_power = computeALinkPower();
  double now           = surf_get_clock();
  this->a_link_total_energy += current_power * (now - last_updated);
  last_updated = now;
}

double LinkEnergy::getLinkUsage()
{
  return this->current_link_usage;
}

double LinkEnergy::getALinkTotalEnergy(sg_link_t link)
{
  return this->a_link_total_energy;
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
  double total_power  = 0.0; // Total power consumption (whole platform)
  double total_energy = 0.0;
  for (const auto link : link_list) {
    LinkEnergy* link_energy = link->extension<LinkEnergy>();
    link_energy->update();

    double a_link_total_energy = link_energy->getALinkTotalEnergy(link);
    total_energy += a_link_total_energy;
    const char* name = link->getCname();
    if (strcmp(name, "__loopback__")) {
      XBT_INFO("%s Usage %f Bandwidth %f Energy %f", name, link_energy->getLinkUsage(), link->bandwidth(),
               a_link_total_energy);
    }
  }

  XBT_INFO("SgTotalPower %f SgTotalEnergy %f SgTransferTime %f", total_power, total_energy, surf_get_clock());
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

/** @brief Returns the total energy consumed by the link so far (in Joules)
 *
 *  See also @ref SURF_plugin_energy.
 */

double sg_link_get_usage(sg_link_t link)
{
  xbt_assert(LinkEnergy::EXTENSION_ID.valid(),
             "The Energy plugin is not active. Please call sg_energy_plugin_init() during initialization.");
  LinkEnergy* link_energy = link->extension<LinkEnergy>();
  return link_energy->getLinkUsage();
}

SG_END_DECL()
