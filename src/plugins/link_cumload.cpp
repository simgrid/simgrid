/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/host.h"
#include "simgrid/plugins/load.h"
#include "simgrid/s4u/Link.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"

#include <limits>

SIMGRID_REGISTER_PLUGIN(link_energy, "Link cumulated load.", &sg_link_cumload_plugin_init)

/** @defgroup plugin_link_cumload Plugin Link Cumulated Load

 This is the link cumulated load plugin.
 It enables to monitor how links are used over time by cumulating the amount of bytes that go through the link.

 Usage:
 - Call sg_link_cumload_plugin_init() between the SimGrid engine initialization and the platform loading.
 - Track the links you want to monitor with sg_link_cumload_track() — you can untrack them later with sg_link_cumload_untrack().
 - Query the cumulated data about your link via these functions:
   - sg_link_get_cum_load() to get the total load (in bytes) that went though the link since last reset.
   - sg_link_get_avg_load() to get the average load (in bytes) that went though the link since last reset.
   - sg_link_get_min_instantaneous_load() to get the peak minimum load (in bytes per second) observed on the link since last reset.
   - sg_link_get_max_instantaneous_load() to get the peak maximum load (in bytes per second) observer on the link since last reset.
 - Reset the counters on any tracked link via sg_link_cumload_reset().
*/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(link_cumload, surf, "Logging specific to the SURF LinkCumload plugin");

namespace simgrid {
namespace plugin {

class LinkCumload {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Link, LinkCumload> EXTENSION_ID;

  explicit LinkCumload(simgrid::s4u::Link* ptr);
  ~LinkCumload() = default;

  void track();
  void untrack();
  void reset();
  void update();
  double get_average_bytes();

  /// Getter methods.
  bool is_tracked() const;
  double get_cumulated_bytes();
  double get_min_bytes_per_second();
  double get_max_bytes_per_second();

private:
  s4u::Link* link_{}; /*< The link onto which this data is attached*/
  bool is_tracked_{false}; /*<Whether the link is tracked or not*/

  double cumulated_bytes_{}; /*< Cumulated load since last reset*/
  double min_bytes_per_second_{}; /*< Minimum instantaneous load observed since last reset*/
  double max_bytes_per_second_{}; /*< Maximum instantaneous load observed since last reset*/
  double last_reset_{}; /*< Timestamp of the last reset (init timestamp by default)*/
  double last_updated_{}; /*< Timestamp of the last energy update event*/
};

xbt::Extension<s4u::Link, LinkCumload> LinkCumload::EXTENSION_ID;

LinkCumload::LinkCumload(simgrid::s4u::Link* ptr) : link_(ptr), is_tracked_(false)
{
  XBT_DEBUG("Instantiating a LinkCumload for link '%s'", link_->get_cname());
}

void LinkCumload::track()
{
  xbt_assert(!is_tracked_, "Trying to track cumload of link '%s' while it is already tracked, aborting.", link_->get_cname());
  XBT_DEBUG("Tracking load of link '%s'", link_->get_cname());

  is_tracked_ = true;
  reset();
}

void LinkCumload::untrack()
{
  xbt_assert(is_tracked_, "Trying to untrack cumload of link '%s' while it is not tracked, aborting.", link_->get_cname());
  XBT_DEBUG("Untracking load of link '%s'", link_->get_cname());

  is_tracked_ = false;
}

void LinkCumload::reset()
{
  XBT_DEBUG("Resetting load of link '%s'", link_->get_cname());

  cumulated_bytes_ = 0.0;
  min_bytes_per_second_ = std::numeric_limits<double>::max();
  max_bytes_per_second_ = std::numeric_limits<double>::lowest();
  XBT_DEBUG("min_bytes_per_second_ = %g", min_bytes_per_second_);
  XBT_DEBUG("max_bytes_per_second_ = %g", max_bytes_per_second_);
  last_reset_ = surf_get_clock();
  last_updated_ = last_updated_;
}

void LinkCumload::update()
{
  XBT_DEBUG("Updating load of link '%s'", link_->get_cname());
  xbt_assert(is_tracked_,
    "Trying to update cumload of link '%s' while it is NOT tracked, aborting."
    " Please track your link with sg_link_cumload_track before trying to access any of its load metrics.",
    link_->get_cname());

  double current_instantaneous_bytes_per_second = link_->get_usage();
  double now = surf_get_clock();

  // Update minimum/maximum observed values if needed
  min_bytes_per_second_ = std::min(min_bytes_per_second_, current_instantaneous_bytes_per_second);
  max_bytes_per_second_ = std::max(max_bytes_per_second_, current_instantaneous_bytes_per_second);

  // Update cumulated load
  double duration_since_last_update = now - last_updated_;
  double bytes_since_last_update = duration_since_last_update * current_instantaneous_bytes_per_second;
  XBT_DEBUG("Cumulated %g bytes since last update (duration of %g seconds)", bytes_since_last_update, duration_since_last_update);
  xbt_assert(bytes_since_last_update >= 0, "LinkCumload plugin inconsistency: negative amount of bytes is accumulated.");

  cumulated_bytes_ += bytes_since_last_update;
  last_updated_ = now;
}

bool LinkCumload::is_tracked() const { return is_tracked_; }
double LinkCumload::get_cumulated_bytes() { update(); return cumulated_bytes_; }
double LinkCumload::get_min_bytes_per_second() { update(); return min_bytes_per_second_; }
double LinkCumload::get_max_bytes_per_second() { update(); return max_bytes_per_second_; }

double LinkCumload::get_average_bytes() {
  update();

  double now = surf_get_clock();
  if (now > last_reset_)
    return cumulated_bytes_ / (now - last_reset_);
  else
    return 0;
}

} // namespace plugin
} // namespace simgrid

