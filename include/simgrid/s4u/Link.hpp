/* Copyright (c) 2004-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef S4U_LINK_HPP
#define S4U_LINK_HPP

#include <simgrid/forward.h>
#include <simgrid/kernel/resource/Action.hpp>
#include <simgrid/link.h>
#include <string>
#include <unordered_map>
#include <xbt/Extendable.hpp>
#include <xbt/base.h>
#include <xbt/signal.hpp>

/***********
 * Classes *
 ***********/

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::Link>;

namespace s4u {
/**
 * @beginrst
 * A Link represents the network facilities between :cpp:class:`hosts <simgrid::s4u::Host>`.
 * @endrst
 */
class XBT_PUBLIC Link : public xbt::Extendable<Link> {

#ifndef DOXYGEN
  friend kernel::resource::StandardLinkImpl;
#endif

protected:
  // Links are created from the NetZone, and destroyed by their private implementation when the simulation ends
  explicit Link(kernel::resource::LinkImpl* pimpl) : pimpl_(pimpl) {}
  virtual ~Link() = default;
  // The implementation that never changes
  kernel::resource::LinkImpl* const pimpl_;
#ifndef DOXYGEN
  friend kernel::resource::NetworkAction; // signal on_communication_state_change
#endif

public:
  /** Specifies how a given link is shared between concurrent communications */
  enum class SharingPolicy {
    /// This policy takes a callback that specifies the maximal capacity as a function of the number of usage. See the
    /// examples with 'degradation' in their name.
    NONLINEAR = 4,
    /// Pseudo-sharing policy requesting wifi-specific sharing.
    WIFI = 3,
    /// Each link is split in 2, UP and DOWN, one per direction. These links are SHARED.
    SPLITDUPLEX = 2,
    /// The bandwidth is shared between all comms using that link, regardless of their direction.
    SHARED = 1,
    /// Each comm can use the link fully, with no sharing (only a maximum). This is intended to represent the backbone
    /// links that cannot be saturated by concurrent links, but have a maximal bandwidth.
    FATPIPE = 0
  };

  kernel::resource::StandardLinkImpl* get_impl() const;

  /** \static @brief Retrieve a link from its name */
  static Link* by_name(const std::string& name);
  static Link* by_name_or_null(const std::string& name);

  /** @brief Returns the networking zone englobing that host */
  NetZone* get_englobing_zone() const;

  /** @brief Retrieves the name of that link as a C++ string */
  const std::string& get_name() const;
  /** @brief Retrieves the name of that link as a C string */
  const char* get_cname() const;

  /** Get/Set the bandwidth of the current Link (in bytes per second) */
  double get_bandwidth() const;
  Link* set_bandwidth(double value);

  /** Get/Set the latency of the current Link (in seconds) */
  double get_latency() const;
  /**
   * @brief Set link's latency
   *
   * @param value New latency value (in s)
   */
  Link* set_latency(double value);
  /**
   * @brief Set latency (string version)
   *
   * Accepts values with units, such as '1s' or '7ms'.
   *
   * Full list of accepted units: w (week), d (day), h, s, ms, us, ns, ps.
   *
   * @throw std::invalid_argument if latency format is incorrect.
   */
  Link* set_latency(const std::string& value);

  /** @brief Describes how the link is shared between flows
   *
   *  Note that the NONLINEAR callback is in the critical path of the solver, so it should be fast.
   */
  Link* set_sharing_policy(SharingPolicy policy, const NonLinearResourceCb& cb = {});
  SharingPolicy get_sharing_policy() const;

  /** Setup the profile with states events (ON or OFF). The profile must contain boolean values. */
  Link* set_state_profile(kernel::profile::Profile* profile);
  /** Setup the profile with bandwidth events (peak speed changes due to external load).
   * The profile must contain percentages (value between 0 and 1). */
  Link* set_bandwidth_profile(kernel::profile::Profile* profile);
  /** Setup the profile file with latency events (peak latency changes due to external load).
   * The profile must contain absolute values */
  Link* set_latency_profile(kernel::profile::Profile* profile);

  const std::unordered_map<std::string, std::string>* get_properties() const;
  const char* get_property(const std::string& key) const;
  Link* set_properties(const std::unordered_map<std::string, std::string>& properties);
  Link* set_property(const std::string& key, const std::string& value);

  /**
   * @brief Set the number of communications that can shared this link at the same time
   *
   * Use this method to serialize communication flows going through this link.
   * Use -1 to set no limit.
   *
   * @param limit  Number of concurrent flows
   */
  Link* set_concurrency_limit(int limit);
  int get_concurrency_limit() const;

