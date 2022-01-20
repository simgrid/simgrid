/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/load.h>
#include <simgrid/s4u/Engine.hpp>

#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"

#include <limits>

SIMGRID_REGISTER_PLUGIN(link_load, "Link cumulated load.", &sg_link_load_plugin_init)

/** @defgroup plugin_link_load Plugin Link Cumulated Load

 This is the link cumulated load plugin.
 It enables to monitor how links are used over time by cumulating the amount of bytes that go through the link.

 Usage:
 - Call sg_link_load_plugin_init() between the SimGrid engine initialization and the platform loading.
 - Track the links you want to monitor with sg_link_load_track() — you can untrack them later with
 sg_link_load_untrack().
 - Query the cumulated data about your link via these functions:
   - sg_link_get_cum_load() to get the total load (in bytes) that went though the link since last reset.
   - sg_link_get_avg_load() to get the average load (in bytes) that went though the link since last reset.
   - sg_link_get_min_instantaneous_load() to get the peak minimum load (in bytes per second) observed on the link since
 last reset.
   - sg_link_get_max_instantaneous_load() to get the peak maximum load (in bytes per second) observer on the link since
 last reset.
 - Reset the counters on any tracked link via sg_link_load_reset().
*/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(link_load, kernel, "Logging specific to the LinkLoad plugin");

namespace simgrid {
namespace plugin {

class LinkLoad {
  s4u::Link* link_{};      /*< The link onto which this data is attached*/
  bool is_tracked_{false}; /*<Whether the link is tracked or not*/

  double cumulated_bytes_{};      /*< Cumulated load since last reset*/
  double min_bytes_per_second_{}; /*< Minimum instantaneous load observed since last reset*/
  double max_bytes_per_second_{}; /*< Maximum instantaneous load observed since last reset*/
  double last_reset_{};           /*< Timestamp of the last reset (init timestamp by default)*/
  double last_updated_{};         /*< Timestamp of the last energy update event*/

public:
  static xbt::Extension<s4u::Link, LinkLoad> EXTENSION_ID;

  explicit LinkLoad(s4u::Link* ptr);

  void track();
  void untrack();
  void reset();
  void update();
  double get_average_bytes();

  /// Getter methods.
  bool is_tracked() const { return is_tracked_; }
  double get_cumulated_bytes();
  double get_min_bytes_per_second();
  double get_max_bytes_per_second();
};

xbt::Extension<s4u::Link, LinkLoad> LinkLoad::EXTENSION_ID;

LinkLoad::LinkLoad(s4u::Link* ptr) : link_(ptr), is_tracked_(false)
{
  XBT_DEBUG("Instantiating a LinkLoad for link '%s'", link_->get_cname());
}

void LinkLoad::track()
{
  xbt_assert(not is_tracked_, "Trying to track load of link '%s' while it is already tracked, aborting.",
             link_->get_cname());
  XBT_DEBUG("Tracking load of link '%s'", link_->get_cname());

  is_tracked_ = true;
  reset();
}

void LinkLoad::untrack()
{
  xbt_assert(is_tracked_, "Trying to untrack load of link '%s' while it is not tracked, aborting.", link_->get_cname());
  XBT_DEBUG("Untracking load of link '%s'", link_->get_cname());

  is_tracked_ = false;
}

void LinkLoad::reset()
{
  XBT_DEBUG("Resetting load of link '%s'", link_->get_cname());

  cumulated_bytes_      = 0.0;
  min_bytes_per_second_ = std::numeric_limits<double>::max();
  max_bytes_per_second_ = std::numeric_limits<double>::lowest();
  XBT_DEBUG("min_bytes_per_second_ = %g", min_bytes_per_second_);
  XBT_DEBUG("max_bytes_per_second_ = %g", max_bytes_per_second_);
  last_reset_   = simgrid::s4u::Engine::get_clock();
  last_updated_ = last_reset_;
}

void LinkLoad::update()
{
  XBT_DEBUG("Updating load of link '%s'", link_->get_cname());
  xbt_assert(is_tracked_,
             "Trying to update load of link '%s' while it is NOT tracked, aborting."
             " Please track your link with sg_link_load_track before trying to access any of its load metrics.",
             link_->get_cname());

  double current_instantaneous_bytes_per_second = link_->get_usage();
  double now                                    = simgrid::s4u::Engine::get_clock();

  // Update minimum/maximum observed values if needed
  min_bytes_per_second_ = std::min(min_bytes_per_second_, current_instantaneous_bytes_per_second);
  max_bytes_per_second_ = std::max(max_bytes_per_second_, current_instantaneous_bytes_per_second);

  // Update cumulated load
  double duration_since_last_update = now - last_updated_;
  double bytes_since_last_update    = duration_since_last_update * current_instantaneous_bytes_per_second;
  XBT_DEBUG("Cumulated %g bytes since last update (duration of %g seconds)", bytes_since_last_update,
            duration_since_last_update);
  xbt_assert(bytes_since_last_update >= 0, "LinkLoad plugin inconsistency: negative amount of bytes is accumulated.");

  cumulated_bytes_ += bytes_since_last_update;
  last_updated_ = now;
}

double LinkLoad::get_cumulated_bytes()
{
  update();
  return cumulated_bytes_;
}
double LinkLoad::get_min_bytes_per_second()
{
  update();
  return min_bytes_per_second_;
}
double LinkLoad::get_max_bytes_per_second()
{
  update();
  return max_bytes_per_second_;
}

double LinkLoad::get_average_bytes()
{
  update();

  double now = simgrid::s4u::Engine::get_clock();
  if (now > last_reset_)
    return cumulated_bytes_ / (now - last_reset_);
  else
    return 0;
}

} // namespace plugin
} // namespace simgrid