using simgrid::plugin::LinkCumload;

/* **************************** events  callback *************************** */
static void on_communicate(const simgrid::kernel::resource::NetworkAction& action)
{
  XBT_DEBUG("on_communicate is called");
  for (simgrid::kernel::resource::LinkImpl* link : action.get_links()) {
    if (link == nullptr || link->get_sharing_policy() == simgrid::s4u::Link::SharingPolicy::WIFI)
      continue;

    auto link_cumload = link->get_iface()->extension<LinkCumload>();
    if (link_cumload->is_tracked()) {
      link_cumload->update();
    }
  }
}

/* **************************** Public interface *************************** */

/**
 * @ingroup plugin_link_cumload
 * @brief Initialize the link cumulated load plugin.
 * @pre The energy plugin should NOT be initialized.
 */
void sg_link_cumload_plugin_init()
{
  xbt_assert(sg_host_count() == 0, "Please call sg_link_cumload_plugin_init() BEFORE initializing the platform.");
  xbt_assert(!LinkCumload::EXTENSION_ID.valid(), "Double call to sg_link_cumload_plugin_init. Aborting.");
  LinkCumload::EXTENSION_ID = simgrid::s4u::Link::extension_create<LinkCumload>();

  // Attach new LinkCumload links created in the future.
  simgrid::s4u::Link::on_creation.connect([](simgrid::s4u::Link& link) {
    if (link.get_sharing_policy() != simgrid::s4u::Link::SharingPolicy::WIFI) {
      XBT_DEBUG("Wired link '%s' created. Attaching a LinkCumload to it.", link.get_cname());
      link.extension_set(new LinkCumload(&link));
    } else {
      XBT_DEBUG("Wireless link '%s' created. NOT attaching any LinkCumload to it.", link.get_cname());
    }
  });

  // Call this plugin on some of the links' events.
  simgrid::s4u::Link::on_communicate.connect(&on_communicate);
  simgrid::s4u::Link::on_state_change.connect([](simgrid::s4u::Link const& link) {
    if (link.get_sharing_policy() != simgrid::s4u::Link::SharingPolicy::WIFI) {
      auto link_cumload = link.extension<LinkCumload>();
      if (link_cumload->is_tracked())
        link_cumload->update();
    }
  });
  simgrid::s4u::Link::on_communication_state_change.connect(
      [](simgrid::kernel::resource::NetworkAction const& action,
         simgrid::kernel::resource::Action::State /* previous */) {
        for (simgrid::kernel::resource::LinkImpl* link : action.get_links()) {
          if (link != nullptr && link->get_sharing_policy() != simgrid::s4u::Link::SharingPolicy::WIFI) {
            auto link_cumload = link->get_iface()->extension<LinkCumload>();
            if (link_cumload->is_tracked())
              link_cumload->update();
          }
        }
      }
  );
}

/**
 * @ingroup plugin_link_cumload
 * @brief Start the tracking of a link.
 * @details This is required so the link cumulated load can be obtained later on.
 * @pre The energy plugin should be initialized (cf. sg_link_cumload_plugin_init()).
 * @pre The link should be in "untracked" state. In other words, do not call this function twice on the same link without a sg_link_cumload_untrack() call between them.
 *
 * @param link The link to track.
 */
