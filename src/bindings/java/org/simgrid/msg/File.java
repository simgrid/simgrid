package org.simgrid.msg;
/**
* Copyright 2012 The SimGrid team. All right reserved. 
*
* This program is free software; you can redistribute 
* it and/or modify it under the terms of the license 
* (GNU LGPL) which comes with this package.
*
*/

public class File {
	protected String storage;
	/**
	 * Represents the bind between the java comm and the
	 * native C comm. You must never access it, since it is 
	 * automatically set.
	 */
	private long bind = 0;
	/**
	 * Constructor, opens the file.
	 * @param storage is the name where you can find the file descriptor 
	 * @param path is the file location on the storage 
	 */
	public File(String storage, String path) {
		this.storage = storage;
		open(storage, path);
	}
	protected void finalize() {

	}
	/**
	 * Opens the file whose name is the string pointed to by path. 
	 * @param storage is the name where you can find the file descriptor 
	 * @param path is the file location on the storage
	 */
	protected native void open(String storage, String path);
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
	/**
	 * Close the file. 	
	 */
	public native void close();
	
	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		Msg.nativeInit();
		nativeInit();
	}	
}