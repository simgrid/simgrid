/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

public class File {
	/**
	 * Represents the bind between the java comm and the
	 * native C comm. You must never access it, since it is 
	 * automatically set.
	 */
	private long bind = 0;
	/**
	 * Constructor, opens the file.
	 * @param path is the file location on the storage 
	 */
	public File(String path) {
		open(path);
	}
	@Override
	protected void finalize() {

	}
	/**
	 * Opens the file whose name is the string pointed to by path.  
	 * @param path is the file location on the storage
	 */
	protected native void open(String path);
	/**
	 * Read elements of a file. 
	 * @param size of each element
	 * @param nMemb is the number of elements of data to write 
	 */
	public native long read(long size, long nMemb);
	/**
	 * Write elements into a file. 
	 * @param size of each element  
	 * @param nMemb is the number of elements of data to write 
	 */
	public native long write(long size, long nMemb);
	/** Close the file. */
	public native void close();

	/** Class initializer, to initialize various JNI stuff */
	public static native void nativeInit();
	static {
		org.simgrid.NativeLib.nativeInit();
		nativeInit();
	}	
}