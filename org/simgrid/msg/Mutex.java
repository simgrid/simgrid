package org.simgrid.msg;
/** A mutex  implemented on top of SimGrid synchronization mechanisms. 
 * You can use it exactly the same way that you use the mutexes, 
 * but to handle the interactions between the threads within the simulation.   
 * 
 * Copyright 2012 The SimGrid team. All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 *
 */
public class Mutex {
	private long bind; // The C object -- don't touch it
	
	public Mutex(int capa) {
		init(capa);
	}
	protected void finalize() {
		exit();
	}
	private native void exit();
	private native void init(int capacity);
	public native void acquire();
	public native void release();
	
	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		nativeInit();
	}	
}