using simgrid::plugin::LinkLoad;

/* **************************** events  callback *************************** */
static void on_communication(const simgrid::kernel::activity::CommImpl& comm)
{
  for (const auto* link : comm.get_traversed_links()) {
    if (link != nullptr && link->get_sharing_policy() != simgrid::s4u::Link::SharingPolicy::WIFI) {
      auto* link_load = link->extension<LinkLoad>();
      XBT_DEBUG("Update %s on Comm Start/End", link->get_cname());
      if (link_load->is_tracked())
        link_load->update();
    }
  }
}

/* **************************** Public interface *************************** */

/**
 * @ingroup plugin_link_load
 * @brief Initialize the link cumulated load plugin.
 * @pre The energy plugin should NOT be initialized.
 */
void sg_link_load_plugin_init()
{
  xbt_assert(simgrid::s4u::Engine::get_instance()->get_host_count() == 0,
             "Please call sg_link_load_plugin_init() BEFORE initializing the platform.");
  xbt_assert(not LinkLoad::EXTENSION_ID.valid(), "Double call to sg_link_load_plugin_init. Aborting.");
  LinkLoad::EXTENSION_ID = simgrid::s4u::Link::extension_create<LinkLoad>();

  // Attach new LinkLoad links created in the future.
  simgrid::s4u::Link::on_creation_cb([](simgrid::s4u::Link& link) {
    if (link.get_sharing_policy() != simgrid::s4u::Link::SharingPolicy::WIFI) {
      XBT_DEBUG("Wired link '%s' created. Attaching a LinkLoad to it.", link.get_cname());
      link.extension_set(new LinkLoad(&link));
    } else {
      XBT_DEBUG("Wireless link '%s' created. NOT attaching any LinkLoad to it.", link.get_cname());
    }
  });

  // Call this plugin on some of the links' events.
  simgrid::kernel::activity::CommImpl::on_start.connect(&on_communication);
  simgrid::kernel::activity::CommImpl::on_completion.connect(&on_communication);

  simgrid::s4u::Link::on_state_change_cb([](simgrid::s4u::Link const& link) {
    if (link.get_sharing_policy() != simgrid::s4u::Link::SharingPolicy::WIFI) {
      auto link_load = link.extension<LinkLoad>();
      if (link_load->is_tracked())
        link_load->update();
    }
  });
  simgrid::s4u::Link::on_communication_state_change_cb([](simgrid::kernel::resource::NetworkAction const& action,
                                                          simgrid::kernel::resource::Action::State /* previous */) {
    for (auto const* link : action.get_links()) {
      if (link != nullptr && link->get_sharing_policy() != simgrid::s4u::Link::SharingPolicy::WIFI) {
        auto link_load = link->get_iface()->extension<LinkLoad>();
        if (link_load->is_tracked())
          link_load->update();
      }
    }
  });
}

/**
 * @ingroup plugin_link_load
 * @brief Start the tracking of a link.
 * @details This is required so the link cumulated load can be obtained later on.
 * @pre The energy plugin should be initialized (cf. sg_link_load_plugin_init()).
 * @pre The link should be in "untracked" state. In other words, do not call this function twice on the same link
 * without a sg_link_load_untrack() call between them.
 *
 * @param link The link to track.
 */
