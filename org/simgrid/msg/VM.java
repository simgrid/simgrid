/*
 * JNI interface to Cloud interface in Simgrid
 * 
 * Copyright 2006,2007,2010,2012 The SimGrid Team.           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */
package org.simgrid.msg;

import org.simgrid.msg.Process;

public class VM {
	/**
	 * This attribute represents a bind between a java task object and
	 * a native task. Even if this attribute is public you must never
	 * access to it. It is set automatically during the build of the object.
	 */
	private long bind = 0;
	
	private int coreAmount;
	/**
	 * @brief Create a new empty VM.
	 * @bug it is expected that in the future, the coreAmount parameter will be used
	 * to add extra constraints on the execution, but the argument is ignored for now.
	 */
	public VM(int coreAmount) {
		this.coreAmount = coreAmount;
	}
	/**
	 * Natively implemented method starting the VM.
	 * @param coreAmount
	 */
	private native void start(int coreAmount);
	
	/**
	 * @brief Returns a new array containing all existing VMs.
	 */
	public static native VM[] all();
	
	/** @brief Returns whether the given VM is currently suspended
	 */	
	public native boolean isSuspended();
	/** @brief Returns whether the given VM is currently running
	 */
	public native boolean isRunning();
	/** @brief Add the given process into the VM.
	 * Afterward, when the VM is migrated or suspended or whatever, the process will have the corresponding handling, too.
	 */	
	public native void bind(Process process);
	/** @brief Removes the given process from the given VM, and kill it
	 *  Will raise a ProcessNotFound exception if the process were not binded to that VM
	 */	
	public native void unbind(Process process);
	/** @brief Immediately change the host on which all processes are running
	 *
	 * No migration cost occurs. If you want to simulate this too, you want to use a
	 * Task.send() before or after, depending on whether you want to do cold or hot
	 * migration.
	 */	
	public native void migrate(Host destination);
	/** @brief Immediately suspend the execution of all processes within the given VM
	 *
	 * No suspension cost occurs. If you want to simulate this too, you want to
	 * use a \ref File.write() before or after, depending on the exact semantic
	 * of VM suspend to you.
	 */	
	public native void suspend();
	/** @brief Immediately resumes the execution of all processes within the given VM
	 *
	 * No resume cost occurs. If you want to simulate this too, you want to
	 * use a \ref File.read() before or after, depending on the exact semantic
	 * of VM resume to you.
	 */
	public native void resume();
		
}