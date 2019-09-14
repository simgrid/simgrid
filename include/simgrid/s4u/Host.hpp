/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

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

namespace xbt {
extern template class XBT_PUBLIC Extendable<s4u::Host>;
} // namespace xbt

namespace s4u {
/** @ingroup s4u_api
 *
 * @tableofcontents
 *
 * Some physical resource with computing and networking capabilities on which Actors execute.
 *
 * All hosts are automatically created during the call of the method
 * @ref simgrid::s4u::Engine::load_platform().
 * You cannot create a host yourself.
 *
 * You can retrieve a particular host using @ref simgrid::s4u::Host::by_name()
 * and actors can retrieve the host on which they run using @ref simgrid::s4u::Host::current() or
 * @ref simgrid::s4u::this_actor::get_host().
 */
class XBT_PUBLIC Host : public xbt::Extendable<Host> {
  friend vm::VMModel;            // Use the pimpl_cpu to compute the VM sharing
  friend vm::VirtualMachineImpl; // creates the the pimpl_cpu

public:
  explicit Host(const std::string& name);

  /** Host destruction logic */
protected:
  virtual ~Host();

private:
  bool currently_destroying_ = false;

public:
  /*** Called on each newly created host */
  static xbt::signal<void(Host&)> on_creation;
  /*** Called just before destructing a host */
  static xbt::signal<void(Host const&)> on_destruction;
  /*** Called when the machine is turned on or off (called AFTER the change) */
  static xbt::signal<void(Host const&)> on_state_change;
  /*** Called when the speed of the machine is changed (called AFTER the change)
   * (either because of a pstate switch or because of an external load event coming from the profile) */
  static xbt::signal<void(Host const&)> on_speed_change;

  virtual void destroy();
  // No copy/move
  Host(Host const&) = delete;
  Host& operator=(Host const&) = delete;

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

  int get_actor_count();
  std::vector<ActorPtr> get_all_actors();

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
  /** Returns if that host is currently down and offline */
  XBT_ATTRIB_DEPRECATED_v325("Please use !is_on()") bool is_off() const { return not is_on(); }

  const char* get_property(const std::string& key) const;
  void set_property(const std::string& key, const std::string& value);
  const std::unordered_map<std::string, std::string>* get_properties() const;
  void set_properties(const std::map<std::string, std::string>& properties);

  void set_state_profile(kernel::profile::Profile* p);
  void set_speed_profile(kernel::profile::Profile* p);

  double get_speed() const;
  double get_available_speed() const;
  int get_core_count() const;
  double get_load() const;

  double get_pstate_speed(int pstate_index) const;
  int get_pstate_count() const;
  void set_pstate(int pstate_index);
  int get_pstate() const;

  std::vector<const char*> get_attached_storages() const;

  /** Get an associative list [mount point]->[Storage] of all local mount points.
   *
   *  This is defined in the platform file, and cannot be modified programatically (yet).
   */
  std::unordered_map<std::string, Storage*> const& get_mounted_storages();

  void route_to(Host* dest, std::vector<Link*>& links, double* latency);
  void route_to(Host* dest, std::vector<kernel::resource::LinkImpl*>& links, double* latency);
  void send_to(Host* dest, double byte_amount);

  NetZone* get_englobing_zone();
  /** Block the calling actor on an execution located on the called host
   *
   * It is not a problem if the actor is not located on the called host.
   * The actor will not be migrated in this case. Such remote execution are easy in simulation.
   */
  void execute(double flops);
  /** Start an asynchronous computation on that host (possibly remote) */
  ExecPtr exec_async(double flops_amounts);

  /** Block the calling actor on an execution located on the called host (with explicit priority) */
  void execute(double flops, double priority);

private:
  xbt::string name_{"noname"};
  std::unordered_map<std::string, Storage*>* mounts_ = nullptr; // caching

public:
#ifndef DOXYGEN
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  kernel::resource::Cpu* pimpl_cpu = nullptr;
  // TODO, this could be a unique_ptr
  surf::HostImpl* pimpl_ = nullptr;
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  kernel::routing::NetPoint* pimpl_netpoint = nullptr;
#endif
};
} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_HOST_HPP */
