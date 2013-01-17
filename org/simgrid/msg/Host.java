/*
 * Bindings to the MSG hosts
 *
 * Copyright 2006-2012 The SimGrid Team           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
package org.simgrid.msg;

/**
 * A host object represents a location (any possible place) where a process may run. 
 * Thus it is represented as a physical resource with computing capabilities, some 
 * mailboxes to enable running process to communicate with remote ones, and some private 
 * data that can be only accessed by local process. An instance of this class is always 
 * binded with the corresponding native host. All the native hosts are automatically created 
 * during the call of the method Msg.createEnvironment(). This method take as parameter a
 * platform file which describes all elements of the platform (host, link, root..).
 * You cannot create a host yourself.
 *
 * The best way to get an host instance is to call the static method 
 * Host.getByName().
 *
 * For example to get the instance of the host. If your platform
 * file description contains an host named "Jacquelin" :
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
	/**
	 * Host name
	 */
	private String name;

	/**
	 * User data.
	 */ 
	private Object data;
    /**
     *
     */
    protected Host() {
		this.bind = 0;
		this.data = null;
	};

	/**
	 * This static method gets an host instance associated with a native
	 * host of your platform. This is the best way to get a java host object.
	 *
	 * @param name		The name of the host to get.
	 *
     * @return
     * @exception		HostNotFoundException if the name of the host is not valid.
	 *					NativeException if the native version of this method failed.
	 */ 
	public native static Host getByName(String name) 
	throws HostNotFoundException, NullPointerException;
	/**
	 * This static method returns the count of the installed hosts.
	 *
	 * @return			The count of the installed hosts.
	 */ 
	public native static int getCount();

	/**
	 * This static method return an instance to the host of the current process.
	 *
	 * @return			The host on which the current process is executed.
	 */ 
	public native static Host currentHost();

	/**
	 * This static method returns all of the hosts of the installed platform.
	 *
	 * @return			An array containing all the hosts installed.
	 *
	 */ 
	public native static Host[] all();

	/**
	 * This method returns the name of a host.
	 * @return			The name of the host.
	 *
	 */ 
	public String getName() {
		return name;
	}
	/**
	 * Sets the data of the host.
     * @param data
     */
	public void setData(Object data) {
		this.data = data;
	} 
	/**
	 * Gets the d	ata of the host.
     *
     * @return
     */
	public Object getData() {
		return this.data;
	}

	/**
	 * Checks whether a host has data.
     *
     * @return
     */
	public boolean hasData() {
		return null != this.data;
	}

	/**
	 * This method returns the number of tasks currently running on a host.
	 * The external load is not taken in account.
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
	 * @brief Returns the value of a given host property. 
	 */
	public native String getProperty(String name);
	/**
	 * @brief Change the value of a given host property. 
	 */
	public native void setProperty(String name, String value);
    /** This method tests if a host is avail.
     * @return
     */
	public native boolean isAvail();
	
	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		nativeInit();
	}	
} 
