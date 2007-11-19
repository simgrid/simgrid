/*
 * simgrid.msg.Host.java	1.00 07/05/01
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
  package simgrid.msg;
import java.lang.String;

/**
 * A host object represents a location (any possible place) where a process may run. 
 * Thus it is represented as a physical resource with computing capabilities, some 
 * mailboxes to enable running process to communicate with remote ones, and some private 
 * data that can be only accessed by local process. An instance of this class is always 
 * binded with the corresponding native host. All the native hosts are automaticaly created 
 * during the call of the method Msg.createEnvironment(). This method take as parameter a
 * platform file which describes all elements of the platform (host, link, root..).
 * You never need to create an host your self.
 *
 * The best way to get an host instance is to call the static method 
 * Host.getByName().
 *
 * For example to get the instance of the host. If your platform
 * file description contains an host named "Jacquelin" :
 *
 * \verbatim
 *	Host jacquelin;
 *
 * 	try { 
 * 		jacquelin = Host.getByName("Jacquelin");
 *	} catch(HostNotFoundException e) {
 *		System.err.println(e.toString());
 *	}
 *	...
 * \endverbatim
 *
 * @author  Abdelmalek Cherier
 * @author  Martin Quinson
 * @since SimGrid 3.3
 */ 
  public class Host {
  
        /**
	 * This attribute represents a bind between a java host object and
	 * a native host. Even if this attribute is public you must never
	 * access to it. It is set automaticatly during the call of the 
	 * static method Host.getByName().
	 *
	 * @see				Host.getByName().
	 */ 
  public long bind;

  
        /**
	 * User data.
	 */ 
    private Object data;
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
	 * @exception		HostNotFoundException if the name of the host is not valid.
	 *					MsgException if the native version of this method failed.
	 */ 
  public static Host getByName(String name) 
    throws HostNotFoundException, NativeException, JniException {
    return MsgNative.hostGetByName(name);
  }
  
        /**
	 * This static method returns the number of the installed hosts.
	 *
	 * @return			The number of the installed hosts.
	 *
	 */ 
  public static int getNumber() throws NativeException, JniException {
    return MsgNative.hostGetNumber();
  }
  
        /**
	 * This static method return an instance to the host of the current process.
	 *
	 * @return			The host on which the current process is executed.
	 *
	 * @exception		MsgException if the native version of this method failed.
	 */ 
    public static Host currentHost() throws JniException {
    return MsgNative.hostSelf();
  }
  
        /**
	 * This static method returns all of the hosts of the installed platform.
	 *
	 * @return			An array containing all the hosts installed.
	 *
	 * @exception		MsgException if the native version of this method failed.
	 */ 
    public static Host[] all() throws JniException, NativeException {
    return MsgNative.allHosts();
  }
  
        /**
	 * This method returns the name of a host.
	 *
	 * @return			The name of the host.
	 *
	 * @exception		InvalidHostException if the host is not valid.
	 */ 
    public String getName() throws NativeException, JniException {
    return MsgNative.hostGetName(this);
  }
  
        /**
	 * This method sets the data of the host.
	 *
	 */ 
    public void setData(Object data) {
    this.data = data;
  } 
        /**
	 * This method gets the data of the host.
	 */ 
    public Object getData() {
    return this.data;
  }
  
        /**
	 * This function tests if a host has data.
	 */ 
    public boolean hasData() {
    return null != this.data;
  }
  
        /**
	 * This method returns the number of tasks currently running on a host.
	 * The external load is not taken in account.
	 *
	 * @return			The number of tasks currently running on a host.
	 *
	 * @exception		InvalidHostException if the host is invalid.
	 *
	 */ 
  public int getLoad() throws JniException {
    return MsgNative.hostGetLoad(this);
  }
  
        /**
	 * This method returns the speed of the processor of a host,
	 * regardless of the current load of the machine.
	 *
	 * @return			The speed of the processor of the host in flops.
	 *
	 * @exception		InvalidHostException if the host is not valid.
	 *
	 */ 
   public double getSpeed() throws JniException {
    return MsgNative.hostGetSpeed(this);
  }
  
        /**
	 * This method tests if a host is avail.
	 * 
	 * @exception		JniException if the host is not valid.
	 */ 
    public boolean isAvail() throws JniException {
    return MsgNative.hostIsAvail(this);
  }
  
   /** Send the given task to the given channel of the host */ 
   
    public void put(int channel, Task task) throws JniException,
    NativeException {
    MsgNative.hostPut(this, channel, task, -1);
  } 
   /** Send the given task to the given channel of the host (waiting at most #timeout seconds) */ 
   
    public void put(int channel, Task task,
                    double timeout) throws JniException, NativeException {
    MsgNative.hostPut(this, channel, task, timeout);
  } 
   /** Send the given task to the given channel of the host (capping the emision rate to #maxrate) */ 
   
    public void putBounded(int channel, Task task,
                           double maxrate) throws JniException,
    NativeException {
    MsgNative.hostPutBounded(this, channel, task, maxrate);
} } 
