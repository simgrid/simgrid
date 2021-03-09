/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_HOST_HPP
#define SIMGRID_S4U_HOST_HPP

#include <simgrid/forward.h>
#include <xbt/Extendable.hpp>
#include <xbt/signal.hpp>
#include <xbt/string.hpp>

#include <map>
#include <unordered_map>

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::Host>;

namespace s4u {
/** @ingroup s4u_api
 *
 * @tableofcontents
 *
 * Some physical resource with computing and networking capabilities on which Actors execute.
 *
 * @beginrst
 * All hosts are automatically created during the call of the method
 * :cpp:func:`simgrid::s4u::Engine::load_platform()`.
 * You cannot create a host yourself.
 *
 * You can retrieve a particular host using :cpp:func:`simgrid::s4u::Host::by_name()`
 * and actors can retrieve the host on which they run using :cpp:func:`simgrid::s4u::Host::current()` or
 * :cpp:func:`simgrid::s4u::this_actor::get_host()`
 * @endrst
 */
class XBT_PUBLIC Host : public xbt::Extendable<Host> {
#ifndef DOXYGEN
  friend vm::VMModel;            // Use the pimpl_cpu to compute the VM sharing
  friend vm::VirtualMachineImpl; // creates the the pimpl_cpu
  friend kernel::routing::NetZoneImpl;

public:
  explicit Host(const std::string& name);

protected:
  virtual ~Host(); // Call destroy() instead of manually deleting it.
  Host* set_netpoint(kernel::routing::NetPoint* netpoint);
#endif

public:
  /** Called on each newly created host */
  static xbt::signal<void(Host&)> on_creation;
  /** Called just before destructing a host */
  static xbt::signal<void(Host const&)> on_destruction;
  /** Called when the machine is turned on or off (called AFTER the change) */
  static xbt::signal<void(Host const&)> on_state_change;
  /** Called when the speed of the machine is changed (called AFTER the change)
   * (either because of a pstate switch or because of an external load event coming from the profile) */
  static xbt::signal<void(Host const&)> on_speed_change;

  virtual void destroy();
#ifndef DOXYGEN
  // No copy/move
  Host(Host const&) = delete;
  Host& operator=(Host const&) = delete;
#endif

  /** Retrieve a host from its name, or return nullptr */
  static Host* by_name_or_null(const std::string& name);
  /** Retrieve a host from its name, or die */
  static Host* by_name(const std::string& name);
  /** Retrieves the host on which the running actor is located */
  static Host* current();

  /** Retrieves the name of that host as a C++ string */
  xbt::string const& get_name() const { return name_; }
  /** Retrieves the name of that host as a C string */
  const char* get_cname() const { return name_.c_str(); }

  kernel::routing::NetPoint* get_netpoint() const { return pimpl_netpoint_; }

  int get_actor_count() const;
  std::vector<ActorPtr> get_all_actors() const;

  /** Turns that host on if it was previously off
   *
   * This call does nothing if the host is already on. If it was off, all actors which were marked 'autorestart' on that
   * host will be restarted automatically (note that this may differ from the actors that were initially running on the
   * host).
   *
   * All other Host's properties are left unchanged; in particular, the pstate is left unchanged and not reset to its
   * initial value.
   */
  void turn_on();
  /** Turns that host off. All actors are forcefully stopped. */
  void turn_off();
  /** Returns if that host is currently up and running */
  bool is_on() const;

  const char* get_property(const std::string& key) const;
  Host* set_property(const std::string& key, const std::string& value);
  const std::unordered_map<std::string, std::string>* get_properties() const;
  Host* set_properties(const std::unordered_map<std::string, std::string>& properties);

  void set_state_profile(kernel::profile::Profile* p);
  void set_speed_profile(kernel::profile::Profile* p);

  /** @brief Get the peak computing speed in flops/s at the current pstate, NOT taking the external load into account.
   *
   *  The amount of flops per second available for computing depends on several things:
   *    - The current pstate determines the maximal peak computing speed (use @ref get_pstate_speed() to retrieve the
   *      computing speed you would get at another pstate)
   *    - If you declared an external load (with @ref simgrid::surf::Cpu::set_speed_profile()), you must multiply the
   * result of get_speed() by get_available_speed() to retrieve what a new computation would get.
   *
   *  The remaining speed is then shared between the executions located on this host.
   *  You can retrieve the amount of tasks currently running on this host with @ref get_load().
   *
   *  The host may have multiple cores, and your executions may be able to use more than a single core.
   *
   *  Finally, executions of priority 2 get twice the amount of flops than executions of priority 1.
   */
  double get_speed() const;
  /** @brief Get the available speed ratio, between 0 and 1.
   *
   * This accounts for external load (see @ref simgrid::surf::Cpu::set_speed_profile()).
   */
  double get_available_speed() const;

  /** Returns the number of core of the processor. */
  int get_core_count() const;
  Host* set_core_count(int core_count);

  /** Returns the current computation load (in flops per second)
   *
   * The external load (coming from an availability trace) is not taken in account.
   * You may also be interested in the load plugin.
   */
  double get_load() const;

  double get_pstate_speed(int pstate_index) const;
  int get_pstate_count() const;
  void set_pstate(int pstate_index);
  int get_pstate() const;

  std::vector<Disk*> get_disks() const;
  Disk* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth);
  void add_disk(const Disk* disk);
  void remove_disk(const std::string& disk_name);

  void route_to(const Host* dest, std::vector<Link*>& links, double* latency) const;
  void route_to(const Host* dest, std::vector<kernel::resource::LinkImpl*>& links, double* latency) const;

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v331("Please use Comm::sendto()") void sendto(Host* dest, double byte_amount);

  XBT_ATTRIB_DEPRECATED_v331("Please use Comm::sendto_async()") CommPtr sendto_async(Host* dest, double byte_amount);

  XBT_ATTRIB_DEPRECATED_v330("Please use Host::sendto()") void send_to(Host* dest, double byte_amount);
#endif

  NetZone* get_englobing_zone();
  /** Block the calling actor on an execution located on the called host
   *
   * It is not a problem if the actor is not located on the called host.
   * The actor will not be migrated in this case. Such remote execution are easy in simulation.
   */
  void execute(double flops) const;
  /** Start an asynchronous computation on that host (possibly remote) */
  ExecPtr exec_init(double flops_amounts) const;
  ExecPtr exec_async(double flops_amounts) const;

  /** Block the calling actor on an execution located on the called host (with explicit priority) */
  void execute(double flops, double priority) const;

private:
  xbt::string name_{"noname"};
  kernel::routing::NetPoint* pimpl_netpoint_         = nullptr;

public:
#ifndef DOXYGEN
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  kernel::resource::Cpu* pimpl_cpu = nullptr;
  // TODO, this could be a unique_ptr
  surf::HostImpl* pimpl_ = nullptr;
#endif
};
} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_HOST_HPP */
