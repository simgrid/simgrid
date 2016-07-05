/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_HOST_HPP
#define SIMGRID_S4U_HOST_HPP

#include <string>

#include <boost/unordered_map.hpp>

#include <xbt/base.h>
#include <xbt/string.hpp>
#include <xbt/signal.hpp>
#include <xbt/Extendable.hpp>

#include <simgrid/simix.h>
#include <simgrid/datatypes.h>
#include <simgrid/s4u/forward.hpp>

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
 * @link{simgrid::s4u::Engine::loadPlatform()}.
 * You cannot create a host yourself.
 *
 * You can retrieve a particular host using simgrid::s4u::Host::byName()
 * and actors can retrieve the host on which they run using simgrid::s4u::Host::current().
 */
XBT_PUBLIC_CLASS Host :
  public simgrid::xbt::Extendable<Host> {

private:
  explicit Host(const char *name);
public: // TODO, make me private
  ~Host();
  /** Do not use this function, it should be private */
  static Host* by_name_or_create(const char* name);

  /** Retrieves an host from its name, or return nullptr */
  static Host* by_name_or_null(const char* name);
  /** Retrieves an host from its name, or die */
  static s4u::Host *by_name(std::string name);
  /** Retrieves the host on which the current actor is running */
  static s4u::Host *current();

  simgrid::xbt::string const& name() const { return name_; }

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
  bool isOff() { return !isOn(); }

  double speed();
  int coresCount();
  xbt_dict_t properties();
  const char*property(const char*key);
  void setProperty(const char*key, const char *value);
  xbt_swag_t processes();
  double getPstateSpeedCurrent();
  double getPstateSpeed(int pstate_index);
  int pstatesCount() const;
  void setPstate(int pstate_index);
  int pstate();
  void parameters(vm_params_t params);
  void setParameters(vm_params_t params);
  xbt_dict_t mountedStoragesAsDict(); // HACK
  xbt_dynar_t attachedStorages();

  /** Get an associative list [mount point]->[Storage] of all local mount points.
   *
   *  This is defined in the platform file, and cannot be modified programatically (yet).
   */
  boost::unordered_map<std::string, Storage*> const &mountedStorages();

private:
  simgrid::xbt::string name_ = "noname";
  boost::unordered_map<std::string, Storage*> *mounts = nullptr; // caching

public:
  // TODO, this could be a unique_ptr
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  surf::Cpu     *pimpl_cpu = nullptr;
  /** DO NOT USE DIRECTLY (@todo: these should be protected, once our code is clean) */
  routing::NetCard *pimpl_netcard = nullptr;

public:
  /*** Called on each newly created object */
  static simgrid::xbt::signal<void(Host&)> onCreation;
  /*** Called just before destructing an object */
  static simgrid::xbt::signal<void(Host&)> onDestruction;
  /*** Called when the machine is turned on or off */
  static simgrid::xbt::signal<void(Host&)> onStateChange;
};

}} // namespace simgrid::s4u

extern int MSG_HOST_LEVEL;
extern int SIMIX_HOST_LEVEL;
extern int USER_HOST_LEVEL;

#endif /* SIMGRID_S4U_HOST_HPP */

#if 0
/* Bindings to the MSG hosts */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

public class Host {
  /**
   * This static method returns all of the hosts of the installed platform.
   *
   * @return      An array containing all the hosts installed.
   *
   */ 
  public native static Host[] all();

  /** 
   * This static method sets a mailbox to receive in asynchronous mode.
   * 
   * All messages sent to this mailbox will be transferred to 
   * the receiver without waiting for the receive call. 
   * The receive call will still be necessary to use the received data.
   * If there is a need to receive some messages asynchronously, and some not, 
   * two different mailboxes should be used.
   *
   * @param mailboxName The name of the mailbox
   */
  public static native void setAsyncMailbox(String mailboxName);

  /**
   * This method returns the number of tasks currently running on a host.
   * The external load (coming from an availability trace) is not taken in account.
   *
   * @return      The number of tasks currently running on a host.
   */ 
  public native int getLoad();


  /** This methods returns the list of storages attached to an host
   * @return An array containing all storages (name) attached to the host
   */
  public native String[] getAttachedStorage();


} 
#endif