void sg_link_cumload_track(const_sg_link_t link)
{
  xbt_assert(LinkCumload::EXTENSION_ID.valid(), "Please call sg_link_cumload_plugin_init before sg_link_cumload_track. Aborting.");
  link->extension<LinkCumload>()->track();
}

/**
 * @ingroup plugin_link_cumload
 * @brief Stop the tracking of a link.
 * @details Once the tracking is stopped, the cumulated load of the link can no longer be obtained until sg_link_cumload_track() is called again on this link.
 * @pre The energy plugin should be initialized (cf. sg_link_cumload_plugin_init()).
 * @pre The link should be in "tracked" state. In other words, do not call this function twice on the same link without a sg_link_cumload_track() call between them.
 *
 * @param link The link to untrack.
 */
void sg_link_cumload_untrack(const_sg_link_t link)
{
  xbt_assert(LinkCumload::EXTENSION_ID.valid(), "Please call sg_link_cumload_plugin_init before sg_link_cumload_untrack. Aborting.");
  link->extension<LinkCumload>()->untrack();
}

/**
 * @ingroup plugin_link_cumload
 * @brief Resets the cumulated load counters of a link.
 * @pre The energy plugin should be initialized (cf. sg_link_cumload_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_cumload_track()).
 *
 * @param link The link whose counters should be reset.
 */
void sg_link_cumload_reset(const_sg_link_t link)
{
  xbt_assert(LinkCumload::EXTENSION_ID.valid(), "Please call sg_link_cumload_plugin_init before sg_link_cumload_reset. Aborting.");
  link->extension<LinkCumload>()->reset();
}

/**
 * @ingroup plugin_link_cumload
 * @brief Get the cumulated load of a link (since the last call to sg_link_cumload_reset()).
 * @pre The energy plugin should be initialized (cf. sg_link_cumload_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_cumload_track()).

 * @param link The link whose cumulated load is requested.
 * @return The load (in bytes) that passed through the given link since the last call to sg_link_cumload_reset.
 */
double sg_link_get_cum_load(const_sg_link_t link)
{
  xbt_assert(LinkCumload::EXTENSION_ID.valid(), "Please call sg_link_cumload_plugin_init before sg_link_get_cum_load. Aborting.");
  return link->extension<LinkCumload>()->get_cumulated_bytes();
}

/**
 * @ingroup plugin_link_cumload
 * @brief Get the average load of a link (since the last call to sg_link_cumload_reset()).
 * @pre The energy plugin should be initialized (cf. sg_link_cumload_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_cumload_track()).

 * @param link The link whose average load is requested.
 * @return The average load (in bytes) that passed through the given link since the last call to sg_link_cumload_reset.
 */
double sg_link_get_avg_load(const_sg_link_t link)
{
  xbt_assert(LinkCumload::EXTENSION_ID.valid(), "Please call sg_link_cumload_plugin_init before sg_link_get_avg_load. Aborting.");
  return link->extension<LinkCumload>()->get_average_bytes();
}

/**
 * @ingroup plugin_link_cumload
 * @brief Get the minimum instantaneous load of a link (since the last call to sg_link_cumload_reset()).
 * @pre The energy plugin should be initialized (cf. sg_link_cumload_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_cumload_track()).

 * @param link The link whose average load is requested.
 * @return The minimum load instantaneous load (in bytes per second) that passed through the given link since the last call to sg_link_cumload_reset.
 */
double sg_link_get_min_instantaneous_load(const_sg_link_t link)
{
  xbt_assert(LinkCumload::EXTENSION_ID.valid(), "Please call sg_link_cumload_plugin_init before sg_link_get_min_load. Aborting.");
  return link->extension<LinkCumload>()->get_min_bytes_per_second();
}

/**
 * @ingroup plugin_link_cumload
 * @brief Get the maximum instantaneous load of a link (since the last call to sg_link_cumload_reset()).
 * @pre The energy plugin should be initialized (cf. sg_link_cumload_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_cumload_track()).

 * @param link The link whose average load is requested.
 * @return The maximum load instantaneous load (in bytes per second) that passed through the given link since the last call to sg_link_cumload_reset.
 */
double sg_link_get_max_instantaneous_load(const_sg_link_t link)
{
  xbt_assert(LinkCumload::EXTENSION_ID.valid(), "Please call sg_link_cumload_plugin_init before sg_link_get_max_load. Aborting.");
  return link->extension<LinkCumload>()->get_max_bytes_per_second();
}