  /** @brief Set the level of communication speed of the given host on this wifi link.
   *
   * The bandwidth of a wifi link for a given host depends on its SNR (signal to noise ratio),
   * which ultimately depends on the distance between the host and the station and the material between them.
   *
   * This is modeled in SimGrid by providing several bandwidths to wifi links, one per SNR level (just provide
   * comma-separated values in the XML file). By default, the first level in the list is used, but you can use the
   * current function to specify that a given host uses another level of bandwidth. This can be used to take the
   * location of hosts into account, or even to model mobility in your SimGrid simulation.
   *
   * Note that this function asserts that the link is actually a wifi link
   *
   * warning: in the case where a 0Mbps data rate should be used, set that rate only once during the
   * experiment, and don't modify the bandwidth of that host later */
  void set_host_wifi_rate(const s4u::Host* host, int level) const;

  /** @brief Returns the current load (in bytes per second) */
  double get_load() const;

  /** @brief Check if the Link is used (at least one flow uses the link) */
  bool is_used() const;

  /** @brief Check if the Link is shared (not a FATPIPE) */
  bool is_shared() const;

  /** Turns the link on. */
  void turn_on();
  /** Turns the link off. */
  void turn_off();
  /** Checks whether the link is on. */
  bool is_on() const;

  Link* seal();

private:
#ifndef DOXYGEN
  static xbt::signal<void(Link&)> on_creation;
  static xbt::signal<void(Link const&)> on_onoff;
  xbt::signal<void(Link const&)> on_this_onoff;
  static xbt::signal<void(Link const&)> on_bandwidth_change;
  xbt::signal<void(Link const&)> on_this_bandwidth_change;
  static xbt::signal<void(kernel::resource::NetworkAction&, kernel::resource::Action::State)>
      on_communication_state_change;
  static xbt::signal<void(Link const&)> on_destruction;
  xbt::signal<void(Link const&)> on_this_destruction;
#endif

public:
  /* The signals */
  /** \static @brief Add a callback fired when a new Link is created */
  static void on_creation_cb(const std::function<void(Link&)>& cb) { on_creation.connect(cb); }
  /** \static @brief Add a callback fired when any Link is turned on or off */
  static void on_onoff_cb(const std::function<void(Link const&)>& cb)
  {
    on_onoff.connect(cb);
  }
  /** @brief Add a callback fired when this specific Link is turned on or off */
  void on_this_onoff_cb(const std::function<void(Link const&)>& cb)
  {
    on_this_onoff.connect(cb);
  }
  /** \static @brief Add a callback fired when the bandwidth of any Link changes */
  static void on_bandwidth_change_cb(const std::function<void(Link const&)>& cb) { on_bandwidth_change.connect(cb); }
  /** @brief Add a callback fired when the bandwidth of this specific Link changes */
  void on_this_bandwidth_change_cb(const std::function<void(Link const&)>& cb)
  {
    on_this_bandwidth_change.connect(cb);
  }
  /** \static @brief Add a callback fired when a communication changes it state (ready/done/cancel) */
  static void on_communication_state_change_cb(
      const std::function<void(kernel::resource::NetworkAction&, kernel::resource::Action::State)>& cb)
  {
    on_communication_state_change.connect(cb);
  }
  /** \static @brief Add a callback fired when any Link is destroyed */
  static void on_destruction_cb(const std::function<void(Link const&)>& cb) { on_destruction.connect(cb); }
  /** @brief Add a callback fired when this specific Link is destroyed */
  void on_this_destruction_cb(const std::function<void(Link const&)>& cb) { on_this_destruction.connect(cb); }
};

/**
 * @beginrst
 * A SplitDuplexLink encapsulates the :cpp:class:`links <simgrid::s4u::Link>` which
 * compose a Split-Duplex link. Remember that a Split-Duplex link is nothing more than
 * a pair of up/down links.
 * @endrst
 */
class XBT_PUBLIC SplitDuplexLink : public Link {
public:
  explicit SplitDuplexLink(kernel::resource::LinkImpl* pimpl) : Link(pimpl) {}
  /** @brief Get the link direction up*/
  Link* get_link_up() const;
  /** @brief Get the link direction down */
  Link* get_link_down() const;

  /** \static @brief Retrieve a link from its name */
  static SplitDuplexLink* by_name(const std::string& name);
};

/**
 * @beginrst
 * Another encapsulation for using links in the :cpp:func:`NetZone::add_route`
 *
 * When adding a route with split-duplex links, you need to specify the direction of the link
 * so SimGrid can know exactly which physical link to insert in the route.
 *
 * For shared/fat-pipe links, use the :cpp:enumerator:`Direction::NONE` since they don't have
 * the concept of UP/DOWN links.
 * @endrst
 */
class XBT_PUBLIC LinkInRoute {
public:
  enum class Direction { UP = 2, DOWN = 1, NONE = 0 };

  explicit LinkInRoute(const Link* link) : link_(link) {}
  LinkInRoute(const Link* link, Direction d) : link_(link), direction_(d) {}

  /** @brief Get direction of this link in the route: UP or DOWN */
  Direction get_direction() const { return direction_; }
  /** @brief Get pointer to the link */
  const Link* get_link() const { return link_; }

private:
  const Link* link_;
  Direction direction_ = Direction::NONE;
};

} // namespace s4u
} // namespace simgrid

#endif /* S4U_LINK_HPP */
