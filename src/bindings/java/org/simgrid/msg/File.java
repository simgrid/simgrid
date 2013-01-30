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
	 * @param storage is the name where you can find the stream 
	 * @param path is the file location on the storage 
	 * @param mode points to a string beginning with one of the following sequences (Additional characters may follow these sequences.): r Open text file for reading. The stream is positioned at the beginning of the file. r+ Open for reading and writing. The stream is positioned at the beginning of the file. w Truncate file to zero length or create text file for writing. The stream is positioned at the beginning of the file. w+ Open for reading and writing. The file is created if it does not exist, otherwise it is truncated. The stream is positioned at the beginning of the file. a Open for appending (writing at end of file). The file is created if it does not exist. The stream is positioned at the end of the file. a+ Open for reading and appending (writing at end of file). The file is created if it does not exist. The initial file position for reading is at the beginning of the file, but output is always appended to the end of the file.
	 */
	public File(String storage, String path, String mode) {
		this.storage = storage;
		open(storage, path, mode);
	}
	protected void finalize() {

	}
	/**
	 * Opens the file whose name is the string pointed to by path. 
	 * @param storage is the name where you can find the stream 
	 * @param path is the file location on the storage
	 * @param mode points to a string beginning with one of the following sequences (Additional characters may follow these sequences.): r Open text file for reading. The stream is positioned at the beginning of the file. r+ Open for reading and writing. The stream is positioned at the beginning of the file. w Truncate file to zero length or create text file for writing. The stream is positioned at the beginning of the file. w+ Open for reading and writing. The file is created if it does not exist, otherwise it is truncated. The stream is positioned at the beginning of the file. a Open for appending (writing at end of file). The file is created if it does not exist. The stream is positioned at the end of the file. a+ Open for reading and appending (writing at end of file). The file is created if it does not exist. The initial file position for reading is at the beginning of the file, but output is always appended to the end of the file.
	 */
	protected native void open(String storage, String path, String mode);
	/**
	 * Read elements of a file. 
	 * @param storage is the name where you can find the stream
	 * @param size of each element
	 * @param nMemb is the number of elements of data to write 
	 */
	public native long read(long size, long nMemb);
	/**
	 * Write elements into a file. 
	 * @param storage is the name where you can find the stream 
	 * @param size of each element  
	 * @param nMemb is the number of elements of data to write 
	 */
	public native long write(long size, long nMemb);
	/**
	 * Close the file. 	
	 * @param storage is the name where you can find the stream 
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