/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

public class As {

	private long bind;

	protected As() {
	};

	@Override
	public String toString (){
		return this.getName(); 
	}
	public native String getName();

	public native As[] getSons();

	public native String getProperty(String name);

	public native Host[] getHosts();

	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		nativeInit();
	}
}
