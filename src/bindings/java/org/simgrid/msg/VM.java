/* Java bindings of the s4u::VirtualMachine */

/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

public class VM extends Host {
	// No need to declare a new bind variable: we use the one inherited from the super class Host

	private Host currentHost;
	private int coreAmount = 1;

	/**
	 * Create a `basic` VM : 1 core and 1GB of RAM.
	 * @param host Host node
	 * @param name name of the machine
	 */
	public VM(Host host, String name) {
		this(host,name, /*coreAmount*/1, 1024, 0, 0);
	}

	/**
	 * Create a VM without useless values (for humans)
	 * @param host Host node
	 * @param name name of the machine
	 * @param coreAmount the amount of cores of the VM
	 */
	public VM(Host host, String name, int coreAmount) {
		this(host,name, coreAmount, 1024, 0, 0);
	}

	/**
	 * Create a VM with 1 core
	 * @param host Host node
	 * @param name name of the machine
	 * @param ramSize size of the RAM that should be allocated (in MBytes)
	 * @param migNetSpeed (network bandwidth allocated for migrations in MB/s, if you don't know put zero ;))
	 * @param dpIntensity (dirty page percentage according to migNetSpeed, [0-100], if you don't know put zero ;))
	 */
	public VM(Host host, String name, int ramSize, int migNetSpeed, int dpIntensity){
		this(host, name, /*coreAmount*/1, ramSize, migNetSpeed, dpIntensity);
	}

	/**
	 * Create a VM
	 * @param host Host node
	 * @param name name of the machine
	 * @param coreAmount the amount of cores of the VM
	 * @param ramSize size of the RAM that should be allocated (in MBytes)
	 * @param migNetSpeed (network bandwidth allocated for migrations in MB/s, if you don't know put zero ;))
	 * @param dpIntensity (dirty page percentage according to migNetSpeed, [0-100], if you don't know put zero ;))
	 */
	public VM(Host host, String name, int coreAmount, int ramSize, int migNetSpeed, int dpIntensity){
		super();
		super.name = name;
		this.currentHost = host;
		this.coreAmount = coreAmount;
		create(host, name, coreAmount, ramSize, migNetSpeed, dpIntensity);
	}

	/** Retrieve the list of all existing VMs */
	public static native VM[] all();

	/** Retrieve a VM from its name */
	public static native VM getVMByName(String name);

	/**
	 * Make sure that the GC also destroys the C object
	 * @deprecated (from Java9 onwards)
	 */
	@Deprecated @Override
	protected void finalize() throws Throwable {
		nativeFinalize();
	}
	private native void nativeFinalize();

	/** Returns whether the given VM is currently suspended */
	public native boolean isCreated();

	/** Returns whether the given VM is currently running */
	public native boolean isRunning();

	/** Returns whether the given VM is currently running */
	public native boolean isMigrating();

	/** Returns whether the given VM is currently suspended */
	public native boolean isSuspended();

	/** Returns the amount of virtual CPUs provided */
	public int getCoreAmount() {
		return coreAmount;
	}

	/**
	 * Natively implemented method create the VM.
	 * @param ramSize size of the RAM that should be allocated (in MB)
	 * @param migNetSpeed (network bandwidth allocated for migrations in MB/s, if you don't know put zero ;))
	 * @param dpIntensity (dirty page intensity, a percentage of migNetSpeed [0-100],  if you don't know put zero ;))
	 */
	private native void create(Host host, String name, int coreAmount, int ramSize, int migNetSpeed, int dpIntensity);

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

	/** Shutdown and unref the VM. */
	public native void destroy();

	/** Change the host on which all processes are running
	 * (pre-copy is implemented)
	 */
	public void migrate(Host destination) throws HostFailureException{
		try {
			this.nativeMigration(destination);
		} catch (Exception e){
		  Msg.info("Migration of VM "+this.getName()+" to "+destination.getName()+" is impossible ("+e.getMessage()+")");
		  throw new HostFailureException(e.getMessage());
		}
		// If the migration correctly returned, then we should change the currentHost value.
		this.currentHost = destination;
	}
	private native void nativeMigration(Host destination) throws MsgException;

	/** Immediately suspend the execution of all processes within the given VM
	 *
	 * No suspension cost occurs. If you want to simulate this too, you want to use a @ref File.write() before or
	 * after, depending on the exact semantic of VM suspend to you.
	 */
	public native void suspend();

	/** Immediately resumes the execution of all processes within the given VM
	 *
	 * No resume cost occurs. If you want to simulate this too, you want to use a @ref File.read() before or after,
	 * depending on the exact semantic of VM resume to you.
	 */
	public native void resume();

	/**  Class initializer (for JNI), don't do it yourself */
	private static native void nativeInit();
	static {
		nativeInit();
	}
}
