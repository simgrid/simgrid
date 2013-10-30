/*
 * JNI interface to Cloud interface in Simgrid
 * 
 * Copyright (c) 2006-2013. The SimGrid Team.
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */
package org.simgrid.msg;

import org.simgrid.msg.Host;
import org.simgrid.msg.Process;

public class VM {
	/**
	 * This attribute represents a bind between a java task object and
	 * a native task. Even if this attribute is public you must never
	 * access to it. It is set automatically during the build of the object.
	 */
	private long bind = 0;
	
	private int coreAmount;

        private String name;
	/**
	 * Create a new empty VM.
	 * NOTE: it is expected that in the future, the coreAmount parameter will be used
	 * to add extra constraints on the execution, but the argument is ignored for now.
	 */
	public VM(Host host, String name, int coreAmount) {
		this.coreAmount = coreAmount;
		this.name = name;
		start(host,name,coreAmount);
	}
	protected void finalize() {
		destroy();
	}
	/**
	 * Destroy the VM
	 */
	protected native void destroy();
	/**
	 * Natively implemented method starting the VM.
	 * @param coreAmount
	 */
	private native void start(Host host, String name, int coreAmount);
		
	/** Returns whether the given VM is currently suspended
	 */	
	public native boolean isSuspended();
	/** Returns whether the given VM is currently running
	 */
	public native boolean isRunning();
	/** Add the given process into the VM.
	 * Afterward, when the VM is migrated or suspended or whatever, the process will have the corresponding handling, too.
	 */	
	public native void bind(Process process);
	/** Removes the given process from the given VM, and kill it
	 *  Will raise a ProcessNotFound exception if the process were not bound to that VM
	 */	
	public native void unbind(Process process);
	/** Immediately change the host on which all processes are running
	 *
	 * No migration cost occurs. If you want to simulate this too, you want to use a
	 * Task.send() before or after, depending on whether you want to do cold or hot
	 * migration.
	 */	
	public native void migrate(Host destination);
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
	/**
	 * Immediately kills all processes within the given VM. Any memory that they allocated will be leaked.
	 * No extra delay occurs. If you want to simulate this too, you want to use a MSG_process_sleep() or something
	 */
	public native void shutdown();
	/**
	 * Reboot the VM, restarting all the processes in it.
	 */
	public native void reboot();

	public String getName() {
		return name;
	}		

	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		nativeInit();
	}
}
