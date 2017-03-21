/* JNI interface to virtual machine in Simgrid */

/* Copyright (c) 2006-2014. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;
import java.util.ArrayList;

public class VM extends Host {
	// No need to declare a new bind variable: we use the one inherited from the super class Host

	/* Static functions */ 

	private static ArrayList<VM> vms= new ArrayList<>();
	private Host currentHost; 

	/** Create a `basic' VM (i.e. 1GB of RAM, other values are not taken into account). */
	public VM(Host host, String name) {
		this(host,name,1024, 0, 0);
	}

	/**
	 * Create a VM
	 * @param host Host node
	 * @param name name of the machine
	 * @param ramSize size of the RAM that should be allocated (in MBytes)
	 * @param migNetSpeed (network bandwith allocated for migrations in MB/s, if you don't know put zero ;))
	 * @param dpIntensity (dirty page percentage according to migNetSpeed, [0-100], if you don't know put zero ;))
	 */
	public VM(Host host, String name, int ramSize, int migNetSpeed, int dpIntensity){
		super();
		super.name = name;
		this.currentHost = host; 
		create(host, name, ramSize, migNetSpeed, dpIntensity);
		vms.add(this);
	}

	public static VM[] all(){
		VM[] allvms = new VM[vms.size()];
		vms.toArray(allvms);
		return allvms;
	}

	public static VM getVMByName(String name){
		for (VM vm : vms){
			if (vm.getName().equals(name))
				return vm;
		}
		return null; 
	}
	
	/** Shutdown and unref the VM. 
	 * 
	 * Actually, this strictly equivalent to shutdown().
	 * In C and in libvirt, the destroy function also releases the memory associated to the VM, 
	 * but this is not the way it goes in Java. The VM will only get destroyed by the garbage 
	 * collector when it is not referenced anymore by your variables. So, to see the VM really 
	 * destroyed, don't call this function but simply release any ref you have on it. 
	 */
	public void destroy() {
		shutdown();
///		vms.remove(this);
	}

	/* Make sure that the GC also destroys the C object */
	protected void finalize() throws Throwable {
		nativeFinalize();
	}
	public native void nativeFinalize();

	/** Returns whether the given VM is currently suspended */	
	public native int isCreated();

	/** Returns whether the given VM is currently running */
	public native int isRunning();

	/** Returns whether the given VM is currently running */
	public native int isMigrating();

	/** Returns whether the given VM is currently suspended */	
	public native int isSuspended();

	/**
	 * Natively implemented method create the VM.
	 * @param ramSize size of the RAM that should be allocated (in MB)
	 * @param migNetSpeed (network bandwith allocated for migrations in MB/s, if you don't know put zero ;))
	 * @param dpIntensity (dirty page intensity, a percentage of migNetSpeed [0-100],  if you don't know put zero ;))
	 */
	private native void create(Host host, String name, int ramSize, int migNetSpeed, int dpIntensity);


	/**
	 * Set a CPU bound for a given VM.
	 * @param bound in flops/s
	 */
	public native void setBound(double bound);

	/**  start the VM */
	public native void start();


	/**
	 * Immediately kills all processes within the given VM. 
	 * 
	 * No extra delay occurs. If you want to simulate this too, you want to use a MSG_process_sleep()
	 */
	public native void shutdown();

	/** native migration routine */
	private native void nativeMigration(Host destination) throws Exception;

	/** Change the host on which all processes are running
	 * (pre-copy is implemented)
	 */	
	public void migrate(Host destination) throws HostFailureException{
		try {
			this.nativeMigration(destination);
		} catch (Exception e){
		  Msg.info("Migration of VM "+this.getName()+" to "+destination.getName()+" is impossible ("+e.getMessage()+")");
		  throw new HostFailureException();
		}
		// If the migration correcly returned, then we should change the currentHost value. 
		this.currentHost = destination; 
	}

	/** Immediately suspend the execution of all processes within the given VM
	 *
	 * No suspension cost occurs. If you want to simulate this too, you want to
	 * use a \ref File.write() before or after, depending on the exact semantic
	 * of VM suspend to you.
	 */	
	public native void suspend();

	/** Immediately resumes the execution of all processes within the given VM
	 *
	 * No resume cost occurs. If you want to simulate this too, you want to
	 * use a \ref File.read() before or after, depending on the exact semantic
	 * of VM resume to you.
	 */
	public native void resume();

	/**  Class initializer (for JNI), don't do it yourself */
	public static native void nativeInit();
	static {
		nativeInit();
	}
}
