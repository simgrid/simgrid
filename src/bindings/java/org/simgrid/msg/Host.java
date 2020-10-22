/* Bindings to the MSG hosts */

/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/**
 * A host object represents a location (any possible place) where a process may run.
 * Thus it is represented as a physical resource with computing capabilities, some
 * mailboxes to enable running process to communicate with remote ones, and some private
 * data that can be only accessed by local process. An instance of this class is always
 * bound with the corresponding native host. All the native hosts are automatically created
 * during the call of the method Msg.createEnvironment(). This method take as parameter a
 * platform file which describes all elements of the platform (host, link, root..).
 * You cannot create a host yourself.
 *
 * The best way to get a host instance is to call the static method
 * Host.getByName().
 *
 * For example to get the instance of the host. If your platform
 * file description contains a host named "Jacquelin" :
 *
 * \verbatim
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
	 * This attribute represents a bind between a java host object and
	 * a native host. Even if this attribute is public you must never
	 * access to it. It is set automatically during the call of the
	 * static method Host.getByName().
	 *
	 * @see				Host.getByName().
	 */
	private long bind;
	protected String name;

	/** User data. */
	private Object data;
	protected Host() {
		this.bind = 0;
		this.data = null;
	}

	@Override
	public String toString (){
		return this.name;
	}

	/**
	 * This static method gets a host instance associated with a native
	 * host of your platform. This is the best way to get a java host object.
	 *
	 * @param name		The name of the host to get.
	 *
	 * @return		The host object with the given name.
	 * @exception		HostNotFoundException if the name of the host is not valid.
	 */
	public static native Host getByName(String name) throws HostNotFoundException;
	/** Counts the installed hosts. */
	public static native int getCount();

	/** Returns the host of the current process. */
	public static native Host currentHost();

	/** Returns all hosts of the installed platform. */
	public static native Host[] all();

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

	public String getName() {
		return name;
	}

	public void setData(Object data) {
		this.data = data;
	}

	public Object getData() {
		return this.data;
	}
	/** Returns true if the host has an associated data object. */
	public boolean hasData() {
		return null != this.data;
	}

	/** Starts the host if it is off */
	public native void on();
	/** Stops the host if it is on */
	public native void off() throws ProcessKilledError;

	/**
	 * This method returns the speed of the processor of a host (in flops),
	 * regardless of the current load of the machine.
	 */
	public native double getSpeed();
	public native double getCoreNumber();

	public native String getProperty(String name);
	public native void setProperty(String name, String value);
	/** Tests if a host is up and running. */
	public native boolean isOn();

	/** Returns the list of mount point names on a host */
	public native Storage[] getMountedStorage();
	/** This methods returns the list of storages (names) attached to a host */
	public native String[] getAttachedStorage();

	/** After this call, sg_host_get_consumed_energy() will not interrupt your process
	 * (until after the next clock update).
	 */
	public static native void updateAllEnergyConsumptions();
	/** Returns the amount of Joules consumed by that host so far
	 *
	 * Please note that since the consumption is lazily updated, it may require a simcall to update it.
	 * The result is that the actor requesting this value will be interrupted,
	 * the value will be updated in kernel mode before returning the control to the requesting actor.
	 */
	public native double getConsumedEnergy();

	/** Returns the current load of the host, as a ratio = achieved_flops / (core_current_speed * core_amount)
	 *	
	 * See simgrid::plugin::HostLoad::get_current_load() for the full documentation.
	 */
	public native double getCurrentLoad();
	/** Returns the number of flops computed of the host since the beginning of the simulation */
	public native double getComputedFlops();
	/** Returns the average load of the host as a ratio since the beginning of the simulation*/
	public native double getAvgLoad();
   
	/** Returns the current pstate */
	public native int getPstate();
	/** Changes the current pstate */
	public native void setPstate(int pstate);
	public native int getPstatesCount();
	/** Returns the speed of the processor (in flop/s) at the current pstate. See also @ref plugin_energy. */
	public native double getCurrentPowerPeak();
	/** Returns the speed of the processor (in flop/s) at a given pstate. See also @ref plugin_energy. */
	public native double getPowerPeakAt(int pstate);

	/** Returns the current computation load (in flops per second) */
	public native double getLoad();

	/** Class initializer, to initialize various JNI stuff */
	private static native void nativeInit();
	static {
		nativeInit();
	}	
}