void sg_link_load_track(const_sg_link_t link)
{
  xbt_assert(LinkLoad::EXTENSION_ID.valid(),
             "Please call sg_link_load_plugin_init before sg_link_load_track. Aborting.");
  link->extension<LinkLoad>()->track();
}

/**
 * @ingroup plugin_link_load
 * @brief Stop the tracking of a link.
 * @details Once the tracking is stopped, the cumulated load of the link can no longer be obtained until
 * sg_link_load_track() is called again on this link.
 * @pre The energy plugin should be initialized (cf. sg_link_load_plugin_init()).
 * @pre The link should be in "tracked" state. In other words, do not call this function twice on the same link without
 * a sg_link_load_track() call between them.
 *
 * @param link The link to untrack.
 */
void sg_link_load_untrack(const_sg_link_t link)
{
  xbt_assert(LinkLoad::EXTENSION_ID.valid(),
             "Please call sg_link_load_plugin_init before sg_link_load_untrack. Aborting.");
  link->extension<LinkLoad>()->untrack();
}

/**
 * @ingroup plugin_link_load
 * @brief Resets the cumulated load counters of a link.
 * @pre The energy plugin should be initialized (cf. sg_link_load_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_load_track()).
 *
 * @param link The link whose counters should be reset.
 */
void sg_link_load_reset(const_sg_link_t link)
{
  xbt_assert(LinkLoad::EXTENSION_ID.valid(),
             "Please call sg_link_load_plugin_init before sg_link_load_reset. Aborting.");
  link->extension<LinkLoad>()->reset();
}

/**
 * @ingroup plugin_link_load
 * @brief Get the cumulated load of a link (since the last call to sg_link_load_reset()).
 * @pre The energy plugin should be initialized (cf. sg_link_load_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_load_track()).

 * @param link The link whose cumulated load is requested.
 * @return The load (in bytes) that passed through the given link since the last call to sg_link_load_reset.
 */
double sg_link_get_cum_load(const_sg_link_t link)
{
  xbt_assert(LinkLoad::EXTENSION_ID.valid(),
             "Please call sg_link_load_plugin_init before sg_link_get_cum_load. Aborting.");
  return link->extension<LinkLoad>()->get_cumulated_bytes();
}

/**
 * @ingroup plugin_link_load
 * @brief Get the average load of a link (since the last call to sg_link_load_reset()).
 * @pre The energy plugin should be initialized (cf. sg_link_load_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_load_track()).

 * @param link The link whose average load is requested.
 * @return The average load (in bytes) that passed through the given link since the last call to sg_link_load_reset.
 */
double sg_link_get_avg_load(const_sg_link_t link)
{
  xbt_assert(LinkLoad::EXTENSION_ID.valid(),
             "Please call sg_link_load_plugin_init before sg_link_get_avg_load. Aborting.");
  return link->extension<LinkLoad>()->get_average_bytes();
}

/**
 * @ingroup plugin_link_load
 * @brief Get the minimum instantaneous load of a link (since the last call to sg_link_load_reset()).
 * @pre The energy plugin should be initialized (cf. sg_link_load_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_load_track()).

 * @param link The link whose average load is requested.
 * @return The minimum load instantaneous load (in bytes per second) that passed through the given link since the last
 call to sg_link_load_reset.
 */
double sg_link_get_min_instantaneous_load(const_sg_link_t link)
{
  xbt_assert(LinkLoad::EXTENSION_ID.valid(),
             "Please call sg_link_load_plugin_init before sg_link_get_min_load. Aborting.");
  return link->extension<LinkLoad>()->get_min_bytes_per_second();
}

/**
 * @ingroup plugin_link_load
 * @brief Get the maximum instantaneous load of a link (since the last call to sg_link_load_reset()).
 * @pre The energy plugin should be initialized (cf. sg_link_load_plugin_init()).
 * @pre The link should be in "tracked" state (cf. sg_link_load_track()).

 * @param link The link whose average load is requested.
 * @return The maximum load instantaneous load (in bytes per second) that passed through the given link since the last
 call to sg_link_load_reset.
 */
double sg_link_get_max_instantaneous_load(const_sg_link_t link)
{
  xbt_assert(LinkLoad::EXTENSION_ID.valid(),
             "Please call sg_link_load_plugin_init before sg_link_get_max_load. Aborting.");
  return link->extension<LinkLoad>()->get_max_bytes_per_second();
}
