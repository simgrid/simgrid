/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

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
extern template class XBT_PUBLIC Extendable<simgrid::s4u::Host>;
}
namespace s4u {

/** @ingroup s4u_api
 *
 * @tableofcontents
 *
 * An host represents some physical resource with computing and networking capabilities.
 *
 * All hosts are automatically created during the call of the method
 * @ref simgrid::s4u::Engine::loadPlatform().
 * You cannot create a host yourself.
 *
 * You can retrieve a particular host using simgrid::s4u::Host::byName()
 * and actors can retrieve the host on which they run using simgrid::s4u::Host::current().
 */
class XBT_PUBLIC Host : public simgrid::xbt::Extendable<Host> {
  friend simgrid::vm::VMModel;            // Use the pimpl_cpu to compute the VM sharing
  friend simgrid::vm::VirtualMachineImpl; // creates the the pimpl_cpu

public:
  explicit Host(const char* name);

  /** Host destruction logic */
protected:
  virtual ~Host();

private:
  bool currentlyDestroying_ = false;

public:
  /*** Called on each newly created host */
  static simgrid::xbt::signal<void(Host&)> on_creation;
  /*** Called just before destructing an host */
  static simgrid::xbt::signal<void(Host&)> on_destruction;
  /*** Called when the machine is turned on or off (called AFTER the change) */
  static simgrid::xbt::signal<void(Host&)> on_state_change;
  /*** Called when the speed of the machine is changed (called AFTER the change)
   * (either because of a pstate switch or because of an external load event coming from the profile) */
  static simgrid::xbt::signal<void(Host&)> on_speed_change;

  virtual void destroy();
  // No copy/move
  Host(Host const&) = delete;
  Host& operator=(Host const&) = delete;

  /** Retrieves an host from its name, or return nullptr */
  static Host* by_name_or_null(const char* name);
  /** Retrieves an host from its name, or return nullptr */
  static Host* by_name_or_null(std::string name);
  /** Retrieves an host from its name, or die */
  static s4u::Host* by_name(const char* name);
  /** Retrieves an host from its name, or die */
  static s4u::Host* by_name(std::string name);
  /** Retrieves the host on which the current actor is running */
  static s4u::Host* current();

  /** Retrieves the name of that host as a C++ string */
  simgrid::xbt::string const& get_name() const { return name_; }
  /** Retrieves the name of that host as a C string */
  const char* get_cname() const { return name_.c_str(); }

  int get_actor_count();
  std::vector<ActorPtr> get_all_actors();

  /** Turns that host on if it was previously off
   *
   * All actors on that host which were marked autorestart will be restarted automatically.
   * This call does nothing if the host is already on.
   */
  void turn_on();
  /** Turns that host off. All actors are forcefully stopped. */
  void turn_off();
  /** Returns if that host is currently up and running */
  bool is_on();
  /** Returns if that host is currently down and offline */
  bool is_off() { return not is_on(); }

  const char* get_property(const char* key);
  void set_property(std::string key, std::string value);
  std::map<std::string, std::string>* getProperties();

  double getSpeed();
  double get_available_speed();
  int getCoreCount();

  double getPstateSpeed(int pstate_index);
  int getPstatesCount() const;
  void setPstate(int pstate_index);
  int getPstate();

  std::vector<const char*> get_attached_storages();
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::get_attached_storages() instead.") void getAttachedStorages(
      std::vector<const char*>* storages);

  /** Get an associative list [mount point]->[Storage] of all local mount points.
   *
   *  This is defined in the platform file, and cannot be modified programatically (yet).
   */
  std::unordered_map<std::string, Storage*> const& getMountedStorages();

  void routeTo(Host* dest, std::vector<Link*>& links, double* latency);
  void routeTo(Host* dest, std::vector<kernel::resource::LinkImpl*>& links, double* latency);

  /** Block the calling actor on an execution located on the called host
   *
   * It is not a problem if the actor is not located on the called host.
   * The actor will not be migrated in this case. Such remote execution are easy in simulation.
   */
  void execute(double flops);
  /** Block the calling actor on an execution located on the called host (with explicit priority) */
  void execute(double flops, double priority);

  /** @brief Returns the current computation load (in flops per second)
   * The external load (coming from an availability trace) is not taken in account.
   *
   * @return      The number of activities currently running on a host (an activity at priority 2 is counted twice).
   */
  double getLoad();

  // Deprecated functions
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::get_name()") simgrid::xbt::string const& getName() const
  {
    return name_;
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::get_cname()") const char* getCname() const { return name_.c_str(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::get_all_actors()") void actorList(std::vector<ActorPtr>* whereto);
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::get_all_actors()") void getProcesses(std::vector<ActorPtr>* list);
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::turn_on()") void turnOn() { turn_on(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::turn_off()") void turnOff() { turn_off(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::is_on()") bool isOn() { return is_on(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::is_off()") bool isOff() { return is_off(); }

  XBT_ATTRIB_DEPRECATED_v323("Please use Host::get_property()") const char* getProperty(const char* key)
  {
    return get_property(key);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Host::set_property()") void setProperty(std::string key, std::string value)
  {
    set_property(key, value);
  }

private:
  simgrid::xbt::string name_{"noname"};
  std::unordered_map<std::string, Storage*>* mounts_ = nullptr; // caching

public:
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  surf::Cpu* pimpl_cpu = nullptr;
  // TODO, this could be a unique_ptr
  surf::HostImpl* pimpl_ = nullptr;
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  kernel::routing::NetPoint* pimpl_netpoint = nullptr;
};
}
} // namespace simgrid::s4u

extern int USER_HOST_LEVEL;

#endif /* SIMGRID_S4U_HOST_HPP */
