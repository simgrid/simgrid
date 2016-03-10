/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;
/** A mutex  implemented on top of SimGrid synchronization mechanisms. 
 * You can use it exactly the same way that you use the mutexes, 
 * but to handle the interactions between the processes within the simulation.   
 * 
 * Don't mix simgrid synchronization with Java native one, or it will deadlock!
 */
public class Mutex {
	private long bind; // The C object -- don't touch it

	public Mutex() {
		init();
	}
	@Override
	protected void finalize() {
		try {
			nativeFinalize();
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
	private native void nativeFinalize();
	private native void init();
	public native void acquire();
	public native void release();

	/** Class initializer, to initialize various JNI stuff */
	public static native void nativeInit();
	static {
		org.simgrid.NativeLib.nativeInit();
		nativeInit();
	}	
}


