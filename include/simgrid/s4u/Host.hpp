/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_HOST_HPP
#define SIMGRID_S4U_HOST_HPP

#include <map>
#include <string>
#include <unordered_map>

#include "xbt/Extendable.hpp"
#include "xbt/signal.hpp"
#include "xbt/string.hpp"

#include "simgrid/forward.h"
#include "simgrid/s4u/forward.hpp"

namespace simgrid {

namespace xbt {
extern template class XBT_PUBLIC() Extendable<simgrid::s4u::Host>;
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
XBT_PUBLIC_CLASS Host : public simgrid::xbt::Extendable<Host>
{

public:
  explicit Host(const char* name);

  /** Host destruction logic */
protected:
  virtual ~Host();

private:
  bool currentlyDestroying_ = false;

public:
  void destroy();
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
  simgrid::xbt::string const& getName() const { return name_; }
  /** Retrieves the name of that host as a C string */
  const char* getCname() const { return name_.c_str(); }

  void actorList(std::vector<ActorPtr> * whereto);

  /** Turns that host on if it was previously off
   *
   * All actors on that host which were marked autorestart will be restarted automatically.
   * This call does nothing if the host is already on.
   */
  void turnOn();
  /** Turns that host off. All actors are forcefully stopped. */
  void turnOff();
  /** Returns if that host is currently up and running */
  bool isOn();
  /** Returns if that host is currently down and offline */
  bool isOff() { return not isOn(); }

  double getSpeed();
  int getCoreCount();
  std::map<std::string, std::string>* getProperties();
  const char* getProperty(const char* key);
  void setProperty(std::string key, std::string value);
  void getProcesses(std::vector<ActorPtr> * list);
  double getPstateSpeed(int pstate_index);
  int getPstatesCount() const;
  void setPstate(int pstate_index);
  int getPstate();
  void getAttachedStorages(std::vector<const char*> * storages);

  /** Get an associative list [mount point]->[Storage] of all local mount points.
   *
   *  This is defined in the platform file, and cannot be modified programatically (yet).
   */
  std::unordered_map<std::string, Storage*> const& getMountedStorages();

  void routeTo(Host* dest, std::vector<Link*>& links, double* latency);
  void routeTo(Host* dest, std::vector<surf::LinkImpl*>& links, double* latency);

  /** Block the calling actor on an execution located on the called host
   *
   * It is not a problem if the actor is not located on the called host.
   * The actor will not be migrated in this case. Such remote execution are easy in simulation.
   */
  void execute(double flops);

  /** @brief Returns the current computation load (in flops per second) */
  double getLoad();

private:
  simgrid::xbt::string name_{"noname"};
  std::unordered_map<std::string, Storage*>* mounts = nullptr; // caching

public:
  // TODO, this could be a unique_ptr
  surf::HostImpl* pimpl_ = nullptr;
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  surf::Cpu* pimpl_cpu = nullptr;
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  kernel::routing::NetPoint* pimpl_netpoint = nullptr;

  /*** Called on each newly created host */
  static simgrid::xbt::signal<void(Host&)> onCreation;
  /*** Called just before destructing an host */
  static simgrid::xbt::signal<void(Host&)> onDestruction;
  /*** Called when the machine is turned on or off (called AFTER the change) */
  static simgrid::xbt::signal<void(Host&)> onStateChange;
  /*** Called when the speed of the machine is changed (called AFTER the change)
   * (either because of a pstate switch or because of an external load event coming from the profile) */
  static simgrid::xbt::signal<void(Host&)> onSpeedChange;
};
}
} // namespace simgrid::s4u

extern int USER_HOST_LEVEL;

#endif /* SIMGRID_S4U_HOST_HPP */

#if 0

public class Host {

  /**
   * This method returns the number of tasks currently running on a host.
   * The external load (coming from an availability trace) is not taken in account.
   *
   * @return      The number of tasks currently running on a host.
   */
  public native int getLoad();

}
#endif
