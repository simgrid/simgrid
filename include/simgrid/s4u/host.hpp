/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_HOST_HPP
#define SIMGRID_S4U_HOST_HPP

#include <boost/unordered_map.hpp>
#include <vector>

#include "xbt/base.h"
#include "simgrid/simix.h"

namespace simgrid {
namespace s4u {

class Actor;
class Storage;
class File;

/** @brief Simulated machine that can host some actors
 *
 * It represents some physical resource with computing and networking capabilities.
 *
 * All hosts are automatically created during the call of the method
 * @link{simgrid::s4u::Engine::loadPlatform()}.
 * You cannot create a host yourself.
 *
 * You can retrieve a particular host using @link{simgrid::s4u::Host.byName()},
 * and actors can retrieve the host on which they run using @link{simgrid::s4u::Host.current()}.
 */
XBT_PUBLIC_CLASS Host {
	friend Actor;
	friend File;
private:
	Host(const char *name);
protected:
	~Host();
public:
	/** Retrieves an host from its name. */
	static s4u::Host *byName(std::string name);
	/** Retrieves the host on which the current actor is running */
	static s4u::Host *current();

	const char* name();

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


	/** Allows to store user data on that host */
	void set_userdata(void *data) {p_userdata = data;}
	/** Retrieves the previously stored data */
	void* userdata() {return p_userdata;}

	/** Get an associative list [mount point]->[Storage] off all local mount points.
	 *
	 *	This is defined in the platform file, and cannot be modified programatically (yet).
	 *
	 *	Do not change the returned value in any way.
	 */
	boost::unordered_map<std::string, Storage&> &mountedStorages();
private:
	boost::unordered_map<std::string, Storage&> *mounts = NULL; // caching

protected:
	sg_host_t inferior() {return p_inferior;}
private:
	void*p_userdata=NULL;
	sg_host_t p_inferior;
	static boost::unordered_map<std::string, s4u::Host *> *hosts;
};

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_HOST_HPP */

#if 0
/* Bindings to the MSG hosts */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

import org.simgrid.msg.Storage;

/*
Host jacquelin;

try { 
	jacquelin = Host.getByName("Jacquelin");
} catch(HostNotFoundException e) {
	System.err.println(e.toString());
}
...
\endverbatim
 *
 */ 
public class Host {
	/**
	 * This static method returns all of the hosts of the installed platform.
	 *
	 * @return			An array containing all the hosts installed.
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
	 * The external load (comming from an availability trace) is not taken in account.
	 *
	 * @return			The number of tasks currently running on a host.
	 */ 
	public native int getLoad();

	/**
	 * This method returns the speed of the processor of a host,
	 * regardless of the current load of the machine.
	 *
	 * @return			The speed of the processor of the host in flops.
	 *
	 */ 
	public native double getSpeed();

	/**
	 * This method returns the number of core of a host.
	 *
	 * @return			The speed of the processor of the host in flops.
	 *
	 */ 
	public native double getCoreNumber();

	/**
	 * Returns the value of a given host property (set from the platform file).
	 */
	public native String getProperty(String name);

	/**
	 * Change the value of a given host property. 
	 */
	public native void setProperty(String name, String value);

	/** This methods returns the list of storages attached to an host
	 * @return An array containing all storages (name) attached to the host
	 */
	public native String[] getAttachedStorage();


} 
#endif
